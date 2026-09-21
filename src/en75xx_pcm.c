// SPDX-License-Identifier: GPL-2.0
/*
 * EcoNet/Airoha EN75xx PCM/TDM controller.
 *
 * The descriptor layout and register programming are shared by the vendor
 * EN751221, EN7528 and EN7523 PCM drivers. EN7523 uses the later PCM v2 ring
 * geometry and a different DMA bus-address encoding; those differences are
 * described by match data instead of being spread through the data path.
 */
#include <linux/bitfield.h>
#include <linux/build_bug.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kfifo.h>
#include <linux/list.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/poll.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/unaligned.h>
#include <linux/workqueue.h>

#include "../include/en75xx_voice.h"
#include "en75xx_pcm_regs.h"
#include "en75xx_g711.h"
#include "oslec.h"

static bool clock_on_probe = true;
module_param_named(clock_on_probe, clock_on_probe, bool, 0444);
MODULE_PARM_DESC(clock_on_probe, "Start PCM interface clocking (PCLK/FSYNC) on probe");

static bool lec_enable;
module_param_named(lec_enable, lec_enable, bool, 0644);
MODULE_PARM_DESC(lec_enable, "Enable OSLEC line echo cancellation on the RX path");

/*
 * 128 samples = 16ms at 8kHz, shorter than any of the vendor's own
 * documented echo-canceller tail-length options (MediaTek ADAM API:
 * EC_TAIL_LENGTH_36MS/48MS/60MS/72MS). Tried 576 (72ms, the vendor's
 * longest option) on the theory that a longer real echo path was
 * defeating a too-short filter -- tested live and made audio quality
 * worse, not better ("horror movie" distortion), so reverted. Longer
 * taps take longer to converge and are more prone to misadaptation if
 * the adaptation step size isn't adjusted for the new length; whatever
 * this board's real problem is, it isn't simply "the tail is too
 * short."
 */
static int lec_taps = 128;
module_param_named(lec_taps, lec_taps, int, 0444);
MODULE_PARM_DESC(lec_taps, "OSLEC length: 0 disables, otherwise power of two from 2 to 4096 samples");

#define EN75XX_PCM_LEC_ADAPTION_MODE \
	(ECHO_CAN_USE_ADAPTION | ECHO_CAN_USE_NLP | ECHO_CAN_USE_CLIP)

static int lec_adaption_mode = EN75XX_PCM_LEC_ADAPTION_MODE;
module_param_named(lec_adaption_mode, lec_adaption_mode, int, 0444);
MODULE_PARM_DESC(lec_adaption_mode, "OSLEC mode bits: 0 adapt, 1 NLP, 2 CNG, 3 clip, 4 tx_hpf, 5 rx_hpf, 6 disable");

static int swap_samples = -1;
module_param_named(swap_samples, swap_samples, int, 0644);
MODULE_PARM_DESC(swap_samples, "Sample pair swap for 16-bit linear PCM (-1: auto/DTS, 0: disabled, 1: enabled/swap)");

static bool alc_enable = true;
module_param_named(alc_enable, alc_enable, bool, 0644);
MODULE_PARM_DESC(alc_enable, "Enable vendor-derived Auto Level Control (ALC) and noise gate on RX");

static bool alc_noise_gate = true;
module_param_named(alc_noise_gate, alc_noise_gate, bool, 0644);
MODULE_PARM_DESC(alc_noise_gate, "Attenuate baseline analog line hiss when idle");

static int alc_ref_gain = 24;
module_param_named(alc_ref_gain, alc_ref_gain, int, 0644);
MODULE_PARM_DESC(alc_ref_gain, "Target speech dB index for ALC (default 24)");

static int alc_noise_thresh = 40;
module_param_named(alc_noise_thresh, alc_noise_thresh, int, 0644);
MODULE_PARM_DESC(alc_noise_thresh, "Noise floor threshold dB index (default 40)");

static int alc_speech_thresh = 35;
module_param_named(alc_speech_thresh, alc_speech_thresh, int, 0644);
MODULE_PARM_DESC(alc_speech_thresh, "Speech activity threshold dB index (default 35)");

static int alc_delta_thresh = 2;
module_param_named(alc_delta_thresh, alc_delta_thresh, int, 0644);
MODULE_PARM_DESC(alc_delta_thresh, "ALC hysteresis deadband in dB (default 2)");

static int alc_up_shift = 1;
module_param_named(alc_up_shift, alc_up_shift, int, 0644);
MODULE_PARM_DESC(alc_up_shift, "ALC max upward gain step in dB (default 1)");

static int alc_down_shift = 2;
module_param_named(alc_down_shift, alc_down_shift, int, 0644);
MODULE_PARM_DESC(alc_down_shift, "ALC max downward gain step in dB (default 2)");

/*
 * MediaTek / EcoNet VoIP Auto Level Control (ALC) Tables
 * Extracted from stock firmware fxs3_silicon_si32192.ko (GPL / Flash dump)
 *
 * attenTable_4db: Coarse 4.0 dB steps (19 entries: +20 dB down to -52 dB).
 *                 Entry [5] is 0x0ccc = 3276 (0.0 dB unity gain).
 * attenTable_05db: Fine 0.5 dB steps (7 entries: -0.5 dB down to -3.5 dB).
 * alc_energy_table: L1 norm energy conversion table for 80 samples of 16-bit PCM.
 *                   Entry [0] = 2621360 (max possible 80*32767), down to [59] = 2590.
 */
static const u16 attenTable_4db[19] = {
	0x7fff, 0x50c2, 0x32f4, 0x2026, 0x1449, 0x0ccc, 0x0813, 0x0518,
	0x0337, 0x0207, 0x0147, 0x00ce, 0x0082, 0x0052, 0x0033, 0x0020,
	0x0014, 0x000d, 0x0008,
};

static const u16 attenTable_05db[7] = {
	0x78d6, 0x7213, 0x6bb1, 0x65ab, 0x5ffb, 0x5a9d, 0x558b,
};

static const u32 alc_energy_table[60] = {
	2621360, 2200000, 1820000, 1630000, 1400000, 1150000, 1050000,  970000,
	 850000,  770000,  680000,  615000,  530000,  480000,  420000,  380000,
	 335000,  305000,  270000,  240000,  210000,  190000,  170000,  150000,
	 139000,  120000,  110000,   97000,   85000,   76000,   69000,   60000,
	  54000,   48000,   42000,   38000,   34000,   30000,   26000,   23000,
	  21000,   18000,   16500,   15000,   13500,   12200,   10600,    9800,
	   9000,    8200,    7300,    6500,    5800,    5200,    4600,    4150,
	   3650,    3300,    2930,    2590,
};

struct en75xx_pcm_alc {
	s8 cur_gain_half_db;
	s32 cur_mult;
	u32 last_sum[8];
	u32 last_avg_sum[8];
	u8 sum_idx;
	u8 avg_count;
	u8 noise_flag;
};

#define EN75XX_PCM_MAX_RING_COUNT 16

struct en75xx_pcm_soc_data {
	const char *name;
	unsigned int ring_count;
	size_t desc_size;
	u32 ring_cfg;
	u32 dma_mask;
	u32 dma_or;
	u8 channel_mask;
	bool pcm_v2;		/* EN7523: 12-byte descriptor, CHAN_ENABLE */
	bool swap_samples;	/* swap 16-bit sample pairs on LE bus */
};

