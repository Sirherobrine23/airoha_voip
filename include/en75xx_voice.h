/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _EN75XX_VOICE_H
#define _EN75XX_VOICE_H

#include <linux/device.h>
#include <linux/fwnode.h>
#include <linux/poll.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <uapi/linux/en75xx_voice.h>

struct en75xx_pcm;
struct en75xx_voice_line;

/*
 * Wire format of one PCM timeslot. Some SLICs (notably the Le9642)
 * only drive 8 bits per slot and must run their codec in G.711; the
 * companding then happens in the PCM data path, and the character
 * device stays 16-bit linear either way.
 */
enum en75xx_pcm_codec {
	EN75XX_PCM_CODEC_LINEAR16 = 0,
	EN75XX_PCM_CODEC_ULAW,
	EN75XX_PCM_CODEC_ALAW,
};

struct en75xx_pcm_line_ops {
	int (*start)(struct en75xx_pcm *pcm, unsigned int channel);
	void (*stop)(struct en75xx_pcm *pcm, unsigned int channel);
	ssize_t (*read)(struct en75xx_pcm *pcm, unsigned int channel,
			void *buf, size_t count, bool nonblock);
	ssize_t (*write)(struct en75xx_pcm *pcm, unsigned int channel,
			 const void *buf, size_t count, bool nonblock);
	void (*flush)(struct en75xx_pcm *pcm, unsigned int channel);
	void (*get_stats)(struct en75xx_pcm *pcm, unsigned int channel,
			  struct en75xx_voice_stats *stats);
	/*
	 * @tx_msb selects which byte of the 16-bit slot carries the
	 * G.711 code on playback: the SLIC can use different clock-slot
	 * offsets for capture and playback, so the code does not
	 * necessarily land in the same byte in both directions.
	 */
	int (*set_format)(struct en75xx_pcm *pcm, unsigned int channel,
			  enum en75xx_pcm_codec codec, bool tx_msb);
	/*
	 * Needed so poll() can report readability honestly. Without it
	 * a poll-driven consumer (an Asterisk channel driver, say) is
	 * told the fd is always readable and then blocks in read().
	 */
	void (*poll_wait)(struct en75xx_pcm *pcm, unsigned int channel,
			  struct file *file, struct poll_table_struct *wait);
	unsigned int (*rx_avail)(struct en75xx_pcm *pcm, unsigned int channel);
	unsigned int (*tx_space)(struct en75xx_pcm *pcm, unsigned int channel);
};

struct en75xx_voice_slic_ops {
	int (*get_hook)(void *priv);
	int (*ring)(void *priv, bool enable, unsigned int on_ms,
		    unsigned int off_ms);
	int (*set_linefeed)(void *priv, enum en75xx_voice_linefeed state);
	int (*get_faults)(void *priv, u32 *faults);
};

struct en75xx_pcm {
	struct device *dev;
	struct fwnode_handle *fwnode;
	const struct en75xx_pcm_line_ops *line_ops;
};

struct en75xx_pcm *en75xx_pcm_get_by_fwnode(struct fwnode_handle *fwnode);
/*
 * Map a PCM bus timeslot to the DMA channel carrying it, using the
 * timeslot table the PCM engine was actually programmed with. Returns
 * -ENOENT when no configured channel covers the slot.
 */
int en75xx_pcm_channel_for_slot(struct en75xx_pcm *pcm, unsigned int bus_slot);
/*
 * Where a DMA channel's audio sits in the frame, as a bit offset. This
 * is the number a SLIC's own PCM timeslot registers want.
 */
int en75xx_pcm_channel_bit_offset(struct en75xx_pcm *pcm, unsigned int channel);
void en75xx_pcm_put(struct en75xx_pcm *pcm);
int en75xx_pcm_register(struct en75xx_pcm *pcm);
void en75xx_pcm_unregister(struct en75xx_pcm *pcm);

struct en75xx_voice_line *
en75xx_voice_register_line(struct device *dev, struct en75xx_pcm *pcm,
			   unsigned int pcm_channel, unsigned int line,
			   const char *slic_name,
			   const struct en75xx_voice_slic_ops *slic_ops,
			   void *slic_priv);
void en75xx_voice_unregister_line(struct en75xx_voice_line *line);
void en75xx_voice_hook_changed(struct en75xx_voice_line *line, bool offhook);

#endif /* _EN75XX_VOICE_H */
