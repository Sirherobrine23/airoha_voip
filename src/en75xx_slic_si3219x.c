// SPDX-License-Identifier: GPL-2.0
/*
 * Silicon Labs Si3219x ProSLIC integration for EcoNet/Airoha voice hardware.
 *
 * The electrical initialization and DSP patch are handled by the ProSLIC API.
 * This file only supplies the Linux SPI/reset services and connects one FXS
 * channel to the common EN75xx PCM/userspace interface.
 *
 * Interface note. Airoha's own SLIC support matrix lists the Si32192 and
 * Si32193 as ISI parts -- ISI multiplexes the ProSLIC control channel over
 * the PCM bus the same way ZSI does for the Microchip parts -- while the
 * Si32184/Si32185 and Si3228x are the ones it drives over plain SPI. This
 * adapter currently implements SPI transactions only. No supported SPI mode
 * for Si32192/Si32193 has been established by the available documentation.
 * Their known compatibles are refused before reset or bus configuration;
 * implementing ISI is required before enabling those board nodes.
 *
 * Three register fix-ups below come from the EcoNet mod-slic3 ProSLIC
 * integration in the TP-Link VB430 GPL drop and are applied after
 * ProSLIC_Init(), because the API's presets do not cover them:
 * the PCM format field, the transmit clock edge, and the interrupt mask.
 */
#include <linux/bits.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/spi/spi.h>
#include <linux/workqueue.h>

#include "../include/en75xx_voice.h"
#include "proslic.h"
#include "si_voice.h"
#include "si3219x.h"
#include "si3219x_LCCB_constants.h"
#include "en75xx_proslic_fw.h"

#define SI3219X_REG_RAM_WAIT	4
#define SI3219X_REG_RAM_HI	5
#define SI3219X_REG_RAM_D0	6
#define SI3219X_REG_RAM_D1	7
#define SI3219X_REG_RAM_D2	8
#define SI3219X_REG_RAM_D3	9
#define SI3219X_REG_RAM_LO	10
#define SI3219X_CW_RD		0x60
#define SI3219X_CW_WR		0x20
#define SI3219X_CW_BCAST	0x80
#define SI3219X_BCAST		0xff
#define SI3219X_RAM_HIGH(addr)	(((addr) >> 3) & 0xe0)
#define SI3219X_RAM_WAIT_TRIES	100

/*
 * Post-init register fix-ups, from the vendor integration.
 *
 * PCMMODE bits 1:0 are PCM_FMT; 0x3 selects 16-bit linear, matching the
 * PCM engine's 16-bit timeslots. PCMTXHI bit 4 selects the clock edge on
 * which DTX is driven, and it must be clear so the ProSLIC drives on the
 * rising edge of PCLK. IRQEN2 bit 1 is the loop-status (hook) interrupt;
 * the rest stay masked so the chip does not assert on events nothing
 * here consumes.
 */
#define SI3219X_PCM_FMT_MASK	0x03
#define SI3219X_PCM_FMT_LINEAR	0x03
#define SI3219X_PCM_TX_EDGE	BIT(4)
#define SI3219X_IRQEN2_HOOK	BIT(1)

/*
 * Patch BOM selector. Electrical parameters are compiled separately in
 * si3219x_LCCB_constants.c; changing a firmware filename cannot change
 * the converter topology. Reject unsupported variants before touching HW.
 */
/* Only the LCCB electrical configuration is linked by src/Makefile. */
static char *bom = "lcqc";
module_param(bom, charp, 0444);
MODULE_PARM_DESC(bom, "ProSLIC patch BOM (only lcqc with the compiled LCCB configuration)");

/* The vendor API resolves patches through process-wide symbols. */
static DEFINE_MUTEX(si3219x_init_lock);

struct en75xx_si3219x {
	struct spi_device *spi;
	struct gpio_desc *reset_gpio;
	struct mutex io_lock;
	SiVoiceControlInterfaceType ctrl;
	SiVoiceDeviceType *device;
	SiVoiceChanType_ptr channel;
	struct en75xx_proslic_fw fw;
	proslicChanType_ptr channel_ptrs[1];
	struct en75xx_pcm *pcm;
	struct en75xx_voice_line *voice_line;
	struct delayed_work hook_work;
	struct delayed_work ring_work;
	unsigned int pcm_channel;
	unsigned int line;
	unsigned int ring_on_ms;
	unsigned int ring_off_ms;
	bool ring_enabled;
	bool ring_phase;
	bool last_hook;
	bool init_attempted;
};