struct en75xx_pcm_chan {
	struct kfifo rx_fifo;
	struct kfifo tx_fifo;
	s16 tx_desc_ref[EN75XX_PCM_MAX_RING_COUNT][EN75XX_PCM_FRAME_SAMPLES];
	struct oslec_state *lec;
	bool lec_active;
	struct en75xx_pcm_alc alc;
	spinlock_t fifo_lock;
	wait_queue_head_t rx_wait;
	wait_queue_head_t tx_wait;
	u64 rx_bytes;
	u64 tx_bytes;
	u64 rx_overruns;
	u64 tx_underruns;
	/*
	 * Wire format for this channel. The Le9642 drives only 8 bits per
	 * timeslot, so it must run the codec in u-law and companding
	 * happens here; the character device always stays 16-bit linear.
	 */
	enum en75xx_pcm_codec codec;
	bool tx_msb;
	/*
	 * ALC is suspended while the line is dialling. The vendor's own
	 * voice_autogain is gated the same way -- it carries a per-line
	 * enable flag and is switched off during fax negotiation -- because
	 * an adaptive gain loop has no business tracking call-progress
	 * tones. Ours would pull gain down against the continuous dial tone
	 * reflecting through the hybrid and bury the DTMF digits that
	 * follow it.
	 */
	bool alc_suspended;
};

struct en75xx_pcm_dev {
	struct en75xx_pcm pub;
	struct list_head node;
	struct device *dev;
	const struct en75xx_pcm_soc_data *soc;
	void __iomem *base;
	struct reset_control *rst;
	int irq;
	struct mutex lock;
	struct delayed_work poll_work;
	void *tx_ring;
	void *rx_ring;
	dma_addr_t tx_ring_dma;
	dma_addr_t rx_ring_dma;
	u8 *tx_buf;
	u8 *rx_buf;
	dma_addr_t tx_buf_dma;
	dma_addr_t rx_buf_dma;
	struct en75xx_pcm_chan chan[EN75XX_PCM_MAX_CHANNELS];
	u32 tx_slots[EN75XX_PCM_SLOT_REGS];
	u32 rx_slots[EN75XX_PCM_SLOT_REGS];
	u32 iface_ctrl;
	u8 dma_channel_mask;
	u8 active_mask;
	bool running;
	bool swap_samples;
	bool big_endian_samples;
	u64 dma_errors;
};

static LIST_HEAD(en75xx_pcm_list);
static DEFINE_MUTEX(en75xx_pcm_list_lock);

static inline struct en75xx_pcm_dev *to_pcm_dev(struct en75xx_pcm *pcm)
{
	return container_of(pcm, struct en75xx_pcm_dev, pub);
}

static inline bool en75xx_pcm_is_swap_samples(struct en75xx_pcm_dev *pcm)
{
	if (swap_samples >= 0)
		return !!swap_samples;
	return pcm->swap_samples;
}

static __maybe_unused u32 pcm_int_sqrt(u64 val)
{
	u64 b, m;

	m = 1ULL << 62;
	b = 0;
	while (m > 0) {
		if (val >= b + m) {
			val -= b + m;
			b = (b >> 1) + m;
		} else {
			b >>= 1;
		}
		m >>= 2;
	}
	return (u32)b;
}

static inline u32 pcm_read(struct en75xx_pcm_dev *pcm, u32 reg)
{
	return readl(pcm->base + reg);
}

static inline void pcm_write(struct en75xx_pcm_dev *pcm, u32 reg, u32 val)
{
	writel(val, pcm->base + reg);
}

static u32 en75xx_pcm_dma_addr(struct en75xx_pcm_dev *pcm, dma_addr_t addr)
{
	WARN_ON_ONCE((u64)addr & ~(u64)pcm->soc->dma_mask);
	return (lower_32_bits(addr) & pcm->soc->dma_mask) | pcm->soc->dma_or;
}

static bool en75xx_pcm_dma_range_valid(struct en75xx_pcm_dev *pcm,
				       dma_addr_t addr, size_t size)
{
	u64 first = addr;
	u64 last;

	if (!size)
		return false;
	last = first + size - 1;
	return last >= first && !(first & ~(u64)pcm->soc->dma_mask) &&
		!(last & ~(u64)pcm->soc->dma_mask);
}

static void en75xx_pcm_hw_stop(struct en75xx_pcm_dev *pcm)
{
	u32 val = pcm_read(pcm, EN75XX_PCM_DMA_CTRL);

	val &= ~(EN75XX_PCM_DMA_TX_EN | EN75XX_PCM_DMA_RX_EN);
	pcm_write(pcm, EN75XX_PCM_DMA_CTRL, val);
	pcm_write(pcm, EN75XX_PCM_IMR, 0);
	pcm_write(pcm, EN75XX_PCM_ISR, pcm_read(pcm, EN75XX_PCM_ISR));
}

static int en75xx_pcm_soft_reset(struct en75xx_pcm_dev *pcm)
{
	if (!pcm->rst)
		return 0;

	return reset_control_reset(pcm->rst);
}

/*
 * Descriptor accessors. gen1 is 36 bytes with one buffer pointer per
 * channel; gen2 is 12 bytes with the channel mask in its own word and a
 * single buffer pointer. See en75xx_pcm_regs.h for the evidence.
 */
static inline void *en75xx_pcm_desc(struct en75xx_pcm_dev *pcm, void *ring,
				    unsigned int index)
{
	return (u8 *)ring + index * pcm->soc->desc_size;
}

static inline u32 en75xx_pcm_desc_status(struct en75xx_pcm_dev *pcm,
					 void *ring, unsigned int index)
{
	return READ_ONCE(*(u32 *)en75xx_pcm_desc(pcm, ring, index));
}

static void en75xx_pcm_desc_prepare(struct en75xx_pcm_dev *pcm, void *ring,
				    unsigned int index, dma_addr_t frame_dma,
				    u8 mask)
{
	u32 status = EN75XX_PCM_DESC_OWN |
		FIELD_PREP(EN75XX_PCM_DESC_SAMPLE_SIZE,
			   EN75XX_PCM_FRAME_SAMPLES);

	if (pcm->soc->pcm_v2) {
		struct en75xx_pcm_desc_v2 *d = en75xx_pcm_desc(pcm, ring, index);

		d->ch_valid = mask;
		d->buf_addr = en75xx_pcm_dma_addr(pcm, frame_dma);
		dma_wmb();
		WRITE_ONCE(d->status, status);
	} else {
		struct en75xx_pcm_desc_v1 *d = en75xx_pcm_desc(pcm, ring, index);
		unsigned int channel;

		status |= FIELD_PREP(EN75XX_PCM_DESC_CH_VALID, mask);

		for (channel = 0; channel < EN75XX_PCM_MAX_CHANNELS; channel++)
			d->buf_addr[channel] = en75xx_pcm_dma_addr(pcm,
				frame_dma + channel * EN75XX_PCM_FRAME_BYTES);
		dma_wmb();
		WRITE_ONCE(d->status, status);
	}
	dma_wmb();
}

static void en75xx_pcm_compand_tx(struct en75xx_pcm_chan *ch, u8 *dst)
{
	unsigned int i;

	if (ch->codec == EN75XX_PCM_CODEC_LINEAR16)
		return;

	for (i = 0; i < EN75XX_PCM_FRAME_SAMPLES; i++) {
		s16 linear = get_unaligned_le16(dst + i * 2);
		u8 code = (ch->codec == EN75XX_PCM_CODEC_ULAW) ?
			en75xx_ulaw_encode(linear) :
			en75xx_alaw_encode(linear);
		u16 slot = ch->tx_msb ? ((u16)code << 8) : code;

		dst[i * 2] = slot & 0xff;
		dst[i * 2 + 1] = slot >> 8;
	}
}

