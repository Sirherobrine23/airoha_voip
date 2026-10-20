// SPDX-License-Identifier: GPL-2.0
/* MaxLinear DXS TAPI to EN75xx PCM/FXS adapter. */
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/workqueue.h>
#include <en75xx_voice.h>
#include <en75xx_dxs_kernel.h>
#include <ifx_types.h>
#include <drv_tapi_io.h>
#include <drv_tapi_kio.h>
#include <drv_dxs_io.h>

#define DXS_LINES 2

static char *bringup_mode;
module_param(bringup_mode, charp, 0444);
MODULE_PARM_DESC(bringup_mode, "Override DTS mode: identify, firmware, start, bbd, calibrate or voice");

static unsigned int stage_wait_ms = 1000;
module_param(stage_wait_ms, uint, 0444);
MODULE_PARM_DESC(stage_wait_ms, "Diagnostic stage settling time (100..60000 ms); unused in voice mode");

struct en75xx_dxs;
struct en75xx_dxs_line {
	struct en75xx_dxs *dxs;
	IFX_TAPI_KIO_HANDLE handle;
	struct en75xx_voice_line *voice;
	u32 pcm_channel;
	bool pcm_active;
	bool hook;
	bool notified_hook;
	bool ringing;
};

struct en75xx_dxs {
	struct device *dev;
	struct en75xx_pcm *pcm;
	IFX_TAPI_KIO_HANDLE handle;
	struct en75xx_dxs_line line[DXS_LINES];
	struct delayed_work events;
	struct mutex lock;
	IFX_TAPI_PCM_IF_CFG_t pcm_cfg;
	unsigned int index;
	unsigned int count;
	bool basic;
	bool started;
	bool stopping;
	u32 faults;
	u32 loaded_fw;
};

static int dxs_ioctl(struct en75xx_dxs *dxs, IFX_TAPI_KIO_HANDLE handle,
		     unsigned int cmd, void *arg)
{
	int ret = ifx_tapi_kioctl(handle, cmd, arg);

	if (ret) {
		dev_err_ratelimited(dxs->dev, "TAPI command %#x failed: %#x\n", cmd, ret);
		return -EIO;
	}
	return 0;
}

static int dxs_scalar(struct en75xx_dxs *dxs, IFX_TAPI_KIO_HANDLE handle,
		      unsigned int cmd, unsigned long value)
{
	/* TAPI3 commands described as TAPI_ARG_INTEGER take the value itself. */
	return dxs_ioctl(dxs, handle, cmd, (void *)value);
}

static int dxs_hook(void *priv)
{
	struct en75xx_dxs_line *line = priv;
	struct en75xx_dxs *dxs = line->dxs;
	IFX_TAPI_LINE_HOOK_STATUS_GET_t hook = 0;
	int ret;

	mutex_lock(&dxs->lock);
	ret = dxs_ioctl(dxs, line->handle, IFX_TAPI_LINE_HOOK_STATUS_GET, &hook);
	if (!ret)
		WRITE_ONCE(line->hook, !!hook);
	mutex_unlock(&dxs->lock);
	return ret ? ret : !!hook;
}

static int dxs_feed(void *priv, enum en75xx_voice_linefeed feed)
{
	struct en75xx_dxs_line *line = priv;
	struct en75xx_dxs *dxs = line->dxs;
	unsigned int mode;
	int ret;

	switch (feed) {
	case EN75XX_VOICE_LINEFEED_OPEN: mode = IFX_TAPI_LINE_FEED_DISABLED; break;
	case EN75XX_VOICE_LINEFEED_STANDBY: mode = IFX_TAPI_LINE_FEED_STANDBY; break;
	case EN75XX_VOICE_LINEFEED_ACTIVE: mode = IFX_TAPI_LINE_FEED_ACTIVE; break;
	case EN75XX_VOICE_LINEFEED_REVERSE: mode = IFX_TAPI_LINE_FEED_ACTIVE_REV; break;
	default: return -EINVAL;
	}
	mutex_lock(&dxs->lock);
	if (line->ringing) {
		ret = dxs_ioctl(dxs, line->handle, IFX_TAPI_RING_STOP, NULL);
		if (ret) {
			mutex_unlock(&dxs->lock);
			return ret;
		}
		line->ringing = false;
	}
	ret = dxs_scalar(dxs, line->handle, IFX_TAPI_LINE_FEED_SET, mode);
	mutex_unlock(&dxs->lock);
	return ret;
}