static const u8 si3219x_chan_addr[32] = {
	0x00, 0x10, 0x08, 0x18, 0x04, 0x14, 0x0c, 0x1c,
	0x02, 0x12, 0x0a, 0x1a, 0x06, 0x16, 0x0e, 0x1e,
	0x01, 0x11, 0x09, 0x19, 0x05, 0x15, 0x0d, 0x1d,
	0x03, 0x13, 0x0b, 0x1b, 0x07, 0x17, 0x0f, 0x1f,
};

static int si3219x_reset(void *ctrl, int in_reset)
{
	struct en75xx_si3219x *slic = ctrl;

	if (!slic->reset_gpio)
		return RC_NONE;
	/* Descriptor values are logical; GPIO_ACTIVE_LOW supplies inversion. */
	gpiod_set_value_cansleep(slic->reset_gpio, !!in_reset);
	return RC_NONE;
}

static int si3219x_write_reg(void *ctrl, uInt8 channel, uInt8 reg, uInt8 data)
{
	struct en75xx_si3219x *slic = ctrl;
	u8 tx[3];
	int ret;

	if (channel == SI3219X_BCAST)
		tx[0] = SI3219X_CW_BCAST | SI3219X_CW_WR;
	else if (channel < ARRAY_SIZE(si3219x_chan_addr))
		tx[0] = SI3219X_CW_WR | si3219x_chan_addr[channel];
	else
		return -EINVAL;
	tx[1] = reg;
	tx[2] = data;

	mutex_lock(&slic->io_lock);
	ret = spi_write(slic->spi, tx, sizeof(tx));
	mutex_unlock(&slic->io_lock);
	return ret ? RC_SPI_FAIL : RC_NONE;
}

static uInt8 si3219x_read_reg(void *ctrl, uInt8 channel, uInt8 reg)
{
	struct en75xx_si3219x *slic = ctrl;
	u8 tx[2], rx = 0xff;
	int ret;

	if (channel >= ARRAY_SIZE(si3219x_chan_addr))
		return 0xff;
	tx[0] = SI3219X_CW_RD | si3219x_chan_addr[channel];
	tx[1] = reg;

	mutex_lock(&slic->io_lock);
	ret = spi_write_then_read(slic->spi, tx, sizeof(tx), &rx, 1);
	mutex_unlock(&slic->io_lock);
	return ret ? 0xff : rx;
}

static int si3219x_wait_ram(void *ctrl, uInt8 channel)
{
	unsigned int tries = SI3219X_RAM_WAIT_TRIES;

	while (tries--) {
		if (!(si3219x_read_reg(ctrl, channel, SI3219X_REG_RAM_WAIT) & 1))
			return 0;
		usleep_range(1000, 2000);
	}
	return -ETIMEDOUT;
}

static int si3219x_write_ram(void *ctrl, uInt8 channel, uInt16 addr,
			     ramData data)
{
	ramData value = data;
	int ret;

	ret = si3219x_wait_ram(ctrl, channel);
	if (ret)
		return RC_SPI_FAIL;

	si3219x_write_reg(ctrl, channel, SI3219X_REG_RAM_HI,
			 SI3219X_RAM_HIGH(addr));
	si3219x_write_reg(ctrl, channel, SI3219X_REG_RAM_D0, (u8)(value << 3));
	value >>= 5;
	si3219x_write_reg(ctrl, channel, SI3219X_REG_RAM_D1, value & 0xff);
	value >>= 8;
	si3219x_write_reg(ctrl, channel, SI3219X_REG_RAM_D2, value & 0xff);
	value >>= 8;
	si3219x_write_reg(ctrl, channel, SI3219X_REG_RAM_D3, value & 0xff);
	si3219x_write_reg(ctrl, channel, SI3219X_REG_RAM_LO, addr & 0xff);
	return si3219x_wait_ram(ctrl, channel) ? RC_SPI_FAIL : RC_NONE;
}

