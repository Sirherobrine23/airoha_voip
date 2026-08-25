// SPDX-License-Identifier: GPL-2.0
/*
 * Microsemi/Microchip Le9641/Le9642 (VE886/VP886 "miSLIC") over ZSI.
 *
 * The chip is driven with raw MPI command streams taken from the Raw MPI
 * sections of the OEM ZLR964124_Le9641 BB profiles, rather than through
 * the VP-API-II state machine. That decision is deliberate: the API's
 * calibration path (VpCalLine) only refines supervisory offsets and
 * gains, does not touch the PCM audio path, and its "robust" CALCTRL
 * disconnect sequence actively breaks the switcher write below.
 *
 * Four things here are load-bearing and were expensive to find:
 *
 *  1. Never send MPI HWRESET (0x04). It drops the SLIC out of ZSI mode
 *     into a state only a physical power cycle recovers from.
 *  2. The AC profile carries a 6-byte VpProfile header. Streaming it
 *     with the header misaligns every filter coefficient and produces a
 *     self-sustaining hybrid oscillation, so it is streamed from +6.
 *  3. The switcher, not calibration, is the feed gate: a direct
 *     SWCTRL = 0x6f enables the feed.
 *  4. SIGREG is a device-level 4-byte register with each channel's HOOK
 *     bit in its own byte, so the hook index is (ec - 1), not 0.
 *
 * The ring voltage corrupts the SIGREG hook bit, so ringing runs on a
 * cadence and the hook is only sampled during the off gaps.
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include "../include/en75xx_voice.h"
#include "en75xx_pcm_regs.h"
#include "en75xx_zsi.h"

/* MPI opcodes (read opcode = write opcode + 1) */
#define VP886_R_EC_WRT			0x4a
#define VP886_R_NOOP			0x06
#define VP886_R_TXSLOT_WRT		0x40
#define VP886_R_TXSLOT_RD		0x41
#define VP886_R_RXSLOT_WRT		0x42
#define VP886_R_SIGREG_RD		0x4d	/* no update latch */
#define VP886_R_SIGREG_LATCH_RD		0x4f
#define VP886_R_STATE_WRT		0x56
#define VP886_R_STATE_RD		0x57
#define VP886_R_DEVMODE_RD		0x5f
#define VP886_R_OPFUNC_WRT		0x60
#define VP886_R_OPFUNC_RD		0x61
#define VP886_R_OPCOND_WRT		0x70
#define VP886_R_OPCOND_RD		0x71
#define VP886_R_RCNPCN_RD		0x73
#define VP886_R_GX_WRT			0x80
#define VP886_R_GR_WRT			0x82
#define VP886_R_DCFEED_WRT		0xc6
#define VP886_R_SWCTRL_WRT		0xe6

#define VP886_EC_1			0x01
#define VP886_EC_2			0x02
#define VP886_EC_GLOBAL			0x03

/* chip id: le9642 = rcn 0x08 / pcn 0x75 */
#define VP886_RCN_LE964X		0x08
#define VP886_PCN_LE9642		0x75
#define VP886_PCN_LE9641		0x74

/* STATE = CODEC(0x20) | POL(0x10) | SS(bits 3:0) */
#define VP886_STATE_CODEC		0x20
#define VP886_SS_DISCONNECT		0x00
#define VP886_SS_ACTIVE			0x03
#define VP886_SS_IDLE			0x04
#define VP886_SS_BAL_RING		0x07
#define VP886_SS_SHUTDOWN		0x0f
#define VP886_STATE_RING_ACTIVE		0x80	/* readback flag */

#define VP886_SWCTRL_ON			0x6f	/* HP/HP */

/* OPFUNC: CODEC_ULAW 0x40 | all filters 0x3f */
#define VP886_OPFUNC_ULAW		0x7f
#define VP886_OPFUNC_ALAW		0x3f

#define VP886_SIGREG_LEN		4
#define VP886_SIGREG_HOOK		0x01

/* ring cadence, in 150 ms ticks */
#define RING_TICK_MS			150
#define RING_ON_TICKS			5	/* ~0.75 s on  */
#define RING_PERIOD_TICKS		12	/* ~1.05 s off */
#define HOOK_DEBOUNCE			3