static int dxs_ring(void *priv, bool enable, unsigned int on_ms, unsigned int off_ms)
{
	struct en75xx_dxs_line *line = priv;
	struct en75xx_dxs *dxs = line->dxs;
	IFX_TAPI_RING_CADENCE_t cadence = {0};
	unsigned int on, off, i;
	int ret = 0;

	/* TAPI cadence is in 50 ms units, with a maximum of 320 bits. */
	if (enable) {
		if (on_ms > 16000 || off_ms > 16000)
			return -ERANGE;
		on = DIV_ROUND_UP(on_ms ? on_ms : 1000, 50);
		off = DIV_ROUND_UP(off_ms ? off_ms : 4000, 50);
		if (on > 320 || off > 320 || on + off > 320)
			return -ERANGE;
		cadence.nr = on + off;
		for (i = 0; i < on; i++)
			cadence.data[i / 8] |= BIT(7 - i % 8);
	}
	mutex_lock(&dxs->lock);
	if (enable && READ_ONCE(line->hook)) {
		ret = -EBUSY;
		goto out;
	}
	if (line->ringing) {
		ret = dxs_ioctl(dxs, line->handle, IFX_TAPI_RING_STOP, NULL);
		if (ret)
			goto out;
		line->ringing = false;
	}
	if (enable) {
		ret = dxs_ioctl(dxs, line->handle, IFX_TAPI_RING_CADENCE_HR_SET, &cadence);
		if (!ret)
			ret = dxs_ioctl(dxs, line->handle, IFX_TAPI_RING_START, NULL);
		if (!ret)
			line->ringing = true;
	}
out:
	mutex_unlock(&dxs->lock);
	return ret;
}

static int dxs_faults(void *priv, u32 *faults)
{
	struct en75xx_dxs_line *line = priv;
	*faults = READ_ONCE(line->dxs->faults);
	return 0;
}

static const struct en75xx_voice_slic_ops dxs_slic_ops = {
	.get_hook = dxs_hook,
	.set_linefeed = dxs_feed,
	.ring = dxs_ring,
	.get_faults = dxs_faults,
};

static void dxs_events(struct work_struct *work)
{
	struct en75xx_dxs *dxs = container_of(to_delayed_work(work), struct en75xx_dxs, events);
	IFX_TAPI_EVENT_t event;
	unsigned int n, i;
	bool hooks[DXS_LINES], changed[DXS_LINES];

	mutex_lock(&dxs->lock);
	for (n = 0; n < 32; n++) {
		memset(&event, 0, sizeof(event));
		event.ch = IFX_TAPI_EVENT_ALL_CHANNELS;
		if (dxs_ioctl(dxs, dxs->handle, IFX_TAPI_EVENT_GET, &event) ||
		    event.id == IFX_TAPI_EVENT_NONE)
			break;
		if (event.id == IFX_TAPI_EVENT_FXS_OFFHOOK && event.ch < dxs->count) {
			dxs->line[event.ch].hook = true;
			dxs->line[event.ch].ringing = false; /* TAPI ring trip stops ringing */
		} else if (event.id == IFX_TAPI_EVENT_FXS_ONHOOK && event.ch < dxs->count) {
			dxs->line[event.ch].hook = false;
		} else if ((event.id & 0xffff0000) == IFX_TAPI_EVENT_TYPE_FAULT_HW ||
			   (event.id & 0xffff0000) == IFX_TAPI_EVENT_TYPE_FAULT_LINE) {
			dxs->faults = event.id;
			dev_warn_ratelimited(dxs->dev, "DXS fault event %#x on channel %u\n", event.id, event.ch);
		}
		if (!event.more)
			break;
	}
	for (i = 0; i < dxs->count; i++) {
		IFX_TAPI_LINE_HOOK_STATUS_GET_t hook = 0;
		if (!dxs_ioctl(dxs, dxs->line[i].handle, IFX_TAPI_LINE_HOOK_STATUS_GET, &hook))
			dxs->line[i].hook = !!hook;
		hooks[i] = dxs->line[i].hook;
		changed[i] = hooks[i] != dxs->line[i].notified_hook;
		dxs->line[i].notified_hook = hooks[i];
	}
	mutex_unlock(&dxs->lock);
	/* Do not call voice core while holding the SLIC mutex. */
	for (i = 0; i < dxs->count; i++)
		if (changed[i] && dxs->line[i].voice)
			en75xx_voice_hook_changed(dxs->line[i].voice, hooks[i]);
	if (!READ_ONCE(dxs->stopping))
		schedule_delayed_work(&dxs->events, msecs_to_jiffies(20));
}