static void en75xx_pcm_fill_tx_channel(struct en75xx_pcm_dev *pcm,
				       unsigned int channel, u8 *dst,
				       unsigned int index)
{
	struct en75xx_pcm_chan *ch = &pcm->chan[channel];
	unsigned long flags;
	unsigned int copied, i;

	spin_lock_irqsave(&ch->fifo_lock, flags);
	copied = kfifo_out(&ch->tx_fifo, dst, EN75XX_PCM_FRAME_BYTES);
	spin_unlock_irqrestore(&ch->fifo_lock, flags);

	if (copied < EN75XX_PCM_FRAME_BYTES) {
		memset(dst + copied, 0, EN75XX_PCM_FRAME_BYTES - copied);
		ch->tx_underruns++;
	}
	ch->tx_bytes += copied;

	/*
	 * dst is canonical LE16 linear here, before companding/byte-swap.
	 * Save this frame directly into the descriptor's TX reference buffer,
	 * so RX for this exact descriptor index is paired 1:1 with what was
	 * sent on the wire at that instant.
	 */
	if (index < EN75XX_PCM_MAX_RING_COUNT) {
		for (i = 0; i < EN75XX_PCM_FRAME_SAMPLES; i++)
			ch->tx_desc_ref[index][i] = get_unaligned_le16(dst + i * 2);
	}

	if (ch->codec != EN75XX_PCM_CODEC_LINEAR16) {
		en75xx_pcm_compand_tx(ch, dst);
	} else if (pcm->big_endian_samples) {
		for (i = 0; i < EN75XX_PCM_FRAME_BYTES; i += 2)
			swap(dst[i], dst[i + 1]);
	} else if (en75xx_pcm_is_swap_samples(pcm)) {
		u16 *s = (u16 *)dst;

		for (i = 0; i < EN75XX_PCM_FRAME_SAMPLES; i += 2)
			swap(s[i], s[i + 1]);
	}
	wake_up_interruptible(&ch->tx_wait);
}

static inline u32 alc_calc_energy(const s16 *samples, unsigned int n)
{
	u32 sum = 0;
	unsigned int i;

	for (i = 0; i < n; i++) {
		s32 v = samples[i];

		sum += (v < 0) ? -v : v;
	}
	return sum;
}

static inline int alc_energy_to_db(u32 energy)
{
	int i;

	for (i = 0; i < 60; i++) {
		if (energy >= alc_energy_table[i])
			return i + 1;
	}
	return 61;
}

static inline s32 alc_calc_multiplier(int gain_half_db)
{
	int g = clamp_val(gain_half_db, -40, 40);
	int idx = 40 - g; /* 0..80 (0 = +20dB, 40 = 0dB, 80 = -20dB) */
	int coarse = idx >> 3; /* 0..10 */
	int fine = idx & 7;    /* 0..7 */

	if (fine == 0)
		return (s32)attenTable_4db[coarse];
	return ((s32)attenTable_05db[fine - 1] * (s32)attenTable_4db[coarse]) / 32767;
}

static void en75xx_pcm_alc_reset(struct en75xx_pcm_alc *alc)
{
	alc->cur_gain_half_db = 0;
	alc->cur_mult = 3276;
	alc->sum_idx = 0;
	alc->avg_count = 0;
	alc->noise_flag = 0;
	memset(alc->last_sum, 0, sizeof(alc->last_sum));
	memset(alc->last_avg_sum, 0, sizeof(alc->last_avg_sum));
}

static bool en75xx_pcm_alc_active(const struct en75xx_pcm_chan *ch)
{
	return READ_ONCE(alc_enable) && !ch->alc_suspended;
}

static void en75xx_pcm_voice_autogain(struct en75xx_pcm_chan *ch, s16 *samples,
				    unsigned int num_samples)
{
	struct en75xx_pcm_alc *alc = &ch->alc;
	u32 energy;
	int cur_db, deadband = clamp(READ_ONCE(alc_delta_thresh), 0, 40);
	int i;

	if (!en75xx_pcm_alc_active(ch))
		return;

	energy = alc_calc_energy(samples, num_samples);
	cur_db = alc_energy_to_db(energy);

	/* Check if signal is below speech threshold or in noise floor */
	if (cur_db > alc_noise_thresh && cur_db > alc_speech_thresh) {
		alc->noise_flag = 1;
		/* Line is idle / silent. Freeze gain adaptation to avoid amplifying line hiss. */
	} else {
		alc->noise_flag = 0;
		/* Active signal: accumulate into 8-frame rolling window */
		alc->last_sum[alc->sum_idx] = energy;
		alc->sum_idx = (alc->sum_idx + 1) & 7;

		if (alc->sum_idx == 0) {
			/* 8 frames accumulated (80 ms), compute rolling average */
			u32 sum8 = 0;
			u32 avg_energy = 0;
			int avg_db, diff;

			for (i = 0; i < 8; i++)
				sum8 += (alc->last_sum[i] >> 3);

			/* Average only populated history, avoiding startup bias. */
			for (i = 7; i > 0; i--)
				alc->last_avg_sum[i] = alc->last_avg_sum[i - 1];
			alc->last_avg_sum[0] = sum8;
			if (alc->avg_count < 8)
				alc->avg_count++;
			for (i = 0; i < alc->avg_count; i++)
				avg_energy += alc->last_avg_sum[i];
			avg_energy /= alc->avg_count;

			avg_db = alc_energy_to_db(avg_energy);
			/* Table indices are approximately 1 dB; gain is in 0.5 dB. */
			diff = 2 * (avg_db - clamp(READ_ONCE(alc_ref_gain), 1, 61)) -
				alc->cur_gain_half_db;

			/* Hysteresis check (diff is in 0.5 dB units) */
			if (diff > deadband) {
				/* Signal is quieter than reference: increase gain */
				if (diff > deadband + 4)
					alc->cur_gain_half_db += min(diff,
						clamp(READ_ONCE(alc_up_shift), 0, 20) * 2);
				else
					alc->cur_gain_half_db += min(diff,
						clamp(READ_ONCE(alc_up_shift), 0, 20));
			} else if (diff < -deadband) {
				/* Signal is louder than reference: decrease gain */
				if (diff < -(deadband + 4))
					alc->cur_gain_half_db -= min(-diff,
						clamp(READ_ONCE(alc_down_shift), 0, 20) * 2);
				else
					alc->cur_gain_half_db -= min(-diff,
						clamp(READ_ONCE(alc_down_shift), 0, 20));
			}
			alc->cur_gain_half_db = clamp_val(alc->cur_gain_half_db, -40, 40);
			alc->cur_mult = alc_calc_multiplier(alc->cur_gain_half_db);
		}
	}

	/*
	 * Apply digital gain scaling:
	 * When noise_flag is set:
	 * Vendor stock behavior freezes gain and falls back to baseline gain (3276 = unity 0 dB)
	 * so idle background noise is never amplified.
	 * If alc_noise_gate is also enabled, apply gentle attenuation (-6 dB) during idle.
	 */
	if (alc->noise_flag) {
		s32 mult = alc_noise_gate ? ((3276 * 5) / 10) : 3276;

		for (i = 0; i < num_samples; i++) {
			s32 val = ((s32)samples[i] * mult) / 3276;

			samples[i] = clamp_val(val, -32768, 32767);
		}
	} else {
		for (i = 0; i < num_samples; i++) {
			s32 val = ((s32)samples[i] * alc->cur_mult) / 3276;

			samples[i] = clamp_val(val, -32768, 32767);
		}
	}
}