/*
 * Raw MPI sections of the ZLR964124_Le9641 BB profiles in ZSI mode.
 * Each opcode self-delimits, so the whole section is streamed as-is.
 */
static const u8 dev_mpi[] = {	/* DEV_PROFILE_100V_BB_124_ZSI */
	0x46, 0x02, 0x44, 0x06, 0x5e, 0x14, 0x00, 0xf6, 0x95, 0x00,
	0x58, 0x30, 0x5c, 0x30, 0xe4, 0x44, 0x92, 0x0a, 0xe6, 0x60,
};

static const u8 dc_mpi[] = {	/* DC_FXS_miSLIC_BB_DEF */
	0xc6, 0x92, 0x27,
};

/* AC_FXS_RF14_600R_DEF_LE9641 -- streamed from +6, see file header */
static const u8 ac_mpi[] = {
	0xa4, 0x00, 0xf4, 0x4c, 0x01, 0x49, 0xca, 0xf5, 0x98, 0xaa, 0x7b, 0xab,
	0x2c, 0xa3, 0x25, 0xa5, 0x24, 0xb2, 0x3d, 0x9a, 0x2a, 0xaa, 0xa6, 0x9f,
	0x01, 0x8a, 0x1d, 0x01, 0xa3, 0xa0, 0x2e, 0xb2, 0xb2, 0xba, 0xac, 0xa2,
	0xa6, 0xcb, 0x3b, 0x45, 0x88, 0x2a, 0x20, 0x3c, 0xbc, 0x4e, 0xa6, 0x2b,
	0xa5, 0x2b, 0x3e, 0xba, 0x8f, 0x82, 0xa8, 0x71, 0x80, 0xa9, 0xf0, 0x50,
	0x00, 0x86, 0x2a, 0x42, 0xa1, 0xcb, 0x1b, 0xa3, 0xa8, 0xfb, 0x87, 0xaa,
	0xfb, 0x9f, 0xa9, 0xf0, 0x96, 0x2e, 0x01, 0x00,
};
#define AC_MPI_HDR_LEN	6

static const u8 ring_mpi[] = {	/* RING_ZL880_BB90V_DEF, ~24.9 Hz ~70 Vpk */
	0xc0, 0x08, 0x00, 0x00, 0x00, 0x44, 0x3a, 0x9d, 0x00, 0x00, 0x00, 0x00,
};

struct le9642_line {
	struct le9642_slic	*slic;
	struct en75xx_voice_line *voice;
	unsigned int		ec;		/* 1 or 2 */
	unsigned int		bus_slot;
	unsigned int		pcm_channel;
	bool			ringing;
	bool			offhook;
	unsigned int		ring_tick;
	unsigned int		hook_streak;
	bool			hook_pending;
};

struct le9642_slic {
	struct device		*dev;
	struct en75xx_zsi	*zsi;
	struct en75xx_pcm	*pcm;
	struct mutex		lock;
	struct delayed_work	tick_work;
	unsigned int		n_lines;
	struct le9642_line	line[2];
	u8			rcn, pcn;
	bool			alaw;
};

/*
 * Loop current limit. The BB DC profile ships ILA = 0x07 (~25 mA),
 * tuned for a 600R desk phone. A cordless DECT base draws more, and at
 * the low setting the mic modulation barely reaches the voice ADC, so
 * capture sits at idle. mA = 18 + field; 0x14 = 38 mA.
 */
static int feed_ila = 0x14;
module_param(feed_ila, int, 0644);
MODULE_PARM_DESC(feed_ila, "DC feed loop current limit field (mA = 18 + n)");

/*
 * The SLIC uses different clock-slot offsets for capture and playback,
 * so the 8-bit G.711 code lands in a different byte in each direction.
 * Capture always reads the low byte; playback selects with this.
 */
static bool tx_msb = true;
module_param(tx_msb, bool, 0644);
MODULE_PARM_DESC(tx_msb, "place the playback G.711 code in the slot MSB");

/* ------------------------------------------------------------------ */
/* MPI helpers                                                         */
/* ------------------------------------------------------------------ */

static int le9642_select_ec(struct le9642_slic *slic, u8 ec)
{
	u8 cmd[2] = { VP886_R_EC_WRT, ec };

	return en75xx_zsi_write(slic->zsi, cmd, sizeof(cmd));
}

