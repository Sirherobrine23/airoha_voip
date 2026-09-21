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
 * file only ever issues ordinary spi_write()/spi_write_then_read() calls,
 * so it does not care which kind of bus it ends up on: a Si32192/Si32193
 * board wires its DT node under an en75xx-isi-spi controller (which frames
 * these same transactions as ISI underneath), and a Si32184/Si3228x board
 * under a real SPI controller. See en75xx_isi_spi.c.
 *
 * Three register fix-ups below come from the EcoNet mod-slic3 ProSLIC
 * integration in the TP-Link VB430 GPL drop and are applied after
 * ProSLIC_Init(), because the API's presets do not cover them:
 * the PCM format field, the transmit clock edge, and the interrupt mask.
 */
#include <linux/bits.h>
#include <linux/delay.h>
#include <linux/fixp-arith.h>
#include <linux/gpio/consumer.h>
#include <linux/list.h>
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
/* Protect live controls against probe/removal and shared gain presets. */
static DEFINE_MUTEX(si3219x_devices_lock);
static LIST_HEAD(si3219x_devices);

struct en75xx_si3219x {
	struct spi_device *spi;
	struct gpio_desc *reset_gpio;
	struct mutex io_lock;
	struct mutex ram_lock;
	struct list_head node;
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
	unsigned int pcm_slot;
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

static int si3219x_write_ram_unlocked(void *ctrl, uInt8 channel, uInt16 addr,
			     ramData data)
{
	ramData value = data;
	int ret;

	ret = si3219x_wait_ram(ctrl, channel);
	if (ret)
		return RC_SPI_FAIL;

	if (si3219x_write_reg(ctrl, channel, SI3219X_REG_RAM_HI,
			      SI3219X_RAM_HIGH(addr)) != RC_NONE ||
	    si3219x_write_reg(ctrl, channel, SI3219X_REG_RAM_D0,
			      (u8)(value << 3)) != RC_NONE)
		return RC_SPI_FAIL;
	value >>= 5;
	if (si3219x_write_reg(ctrl, channel, SI3219X_REG_RAM_D1, value & 0xff) != RC_NONE)
		return RC_SPI_FAIL;
	value >>= 8;
	if (si3219x_write_reg(ctrl, channel, SI3219X_REG_RAM_D2, value & 0xff) != RC_NONE)
		return RC_SPI_FAIL;
	value >>= 8;
	if (si3219x_write_reg(ctrl, channel, SI3219X_REG_RAM_D3, value & 0xff) != RC_NONE ||
	    si3219x_write_reg(ctrl, channel, SI3219X_REG_RAM_LO, addr & 0xff) != RC_NONE)
		return RC_SPI_FAIL;
	return si3219x_wait_ram(ctrl, channel) ? RC_SPI_FAIL : RC_NONE;
}

static ramData si3219x_read_ram_unlocked(void *ctrl, uInt8 channel, uInt16 addr)
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

static int si3219x_write_ram(void *ctrl, uInt8 channel, uInt16 addr, ramData data)
{
	struct en75xx_si3219x *slic = ctrl;
	int ret;

	mutex_lock(&slic->ram_lock);
	ret = si3219x_write_ram_unlocked(ctrl, channel, addr, data);
	mutex_unlock(&slic->ram_lock);
	return ret;
}

static ramData si3219x_read_ram(void *ctrl, uInt8 channel, uInt16 addr)
{
	struct en75xx_si3219x *slic = ctrl;
	ramData data;

	mutex_lock(&slic->ram_lock);
	data = si3219x_read_ram_unlocked(ctrl, channel, addr);
	mutex_unlock(&slic->ram_lock);
	return data;
}

/*
 * ProSLIC Audio Gain Architecture:
 * - rxgain_db: Analog DAC earpiece gain offset in dB relative to impedance preset.
 *   Vendor ground truth (slic_adaptor_s.c): listenVal = (listenVal >> 1) - 6 (-6 dB).
 *   ProSLIC_AudioGainSetup computes both the coarse gain scale AND the ACEQ
 *   (Audio Equalizer) filter coefficients to maintain a flat frequency response.
 * - txgain_db: Analog ADC mic gain offset in dB. The vendor default is 0, but
 *   that was only ever evaluated against a capture path that was shifted a bit
 *   (see SI3219X_PCM_SLOT_SKEW) and therefore doubled and sign-wrapping. With
 *   the shift corrected, 0 dB puts normal speech on a Beetel handset at
 *   -10.7 dBFS -- about 10 dB hot, clipping on every speech onset faster than
 *   the ALC can pull it back. -10 dB lands speech at -20.4 dBFS, the standard
 *   telephony operating point, with only isolated single-sample glottal peaks
 *   touching full scale. -13 dB was measured too and bought 1 dB of SNR for
 *   2 dB of level, so it was not kept.
 * - rx_acgain / tx_acgain: Optional raw RAM 545/906 and 544 override (0 = disabled,
 *   use ProSLIC_AudioGainSetup). si3219x_apply_gains() reapplies these
 *   after ProSLIC_AudioGainSetup() on init and live changes, so a non-zero
 *   value here silently overrides the corresponding *gain_db above. tx_acgain
 *   is therefore 0: txgain_db governs the capture path.
 */
static int rxgain_db = -6;
static int txgain_db = -10;
static u32 rx_acgain = 0x04000000;
static u32 tx_acgain;

/* Caller holds devices_lock, which also serializes the API gain presets. */
static int si3219x_apply_gains(struct en75xx_si3219x *slic)
{
	if (ProSLIC_AudioGainSetup(slic->channel, rxgain_db, txgain_db,
				  ZSYN_600_0_0_30_0) != RC_NONE)
		return -EIO;
	if (tx_acgain && si3219x_write_ram(slic, 0, 544, tx_acgain) != RC_NONE)
		return -EIO;
	if (rx_acgain &&
	    (si3219x_write_ram(slic, 0, 545, rx_acgain) != RC_NONE ||
	     si3219x_write_ram(slic, 0, 906, rx_acgain) != RC_NONE))
		return -EIO;
	return 0;
}

static int param_set_gain(const char *val, const struct kernel_param *kp)
{
	struct en75xx_si3219x *slic;
	bool raw = kp->arg == &rx_acgain || kp->arg == &tx_acgain;
	u32 raw_value = 0, old_raw = 0;
	int value = 0, old_value = 0, ret;

	if (raw) {
		ret = kstrtouint(val, 0, &raw_value);
		if (ret)
			return ret;
		if (raw_value > 0x0fffffff)
			return -ERANGE;
	} else {
		ret = kstrtoint(val, 0, &value);
		if (ret)
			return ret;
		if (value < PROSLIC_GAIN_MIN || value > PROSLIC_EXTENDED_GAIN_MAX)
			return -ERANGE;
	}

	mutex_lock(&si3219x_devices_lock);
	if (raw) {
		old_raw = *(u32 *)kp->arg;
		*(u32 *)kp->arg = raw_value;
	} else {
		old_value = *(int *)kp->arg;
		*(int *)kp->arg = value;
	}
	ret = 0;
	list_for_each_entry(slic, &si3219x_devices, node) {
		ret = si3219x_apply_gains(slic);
		if (ret)
			break;
	}
	if (ret) {
		if (raw)
			*(u32 *)kp->arg = old_raw;
		else
			*(int *)kp->arg = old_value;
		list_for_each_entry(slic, &si3219x_devices, node) {
			if (si3219x_apply_gains(slic))
				dev_err(&slic->spi->dev, "cannot restore audio gains\n");
		}
	}
	mutex_unlock(&si3219x_devices_lock);
	return ret;
}

static const struct kernel_param_ops gain_db_ops = {
	.set = param_set_gain,
	.get = param_get_int,
};
static const struct kernel_param_ops gain_raw_ops = {
	.set = param_set_gain,
	.get = param_get_uint,
};
module_param_cb(rxgain_db, &gain_db_ops, &rxgain_db, 0644);
MODULE_PARM_DESC(rxgain_db, "RX gain in dB (-30..9); raw override takes precedence");
module_param_cb(txgain_db, &gain_db_ops, &txgain_db, 0644);
MODULE_PARM_DESC(txgain_db, "TX gain in dB (-30..9, default -10); raw override takes precedence");
module_param_cb(rx_acgain, &gain_raw_ops, &rx_acgain, 0644);
MODULE_PARM_DESC(rx_acgain, "Raw RX gain override (0 restores calculated RX gain)");
module_param_cb(tx_acgain, &gain_raw_ops, &tx_acgain, 0644);
MODULE_PARM_DESC(tx_acgain, "Raw TX gain override (0 restores calculated TX gain)");

/*
 * PCM timeslot fine adjustment, in PCLK bits.
 *
 * The ProSLIC counts its transmit and receive start positions in PCLK
 * cycles from frame sync; the PCM engine counts the bit offset it gave
 * the channel. If the two disagree by a single bit the audio still
 * flows and idle line looks perfectly clean, but every captured sample
 * is shifted up: the sign bit falls off the top and the idle bus fills
 * the bottom. Quiet audio survives, anything loud wraps sign and turns
 * to harsh noise. The signature is a capture whose LSB is always zero.
 *
 * Named from this driver's point of view, not the ProSLIC's:
 * capture_slot_adj moves what the ProSLIC transmits (our RX), and
 * playback_slot_adj moves what it receives (our TX).
 */
static int capture_slot_adj;
static int playback_slot_adj;

/*
 * The ProSLIC's PCMTX/PCMRX registers hold the number of PCLK cycles to
 * wait after frame sync, so the first data bit lands on the cycle after
 * the count matches. The PCM engine's timeslot table instead gives the
 * bit position where the data itself starts. The two differ by exactly
 * one PCLK, and feeding the engine's offset in verbatim shifts every
 * sample by a bit in both directions.
 *
 * Measured on an XC220-G3v with the offsets above: at +0 the captured
 * LSB is always zero and the mean is exactly 2x, at +1 the LSB is
 * random (49.6% odd) and the level is exactly half. Capture was
 * therefore arriving doubled with a dead LSB and the sign bit shifted
 * off the top, so anything loud wrapped and turned to noise; playback
 * was shifted the same way, which put wrapped audio on the earpiece and
 * is what made the handset howl.
 */
#define SI3219X_PCM_SLOT_SKEW	1

static int si3219x_apply_timeslots(struct en75xx_si3219x *slic)
{
	int base = (int)slic->pcm_slot + SI3219X_PCM_SLOT_SKEW;
	int tx = base + capture_slot_adj;
	int rx = base + playback_slot_adj;

	if (tx < 0 || rx < 0 || tx > 0x3ff || rx > 0x3ff)
		return -EINVAL;

	return ProSLIC_PCMTimeSlotSetup(slic->channel, rx, tx) == RC_NONE ?
		0 : -EIO;
}

static int param_set_slot_adj(const char *val, const struct kernel_param *kp)
{
	struct en75xx_si3219x *slic;
	int value, old, ret = kstrtoint(val, 0, &value);

	if (ret)
		return ret;
	if (value < -1023 || value > 1023)
		return -ERANGE;
	mutex_lock(&si3219x_devices_lock);
	old = *(int *)kp->arg;
	*(int *)kp->arg = value;
	list_for_each_entry(slic, &si3219x_devices, node) {
		ret = si3219x_apply_timeslots(slic);
		if (ret)
			break;
	}
	if (ret) {
		*(int *)kp->arg = old;
		list_for_each_entry(slic, &si3219x_devices, node) {
			if (si3219x_apply_timeslots(slic))
				dev_err(&slic->spi->dev, "cannot restore PCM timeslots\n");
		}
	}
	mutex_unlock(&si3219x_devices_lock);
	return ret;
}

static const struct kernel_param_ops slot_adj_ops = {
	.set = param_set_slot_adj,
	.get = param_get_int,
};

module_param_cb(capture_slot_adj, &slot_adj_ops, &capture_slot_adj, 0644);
MODULE_PARM_DESC(capture_slot_adj, "PCLK-bit offset applied to capture (ProSLIC TX) timeslot");
module_param_cb(playback_slot_adj, &slot_adj_ops, &playback_slot_adj, 0644);
MODULE_PARM_DESC(playback_slot_adj, "PCLK-bit offset applied to playback (ProSLIC RX) timeslot");

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

/*
 * Hardware tone generation.
 *
 * The ProSLIC has two sine oscillators that can be summed onto the line
 * with an on/off cadence of their own, which keeps a continuous tone
 * entirely off the host: no software synthesis, no 10 ms frames pushed
 * through the PCM ring for as long as the tone plays.
 *
 * The coefficients follow the same law as Silicon Labs' own presets in
 * si3219x_LCCB_constants.c, recovered from them:
 *
 *	OSCxFREQ  = cos(2*pi*f/8000) * 2^27, as a 29-bit two's complement
 *		    value rounded to a multiple of 2^16
 *	OSCxAMP   = 46684600 * 10^(level/20) * tan(pi*f/8000), rounded to
 *		    a multiple of 2^12
 *	OxTA/OxTI = on/off time, in 125 us ticks
 *
 * Checked against all nine presets: exact from 350 to 1004 Hz, and
 * within 0.75 Hz / 0.03 dB at 2130 and 2750 Hz, the residual there
 * being the config tool's own per-frequency correction.
 */
#define SI3219X_TONE_MAX_HZ	3400	/* tan() runs away towards Nyquist */
#define SI3219X_TONE_TWOPI	256000	/* fixp-arith full turn; see below */
#define SI3219X_TONE_AMP_0DBM	46684600
#define SI3219X_RAM_MASK	0x1fffffff

/*
 * OMODE routing. Bit 1 puts oscillator 1 on the line and bit 5 does the
 * same for oscillator 2: ProSLIC_EnableCID() sets bit 1 to get caller-ID
 * FSK onto the line, and the vendor's line-test code writes OMODE=2 to
 * drive its measurement tone there. Bits 2 and 6 additionally route each
 * oscillator into the PCM transmit stream, which the Silicon Labs tone
 * presets do (OMODE 0x66) and we do not want -- a tone mixed into the
 * transmit path is a tone Asterisk's DTMF detector has to listen
 * through. Tunable because the routing bits are inferred from how the
 * vendor code uses them rather than from a register map.
 */
static u8 tone_omode = 0x22;
module_param(tone_omode, byte, 0444);
MODULE_PARM_DESC(tone_omode, "OMODE tone routing (0x22 line only, 0x66 line + PCM TX)");

/*
 * fixp_sin32_rad(x, twopi) is sin(2*pi*x/twopi) in Q31. It interpolates
 * a one-degree table with a step of twopi/360, so twopi wants to be a
 * large multiple of 360 for the step not to lose precision -- but it is
 * also BUG_ON()ed above 2^18. 256000 = 8000 * 32 satisfies both, so a
 * frequency in Hz is passed as f * 32.
 */
static u32 si3219x_osc_freq(unsigned int hz)
{
	s32 c = fixp_cos32_rad(hz * 32, SI3219X_TONE_TWOPI);

	return (u32)(DIV_ROUND_CLOSEST(c / 16, 65536) * 65536) & SI3219X_RAM_MASK;
}

/* 10^(level/20) in Q30, for whole dBm from 0 down to -49. */
static u32 si3219x_db_to_gain(int level_dbm)
{
	static const u32 per_10db[5] = {
		0x40000000, 0x143d1362, 0x06666666, 0x02061b8a, 0x00a3d70a,
	};
	static const u32 per_1db[10] = {
		0x40000000, 0x390a4160, 0x32d64618, 0x2d4efbd6, 0x28619aea,
		0x23fd6678, 0x2013739e, 0x1c9676c7, 0x197a967f, 0x16b54338,
	};
	unsigned int n = -clamp(level_dbm, -49, 0);

	return (u32)(((u64)per_10db[n / 10] * per_1db[n % 10]) >> 30);
}

static u32 si3219x_osc_amp(unsigned int hz, int level_dbm)
{
	/* half the angle, so 16 rather than 32 ticks per Hz */
	u32 sin_half = fixp_sin32_rad(hz * 16, SI3219X_TONE_TWOPI);
	u32 cos_half = fixp_cos32_rad(hz * 16, SI3219X_TONE_TWOPI);
	u64 amp;

	amp = ((u64)SI3219X_TONE_AMP_0DBM * si3219x_db_to_gain(level_dbm)) >> 30;
	amp *= sin_half;
	do_div(amp, cos_half);

	return (DIV_ROUND_CLOSEST((u32)amp, 4096) * 4096) & SI3219X_RAM_MASK;
}

/* Cadence timers are 16-bit counts of 125 us ticks. */
static u16 si3219x_tone_ticks(unsigned int ms)
{
	return min_t(unsigned int, ms, 8191) * 8;
}

static void si3219x_osc_setup(Oscillator_Cfg *osc, unsigned int hz,
			      int level_dbm, u16 on, u16 off)
{
	osc->freq = si3219x_osc_freq(hz);
	osc->amp = si3219x_osc_amp(hz, level_dbm);
	osc->talo = on & 0xff;
	osc->tahi = on >> 8;
	osc->tilo = off & 0xff;
	osc->tihi = off >> 8;
}

static int en75xx_si3219x_set_tone_unlocked(void *priv,
				   const struct en75xx_voice_tone *tone)
{
	struct en75xx_si3219x *slic = priv;
	ProSLIC_Tone_Cfg cfg = {};
	u16 on, off;

	if (tone->freq1_hz > SI3219X_TONE_MAX_HZ ||
	    tone->freq2_hz > SI3219X_TONE_MAX_HZ ||
	    tone->level_dbm > 0 || tone->level_dbm < -49 ||
	    tone->on_ms > 8191 || tone->off_ms > 8191 || tone->reserved)
		return -EINVAL;

	/*
	 * ProSLIC_ToneGenStart() ORs into OCON, so the previous tone's
	 * enable and timer bits have to go first or a continuous tone
	 * inherits the cadence of the one before it.
	 */
	if (ProSLIC_ToneGenStop(slic->channel) != RC_NONE)
		return -EIO;

	if (!tone->freq1_hz && !tone->freq2_hz)
		return 0;

	on = si3219x_tone_ticks(tone->on_ms);
	off = si3219x_tone_ticks(tone->off_ms);

	if (tone->freq1_hz) {
		si3219x_osc_setup(&cfg.osc1, tone->freq1_hz, tone->level_dbm,
				  on, off);
		cfg.omode |= tone_omode & 0x0f;
	}
	if (tone->freq2_hz) {
		si3219x_osc_setup(&cfg.osc2, tone->freq2_hz, tone->level_dbm,
				  on, off);
		cfg.omode |= tone_omode & 0xf0;
	}

	if (ProSLIC_ToneGenSetupPtr(slic->channel, &cfg) != RC_NONE)
		return -EIO;

	return ProSLIC_ToneGenStart(slic->channel, on != 0) == RC_NONE ? 0 : -EIO;
}

static int en75xx_si3219x_set_tone(void *priv,
				 const struct en75xx_voice_tone *tone)
{
	struct en75xx_si3219x *slic = priv;
	int ret;

	mutex_lock(&si3219x_devices_lock);
	ret = en75xx_si3219x_set_tone_unlocked(slic, tone);
	mutex_unlock(&si3219x_devices_lock);
	return ret;
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
	.set_tone = en75xx_si3219x_set_tone,
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

	/*
	 * DC-DC converter powersave (bit 3, PROSLIC_REG_ENHANCE |= 0x08) was
	 * added and tested here mid-session -- user confirmed the audio
	 * symptom was identical before and after adding it, so it's ruled
	 * out as a factor and pulled back out to keep the variable set
	 * simple while the real cause is still open. It's still a real,
	 * vendor-matching improvement (rcS sets it unconditionally on every
	 * boot; DC-DC ripple coupling into the line is a real mechanism,
	 * just not this one) -- RE-ADD once the current investigation
	 * concludes, as a read-modify-write (val | 0x08), not a blind
	 * overwrite: ProSLIC_Init() already sets bit 0 of this same
	 * register for narrowband/wideband HPF config, which a blind write
	 * like the vendor's own "echo 0x8 > slicRegister" would clobber.
	 */

	/*
	 * si3219x_apply_gains() has applied the calibrated gain/ACEQ setup
	 * and any raw gain overrides. Report the resulting hardware values.
	 */
	dev_info(&slic->spi->dev,
		 "ProSLIC audio gain: rxgain=%d dB txgain=%d dB TXACGAIN=0x%08x RXACGAIN=0x%08x (PCMMODE=0x%02x ENHANCE=0x%02x)\n",
		 rxgain_db, txgain_db,
		 (u32)si3219x_read_ram(slic, 0, 544),
		 (u32)si3219x_read_ram(slic, 0, 545),
		 si3219x_read_reg(slic, 0, PROSLIC_REG_PCMMODE),
		 si3219x_read_reg(slic, 0, PROSLIC_REG_ENHANCE));
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
	u8 reg3 = 0, reg0 = 0;
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
	slic->pcm_slot = slot;

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

	/*
	 * Ensure the PCM engine is driving PCLK and FSYNC so the ProSLIC
	 * can synchronize and complete its internal reset (Reset C).
	 */
	en75xx_pcm_iface_kick(slic->pcm);

	mutex_lock(&si3219x_init_lock);
	en75xx_proslic_fw_to_patch(&slic->fw, &si3219xPatchRevALCQC);
	en75xx_proslic_fw_to_patch(&slic->fw, &RevAPatch);

	SiVoice_Reset(slic->channel);

	/*
	 * Diagnostic only: 0xff can be a latched status or a failed read.
	 * ProSLIC_Init clears and verifies MSTRSTAT and tests register/RAM I/O.
	 */
	reg3 = si3219x_read_reg(slic, 0, PROSLIC_REG_MSTRSTAT);
	reg0 = si3219x_read_reg(slic, 0, PROSLIC_REG_ID);

	dev_info(&slic->spi->dev,
		 "Pre-init registers: MSTRSTAT=0x%02x REG0=0x%02x (part=0x%x rev=0x%x)\n",
		 reg3, reg0, (reg0 >> 3) & 0x7, reg0 & 0x7);

	slic->init_attempted = true;
	ret = ProSLIC_Init(slic->channel_ptrs, 1);
	/* No shared symbol may retain pointers into this device's firmware. */
	memset(&si3219xPatchRevALCQC, 0, sizeof(si3219xPatchRevALCQC));
	memset(&RevAPatch, 0, sizeof(RevAPatch));
	mutex_unlock(&si3219x_init_lock);

	dev_info(&slic->spi->dev,
		 "ProSLIC_Init result: ret=%d chan.error=%d channelEnable=%d chipType=%u chipRev=%u\n",
		 ret, slic->channel->error, slic->channel->channelEnable,
		 slic->device->chipType, slic->device->chipRev);

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
	    /*
	     * ZSYN_600_0_0_30_0 (generic 600R resistive), not a regional
	     * complex-impedance preset. Earlier revisions of this file tried
	     * ZSYN_220_820_120_30_0 then ZSYN_270_750_150_30_0 on the theory
	     * that a named "India/TEC" preset must be the real vendor match
	     * -- that was reasoning from the Silicon Labs preset *names*,
	     * not from vendor firmware behavior. Ground truth from the
	     * actual EN7528 SDK (silab_paramReset() in
	     * DSP/MTK/mod-slic3/src/silab/slic_adaptor_s.c, matched via the
	     * build path string embedded in this board's own
	     * slic3_silicon_si32192.ko) shows the country-code switch has no
	     * real per-country branches at all: both the C_DEF case and the
	     * default case set impCountryIdx = 0 unconditionally, with the
	     * source's own comment reading "//ZSYN_600_0_0_30_0". The
	     * vendor's real firmware never selects a regional impedance on
	     * this chip/SDK combination -- it's always generic 600R,
	     * regardless of locale. Matching that here.
	     */
	    ProSLIC_ZsynthSetup(slic->channel, ZSYN_600_0_0_30_0) != RC_NONE ||
	    /*
	     * Apply calibrated audio gains (RX -6 dB, TX -10 dB by default)
	     * via ProSLIC_AudioGainSetup, which computes coarse/fine gain scaling
	     * and configures the ACEQ equalizing filter coefficients for the
	     * selected impedance preset (ZSYN_600_0_0_30_0).
	     */
	    si3219x_apply_gains(slic) != 0 ||
	    ProSLIC_RingSetup(slic->channel, DEFAULT_RINGING) != RC_NONE ||
	    ProSLIC_PCMSetup(slic->channel, PCM_16LIN) != RC_NONE ||
	    si3219x_apply_timeslots(slic) != 0 ||
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

	/* A different patch does not change the compiled converter topology. */
	if (strcasecmp(bom, "lcqc"))
		return dev_err_probe(&spi->dev, -EINVAL,
			"only BOM lcqc with the compiled LCCB configuration is supported\n");

	slic = devm_kzalloc(&spi->dev, sizeof(*slic), GFP_KERNEL);
	if (!slic)
		return -ENOMEM;
	slic->spi = spi;
	mutex_init(&slic->io_lock);
	mutex_init(&slic->ram_lock);
	INIT_LIST_HEAD(&slic->node);
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

	mutex_lock(&si3219x_devices_lock);
	ret = en75xx_si3219x_api_init(slic);
	if (ret) {
		mutex_unlock(&si3219x_devices_lock);
		goto err_pcm;
	}

	slic->voice_line = en75xx_voice_register_line(&spi->dev, slic->pcm,
		slic->pcm_channel, slic->line, "Si32192", &en75xx_si3219x_ops, slic);
	if (IS_ERR(slic->voice_line)) {
		ret = PTR_ERR(slic->voice_line);
		mutex_unlock(&si3219x_devices_lock);
		goto err_api;
	}
	spi_set_drvdata(spi, slic);
	list_add_tail(&slic->node, &si3219x_devices);
	mutex_unlock(&si3219x_devices_lock);
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

	mutex_lock(&si3219x_devices_lock);
	list_del_init(&slic->node);
	mutex_unlock(&si3219x_devices_lock);

	cancel_delayed_work_sync(&slic->hook_work);
	cancel_delayed_work_sync(&slic->ring_work);
	en75xx_voice_unregister_line(slic->voice_line);
	en75xx_si3219x_api_free(slic);
	en75xx_pcm_put(slic->pcm);
}

static void en75xx_si3219x_shutdown(struct spi_device *spi)
{
	struct en75xx_si3219x *slic = spi_get_drvdata(spi);

	mutex_lock(&si3219x_devices_lock);
	list_del_init(&slic->node);
	mutex_unlock(&si3219x_devices_lock);

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

static const struct spi_device_id en75xx_si3219x_id[] = {
	{ "si32192", 0 },
	{ "si3219x", 0 },
	{ }
};
MODULE_DEVICE_TABLE(spi, en75xx_si3219x_id);

static struct spi_driver en75xx_si3219x_driver = {
	.driver = {
		.name = "en75xx-si3219x",
		.of_match_table = en75xx_si3219x_of_match,
	},
	.id_table = en75xx_si3219x_id,
	.probe = en75xx_si3219x_probe,
	.remove = en75xx_si3219x_remove,
	.shutdown = en75xx_si3219x_shutdown,
};
module_spi_driver(en75xx_si3219x_driver);

MODULE_DESCRIPTION("Silicon Labs Si32192/Si3219x FXS driver for EN75xx");
MODULE_FIRMWARE("en75xx/proslic/si3219x_a_lcqc.fw");
MODULE_FIRMWARE("en75xx/proslic/si3219x_a.fw");
MODULE_LICENSE("GPL");