static void en75xx_pcm_push_rx_channel(struct en75xx_pcm_dev *pcm,
				       unsigned int channel, const u8 *src,
				       unsigned int index)
{
	struct en75xx_pcm_chan *ch = &pcm->chan[channel];
	unsigned long flags;
	unsigned int copied;
	u8 tmp[EN75XX_PCM_FRAME_BYTES] __aligned(2);
	const u8 *data = src;
	bool do_alc = en75xx_pcm_alc_active(ch);
	unsigned int i;

	if (ch->codec != EN75XX_PCM_CODEC_LINEAR16) {
		/*
		 * Capture always carries the G.711 code in the low byte
		 * of the slot, regardless of which byte playback uses.
		 */
		for (i = 0; i < EN75XX_PCM_FRAME_SAMPLES; i++) {
			s16 linear = (ch->codec == EN75XX_PCM_CODEC_ULAW) ?
				en75xx_ulaw_decode(src[i * 2]) :
				en75xx_alaw_decode(src[i * 2]);

			put_unaligned_le16(linear, tmp + i * 2);
		}
		data = tmp;
		goto process_dsp;
	}

	if (pcm->big_endian_samples) {
		for (i = 0; i < EN75XX_PCM_FRAME_BYTES; i += 2) {
			tmp[i] = src[i + 1];
			tmp[i + 1] = src[i];
		}
		data = tmp;
	} else if (en75xx_pcm_is_swap_samples(pcm)) {
		const u16 *s = (const u16 *)src;
		u16 *d = (u16 *)tmp;

		for (i = 0; i < EN75XX_PCM_FRAME_SAMPLES; i += 2) {
			d[i] = s[i + 1];
			d[i + 1] = s[i];
		}
		data = tmp;
	} else if (ch->lec || do_alc) {
		memcpy(tmp, src, EN75XX_PCM_FRAME_BYTES);
		data = tmp;
	}

process_dsp:
	if (ch->lec && ch->lec_active != READ_ONCE(lec_enable)) {
		ch->lec_active = READ_ONCE(lec_enable);
		oslec_flush(ch->lec);
	}
	if (ch->lec && ch->lec_active && index < EN75XX_PCM_MAX_RING_COUNT) {
		for (i = 0; i < EN75XX_PCM_FRAME_SAMPLES; i++) {
			s16 rx = get_unaligned_le16(tmp + i * 2);
			s16 tx = ch->tx_desc_ref[index][i];

			put_unaligned_le16(oslec_update(ch->lec, tx, rx),
					   tmp + i * 2);
		}
		data = tmp;
	}

	if (do_alc) {
		en75xx_pcm_voice_autogain(ch, (s16 *)tmp, EN75XX_PCM_FRAME_SAMPLES);
		data = tmp;
	}

	spin_lock_irqsave(&ch->fifo_lock, flags);
	if (kfifo_avail(&ch->rx_fifo) < EN75XX_PCM_FRAME_BYTES) {
		u8 discard[EN75XX_PCM_FRAME_BYTES];

		if (kfifo_out(&ch->rx_fifo, discard,
			      EN75XX_PCM_FRAME_BYTES) != EN75XX_PCM_FRAME_BYTES)
			kfifo_reset(&ch->rx_fifo);
		ch->rx_overruns++;
	}
	copied = kfifo_in(&ch->rx_fifo, data, EN75XX_PCM_FRAME_BYTES);
	spin_unlock_irqrestore(&ch->fifo_lock, flags);
	ch->rx_bytes += copied;
	wake_up_interruptible(&ch->rx_wait);
}

static void en75xx_pcm_fill_tx_desc(struct en75xx_pcm_dev *pcm,
				    unsigned int index)
{
	dma_addr_t frame_dma = pcm->tx_buf_dma + index * EN75XX_PCM_FRAME_STRIDE;
	u8 *frame = pcm->tx_buf + index * EN75XX_PCM_FRAME_STRIDE;
	unsigned int channel;

	for (channel = 0; channel < EN75XX_PCM_MAX_CHANNELS; channel++) {
		u8 *dst = frame + channel * EN75XX_PCM_FRAME_BYTES;

		if (pcm->active_mask & BIT(channel))
			en75xx_pcm_fill_tx_channel(pcm, channel, dst, index);
		else
			memset(dst, 0, EN75XX_PCM_FRAME_BYTES);
	}
	en75xx_pcm_desc_prepare(pcm, pcm->tx_ring, index, frame_dma,
				pcm->dma_channel_mask);
}

static void en75xx_pcm_rearm_rx_desc(struct en75xx_pcm_dev *pcm,
				     unsigned int index)
{
	dma_addr_t frame_dma = pcm->rx_buf_dma + index * EN75XX_PCM_FRAME_STRIDE;

	en75xx_pcm_desc_prepare(pcm, pcm->rx_ring, index, frame_dma,
				pcm->dma_channel_mask);
}

static void en75xx_pcm_process(struct en75xx_pcm_dev *pcm)
{
	unsigned int index, channel;

	if (!pcm->running)
		return;

	for (index = 0; index < pcm->soc->ring_count; index++) {
		u8 *frame;

		dma_rmb();
		if (!(en75xx_pcm_desc_status(pcm, pcm->rx_ring, index) &
		      EN75XX_PCM_DESC_OWN)) {
			frame = pcm->rx_buf + index * EN75XX_PCM_FRAME_STRIDE;
			for (channel = 0; channel < EN75XX_PCM_MAX_CHANNELS;
			     channel++) {
				if (!(pcm->active_mask & BIT(channel)))
					continue;
				en75xx_pcm_push_rx_channel(pcm, channel,
					frame + channel * EN75XX_PCM_FRAME_BYTES,
					index);
			}
			en75xx_pcm_rearm_rx_desc(pcm, index);
			pcm_write(pcm, EN75XX_PCM_RX_POLL, 1);
		}

		dma_rmb();
		if (!(en75xx_pcm_desc_status(pcm, pcm->tx_ring, index) &
		      EN75XX_PCM_DESC_OWN)) {
			en75xx_pcm_fill_tx_desc(pcm, index);
			pcm_write(pcm, EN75XX_PCM_TX_POLL, 1);
		}
	}
}

/*
 * Program the interface control register and timeslots to clock PCLK/FSYNC
 * without starting DMA. Slaves like the Si32192 ProSLIC require continuous
 * PCLK/FSYNC to respond to in-band control (ISI/ZSI) even when on-hook/idle.
 */
static void en75xx_pcm_iface_start(struct en75xx_pcm_dev *pcm)
{
	unsigned int i;

	for (i = 0; i < EN75XX_PCM_SLOT_REGS; i++) {
		pcm_write(pcm, EN75XX_PCM_TX_SLOT0 + i * 4, pcm->tx_slots[i]);
		pcm_write(pcm, EN75XX_PCM_RX_SLOT0 + i * 4, pcm->rx_slots[i]);
	}
	/* The vendor driver commits this register with a clear -> set edge. */
	pcm_write(pcm, EN75XX_PCM_IFACE_CTRL,
		  pcm->iface_ctrl & ~EN75XX_PCM_CTRL_CFG_VALID);
	pcm_write(pcm, EN75XX_PCM_IFACE_CTRL,
		  pcm->iface_ctrl | EN75XX_PCM_CTRL_CFG_VALID);
}

void en75xx_pcm_iface_kick(struct en75xx_pcm *pub)
{
	struct en75xx_pcm_dev *pcm;

	if (!pub)
		return;

	pcm = to_pcm_dev(pub);
	mutex_lock(&pcm->lock);
	en75xx_pcm_iface_start(pcm);
	mutex_unlock(&pcm->lock);
}
EXPORT_SYMBOL_GPL(en75xx_pcm_iface_kick);