static int le9642_cmd(struct le9642_slic *slic, u8 ec, u8 opcode,
		      const u8 *data, size_t len)
{
	u8 buf[8];
	int ret;

	if (len + 1 > sizeof(buf))
		return -EINVAL;

	ret = le9642_select_ec(slic, ec);
	if (ret)
		return ret;

	buf[0] = opcode;
	memcpy(buf + 1, data, len);
	return en75xx_zsi_write(slic->zsi, buf, len + 1);
}

static int le9642_cmd1(struct le9642_slic *slic, u8 ec, u8 opcode, u8 v)
{
	return le9642_cmd(slic, ec, opcode, &v, 1);
}

static int le9642_stream(struct le9642_slic *slic, u8 ec, const char *what,
			 const u8 *buf, size_t len)
{
	int ret;

	if (ec) {
		ret = le9642_select_ec(slic, ec);
		if (ret)
			return ret;
	}

	ret = en75xx_zsi_write(slic->zsi, buf, len);
	if (ret)
		dev_err(slic->dev, "MPI stream '%s' failed: %d\n", what, ret);
	return ret;
}

static int le9642_detect(struct le9642_slic *slic)
{
	u8 id[2];
	int ret;

	ret = en75xx_zsi_read_reg(slic->zsi, VP886_EC_1, VP886_R_RCNPCN_RD,
				  id, sizeof(id));
	if (ret)
		return ret;

	slic->rcn = id[0];
	slic->pcn = id[1];

	if (id[0] != VP886_RCN_LE964X ||
	    (id[1] != VP886_PCN_LE9642 && id[1] != VP886_PCN_LE9641)) {
		dev_err(slic->dev, "unexpected chip id %02x %02x\n",
			id[0], id[1]);
		return -ENODEV;
	}

	dev_info(slic->dev, "Le964%c detected (rcn %02x pcn %02x)\n",
		 id[1] == VP886_PCN_LE9642 ? '2' : '1', id[0], id[1]);
	return 0;
}

/* ------------------------------------------------------------------ */
/* Profiles                                                            */
/* ------------------------------------------------------------------ */

static int le9642_load_device_profile(struct le9642_slic *slic)
{
	u8 devmode;
	int ret;

	ret = le9642_stream(slic, 0, "dev", dev_mpi, sizeof(dev_mpi));
	if (ret)
		return ret;
	ret = le9642_stream(slic, 0, "dc", dc_mpi, sizeof(dc_mpi));
	if (ret)
		return ret;
	ret = le9642_stream(slic, 0, "ac", ac_mpi + AC_MPI_HDR_LEN,
			    sizeof(ac_mpi) - AC_MPI_HDR_LEN);
	if (ret)
		return ret;
	ret = le9642_stream(slic, 0, "ring", ring_mpi, sizeof(ring_mpi));
	if (ret)
		return ret;

	/* read Device Mode back to confirm the profile took */
	ret = en75xx_zsi_read_reg(slic->zsi, VP886_EC_1, VP886_R_DEVMODE_RD,
				  &devmode, 1);
	if (ret)
		return ret;

	dev_dbg(slic->dev, "device profile loaded, devmode=0x%02x\n", devmode);
	return 0;
}

/*
 * The DC/AC/ring profiles are per-channel: device init only programs
 * channel 1, so channel 2 needs its own copy or its hybrid and ring
 * generator are wrong.
 */
static int le9642_load_channel_profile(struct le9642_slic *slic, u8 ec)
{
	int ret;

	ret = le9642_stream(slic, ec, "dc-ch", dc_mpi, sizeof(dc_mpi));
	if (ret)
		return ret;
	ret = le9642_stream(slic, ec, "ac-ch", ac_mpi + AC_MPI_HDR_LEN,
			    sizeof(ac_mpi) - AC_MPI_HDR_LEN);
	if (ret)
		return ret;
	return le9642_stream(slic, ec, "ring-ch", ring_mpi, sizeof(ring_mpi));
}

/* ------------------------------------------------------------------ */
/* Line control                                                        */
/* ------------------------------------------------------------------ */

