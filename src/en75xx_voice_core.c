// SPDX-License-Identifier: GPL-2.0
/*
 * Common userspace interface for EcoNet/Airoha FXS devices.
 *
 * Each registered FXS line is exported as /dev/en75xx-fxsN. Audio is always
 * signed 16-bit little-endian mono at 8 kHz, one 10 ms frame per 160 bytes.
 */
#include <linux/atomic.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#include "../include/en75xx_voice.h"

struct en75xx_voice_line {
	struct device *dev;
	struct en75xx_pcm *pcm;
	unsigned int pcm_channel;
	unsigned int line;
	char slic_name[32];
	const struct en75xx_voice_slic_ops *slic_ops;
	void *slic_priv;
	struct miscdevice misc;
	struct mutex lock;
	wait_queue_head_t event_wait;
	atomic_t event_seq;
	bool opened;
	bool ringing;
	enum en75xx_voice_linefeed linefeed;
	u64 hook_changes;
};

static DEFINE_IDA(en75xx_voice_line_ida);

struct en75xx_voice_file {
	struct en75xx_voice_line *line;
	int event_seq;
};

static int en75xx_voice_open(struct inode *inode, struct file *file)
{
	struct miscdevice *misc = file->private_data;
	struct en75xx_voice_line *line;
	struct en75xx_voice_file *vf;
	int ret;

	line = container_of(misc, struct en75xx_voice_line, misc);
	vf = kzalloc(sizeof(*vf), GFP_KERNEL);
	if (!vf)
		return -ENOMEM;

	mutex_lock(&line->lock);
	if (line->opened) {
		ret = -EBUSY;
		goto out_unlock;
	}

	ret = line->pcm->line_ops->start(line->pcm, line->pcm_channel);
	if (ret)
		goto out_unlock;

	line->opened = true;
	vf->line = line;
	vf->event_seq = atomic_read(&line->event_seq);
	file->private_data = vf;
	mutex_unlock(&line->lock);
	return 0;

out_unlock:
	mutex_unlock(&line->lock);
	kfree(vf);
	return ret;
}

static int en75xx_voice_release(struct inode *inode, struct file *file)
{
	struct en75xx_voice_file *vf = file->private_data;
	struct en75xx_voice_line *line = vf->line;

	mutex_lock(&line->lock);
	if (line->ringing && line->slic_ops->ring) {
		line->slic_ops->ring(line->slic_priv, false, 0, 0);
		line->ringing = false;
	}
	if (line->opened) {
		line->pcm->line_ops->stop(line->pcm, line->pcm_channel);
		line->opened = false;
	}
	mutex_unlock(&line->lock);
	kfree(vf);
	return 0;
}