static ramData si3219x_read_ram(void *ctrl, uInt8 channel, uInt16 addr)
{
	ramData data;

	if (si3219x_wait_ram(ctrl, channel))
		return 0;
	si3219x_write_reg(ctrl, channel, SI3219X_REG_RAM_HI,
			 SI3219X_RAM_HIGH(addr));
	si3219x_write_reg(ctrl, channel, SI3219X_REG_RAM_LO, addr & 0xff);
	if (si3219x_wait_ram(ctrl, channel))
		return 0;

	data = si3219x_read_reg(ctrl, channel, SI3219X_REG_RAM_D3);
	data = (data << 8) | si3219x_read_reg(ctrl, channel, SI3219X_REG_RAM_D2);
	data = (data << 8) | si3219x_read_reg(ctrl, channel, SI3219X_REG_RAM_D1);
	data = (data << 8) | si3219x_read_reg(ctrl, channel, SI3219X_REG_RAM_D0);
	return data >> 3;
}

static int si3219x_delay(void *timer, int ms)
{
	if (ms <= 0)
		return RC_NONE;
	if (ms < 20)
		usleep_range(ms * 1000, ms * 1000 + 1000);
	else
		msleep(ms);
	return RC_NONE;
}

static void si3219x_control_init(struct en75xx_si3219x *slic)
{
	SiVoice_setControlInterfaceCtrlObj(&slic->ctrl, slic);
	SiVoice_setControlInterfaceReset(&slic->ctrl, si3219x_reset);
	SiVoice_setControlInterfaceWriteRegister(&slic->ctrl, si3219x_write_reg);
	SiVoice_setControlInterfaceReadRegister(&slic->ctrl, si3219x_read_reg);
	SiVoice_setControlInterfaceWriteRAM(&slic->ctrl, si3219x_write_ram);
	SiVoice_setControlInterfaceReadRAM(&slic->ctrl, si3219x_read_ram);
	SiVoice_setControlInterfaceTimerObj(&slic->ctrl, NULL);
	SiVoice_setControlInterfaceDelay(&slic->ctrl, si3219x_delay);
	SiVoice_setControlInterfaceTimeElapsed(&slic->ctrl, NULL);
	SiVoice_setControlInterfaceGetTime(&slic->ctrl, NULL);
	SiVoice_setControlInterfaceSemaphore(&slic->ctrl, NULL);
}

static int en75xx_si3219x_get_hook(void *priv)
{
	struct en75xx_si3219x *slic = priv;
	uInt8 hook = PROSLIC_ONHOOK;
	int ret;

	ret = ProSLIC_ReadHookStatus(slic->channel, &hook);
	if (ret != RC_NONE)
		return -EIO;
	return hook == PROSLIC_OFFHOOK;
}

static int en75xx_si3219x_set_linefeed(void *priv,
				       enum en75xx_voice_linefeed state)
{
	struct en75xx_si3219x *slic = priv;
	uInt8 lf;

	switch (state) {
	case EN75XX_VOICE_LINEFEED_OPEN:
		lf = LF_OPEN;
		break;
	case EN75XX_VOICE_LINEFEED_STANDBY:
		lf = LF_FWD_OHT;
		break;
	case EN75XX_VOICE_LINEFEED_ACTIVE:
		lf = LF_FWD_ACTIVE;
		break;
	case EN75XX_VOICE_LINEFEED_REVERSE:
		lf = LF_REV_ACTIVE;
		break;
	default:
		return -EINVAL;
	}
	return ProSLIC_SetLinefeedStatus(slic->channel, lf) == RC_NONE ? 0 : -EIO;
}

static void en75xx_si3219x_ring_work(struct work_struct *work)
{
	struct en75xx_si3219x *slic = container_of(to_delayed_work(work),
		struct en75xx_si3219x, ring_work);
	unsigned int delay;

	if (!READ_ONCE(slic->ring_enabled))
		return;

	slic->ring_phase = !slic->ring_phase;
	if (slic->ring_phase) {
		ProSLIC_RingStart(slic->channel);
		delay = slic->ring_on_ms;
	} else {
		ProSLIC_SetLinefeedStatus(slic->channel, LF_FWD_ACTIVE);
		delay = slic->ring_off_ms;
	}
	mod_delayed_work(system_wq, &slic->ring_work,
			 max_t(unsigned long, 1, msecs_to_jiffies(delay)));
}