static int le9642_set_feed(struct le9642_slic *slic, struct le9642_line *line)
{
	u8 dc[2] = { dc_mpi[1], (u8)((dc_mpi[2] & ~0x1f) | (feed_ila & 0x1f)) };
	int ret;

	ret = le9642_cmd(slic, line->ec, VP886_R_DCFEED_WRT, dc, sizeof(dc));
	if (ret)
		return ret;

	/* the switcher is the feed gate, so write it directly */
	return le9642_cmd1(slic, line->ec, VP886_R_SWCTRL_WRT,
			   VP886_SWCTRL_ON);
}

static int le9642_set_state(struct le9642_slic *slic, struct le9642_line *line,
			    u8 ss, bool codec)
{
	u8 v = ss | (codec ? VP886_STATE_CODEC : 0);

	return le9642_cmd1(slic, line->ec, VP886_R_STATE_WRT, v);
}

/*
 * Audio line-up. OPFUNC must be written before TXSLOT/RXSLOT -- the
 * OEM does it in that order and out of order the capture is degraded.
 */
static int le9642_audio_setup(struct le9642_slic *slic,
			      struct le9642_line *line)
{
	u8 opfunc = slic->alaw ? VP886_OPFUNC_ALAW : VP886_OPFUNC_ULAW;
	int ret;

	ret = le9642_set_feed(slic, line);
	if (ret)
		return ret;

	ret = le9642_cmd1(slic, line->ec, VP886_R_OPFUNC_WRT, opfunc);
	if (ret)
		return ret;

	ret = le9642_cmd1(slic, line->ec, VP886_R_TXSLOT_WRT, line->bus_slot);
	if (ret)
		return ret;
	ret = le9642_cmd1(slic, line->ec, VP886_R_RXSLOT_WRT, line->bus_slot);
	if (ret)
		return ret;

	ret = le9642_set_state(slic, line, VP886_SS_ACTIVE, true);
	if (ret)
		return ret;

	if (slic->pcm && slic->pcm->line_ops->set_format)
		slic->pcm->line_ops->set_format(slic->pcm, line->pcm_channel,
			slic->alaw ? EN75XX_PCM_CODEC_ALAW
				   : EN75XX_PCM_CODEC_ULAW, tx_msb);

	dev_dbg(slic->dev, "EC_%u audio up: slot %u, dma ch %u, %s\n",
		line->ec, line->bus_slot, line->pcm_channel,
		slic->alaw ? "a-law" : "u-law");
	return 0;
}

static int le9642_read_hook(struct le9642_slic *slic, struct le9642_line *line,
			    bool *offhook)
{
	u8 sig[VP886_SIGREG_LEN];
	int ret;

	/* device-level register: one byte per channel, index = ec - 1 */
	ret = en75xx_zsi_read_reg(slic->zsi, 0, VP886_R_SIGREG_RD,
				  sig, sizeof(sig));
	if (ret)
		return ret;

	*offhook = !!(sig[line->ec - 1] & VP886_SIGREG_HOOK);
	return 0;
}

/* ------------------------------------------------------------------ */
/* en75xx_voice_slic_ops                                               */
/* ------------------------------------------------------------------ */

static int le9642_op_get_hook(void *priv)
{
	struct le9642_line *line = priv;

	return line->offhook ? EN75XX_VOICE_OFFHOOK : EN75XX_VOICE_ONHOOK;
}

static int le9642_op_ring(void *priv, bool enable, unsigned int on_ms,
			  unsigned int off_ms)
{
	struct le9642_line *line = priv;
	struct le9642_slic *slic = line->slic;
	int ret = 0;

	mutex_lock(&slic->lock);

	if (enable == line->ringing)
		goto out;

	if (enable) {
		ret = le9642_load_channel_profile(slic, line->ec);
		if (ret)
			goto out;
		ret = le9642_cmd1(slic, line->ec, VP886_R_SWCTRL_WRT,
				  VP886_SWCTRL_ON);
		if (ret)
			goto out;
		ret = le9642_set_state(slic, line, VP886_SS_ACTIVE, false);
		if (ret)
			goto out;

		line->ring_tick = 0;
		line->ringing = true;
		/* the cadence itself is driven from the tick worker */
	} else {
		line->ringing = false;
		ret = le9642_set_state(slic, line, VP886_SS_ACTIVE, true);
	}
out:
	mutex_unlock(&slic->lock);
	return ret;
}

