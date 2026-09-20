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

struct en75xx_pcm_soc_data {
	const char *name;
	unsigned int ring_count;
	size_t desc_size;
	u32 ring_cfg;
	u32 dma_mask;
	u32 dma_or;
	u8 channel_mask;
	bool pcm_v2;		/* EN7523: 12-byte descriptor, CHAN_ENABLE */
};

struct en75xx_pcm_chan {
	struct kfifo rx_fifo;
	struct kfifo tx_fifo;
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
	bool big_endian_samples;	/* byte order within each 16-bit sample */
	bool swap_samples;		/* order of samples within each 32-bit DMA word */
	u64 dma_errors;
};

static LIST_HEAD(en75xx_pcm_list);
static DEFINE_MUTEX(en75xx_pcm_list_lock);

static inline struct en75xx_pcm_dev *to_pcm_dev(struct en75xx_pcm *pcm)
{
	return container_of(pcm, struct en75xx_pcm_dev, pub);
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
				       unsigned int channel, u8 *dst)
{
	struct en75xx_pcm_chan *ch = &pcm->chan[channel];
	unsigned long flags;
	unsigned int copied;

	spin_lock_irqsave(&ch->fifo_lock, flags);
	copied = kfifo_out(&ch->tx_fifo, dst, EN75XX_PCM_FRAME_BYTES);
	spin_unlock_irqrestore(&ch->fifo_lock, flags);

	if (copied < EN75XX_PCM_FRAME_BYTES) {
		memset(dst + copied, 0, EN75XX_PCM_FRAME_BYTES - copied);
		ch->tx_underruns++;
	}
	ch->tx_bytes += copied;
	if (ch->codec != EN75XX_PCM_CODEC_LINEAR16) {
		en75xx_pcm_compand_tx(ch, dst);
	} else if (pcm->big_endian_samples) {
		unsigned int i;

		for (i = 0; i < EN75XX_PCM_FRAME_BYTES; i += 2)
			swap(dst[i], dst[i + 1]);
	} else if (pcm->swap_samples) {
		/*
		 * EN7528's gen1 PCM DMA packs two 16-bit linear samples per
		 * 32-bit word in reverse chronological order on this SoC.
		 * Swapping the pair here restores playback order; see
		 * en75xx_pcm_push_rx_channel() for the matching RX unswap.
		 */
		u16 *s = (u16 *)dst;
		unsigned int i;

		for (i = 0; i < EN75XX_PCM_FRAME_SAMPLES; i += 2)
			swap(s[i], s[i + 1]);
	}
	wake_up_interruptible(&ch->tx_wait);
}

static void en75xx_pcm_push_rx_channel(struct en75xx_pcm_dev *pcm,
				       unsigned int channel, const u8 *src)
{
	struct en75xx_pcm_chan *ch = &pcm->chan[channel];
	unsigned long flags;
	unsigned int copied;
	u8 tmp[EN75XX_PCM_FRAME_BYTES];
	const u8 *data = src;
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
		goto queue;
	}

	if (pcm->big_endian_samples) {
		for (i = 0; i < EN75XX_PCM_FRAME_BYTES; i += 2) {
			tmp[i] = src[i + 1];
			tmp[i + 1] = src[i];
		}
		data = tmp;
	} else if (pcm->swap_samples) {
		const u16 *s = (const u16 *)src;
		u16 *d = (u16 *)tmp;

		for (i = 0; i < EN75XX_PCM_FRAME_SAMPLES; i += 2) {
			d[i] = s[i + 1];
			d[i + 1] = s[i];
		}
		data = tmp;
	}

queue:
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
			en75xx_pcm_fill_tx_channel(pcm, channel, dst);
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
					frame + channel * EN75XX_PCM_FRAME_BYTES);
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
		pcm->active_mask |= BIT(channel);
		if (!pcm->running)
			ret = en75xx_pcm_hw_start(pcm);
		if (ret)
			pcm->active_mask &= ~BIT(channel);
		else if (pcm->irq < 0)
			mod_delayed_work(system_highpri_wq, &pcm->poll_work, 1);
	}
	mutex_unlock(&pcm->lock);
	return ret;
}

static void en75xx_pcm_line_stop(struct en75xx_pcm *pub, unsigned int channel)
{
	struct en75xx_pcm_dev *pcm = to_pcm_dev(pub);

	if (channel >= EN75XX_PCM_MAX_CHANNELS)
		return;

	mutex_lock(&pcm->lock);
	pcm->active_mask &= ~BIT(channel);
	if (!pcm->active_mask) {
		en75xx_pcm_hw_stop(pcm);
		pcm->running = false;
	}
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

static void en75xx_pcm_line_flush(struct en75xx_pcm *pub, unsigned int channel)
{
	struct en75xx_pcm_dev *pcm = to_pcm_dev(pub);
	struct en75xx_pcm_chan *ch;
	unsigned long flags;

	if (channel >= EN75XX_PCM_MAX_CHANNELS)
		return;
	ch = &pcm->chan[channel];
	spin_lock_irqsave(&ch->fifo_lock, flags);
	kfifo_reset(&ch->rx_fifo);
	kfifo_reset(&ch->tx_fifo);
	spin_unlock_irqrestore(&ch->fifo_lock, flags);
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
	pcm->big_endian_samples = device_property_read_bool(dev,
						    "airoha,pcm-big-endian");
	pcm->swap_samples = device_property_read_bool(dev,
						    "airoha,pcm-swap-samples");

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
	ret = en75xx_pcm_register(&pcm->pub);
	if (ret)
		goto err_fifo;

	dev_info(dev, "%s PCM: %u descriptors, irq=%d\n",
		 pcm->soc->name, pcm->soc->ring_count, pcm->irq);
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
	pcm->running = false;
	mutex_unlock(&pcm->lock);
	for (channel = 0; channel < EN75XX_PCM_MAX_CHANNELS; channel++) {
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