static int en75xx_si3219x_ring(void *priv, bool enable, unsigned int on_ms,
			       unsigned int off_ms)
{
	struct en75xx_si3219x *slic = priv;

	cancel_delayed_work_sync(&slic->ring_work);
	slic->ring_enabled = enable;
	if (!enable) {
		slic->ring_phase = false;
		return ProSLIC_SetLinefeedStatus(slic->channel, LF_FWD_ACTIVE) ==
			RC_NONE ? 0 : -EIO;
	}

	slic->ring_on_ms = on_ms ? on_ms : 1000;
	slic->ring_off_ms = off_ms ? off_ms : 4000;
	slic->ring_phase = true;
	if (ProSLIC_RingStart(slic->channel) != RC_NONE)
		return -EIO;
	mod_delayed_work(system_wq, &slic->ring_work,
			msecs_to_jiffies(slic->ring_on_ms));
	return 0;
}

static int en75xx_si3219x_get_faults(void *priv, u32 *faults)
{
	struct en75xx_si3219x *slic = priv;
	int error = 0;

	SiVoice_getErrorFlag(slic->channel, &error);
	*faults = error;
	return 0;
}

static const struct en75xx_voice_slic_ops en75xx_si3219x_ops = {
	.get_hook = en75xx_si3219x_get_hook,
	.ring = en75xx_si3219x_ring,
	.set_linefeed = en75xx_si3219x_set_linefeed,
	.get_faults = en75xx_si3219x_get_faults,
};

static void en75xx_si3219x_hook_work(struct work_struct *work)
{
	struct en75xx_si3219x *slic = container_of(to_delayed_work(work),
		struct en75xx_si3219x, hook_work);
	int hook = en75xx_si3219x_get_hook(slic);

	if (hook >= 0 && !!hook != slic->last_hook) {
		slic->last_hook = !!hook;
		en75xx_voice_hook_changed(slic->voice_line, slic->last_hook);
	}
	mod_delayed_work(system_wq, &slic->hook_work, msecs_to_jiffies(20));
}

/*
 * Register fix-ups the ProSLIC presets do not cover. Applied after
 * ProSLIC_PCMStart() so nothing in the API path overwrites them.
 */
static void si3219x_apply_pcm_fixups(struct en75xx_si3219x *slic)
{
	u8 val;

	val = si3219x_read_reg(slic, 0, PROSLIC_REG_PCMMODE);
	val = (val & ~SI3219X_PCM_FMT_MASK) | SI3219X_PCM_FMT_LINEAR;
	si3219x_write_reg(slic, 0, PROSLIC_REG_PCMMODE, val);

	/* drive DTX on the rising edge of PCLK */
	val = si3219x_read_reg(slic, 0, PROSLIC_REG_PCMTXHI);
	si3219x_write_reg(slic, 0, PROSLIC_REG_PCMTXHI,
			  val & ~SI3219X_PCM_TX_EDGE);

	si3219x_write_reg(slic, 0, PROSLIC_REG_IRQEN1, 0);
	si3219x_write_reg(slic, 0, PROSLIC_REG_IRQEN2, SI3219X_IRQEN2_HOOK);
	si3219x_write_reg(slic, 0, PROSLIC_REG_IRQEN3, 0);
}

/* Best effort: bus errors or external supplies can prevent power-down. */
static void en75xx_si3219x_power_down(struct en75xx_si3219x *slic)
{
	int ret;

	if (!slic->init_attempted)
		return;
	ProSLIC_PCMStop(slic->channel);
	ret = ProSLIC_PowerDownConverter(slic->channel);
	if (ret != RC_NONE)
		dev_warn(&slic->spi->dev, "converter power-down failed: %d\n", ret);
	ProSLIC_SetLinefeedStatus(slic->channel, LF_OPEN);
	si3219x_reset(slic, 1);
	slic->init_attempted = false;
}

static void en75xx_si3219x_api_free(struct en75xx_si3219x *slic)
{
	en75xx_si3219x_power_down(slic);
	SiVoice_destroyChannels(&slic->channel);
	SiVoice_destroyDevices(&slic->device);
	en75xx_proslic_fw_free(&slic->fw);
}