static int le9642_op_set_linefeed(void *priv,
				  enum en75xx_voice_linefeed state)
{
	struct le9642_line *line = priv;
	struct le9642_slic *slic = line->slic;
	int ret;
	u8 ss;

	switch (state) {
	case EN75XX_VOICE_LINEFEED_OPEN:
		ss = VP886_SS_DISCONNECT;
		break;
	case EN75XX_VOICE_LINEFEED_STANDBY:
		ss = VP886_SS_IDLE;
		break;
	case EN75XX_VOICE_LINEFEED_ACTIVE:
		ss = VP886_SS_ACTIVE;
		break;
	case EN75XX_VOICE_LINEFEED_REVERSE:
		ss = VP886_SS_ACTIVE;
		break;
	default:
		return -EINVAL;
	}

	mutex_lock(&slic->lock);
	ret = le9642_set_state(slic, line, ss,
			       state == EN75XX_VOICE_LINEFEED_ACTIVE);
	mutex_unlock(&slic->lock);
	return ret;
}

static int le9642_op_get_faults(void *priv, u32 *faults)
{
	*faults = 0;
	return 0;
}

static const struct en75xx_voice_slic_ops le9642_slic_ops = {
	.get_hook = le9642_op_get_hook,
	.ring = le9642_op_ring,
	.set_linefeed = le9642_op_set_linefeed,
	.get_faults = le9642_op_get_faults,
};

/* ------------------------------------------------------------------ */
/* Cadence + hook sampling                                             */
/* ------------------------------------------------------------------ */

static void le9642_tick_line(struct le9642_slic *slic,
			     struct le9642_line *line)
{
	bool offhook, in_gap;

	if (line->ringing) {
		unsigned int phase = line->ring_tick % RING_PERIOD_TICKS;

		in_gap = phase >= RING_ON_TICKS;
		if (phase == 0)
			le9642_set_state(slic, line, VP886_SS_BAL_RING, false);
		else if (phase == RING_ON_TICKS)
			le9642_set_state(slic, line, VP886_SS_ACTIVE, true);

		line->ring_tick++;

		/*
		 * The 70-90 V ring voltage corrupts the SIGREG hook bit,
		 * so only sample during the silent part of the cadence.
		 */
		if (!in_gap)
			return;
	}

	if (le9642_read_hook(slic, line, &offhook))
		return;

	if (offhook == line->offhook) {
		line->hook_streak = 0;
		return;
	}

	if (++line->hook_streak < HOOK_DEBOUNCE)
		return;

	line->hook_streak = 0;
	line->offhook = offhook;

	if (offhook && line->ringing) {
		line->ringing = false;
		le9642_set_state(slic, line, VP886_SS_ACTIVE, true);
	}

	dev_dbg(slic->dev, "EC_%u %s\n", line->ec,
		offhook ? "off-hook" : "on-hook");

	if (line->voice)
		en75xx_voice_hook_changed(line->voice, offhook);
}

static void le9642_tick_work(struct work_struct *work)
{
	struct le9642_slic *slic = container_of(to_delayed_work(work),
						struct le9642_slic, tick_work);
	unsigned int i;

	mutex_lock(&slic->lock);
	for (i = 0; i < slic->n_lines; i++)
		le9642_tick_line(slic, &slic->line[i]);
	mutex_unlock(&slic->lock);

	schedule_delayed_work(&slic->tick_work, msecs_to_jiffies(RING_TICK_MS));
}

/* ------------------------------------------------------------------ */
/* Probe                                                               */
/* ------------------------------------------------------------------ */

static int le9642_bringup(struct le9642_slic *slic)
{
	unsigned int i;
	int ret;

	ret = en75xx_zsi_hw_init(slic->zsi);
	if (ret)
		return ret;

	en75xx_zsi_slic_reset(slic->zsi);

	ret = le9642_detect(slic);
	if (ret)
		return ret;

	ret = le9642_load_device_profile(slic);
	if (ret)
		return ret;

	for (i = 0; i < slic->n_lines; i++) {
		struct le9642_line *line = &slic->line[i];

		if (line->ec != VP886_EC_1) {
			ret = le9642_load_channel_profile(slic, line->ec);
			if (ret)
				return ret;
		}
		ret = le9642_audio_setup(slic, line);
		if (ret)
			return ret;
	}

	return 0;
}