static void dxs_shutdown(struct en75xx_dxs *dxs)
{
	unsigned int i;

	WRITE_ONCE(dxs->stopping, true);
	cancel_delayed_work_sync(&dxs->events);
	for (i = 0; i < dxs->count; i++) {
		struct en75xx_dxs_line *line = &dxs->line[i];
		if (line->voice) {
			en75xx_voice_unregister_line(line->voice);
			line->voice = NULL;
		}
		if (line->handle) {
			if (dxs->started) {
				dxs_ioctl(dxs, line->handle, IFX_TAPI_RING_STOP, NULL);
				dxs_scalar(dxs, line->handle, IFX_TAPI_LINE_FEED_SET, IFX_TAPI_LINE_FEED_DISABLED);
				if (line->pcm_active)
					dxs_scalar(dxs, line->handle, IFX_TAPI_PCM_ACTIVATION_SET, IFX_DISABLE);
			}
			ifx_tapi_kclose(line->handle);
			line->handle = NULL;
		}
	}
	if (dxs->handle) {
		if (dxs->started)
			dxs_ioctl(dxs, dxs->handle, IFX_TAPI_DEV_STOP, NULL);
		if (dxs->basic)
			dxs_ioctl(dxs, dxs->handle, FIO_DXS_DEV_RESET, NULL);
		ifx_tapi_kclose(dxs->handle);
		dxs->handle = NULL;
	}
	if (dxs->pcm) {
		en75xx_pcm_put(dxs->pcm);
		dxs->pcm = NULL;
	}
}

static int dxs_pcm_settings(struct en75xx_dxs *dxs)
{
	IFX_TAPI_PCM_IF_CFG_t *cfg = &dxs->pcm_cfg;
	struct device_node *np = dxs->dev->of_node;
	u32 freq, edge, offset;

	/* Require board settings instead of silently inheriting ProSLIC timing. */
	if (of_property_read_u32(np, "clock-frequency", &freq) ||
	    of_property_read_u32(np, "maxlinear,pcm-tx-edge", &edge) || edge > 1)
		return -EINVAL;
	cfg->nSlopeTX = edge;
	if (of_property_read_u32(np, "maxlinear,pcm-rx-edge", &edge) || edge > 1)
		return -EINVAL;
	cfg->nSlopeRX = edge;
	/* Bit offsets are separate from the byte slots configured below.
	 * The MitraStar OEM IntelSlicInit programs TX=7 and RX=1.
	 * Keep zero for boards without an explicit offset property.
	 */
	if (of_find_property(np, "maxlinear,pcm-tx-offset", NULL)) {
		if (of_property_read_u32(np, "maxlinear,pcm-tx-offset", &offset) || offset > 7)
			return -EINVAL;
		cfg->nOffsetTX = offset;
	}
	if (of_find_property(np, "maxlinear,pcm-rx-offset", NULL)) {
		if (of_property_read_u32(np, "maxlinear,pcm-rx-offset", &offset) || offset > 7)
			return -EINVAL;
		cfg->nOffsetRX = offset;
	}
	cfg->nOpMode = IFX_TAPI_PCM_IF_MODE_SLAVE;
	cfg->nDrive = IFX_TAPI_PCM_IF_DRIVE_ENTIRE;
	cfg->nTsSync = IFX_TAPI_PCM_IF_TS_SYNC_FRAME;
	switch (freq) {
	case 1024000: cfg->nDCLFreq = IFX_TAPI_PCM_IF_DCLFREQ_1024; break;
	case 2048000: cfg->nDCLFreq = IFX_TAPI_PCM_IF_DCLFREQ_2048; break;
	case 4096000: cfg->nDCLFreq = IFX_TAPI_PCM_IF_DCLFREQ_4096; break;
	case 8192000: cfg->nDCLFreq = IFX_TAPI_PCM_IF_DCLFREQ_8192; break;
	default: return -EINVAL;
	}
	dev_info(dxs->dev, "DXS PCM: %u Hz, TX/RX edges %u/%u, offsets %u/%u bits\n",
		 freq, (unsigned int)cfg->nSlopeTX, (unsigned int)cfg->nSlopeRX,
		 (unsigned int)cfg->nOffsetTX, (unsigned int)cfg->nOffsetRX);
	return 0;
}