static int en75xx_si3219x_api_init(struct en75xx_si3219x *slic)
{
	unsigned int slot;
	int ret;

	/*
	 * ProSLIC_PCMTimeSlotSetup() counts PCLK cycles from the frame
	 * sync, so it wants the same bit offset the PCM engine assigned
	 * to this channel. Deriving it from the engine's timeslot table
	 * keeps the two ends in step; computing it as channel * 16 only
	 * happens to agree when the table starts at offset 0, which the
	 * default one does not.
	 */
	ret = en75xx_pcm_channel_bit_offset(slic->pcm, slic->pcm_channel);
	if (ret < 0) {
		dev_err(&slic->spi->dev,
			"PCM channel %u has no timeslot configured\n",
			slic->pcm_channel);
		return ret;
	}
	slot = ret;

	si3219x_control_init(slic);
	ret = SiVoice_createDevice(&slic->device);
	if (ret != RC_NONE)
		return -ENOMEM;
	ret = SiVoice_createChannel(&slic->channel);
	if (ret != RC_NONE) {
		SiVoice_destroyDevices(&slic->device);
		return -ENOMEM;
	}

	ret = SiVoice_SWInitChan(slic->channel, 0, SI3219X_TYPE,
				 slic->device, &slic->ctrl);
	if (ret != RC_NONE)
		goto err;
	slic->channel_ptrs[0] = slic->channel;

	/*
	 * The API picks its patch through fixed symbols. Fill them from
	 * firmware before Init runs, otherwise it loads an empty patch
	 * and the chip comes up without its DSP image.
	 */
	ret = en75xx_proslic_fw_load(&slic->spi->dev, "si3219x", 'A', bom,
				     &slic->fw);
	if (ret)
		goto err;

	mutex_lock(&si3219x_init_lock);
	en75xx_proslic_fw_to_patch(&slic->fw, &si3219xPatchRevALCQC);
	en75xx_proslic_fw_to_patch(&slic->fw, &RevAPatch);

	SiVoice_Reset(slic->channel);
	slic->init_attempted = true;
	ret = ProSLIC_Init(slic->channel_ptrs, 1);
	/* No shared symbol may retain pointers into this device's firmware. */
	memset(&si3219xPatchRevALCQC, 0, sizeof(si3219xPatchRevALCQC));
	memset(&RevAPatch, 0, sizeof(RevAPatch));
	mutex_unlock(&si3219x_init_lock);
	if (ret != RC_NONE)
		goto err;

	/*
	 * Longitudinal balance calibration. The vendor runs it right after
	 * ProSLIC_Init(), once all the batteries are up. Without it the
	 * line keeps the factory-default balance coefficients and common
	 * mode rejection on a long loop is poor.
	 */
	ret = ProSLIC_LBCal(slic->channel_ptrs, 1);
	if (ret != RC_NONE) {
		dev_err(&slic->spi->dev,
			"longitudinal balance calibration failed: %d\n", ret);
		goto err;
	}

	if (ProSLIC_DCFeedSetup(slic->channel, DCFEED_48V_20MA) != RC_NONE ||
	    ProSLIC_ZsynthSetup(slic->channel, ZSYN_600_0_0_30_0) != RC_NONE ||
	    ProSLIC_RingSetup(slic->channel, DEFAULT_RINGING) != RC_NONE ||
	    ProSLIC_PCMSetup(slic->channel, PCM_16LIN) != RC_NONE ||
	    ProSLIC_PCMTimeSlotSetup(slic->channel, slot, slot) != RC_NONE ||
	    ProSLIC_SetLinefeedStatus(slic->channel, LF_FWD_ACTIVE) != RC_NONE ||
	    ProSLIC_PCMStart(slic->channel) != RC_NONE) {
		ret = -EIO;
		goto err;
	}

	dev_dbg(&slic->spi->dev, "PCM channel %u at frame bit offset %u\n",
		slic->pcm_channel, slot);

	si3219x_apply_pcm_fixups(slic);
	return 0;

err:
	en75xx_si3219x_api_free(slic);
	return ret < 0 ? ret : -EIO;
}