static ssize_t en75xx_voice_read(struct file *file, char __user *buf,
				 size_t count, loff_t *ppos)
{
	struct en75xx_voice_file *vf = file->private_data;
	struct en75xx_voice_line *line = vf->line;
	void *tmp;
	ssize_t ret;

	if (!count)
		return 0;
	count &= ~1UL;
	if (!count)
		return -EINVAL;

	tmp = kmalloc(min_t(size_t, count, 4096), GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	ret = line->pcm->line_ops->read(line->pcm, line->pcm_channel, tmp,
					min_t(size_t, count, 4096),
					file->f_flags & O_NONBLOCK);
	if (ret > 0 && copy_to_user(buf, tmp, ret))
		ret = -EFAULT;
	kfree(tmp);
	return ret;
}

static ssize_t en75xx_voice_write(struct file *file, const char __user *buf,
				  size_t count, loff_t *ppos)
{
	struct en75xx_voice_file *vf = file->private_data;
	struct en75xx_voice_line *line = vf->line;
	void *tmp;
	ssize_t ret;

	if (!count)
		return 0;
	count &= ~1UL;
	if (!count)
		return -EINVAL;
	count = min_t(size_t, count, 4096);

	tmp = memdup_user(buf, count);
	if (IS_ERR(tmp))
		return PTR_ERR(tmp);
	ret = line->pcm->line_ops->write(line->pcm, line->pcm_channel, tmp,
					 count, file->f_flags & O_NONBLOCK);
	kfree(tmp);
	return ret;
}

static long en75xx_voice_ioctl(struct file *file, unsigned int cmd,
			       unsigned long arg)
{
	struct en75xx_voice_file *vf = file->private_data;
	struct en75xx_voice_line *line = vf->line;
	struct en75xx_voice_line_state state = {};
	struct en75xx_voice_info info = {};
	struct en75xx_voice_ring ring;
	struct en75xx_voice_stats stats = {};
	u32 linefeed, faults = 0;
	int ret;

	switch (cmd) {
	case EN75XX_VOICE_GET_INFO:
		info.abi_version = EN75XX_VOICE_ABI_VERSION;
		info.line = line->line;
		info.pcm_channel = line->pcm_channel;
		info.sample_rate = EN75XX_VOICE_RATE;
		info.sample_bits = EN75XX_VOICE_SAMPLE_BITS;
		info.frame_samples = EN75XX_VOICE_FRAME_SAMPLES;
		info.capabilities = EN75XX_VOICE_CAP_PCM |
			EN75XX_VOICE_CAP_RING | EN75XX_VOICE_CAP_HOOK |
			EN75XX_VOICE_CAP_LINEFEED;
		strscpy(info.slic, line->slic_name, sizeof(info.slic));
		return copy_to_user((void __user *)arg, &info, sizeof(info)) ?
			-EFAULT : 0;
	case EN75XX_VOICE_GET_STATE:
		ret = line->slic_ops->get_hook ?
			line->slic_ops->get_hook(line->slic_priv) : -EOPNOTSUPP;
		if (ret < 0)
			return ret;
		state.hook = ret;
		state.linefeed = line->linefeed;
		state.ringing = line->ringing;
		if (line->slic_ops->get_faults)
			line->slic_ops->get_faults(line->slic_priv, &faults);
		state.faults = faults;
		vf->event_seq = atomic_read(&line->event_seq);
		return copy_to_user((void __user *)arg, &state, sizeof(state)) ?
			-EFAULT : 0;
	case EN75XX_VOICE_SET_RING:
		if (copy_from_user(&ring, (void __user *)arg, sizeof(ring)))
			return -EFAULT;
		if (!line->slic_ops->ring)
			return -EOPNOTSUPP;
		ret = line->slic_ops->ring(line->slic_priv, !!ring.enable,
					  ring.cadence_on_ms, ring.cadence_off_ms);
		if (!ret)
			line->ringing = !!ring.enable;
		return ret;
	case EN75XX_VOICE_SET_LINEFEED:
		if (copy_from_user(&linefeed, (void __user *)arg, sizeof(linefeed)))
			return -EFAULT;
		if (linefeed > EN75XX_VOICE_LINEFEED_REVERSE)
			return -EINVAL;
		if (!line->slic_ops->set_linefeed)
			return -EOPNOTSUPP;
		ret = line->slic_ops->set_linefeed(line->slic_priv, linefeed);
		if (!ret)
			line->linefeed = linefeed;
		return ret;
	case EN75XX_VOICE_FLUSH:
		line->pcm->line_ops->flush(line->pcm, line->pcm_channel);
		return 0;
	case EN75XX_VOICE_GET_STATS:
		line->pcm->line_ops->get_stats(line->pcm, line->pcm_channel, &stats);
		stats.hook_changes = line->hook_changes;
		return copy_to_user((void __user *)arg, &stats, sizeof(stats)) ?
			-EFAULT : 0;
	default:
		return -ENOTTY;
	}
}

static __poll_t en75xx_voice_poll(struct file *file, poll_table *wait)
{
	struct en75xx_voice_file *vf = file->private_data;
	struct en75xx_voice_line *line = vf->line;
	const struct en75xx_pcm_line_ops *ops = line->pcm->line_ops;
	__poll_t mask = EPOLLOUT | EPOLLWRNORM;

	poll_wait(file, &line->event_wait, wait);
	if (ops->poll_wait)
		ops->poll_wait(line->pcm, line->pcm_channel, file, wait);

	if (vf->event_seq != atomic_read(&line->event_seq))
		mask |= EPOLLPRI;

	/*
	 * Report readability only when a whole frame is queued. Saying
	 * "always readable" makes a poll-driven consumer spin, or block
	 * in read() from inside its I/O loop.
	 */
	if (!ops->rx_avail ||
	    ops->rx_avail(line->pcm, line->pcm_channel) >=
	    EN75XX_VOICE_FRAME_BYTES)
		mask |= EPOLLIN | EPOLLRDNORM;

	if (ops->tx_space &&
	    ops->tx_space(line->pcm, line->pcm_channel) <
	    EN75XX_VOICE_FRAME_BYTES)
		mask &= ~(EPOLLOUT | EPOLLWRNORM);

	return mask;
}

static const struct file_operations en75xx_voice_fops = {
	.owner = THIS_MODULE,
	.open = en75xx_voice_open,
	.release = en75xx_voice_release,
	.read = en75xx_voice_read,
	.write = en75xx_voice_write,
	.unlocked_ioctl = en75xx_voice_ioctl,
	.poll = en75xx_voice_poll,
};

struct en75xx_voice_line *
en75xx_voice_register_line(struct device *dev, struct en75xx_pcm *pcm,
			   unsigned int pcm_channel, unsigned int line_no,
			   const char *slic_name,
			   const struct en75xx_voice_slic_ops *slic_ops,
			   void *slic_priv)
{
	struct en75xx_voice_line *line;
	int ret;

	if (!pcm || !pcm->line_ops || !slic_ops || !slic_ops->get_hook)
		return ERR_PTR(-EINVAL);

	line = devm_kzalloc(dev, sizeof(*line), GFP_KERNEL);
	if (!line)
		return ERR_PTR(-ENOMEM);

	line->dev = dev;
	line->pcm = pcm;
	line->pcm_channel = pcm_channel;
	/* Keep a requested board number when free, but never collide globally. */
	ret = ida_alloc_range(&en75xx_voice_line_ida, line_no, line_no,
			      GFP_KERNEL);
	if (ret == -ENOSPC)
		ret = ida_alloc(&en75xx_voice_line_ida, GFP_KERNEL);
	if (ret < 0)
		return ERR_PTR(ret);
	line->line = ret;
	line->slic_ops = slic_ops;
	line->slic_priv = slic_priv;
	line->linefeed = EN75XX_VOICE_LINEFEED_STANDBY;
	strscpy(line->slic_name, slic_name, sizeof(line->slic_name));
	mutex_init(&line->lock);
	init_waitqueue_head(&line->event_wait);
	atomic_set(&line->event_seq, 0);

	line->misc.minor = MISC_DYNAMIC_MINOR;
	line->misc.name = devm_kasprintf(dev, GFP_KERNEL, "en75xx-fxs%u",
					 line->line);
	if (!line->misc.name) {
		ret = -ENOMEM;
		goto err_ida;
	}
	line->misc.fops = &en75xx_voice_fops;
	line->misc.parent = dev;
	ret = misc_register(&line->misc);
	if (ret)
		goto err_ida;

	dev_info(dev, "registered %s as /dev/%s on PCM channel %u\n",
		 slic_name, line->misc.name, pcm_channel);
	return line;

err_ida:
	ida_free(&en75xx_voice_line_ida, line->line);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_GPL(en75xx_voice_register_line);

void en75xx_voice_unregister_line(struct en75xx_voice_line *line)
{
	if (line) {
		misc_deregister(&line->misc);
		ida_free(&en75xx_voice_line_ida, line->line);
	}
}
EXPORT_SYMBOL_GPL(en75xx_voice_unregister_line);

void en75xx_voice_hook_changed(struct en75xx_voice_line *line, bool offhook)
{
	if (!line)
		return;
	line->hook_changes++;
	atomic_inc(&line->event_seq);
	wake_up_interruptible_poll(&line->event_wait, EPOLLPRI);
}
EXPORT_SYMBOL_GPL(en75xx_voice_hook_changed);

MODULE_DESCRIPTION("EcoNet/Airoha EN75xx voice userspace core");
MODULE_LICENSE("GPL");