static int dxs_configure_pcm(struct en75xx_dxs *dxs)
{
	IFX_TAPI_PCM_CFG_t slot = {0};
	unsigned int i;
	int txbit, rxbit, ret;

	ret = dxs_ioctl(dxs, dxs->handle, IFX_TAPI_PCM_IF_CFG_SET, &dxs->pcm_cfg);
	if (ret)
		return ret;
	for (i = 0; i < dxs->count; i++) {
		struct en75xx_dxs_line *line = &dxs->line[i];
		char name[24];
		snprintf(name, sizeof(name), "dxs%u", (dxs->index + 1) * 10 + i + 1);
		line->handle = ifx_tapi_kopen(name);
		if (!line->handle)
			return -ENODEV;
		ret = en75xx_pcm_channel_offsets(dxs->pcm, line->pcm_channel, &txbit, &rxbit);
		if (ret || txbit % 8 || rxbit % 8)
			return -EINVAL;
		/* TAPI slots count bytes. DXS RX follows SoC TX and vice versa. */
		slot.nTimeslotRX = txbit / 8;
		slot.nTimeslotTX = rxbit / 8;
		slot.nResolution = IFX_TAPI_PCM_RES_NB_LINEAR_16BIT;
		ret = dxs_ioctl(dxs, line->handle, IFX_TAPI_PCM_CFG_SET, &slot);
		if (!ret)
			ret = dxs_scalar(dxs, line->handle, IFX_TAPI_PCM_ACTIVATION_SET, IFX_ENABLE);
		if (ret)
			return ret;
		line->pcm_active = true;
		ret = dxs->pcm->line_ops->set_format(dxs->pcm, line->pcm_channel,
						    EN75XX_PCM_CODEC_LINEAR16, false);
		if (ret)
			return ret;
		dev_info(dxs->dev, "FXS %u: PCM channel %u, TX/RX bits %d/%d (DXS RX/TX slots %d/%d)\n",
			 i, line->pcm_channel, txbit, rxbit, txbit / 8, rxbit / 8);
	}
	return 0;
}

/* This stage only uses control/mailbox commands. No BBD or linefeed. */
static int dxs_boot_firmware(struct en75xx_dxs *dxs, const struct firmware *fw)
{
	u32 hw, version;
	unsigned int count;
	int ret;

	ret = en75xx_dxs_kernel_version(dxs->index, &hw, &version, &count);
	if (ret)
		return ret;
	dev_info(dxs->dev, "DXS ROM: HW %#x FW %#x, %u FXS\n", hw, version, count);
	if (count != dxs->count) {
		dev_err(dxs->dev, "DXS has %u FXS; DTS describes %u\n", count, dxs->count);
		return -ENODEV;
	}
	ret = en75xx_dxs_kernel_firmware(dxs->index, fw->data, fw->size);
	if (ret)
		return ret;
	ret = en75xx_dxs_kernel_version(dxs->index, &hw, &version, &count);
	if (ret)
		return ret;
	if (count != dxs->count)
		return -ENODEV;
	dxs->loaded_fw = version;
	dev_info(dxs->dev, "DXS firmware running: HW %#x FW %#x, %u FXS\n", hw, version, count);
	return 0;
}