static int en75xx_pcm_hw_start(struct en75xx_pcm_dev *pcm)
{
	unsigned int index, i;
	u32 dma_ctrl;
	int ret;

	en75xx_pcm_hw_stop(pcm);
	ret = en75xx_pcm_soft_reset(pcm);
	if (ret)
		return ret;

	for (i = 0; i < EN75XX_PCM_SLOT_REGS; i++) {
		pcm_write(pcm, EN75XX_PCM_TX_SLOT0 + i * 4, pcm->tx_slots[i]);
		pcm_write(pcm, EN75XX_PCM_RX_SLOT0 + i * 4, pcm->rx_slots[i]);
	}
	/* The vendor driver commits this register with a clear -> set edge. */
	pcm_write(pcm, EN75XX_PCM_IFACE_CTRL,
		  pcm->iface_ctrl & ~EN75XX_PCM_CTRL_CFG_VALID);
	pcm_write(pcm, EN75XX_PCM_IFACE_CTRL,
		  pcm->iface_ctrl | EN75XX_PCM_CTRL_CFG_VALID);
	pcm_write(pcm, EN75XX_PCM_TX_DESC_BASE,
		en75xx_pcm_dma_addr(pcm, pcm->tx_ring_dma));
	pcm_write(pcm, EN75XX_PCM_RX_DESC_BASE,
		en75xx_pcm_dma_addr(pcm, pcm->rx_ring_dma));
	pcm_write(pcm, EN75XX_PCM_RING_CFG, pcm->soc->ring_cfg);
	/*
	 * The vendor regMap has no register at 0xa8; an earlier revision
	 * wrote one there. 0xac is real: writable mask 0xf, reset 0xf.
	 */
	if (pcm->soc->pcm_v2)
		pcm_write(pcm, EN75XX_PCM_CHAN_ENABLE,
			  pcm->dma_channel_mask & pcm->soc->channel_mask);

	for (index = 0; index < pcm->soc->ring_count; index++) {
		en75xx_pcm_fill_tx_desc(pcm, index);
		en75xx_pcm_rearm_rx_desc(pcm, index);
	}

	pcm_write(pcm, EN75XX_PCM_ISR, pcm_read(pcm, EN75XX_PCM_ISR));
	pcm_write(pcm, EN75XX_PCM_IMR,
		  pcm->irq >= 0 ? EN75XX_PCM_INT_ALL : 0);

	dma_ctrl = pcm_read(pcm, EN75XX_PCM_DMA_CTRL);
	dma_ctrl &= ~(EN75XX_PCM_DMA_CH_MASK |
		      EN75XX_PCM_DMA_TX_EN | EN75XX_PCM_DMA_RX_EN);
	dma_ctrl |= FIELD_PREP(EN75XX_PCM_DMA_CH_MASK, pcm->dma_channel_mask);
	pcm_write(pcm, EN75XX_PCM_DMA_CTRL, dma_ctrl);
	dma_wmb();
	pcm_write(pcm, EN75XX_PCM_DMA_CTRL,
		dma_ctrl | EN75XX_PCM_DMA_TX_EN | EN75XX_PCM_DMA_RX_EN);
	pcm_write(pcm, EN75XX_PCM_RX_POLL, 1);
	pcm_write(pcm, EN75XX_PCM_TX_POLL, 1);
	pcm->running = true;
	return 0;
}

static irqreturn_t en75xx_pcm_irq(int irq, void *data)
{
	struct en75xx_pcm_dev *pcm = data;
	u32 status = pcm_read(pcm, EN75XX_PCM_ISR);

	if (!(status & EN75XX_PCM_INT_ALL))
		return IRQ_NONE;
	return IRQ_WAKE_THREAD;
}

static irqreturn_t en75xx_pcm_irq_thread(int irq, void *data)
{
	struct en75xx_pcm_dev *pcm = data;
	u32 status;

	mutex_lock(&pcm->lock);
	status = pcm_read(pcm, EN75XX_PCM_ISR);
	pcm_write(pcm, EN75XX_PCM_ISR, status);
	if (status & (EN75XX_PCM_INT_AHB_ERR | EN75XX_PCM_INT_RX_OVERRUN))
		pcm->dma_errors++;
	en75xx_pcm_process(pcm);
	mutex_unlock(&pcm->lock);
	return IRQ_HANDLED;
}

static void en75xx_pcm_poll_work(struct work_struct *work)
{
	struct en75xx_pcm_dev *pcm = container_of(to_delayed_work(work),
		struct en75xx_pcm_dev, poll_work);

	mutex_lock(&pcm->lock);
	if (pcm->running) {
		u32 status = pcm_read(pcm, EN75XX_PCM_ISR) &
			     EN75XX_PCM_ISR_VALID;

		if (status) {
			pcm_write(pcm, EN75XX_PCM_ISR, status);
			if (status & EN75XX_PCM_INT_ERR)
				pcm->dma_errors++;
		}
		en75xx_pcm_process(pcm);
		mod_delayed_work(system_highpri_wq, &pcm->poll_work,
				 max_t(unsigned long, 1, msecs_to_jiffies(2)));
	}
	mutex_unlock(&pcm->lock);
}

static int en75xx_pcm_line_start(struct en75xx_pcm *pub, unsigned int channel)
{
	struct en75xx_pcm_dev *pcm = to_pcm_dev(pub);
	int ret = 0;

	if (channel >= EN75XX_PCM_MAX_CHANNELS ||
	    !(pcm->dma_channel_mask & BIT(channel)))
		return -EINVAL;

	mutex_lock(&pcm->lock);
	if (!(pcm->active_mask & BIT(channel))) {
		struct en75xx_pcm_chan *ch = &pcm->chan[channel];

		/* Allocate once so lec_enable can safely toggle during a call. */
		ch->lec_active = false;
		if (lec_taps > 0) {
			ch->lec = oslec_create(lec_taps,
						lec_adaption_mode);
			if (!ch->lec)
				dev_warn(pcm->dev,
					 "channel %u: failed to allocate echo canceller\n",
					 channel);
		}
		memset(ch->tx_desc_ref, 0, sizeof(ch->tx_desc_ref));
		memset(&ch->alc, 0, sizeof(ch->alc));
		ch->alc.cur_mult = 3276;
		ch->alc.noise_flag = 1;

		pcm->active_mask |= BIT(channel);
		if (!pcm->running)
			ret = en75xx_pcm_hw_start(pcm);
		if (ret) {
			pcm->active_mask &= ~BIT(channel);
			if (ch->lec)
				oslec_free(ch->lec);
			ch->lec = NULL;
		} else if (pcm->irq < 0)
			mod_delayed_work(system_highpri_wq, &pcm->poll_work, 1);
	}
	mutex_unlock(&pcm->lock);
	return ret;
}

static void en75xx_pcm_line_stop(struct en75xx_pcm *pub, unsigned int channel)
{
	struct en75xx_pcm_dev *pcm = to_pcm_dev(pub);
	struct en75xx_pcm_chan *ch;

	if (channel >= EN75XX_PCM_MAX_CHANNELS)
		return;
	ch = &pcm->chan[channel];

	mutex_lock(&pcm->lock);
	pcm->active_mask &= ~BIT(channel);
	if (!pcm->active_mask) {
		en75xx_pcm_hw_stop(pcm);
		pcm->running = false;
	}
	if (ch->lec)
		oslec_free(ch->lec);
	ch->lec = NULL;
	mutex_unlock(&pcm->lock);
}