static int le9642_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct le9642_slic *slic;
	struct device_node *pcm_np;
	u32 slots[2] = { 4, 6 };
	u32 n_lines = 1;
	unsigned int i;
	int ret;

	slic = devm_kzalloc(dev, sizeof(*slic), GFP_KERNEL);
	if (!slic)
		return -ENOMEM;

	slic->dev = dev;
	mutex_init(&slic->lock);
	INIT_DELAYED_WORK(&slic->tick_work, le9642_tick_work);

	slic->zsi = en75xx_zsi_get(dev, "airoha,zsi");
	if (IS_ERR(slic->zsi))
		return dev_err_probe(dev, PTR_ERR(slic->zsi),
				     "no ZSI transport\n");

	pcm_np = of_parse_phandle(np, "airoha,pcm", 0);
	if (pcm_np) {
		slic->pcm = en75xx_pcm_get_by_fwnode(of_fwnode_handle(pcm_np));
		of_node_put(pcm_np);
	}
	if (!slic->pcm) {
		ret = dev_err_probe(dev, -EPROBE_DEFER, "no PCM controller\n");
		goto err_zsi;
	}

	of_property_read_u32(np, "airoha,lines", &n_lines);
	of_property_read_u32_array(np, "airoha,bus-slots", slots, 2);
	slic->alaw = of_property_read_bool(np, "airoha,a-law");
	slic->n_lines = clamp_val(n_lines, 1, 2);

	for (i = 0; i < slic->n_lines; i++) {
		struct le9642_line *line = &slic->line[i];

		line->slic = slic;
		line->ec = i + 1;
		line->bus_slot = slots[i];
		line->pcm_channel = EN75XX_PCM_SLOT_TO_DMA_CH(slots[i]);
	}

	ret = le9642_bringup(slic);
	if (ret) {
		dev_err(dev, "bring-up failed: %d\n", ret);
		goto err_pcm;
	}

	for (i = 0; i < slic->n_lines; i++) {
		struct le9642_line *line = &slic->line[i];

		line->voice = en75xx_voice_register_line(dev, slic->pcm,
					line->pcm_channel, i, "le9642",
					&le9642_slic_ops, line);
		if (IS_ERR(line->voice)) {
			ret = PTR_ERR(line->voice);
			line->voice = NULL;
			goto err_lines;
		}
	}

	platform_set_drvdata(pdev, slic);
	schedule_delayed_work(&slic->tick_work, msecs_to_jiffies(RING_TICK_MS));

	dev_info(dev, "%u FXS line(s) ready\n", slic->n_lines);
	return 0;

err_lines:
	while (i--)
		en75xx_voice_unregister_line(slic->line[i].voice);
err_pcm:
	en75xx_pcm_put(slic->pcm);
err_zsi:
	en75xx_zsi_put(slic->zsi);
	return ret;
}

static void le9642_remove(struct platform_device *pdev)
{
	struct le9642_slic *slic = platform_get_drvdata(pdev);
	unsigned int i;

	cancel_delayed_work_sync(&slic->tick_work);

	for (i = 0; i < slic->n_lines; i++) {
		struct le9642_line *line = &slic->line[i];

		if (line->voice)
			en75xx_voice_unregister_line(line->voice);
		mutex_lock(&slic->lock);
		le9642_set_state(slic, line, VP886_SS_DISCONNECT, false);
		mutex_unlock(&slic->lock);
	}

	en75xx_pcm_put(slic->pcm);
	en75xx_zsi_put(slic->zsi);
}

static const struct of_device_id le9642_of_match[] = {
	{ .compatible = "microsemi,le9642" },
	{ .compatible = "microsemi,le9641" },
	{ .compatible = "microchip,le9642" },
	{ }
};
MODULE_DEVICE_TABLE(of, le9642_of_match);

static struct platform_driver le9642_driver = {
	.probe = le9642_probe,
	.remove = le9642_remove,
	.driver = {
		.name = "en75xx-slic-le9642",
		.of_match_table = le9642_of_match,
	},
};
module_platform_driver(le9642_driver);

MODULE_DESCRIPTION("Microsemi Le9641/Le9642 FXS over ZSI on EcoNet EN75xx");
MODULE_LICENSE("GPL");