/* Always query the chip; never accept the driver's cached firmware version. */
static int dxs_verify_firmware(struct en75xx_dxs *dxs, const char *stage)
{
	u32 hw, version;
	unsigned int count;
	int ret;

	ret = en75xx_dxs_kernel_version(dxs->index, &hw, &version, &count);
	if (ret) {
		dev_err(dxs->dev, "DXS %s: version query failed: %d\n", stage, ret);
		return ret;
	}
	dev_info(dxs->dev, "DXS %s: HW %#x FW %#x, %u FXS (expected FW %#x)\n",
		 stage, hw, version, count, dxs->loaded_fw);
	if (version != dxs->loaded_fw || count != dxs->count) {
		dev_err(dxs->dev, "DXS %s: firmware identity changed; stopping bring-up\n", stage);
		return -EIO;
	}
	return 0;
}

static int dxs_wait_stage(struct en75xx_dxs *dxs, const char *stage)
{
	dev_info(dxs->dev, "DXS %s: waiting %u ms before fresh version query\n",
		 stage, stage_wait_ms);
	msleep(stage_wait_ms);
	return dxs_verify_firmware(dxs, stage);
}

static int en75xx_dxs_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev, *supplier;
	struct device_node *np;
	struct en75xx_dxs *dxs;
	const struct firmware *fw = NULL, *bbd = NULL;
	const char *mode = "identify", *fwname, *bbdname, *topology;
	IFX_TAPI_DEV_START_CFG_t start = {0};
	u32 hw_version, fw_version;
	unsigned int fxs_count;
	u32 channels[DXS_LINES];
	char name[24];
	int count, ret, i, txbit, rxbit;
	bool voice, start_only, bbd_only, calibrate_only;

	dxs = devm_kzalloc(dev, sizeof(*dxs), GFP_KERNEL);
	if (!dxs)
		return -ENOMEM;
	dxs->dev = dev;
	mutex_init(&dxs->lock);
	INIT_DELAYED_WORK(&dxs->events, dxs_events);
	np = of_parse_phandle(dev->of_node, "maxlinear,slic", 0);
	if (!np)
		return -EINVAL;
	supplier = bus_find_device_by_of_node(&spi_bus_type, np);
	ret = en75xx_dxs_index(np);
	of_node_put(np);
	if (!supplier || ret < 0) {
		put_device(supplier);
		return -EPROBE_DEFER;
	}
	dxs->index = ret;
	if (!device_link_add(dev, supplier, DL_FLAG_AUTOREMOVE_CONSUMER)) {
		put_device(supplier);
		return -ENOMEM;
	}
	put_device(supplier);
	np = of_parse_phandle(dev->of_node, "airoha,pcm", 0);
	if (!np)
		return -EINVAL;
	dxs->pcm = en75xx_pcm_get_by_fwnode(of_fwnode_handle(np));
	of_node_put(np);
	if (!dxs->pcm)
		return -EPROBE_DEFER;
	if (IS_ERR(dxs->pcm))
		return PTR_ERR(dxs->pcm);
	if (!device_link_add(dev, dxs->pcm->dev, DL_FLAG_AUTOREMOVE_CONSUMER)) {
		ret = -ENOMEM;
		goto fail;
	}

	of_property_read_string(dev->of_node, "airoha,bringup-mode", &mode);
	if (bringup_mode)
		mode = bringup_mode;
	if (strcmp(mode, "identify") && strcmp(mode, "firmware") &&
	    strcmp(mode, "start") && strcmp(mode, "bbd") &&
	    strcmp(mode, "calibrate") && strcmp(mode, "voice")) {
		ret = -EINVAL;
		goto fail;
	}
	voice = !strcmp(mode, "voice");
	start_only = !strcmp(mode, "start");
	bbd_only = !strcmp(mode, "bbd");
	calibrate_only = !strcmp(mode, "calibrate");
	if (!voice && strcmp(mode, "identify") &&
	    (stage_wait_ms < 100 || stage_wait_ms > 60000)) {
		ret = -EINVAL;
		goto fail;
	}
	count = of_property_count_u32_elems(dev->of_node, "airoha,pcm-channels");
	if (count < 1 || count > DXS_LINES ||
	    of_property_read_u32_array(dev->of_node, "airoha,pcm-channels", channels, count)) {
		ret = -EINVAL;
		goto fail;
	}
	dxs->count = count;
	for (i = 0; i < count; i++) {
		dxs->line[i].dxs = dxs;
		dxs->line[i].pcm_channel = channels[i];
		if (en75xx_pcm_channel_offsets(dxs->pcm, channels[i], &txbit, &rxbit) ||
		    (i && channels[0] == channels[1])) {
			ret = -EINVAL;
			goto fail;
		}
	}
	if (voice || bbd_only || calibrate_only) {
		if (of_property_read_string(dev->of_node, "maxlinear,dcdc-topology", &topology) ||
		    strcmp(topology, "CIBB12") ||
		    of_property_read_string(dev->of_node, "maxlinear,bbd-firmware", &bbdname)) {
			ret = -EINVAL;
			goto fail;
		}
		if (voice) {
			/* Validate required clock/edge settings before any chip download. */
			ret = dxs_pcm_settings(dxs);
			if (ret)
				goto fail;
		}
		ret = request_firmware(&bbd, bbdname, dev);
		if (ret)
			goto fail;
	}
	if (strcmp(mode, "identify")) {
		ret = of_property_read_string(dev->of_node, "firmware-name", &fwname);
		if (ret)
			goto fail;
		ret = request_firmware(&fw, fwname, dev);
		if (ret)
			goto fail;
	}
	snprintf(name, sizeof(name), "dxs%u", (dxs->index + 1) * 10);
	dxs->handle = ifx_tapi_kopen(name);
	if (!dxs->handle) {
		ret = -ENODEV;
		goto fail;
	}
	dev_info(dev, "DXS %u: starting basic access test (%s mode)\n", dxs->index, mode);
	dxs->basic = true;
	ret = en75xx_dxs_basic_init(dxs->index);
	if (ret)
		goto fail;
	if (!strcmp(mode, "identify")) {
		dev_info(dev, "DXS access test passed; no firmware/BBD or linefeed started\n");
		platform_set_drvdata(pdev, dxs);
		return 0;
	}
	ret = dxs_boot_firmware(dxs, fw);
	if (ret)
		goto fail;
	if (!voice && !start_only && !bbd_only && !calibrate_only) {
		ret = dxs_wait_stage(dxs, "after firmware download");
		if (ret)
			goto fail;
		dev_info(dev, "DXS firmware test passed; no BBD, PCM activation or linefeed started\n");
		platform_set_drvdata(pdev, dxs);
		release_firmware(fw);
		return 0;
	}
	ret = dxs_ioctl(dxs, dxs->handle, IFX_TAPI_DEV_START, &start);
	if (ret)
		goto fail;
	dxs->started = true;
	ret = (start_only || bbd_only || calibrate_only) ?
		dxs_wait_stage(dxs, "after DEV_START") :
		dxs_verify_firmware(dxs, "after DEV_START");
	if (ret)
		goto fail;
	if (start_only) {
		dev_info(dev, "DXS DEV_START test passed; no BBD, PCM activation or linefeed started\n");
		platform_set_drvdata(pdev, dxs);
		release_firmware(fw);
		return 0;
	}
	dev_info(dev, "DXS DEV_START completed; loading BBD %s (%zu bytes)\n",
		 bbdname, bbd->size);
	if (bbd_only)
		ret = en75xx_dxs_kernel_bbd_prepare(dxs->index, bbd->data, bbd->size);
	else
		ret = en75xx_dxs_kernel_bbd(dxs->index, bbd->data, bbd->size);
	if (ret) {
		dev_err(dev, "DXS BBD download/calibration failed: %d\n", ret);
		/* Read before dxs_shutdown() resets the chip. A version obtained
		 * after cleanup cannot distinguish an unexpected reboot from ours.
		 * Preserve the BBD error regardless of this diagnostic result.
		 */
		{
			int version_ret = en75xx_dxs_kernel_version(dxs->index,
							 &hw_version, &fw_version, &fxs_count);
			if (!version_ret)
				dev_err(dev, "DXS before cleanup: HW %#x FW %#x, %u FXS\n",
					hw_version, fw_version, fxs_count);
			else
				dev_err(dev, "DXS before cleanup: version query failed: %d\n",
					version_ret);
		}
		goto fail;
	}
	if (bbd_only) {
		ret = dxs_wait_stage(dxs, "after BBD blocks, before calibration");
		if (ret)
			goto fail;
		dev_info(dev, "DXS BBD blocks test passed; calibration pending; no PCM activation or linefeed started\n");
		platform_set_drvdata(pdev, dxs);
		release_firmware(bbd);
		release_firmware(fw);
		return 0;
	}
	dev_info(dev, "DXS BBD accepted: %zu bytes, topology %s\n", bbd->size, topology);
	ret = dxs_verify_firmware(dxs, "after BBD calibration");
	if (ret)
		goto fail;
	if (calibrate_only) {
		ret = dxs_wait_stage(dxs, "after calibration, before PCM activation");
		if (ret)
			goto fail;
		dev_info(dev, "DXS calibration test passed; no PCM activation or linefeed started\n");
		platform_set_drvdata(pdev, dxs);
		release_firmware(bbd);
		release_firmware(fw);
		return 0;
	}
	ret = dxs_configure_pcm(dxs);
	if (ret)
		goto fail;
	for (i = 0; i < count; i++) {
		struct en75xx_dxs_line *line = &dxs->line[i];
		IFX_TAPI_EVENT_t event = {0};
		event.ch = i;
		event.id = IFX_TAPI_EVENT_FXS_ONHOOK;
		ret = dxs_ioctl(dxs, dxs->handle, IFX_TAPI_EVENT_ENABLE, &event);
		if (!ret) {
			event.id = IFX_TAPI_EVENT_FXS_OFFHOOK;
			ret = dxs_ioctl(dxs, dxs->handle, IFX_TAPI_EVENT_ENABLE, &event);
		}
		if (ret)
			goto fail;
		ret = dxs_feed(line, EN75XX_VOICE_LINEFEED_STANDBY);
		if (ret)
			goto fail;
		ret = dxs_hook(line);
		if (ret < 0)
			goto fail;
		line->notified_hook = line->hook;
		line->voice = en75xx_voice_register_line(dev, dxs->pcm, line->pcm_channel,
						   dxs->index * DXS_LINES + i,
						   count == 2 ? "PEF32002" : "PEF32001",
						   &dxs_slic_ops, line);
		if (IS_ERR(line->voice)) {
			ret = PTR_ERR(line->voice);
			line->voice = NULL;
			goto fail;
		}
	}
	platform_set_drvdata(pdev, dxs);
	schedule_delayed_work(&dxs->events, msecs_to_jiffies(20));
	release_firmware(bbd);
	release_firmware(fw);
	return 0;
fail:
	release_firmware(bbd);
	release_firmware(fw);
	dxs_shutdown(dxs);
	return dev_err_probe(dev, ret, "DXS bring-up failed\n");
}

static void en75xx_dxs_remove(struct platform_device *pdev)
{
	dxs_shutdown(platform_get_drvdata(pdev));
}

static const struct of_device_id en75xx_dxs_match[] = {
	{ .compatible = "airoha,en75xx-dxs-fxs" }, { }
};
MODULE_DEVICE_TABLE(of, en75xx_dxs_match);
static struct platform_driver en75xx_dxs_driver = {
	.probe = en75xx_dxs_probe,
	.remove = en75xx_dxs_remove,
	.shutdown = en75xx_dxs_remove,
	.driver = { .name = "en75xx-dxs-fxs", .of_match_table = en75xx_dxs_match },
};
module_platform_driver(en75xx_dxs_driver);
MODULE_DESCRIPTION("MaxLinear PEF32001/PEF32002 TAPI to EN75xx PCM/FXS adapter");
MODULE_LICENSE("GPL");