static ssize_t en75xx_pcm_line_read(struct en75xx_pcm *pub, unsigned int channel,
				    void *buf, size_t count, bool nonblock)
{
	struct en75xx_pcm_dev *pcm = to_pcm_dev(pub);
	struct en75xx_pcm_chan *ch;
	unsigned long flags;
	unsigned int copied;
	int ret;

	if (channel >= EN75XX_PCM_MAX_CHANNELS)
		return -EINVAL;
	ch = &pcm->chan[channel];

	if (nonblock && !kfifo_len(&ch->rx_fifo))
		return -EAGAIN;
	if (!nonblock) {
		ret = wait_event_interruptible(ch->rx_wait,
			kfifo_len(&ch->rx_fifo) || !(pcm->active_mask & BIT(channel)));
		if (ret)
			return ret;
	}

	spin_lock_irqsave(&ch->fifo_lock, flags);
	copied = kfifo_out(&ch->rx_fifo, buf, count);
	spin_unlock_irqrestore(&ch->fifo_lock, flags);
	return copied;
}

static ssize_t en75xx_pcm_line_write(struct en75xx_pcm *pub, unsigned int channel,
				     const void *buf, size_t count, bool nonblock)
{
	struct en75xx_pcm_dev *pcm = to_pcm_dev(pub);
	struct en75xx_pcm_chan *ch;
	unsigned long flags;
	unsigned int copied;
	int ret;

	if (channel >= EN75XX_PCM_MAX_CHANNELS)
		return -EINVAL;
	ch = &pcm->chan[channel];

	if (nonblock && !kfifo_avail(&ch->tx_fifo))
		return -EAGAIN;
	if (!nonblock) {
		ret = wait_event_interruptible(ch->tx_wait,
			kfifo_avail(&ch->tx_fifo) || !(pcm->active_mask & BIT(channel)));
		if (ret)
			return ret;
	}

	spin_lock_irqsave(&ch->fifo_lock, flags);
	copied = kfifo_in(&ch->tx_fifo, buf, count);
	spin_unlock_irqrestore(&ch->fifo_lock, flags);
	return copied;
}

static void en75xx_pcm_line_set_alc(struct en75xx_pcm *pub, unsigned int channel,
				    bool enable)
{
	struct en75xx_pcm_dev *pcm = to_pcm_dev(pub);
	struct en75xx_pcm_chan *ch;

	if (channel >= EN75XX_PCM_MAX_CHANNELS)
		return;
	ch = &pcm->chan[channel];
	mutex_lock(&pcm->lock);
	if (ch->alc_suspended == !enable)
		goto out;
	ch->alc_suspended = !enable;
	/* resume from unity so the loop does not inherit a dialling-era gain */
	if (enable)
		en75xx_pcm_alc_reset(&ch->alc);
out:
	mutex_unlock(&pcm->lock);
}

static void en75xx_pcm_line_flush(struct en75xx_pcm *pub, unsigned int channel)
{
	struct en75xx_pcm_dev *pcm = to_pcm_dev(pub);
	struct en75xx_pcm_chan *ch;
	unsigned long flags;

	if (channel >= EN75XX_PCM_MAX_CHANNELS)
		return;
	ch = &pcm->chan[channel];
	mutex_lock(&pcm->lock);
	spin_lock_irqsave(&ch->fifo_lock, flags);
	kfifo_reset(&ch->rx_fifo);
	kfifo_reset(&ch->tx_fifo);
	en75xx_pcm_alc_reset(&ch->alc);
	spin_unlock_irqrestore(&ch->fifo_lock, flags);
	mutex_unlock(&pcm->lock);
}

static void en75xx_pcm_line_get_stats(struct en75xx_pcm *pub,
				      unsigned int channel,
				      struct en75xx_voice_stats *stats)
{
	struct en75xx_pcm_dev *pcm = to_pcm_dev(pub);
	struct en75xx_pcm_chan *ch;

	if (channel >= EN75XX_PCM_MAX_CHANNELS)
		return;
	ch = &pcm->chan[channel];
	/* All counter writers run from the PCM worker/IRQ under this lock. */
	mutex_lock(&pcm->lock);
	stats->rx_bytes = ch->rx_bytes;
	stats->tx_bytes = ch->tx_bytes;
	stats->rx_overruns = ch->rx_overruns;
	stats->tx_underruns = ch->tx_underruns;
	stats->dma_errors = pcm->dma_errors;
	mutex_unlock(&pcm->lock);
}

static int en75xx_pcm_line_set_format(struct en75xx_pcm *pub,
				      unsigned int channel,
				      enum en75xx_pcm_codec codec, bool tx_msb)
{
	struct en75xx_pcm_dev *pcm = to_pcm_dev(pub);
	struct en75xx_pcm_chan *ch;
	unsigned long flags;

	if (channel >= EN75XX_PCM_MAX_CHANNELS)
		return -EINVAL;
	ch = &pcm->chan[channel];

	spin_lock_irqsave(&ch->fifo_lock, flags);
	ch->codec = codec;
	ch->tx_msb = tx_msb;
	kfifo_reset(&ch->rx_fifo);
	kfifo_reset(&ch->tx_fifo);
	spin_unlock_irqrestore(&ch->fifo_lock, flags);

	dev_dbg(pcm->dev, "channel %u: codec=%d tx_msb=%d\n",
		channel, codec, tx_msb);
	return 0;
}

static void en75xx_pcm_line_poll_wait(struct en75xx_pcm *pub,
				      unsigned int channel, struct file *file,
				      struct poll_table_struct *wait)
{
	struct en75xx_pcm_dev *pcm = to_pcm_dev(pub);

	if (channel >= EN75XX_PCM_MAX_CHANNELS)
		return;

	poll_wait(file, &pcm->chan[channel].rx_wait, wait);
	poll_wait(file, &pcm->chan[channel].tx_wait, wait);
}

static unsigned int en75xx_pcm_line_rx_avail(struct en75xx_pcm *pub,
					     unsigned int channel)
{
	struct en75xx_pcm_dev *pcm = to_pcm_dev(pub);

	if (channel >= EN75XX_PCM_MAX_CHANNELS)
		return 0;
	return kfifo_len(&pcm->chan[channel].rx_fifo);
}

static unsigned int en75xx_pcm_line_tx_space(struct en75xx_pcm *pub,
					     unsigned int channel)
{
	struct en75xx_pcm_dev *pcm = to_pcm_dev(pub);

	if (channel >= EN75XX_PCM_MAX_CHANNELS)
		return 0;
	return kfifo_avail(&pcm->chan[channel].tx_fifo);
}

static const struct en75xx_pcm_line_ops en75xx_pcm_line_ops = {
	.start = en75xx_pcm_line_start,
	.stop = en75xx_pcm_line_stop,
	.read = en75xx_pcm_line_read,
	.write = en75xx_pcm_line_write,
	.flush = en75xx_pcm_line_flush,
	.get_stats = en75xx_pcm_line_get_stats,
	.set_format = en75xx_pcm_line_set_format,
	.poll_wait = en75xx_pcm_line_poll_wait,
	.rx_avail = en75xx_pcm_line_rx_avail,
	.set_alc = en75xx_pcm_line_set_alc,
	.tx_space = en75xx_pcm_line_tx_space,
};

int en75xx_pcm_register(struct en75xx_pcm *pcm)
{
	struct en75xx_pcm_dev *priv = to_pcm_dev(pcm);

	mutex_lock(&en75xx_pcm_list_lock);
	list_add_tail(&priv->node, &en75xx_pcm_list);
	mutex_unlock(&en75xx_pcm_list_lock);
	return 0;
}
EXPORT_SYMBOL_GPL(en75xx_pcm_register);