static int en75xx_si3219x_probe(struct spi_device *spi)
{
	struct en75xx_si3219x *slic;
	struct device_node *pcm_np;
	int ret, hook;

	if (of_device_is_compatible(spi->dev.of_node, "silabs,si32192") ||
	    of_device_is_compatible(spi->dev.of_node, "silabs,si32193"))
		return dev_err_probe(&spi->dev, -EOPNOTSUPP,
			"Si32192/Si32193 require an ISI transport; SPI is not implemented for these parts\n");

	/* A different patch does not change the compiled converter topology. */
	if (strcasecmp(bom, "lcqc"))
		return dev_err_probe(&spi->dev, -EINVAL,
			"only BOM lcqc with the compiled LCCB configuration is supported\n");

	slic = devm_kzalloc(&spi->dev, sizeof(*slic), GFP_KERNEL);
	if (!slic)
		return -ENOMEM;
	slic->spi = spi;
	mutex_init(&slic->io_lock);
	INIT_DELAYED_WORK(&slic->hook_work, en75xx_si3219x_hook_work);
	INIT_DELAYED_WORK(&slic->ring_work, en75xx_si3219x_ring_work);
	device_property_read_u32(&spi->dev, "airoha,pcm-channel", &slic->pcm_channel);
	device_property_read_u32(&spi->dev, "reg", &slic->line);

	pcm_np = of_parse_phandle(spi->dev.of_node, "airoha,pcm", 0);
	if (!pcm_np)
		return dev_err_probe(&spi->dev, -EINVAL, "missing airoha,pcm\n");
	slic->pcm = en75xx_pcm_get_by_fwnode(of_fwnode_handle(pcm_np));
	of_node_put(pcm_np);
	if (!slic->pcm)
		return -EPROBE_DEFER;

	slic->reset_gpio = devm_gpiod_get_optional(&spi->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(slic->reset_gpio)) {
		ret = PTR_ERR(slic->reset_gpio);
		goto err_pcm;
	}

	spi->mode = SPI_MODE_3;
	spi->bits_per_word = 8;
	ret = spi_setup(spi);
	if (ret)
		goto err_pcm;

	ret = en75xx_si3219x_api_init(slic);
	if (ret)
		goto err_pcm;

	slic->voice_line = en75xx_voice_register_line(&spi->dev, slic->pcm,
		slic->pcm_channel, slic->line, "Si32192", &en75xx_si3219x_ops, slic);
	if (IS_ERR(slic->voice_line)) {
		ret = PTR_ERR(slic->voice_line);
		goto err_api;
	}
	spi_set_drvdata(spi, slic);
	hook = en75xx_si3219x_get_hook(slic);
	slic->last_hook = hook > 0;
	mod_delayed_work(system_wq, &slic->hook_work, msecs_to_jiffies(20));
	dev_info(&spi->dev, "Si3219x ProSLIC initialized on PCM channel %u\n",
		 slic->pcm_channel);
	return 0;

err_api:
	en75xx_si3219x_api_free(slic);
err_pcm:
	en75xx_pcm_put(slic->pcm);
	return ret;
}

static void en75xx_si3219x_remove(struct spi_device *spi)
{
	struct en75xx_si3219x *slic = spi_get_drvdata(spi);

	cancel_delayed_work_sync(&slic->hook_work);
	cancel_delayed_work_sync(&slic->ring_work);
	en75xx_voice_unregister_line(slic->voice_line);
	en75xx_si3219x_api_free(slic);
	en75xx_pcm_put(slic->pcm);
}

static void en75xx_si3219x_shutdown(struct spi_device *spi)
{
	struct en75xx_si3219x *slic = spi_get_drvdata(spi);

	cancel_delayed_work_sync(&slic->hook_work);
	cancel_delayed_work_sync(&slic->ring_work);
	en75xx_si3219x_power_down(slic);
}

static const struct of_device_id en75xx_si3219x_of_match[] = {
	{ .compatible = "silabs,si32192" },
	{ .compatible = "silabs,si3219x" },
	{ }
};
MODULE_DEVICE_TABLE(of, en75xx_si3219x_of_match);

static struct spi_driver en75xx_si3219x_driver = {
	.driver = {
		.name = "en75xx-si3219x",
		.of_match_table = en75xx_si3219x_of_match,
	},
	.probe = en75xx_si3219x_probe,
	.remove = en75xx_si3219x_remove,
	.shutdown = en75xx_si3219x_shutdown,
};
module_spi_driver(en75xx_si3219x_driver);

MODULE_DESCRIPTION("Silicon Labs Si32192/Si3219x FXS driver for EN75xx");
MODULE_FIRMWARE("en75xx/proslic/si3219x_a_lcqc.fw");
MODULE_FIRMWARE("en75xx/proslic/si3219x_a.fw");
MODULE_LICENSE("GPL");