void en75xx_pcm_unregister(struct en75xx_pcm *pcm)
{
	struct en75xx_pcm_dev *priv = to_pcm_dev(pcm);

	mutex_lock(&en75xx_pcm_list_lock);
	list_del_init(&priv->node);
	mutex_unlock(&en75xx_pcm_list_lock);
}
EXPORT_SYMBOL_GPL(en75xx_pcm_unregister);

/*
 * Where in the 8 kHz frame a DMA channel's audio sits, as a bit offset.
 *
 * The timeslot-configuration registers pack two channels per word, and
 * their slot field is a bit offset into the frame rather than a byte
 * timeslot index (see en75xx_pcm_regs.h). Everything that has to line a
 * SLIC up with the DMA engine needs this number, so it is derived from
 * the table the engine was actually programmed with rather than from a
 * formula that could drift away from it.
 */
int en75xx_pcm_channel_bit_offset(struct en75xx_pcm *pub, unsigned int channel)
{
	struct en75xx_pcm_dev *pcm = to_pcm_dev(pub);
	u32 cfg;

	if (channel >= EN75XX_PCM_MAX_CHANNELS)
		return -EINVAL;

	cfg = pcm->rx_slots[channel / 2];
	return (channel & 1) ? FIELD_GET(EN75XX_PCM_TS_HI_NUM, cfg)
			     : FIELD_GET(EN75XX_PCM_TS_LO_NUM, cfg);
}
EXPORT_SYMBOL_GPL(en75xx_pcm_channel_bit_offset);

/*
 * Resolve a PCM *bus* timeslot to the DMA channel that carries it.
 *
 * A SLIC is told a bus timeslot; the DMA engine numbers channels. The
 * two are related only by the timeslot table, so the mapping is whatever
 * that table says and cannot be expressed as a fixed formula. An earlier
 * revision used "slot - 4", which disagreed with this driver's own
 * default table: that table puts channel n at bit offset 32 + n * 16,
 * i.e. at byte timeslot 4 + n * 2, so bus slot 6 is channel 1 and not
 * channel 2. Getting it wrong does not fail; the line just never
 * receives audio.
 *
 * Returns the channel, or -ENOENT when no configured channel covers the
 * slot. TX and RX are checked separately because a board may legitimately
 * program them differently, and a mismatch is worth reporting.
 */
int en75xx_pcm_channel_for_slot(struct en75xx_pcm *pub, unsigned int bus_slot)
{
	struct en75xx_pcm_dev *pcm = to_pcm_dev(pub);
	u32 want = EN75XX_PCM_SLOT_TO_BIT(bus_slot);
	unsigned int channel;
	int rx_match = -ENOENT;
	int tx_match = -ENOENT;

	if (want > FIELD_MAX(EN75XX_PCM_TS_LO_NUM))
		return -EINVAL;

	for (channel = 0; channel < EN75XX_PCM_MAX_CHANNELS; channel++) {
		u32 tx = pcm->tx_slots[channel / 2];
		u32 tx_num;
		int rx_num;

		rx_num = en75xx_pcm_channel_bit_offset(pub, channel);
		tx_num = (channel & 1) ? FIELD_GET(EN75XX_PCM_TS_HI_NUM, tx)
				       : FIELD_GET(EN75XX_PCM_TS_LO_NUM, tx);

		if (rx_match < 0 && rx_num == (int)want)
			rx_match = channel;
		if (tx_match < 0 && tx_num == want)
			tx_match = channel;
	}

	if (rx_match < 0)
		return -ENOENT;
	if (tx_match != rx_match)
		dev_warn(pcm->dev,
			 "bus slot %u is RX channel %d but TX channel %d\n",
			 bus_slot, rx_match, tx_match);
	return rx_match;
}
EXPORT_SYMBOL_GPL(en75xx_pcm_channel_for_slot);

struct en75xx_pcm *en75xx_pcm_get_by_fwnode(struct fwnode_handle *fwnode)
{
	struct en75xx_pcm_dev *pcm;
	struct en75xx_pcm *ret = NULL;

	mutex_lock(&en75xx_pcm_list_lock);
	list_for_each_entry(pcm, &en75xx_pcm_list, node) {
		if (pcm->pub.fwnode == fwnode) {
			get_device(pcm->dev);
			ret = &pcm->pub;
			break;
		}
	}
	mutex_unlock(&en75xx_pcm_list_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(en75xx_pcm_get_by_fwnode);

void en75xx_pcm_put(struct en75xx_pcm *pcm)
{
	if (pcm)
		put_device(pcm->dev);
}
EXPORT_SYMBOL_GPL(en75xx_pcm_put);

static int en75xx_pcm_probe(struct platform_device *pdev)
{
	static const u32 default_slots[EN75XX_PCM_SLOT_REGS] = {
		0x10301020, 0x10501040, 0x10701060, 0x10901080,
	};
	struct device *dev = &pdev->dev;
	struct en75xx_pcm_dev *pcm;
	size_t ring_size, buf_size;
	unsigned int channel;
	u32 channel_mask;
	int ret;

	if (lec_taps < 0 || lec_taps > 4096 ||
	    (lec_taps && (lec_taps < 2 || !is_power_of_2(lec_taps))) ||
	    (lec_adaption_mode & ~0x7f))
		return -EINVAL;

	BUILD_BUG_ON(sizeof(struct en75xx_pcm_desc_v1) != 0x24);
	BUILD_BUG_ON(sizeof(struct en75xx_pcm_desc_v2) != 0x0c);

	pcm = devm_kzalloc(dev, sizeof(*pcm), GFP_KERNEL);
	if (!pcm)
		return -ENOMEM;
	pcm->dev = dev;
	pcm->soc = of_device_get_match_data(dev);
	if (!pcm->soc)
		return -EINVAL;

	pcm->base = devm_platform_ioremap_resource_byname(pdev, "pcm");
	if (IS_ERR(pcm->base))
		pcm->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(pcm->base))
		return PTR_ERR(pcm->base);

	pcm->rst = devm_reset_control_get_optional_exclusive(dev, "pcm");
	if (IS_ERR(pcm->rst))
		return dev_err_probe(dev, PTR_ERR(pcm->rst),
				     "cannot get PCM reset\n");

	mutex_init(&pcm->lock);
	INIT_LIST_HEAD(&pcm->node);
	INIT_DELAYED_WORK(&pcm->poll_work, en75xx_pcm_poll_work);
	pcm->iface_ctrl = EN75XX_PCM_CTRL_OEM;
	pcm->dma_channel_mask = pcm->soc->channel_mask;
	memcpy(pcm->tx_slots, default_slots, sizeof(default_slots));
	memcpy(pcm->rx_slots, default_slots, sizeof(default_slots));
	device_property_read_u32(dev, "airoha,pcm-interface-control",
				 &pcm->iface_ctrl);
	channel_mask = pcm->dma_channel_mask;
	device_property_read_u32(dev, "airoha,dma-channel-mask", &channel_mask);
	if (!channel_mask || channel_mask & ~pcm->soc->channel_mask) {
		dev_err(dev, "invalid DMA channel mask %#x (supported %#x)\n",
			channel_mask, pcm->soc->channel_mask);
		return -EINVAL;
	}
	pcm->dma_channel_mask = channel_mask;
	device_property_read_u32_array(dev, "airoha,tx-slot-config",
				       pcm->tx_slots,
				       EN75XX_PCM_SLOT_REGS);
	device_property_read_u32_array(dev, "airoha,rx-slot-config",
				       pcm->rx_slots,
				       EN75XX_PCM_SLOT_REGS);
	pcm->swap_samples = pcm->soc->swap_samples ||
			    device_property_read_bool(dev, "airoha,pcm-swap-samples");
	pcm->big_endian_samples = device_property_read_bool(dev, "airoha,pcm-big-endian");
	dev_info(dev, "PCM 16-bit linear sample pair swap: %s (default: %s, module param: %d)\n",
		 en75xx_pcm_is_swap_samples(pcm) ? "enabled" : "disabled",
		 pcm->swap_samples ? "enabled" : "disabled",
		 swap_samples);

	ret = dma_set_mask_and_coherent(dev, pcm->soc->dma_mask);
	if (ret)
		return ret;

	ring_size = pcm->soc->ring_count * pcm->soc->desc_size;
	pcm->tx_ring = dmam_alloc_coherent(dev, ring_size, &pcm->tx_ring_dma,
					    GFP_KERNEL);
	pcm->rx_ring = dmam_alloc_coherent(dev, ring_size, &pcm->rx_ring_dma,
					    GFP_KERNEL);
	if (!pcm->tx_ring || !pcm->rx_ring)
		return -ENOMEM;

	buf_size = pcm->soc->ring_count * EN75XX_PCM_FRAME_STRIDE;
	pcm->tx_buf = dmam_alloc_coherent(dev, buf_size, &pcm->tx_buf_dma,
					   GFP_KERNEL);
	pcm->rx_buf = dmam_alloc_coherent(dev, buf_size, &pcm->rx_buf_dma,
					   GFP_KERNEL);
	if (!pcm->tx_buf || !pcm->rx_buf)
		return -ENOMEM;
	if (!en75xx_pcm_dma_range_valid(pcm, pcm->tx_ring_dma, ring_size) ||
	    !en75xx_pcm_dma_range_valid(pcm, pcm->rx_ring_dma, ring_size) ||
	    !en75xx_pcm_dma_range_valid(pcm, pcm->tx_buf_dma, buf_size) ||
	    !en75xx_pcm_dma_range_valid(pcm, pcm->rx_buf_dma, buf_size)) {
		dev_err(dev, "DMA allocation is outside the controller address window\n");
		return -ERANGE;
	}

	for (channel = 0; channel < EN75XX_PCM_MAX_CHANNELS; channel++) {
		struct en75xx_pcm_chan *ch = &pcm->chan[channel];

		spin_lock_init(&ch->fifo_lock);
		init_waitqueue_head(&ch->rx_wait);
		init_waitqueue_head(&ch->tx_wait);
		ret = kfifo_alloc(&ch->rx_fifo, EN75XX_PCM_FIFO_BYTES, GFP_KERNEL);
		if (ret)
			goto err_fifo;
		ret = kfifo_alloc(&ch->tx_fifo, EN75XX_PCM_FIFO_BYTES, GFP_KERNEL);
		if (ret) {
			kfifo_free(&ch->rx_fifo);
			goto err_fifo;
		}
		ch->alc.cur_gain_half_db = 0;
		ch->alc.cur_mult = 3276;
		ch->alc.sum_idx = 0;
		ch->alc.noise_flag = 0;
	}

	pcm->irq = platform_get_irq_optional(pdev, 0);
	if (pcm->irq >= 0) {
		ret = devm_request_threaded_irq(dev, pcm->irq, en75xx_pcm_irq,
				en75xx_pcm_irq_thread, IRQF_ONESHOT,
				dev_name(dev), pcm);
		if (ret)
			goto err_fifo;
	} else if (pcm->irq != -ENXIO && pcm->irq != -ENODEV) {
		ret = pcm->irq;
		goto err_fifo;
	}

	pcm->pub.dev = dev;
	pcm->pub.fwnode = dev_fwnode(dev);
	pcm->pub.line_ops = &en75xx_pcm_line_ops;
	platform_set_drvdata(pdev, pcm);

	en75xx_pcm_hw_stop(pcm);
	if (clock_on_probe)
		en75xx_pcm_iface_start(pcm);
	ret = en75xx_pcm_register(&pcm->pub);
	if (ret)
		goto err_fifo;

	dev_info(dev, "%s PCM: %u descriptors, irq=%d%s\n",
		 pcm->soc->name, pcm->soc->ring_count, pcm->irq,
		 clock_on_probe ? ", interface clock active" : "");
	return 0;

err_fifo:
	while (channel--) {
		kfifo_free(&pcm->chan[channel].rx_fifo);
		kfifo_free(&pcm->chan[channel].tx_fifo);
	}
	return ret;
}

static void en75xx_pcm_remove(struct platform_device *pdev)
{
	struct en75xx_pcm_dev *pcm = platform_get_drvdata(pdev);
	unsigned int channel;

	en75xx_pcm_unregister(&pcm->pub);
	cancel_delayed_work_sync(&pcm->poll_work);
	mutex_lock(&pcm->lock);
	en75xx_pcm_hw_stop(pcm);
	pcm_write(pcm, EN75XX_PCM_IFACE_CTRL, 0);
	pcm->running = false;
	mutex_unlock(&pcm->lock);
	for (channel = 0; channel < EN75XX_PCM_MAX_CHANNELS; channel++) {
		if (pcm->chan[channel].lec)
			oslec_free(pcm->chan[channel].lec);
		kfifo_free(&pcm->chan[channel].rx_fifo);
		kfifo_free(&pcm->chan[channel].tx_fifo);
	}
}

static const struct en75xx_pcm_soc_data en751221_pcm_data = {
	.name = "EN751221",
	.ring_count = EN75XX_PCM_RING_COUNT,
	.desc_size = sizeof(struct en75xx_pcm_desc_v1),
	/*
	 * 0x9f, not the 0xc0 reset value: with 0xc0 the RX descriptor
	 * OWN bit never clears and RX times out.
	 */
	.ring_cfg = 0x9f,
	.dma_mask = 0x1fffffff,
	.channel_mask = GENMASK(7, 0),
};

static const struct en75xx_pcm_soc_data en7528_pcm_data = {
	.name = "EN7528",
	.ring_count = EN75XX_PCM_RING_COUNT,
	.desc_size = sizeof(struct en75xx_pcm_desc_v1),
	.ring_cfg = 0x9f,
	.dma_mask = 0x1fffffff,
	.channel_mask = GENMASK(7, 0),
	.swap_samples = true,
};

static const struct en75xx_pcm_soc_data en7523_pcm_data = {
	.name = "EN7523",
	.ring_count = EN75XX_PCM_RING_COUNT,
	.desc_size = sizeof(struct en75xx_pcm_desc_v2),
	.ring_cfg = 0x3f,
	.dma_mask = 0x3fffffff,
	.dma_or = 0x80000000,
	.channel_mask = GENMASK(3, 0),
	.pcm_v2 = true,
};

static const struct of_device_id en75xx_pcm_of_match[] = {
	{ .compatible = "econet,en751221-pcm", .data = &en751221_pcm_data },
	{ .compatible = "econet,en7528-pcm", .data = &en7528_pcm_data },
	{ .compatible = "airoha,en7523-pcm", .data = &en7523_pcm_data },
	{ }
};
MODULE_DEVICE_TABLE(of, en75xx_pcm_of_match);

static struct platform_driver en75xx_pcm_driver = {
	.probe = en75xx_pcm_probe,
	.remove = en75xx_pcm_remove,
	.driver = {
		.name = "en75xx-pcm",
		.of_match_table = en75xx_pcm_of_match,
	},
};
module_platform_driver(en75xx_pcm_driver);

MODULE_DESCRIPTION("EcoNet/Airoha EN751221/EN7528/EN7523 PCM/TDM driver");
MODULE_LICENSE("GPL");
