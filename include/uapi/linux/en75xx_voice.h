/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_LINUX_EN75XX_VOICE_H
#define _UAPI_LINUX_EN75XX_VOICE_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define EN75XX_VOICE_IOC_MAGIC 'V'
#define EN75XX_VOICE_ABI_VERSION 1

#define EN75XX_VOICE_RATE 8000
#define EN75XX_VOICE_SAMPLE_BITS 16
#define EN75XX_VOICE_FRAME_SAMPLES 80
#define EN75XX_VOICE_FRAME_BYTES (EN75XX_VOICE_FRAME_SAMPLES * 2)

enum en75xx_voice_hook_state {
	EN75XX_VOICE_ONHOOK = 0,
	EN75XX_VOICE_OFFHOOK = 1,
};

enum en75xx_voice_linefeed {
	EN75XX_VOICE_LINEFEED_OPEN = 0,
	EN75XX_VOICE_LINEFEED_STANDBY = 1,
	EN75XX_VOICE_LINEFEED_ACTIVE = 2,
	EN75XX_VOICE_LINEFEED_REVERSE = 3,
};

struct en75xx_voice_info {
	__u32 abi_version;
	__u32 line;
	__u32 pcm_channel;
	__u32 sample_rate;
	__u32 sample_bits;
	__u32 frame_samples;
	__u32 capabilities;
	__u32 reserved;
	char slic[32];
};

#define EN75XX_VOICE_CAP_RING (1U << 0)
#define EN75XX_VOICE_CAP_HOOK (1U << 1)
#define EN75XX_VOICE_CAP_LINEFEED (1U << 2)
#define EN75XX_VOICE_CAP_PCM (1U << 3)

struct en75xx_voice_line_state {
	__u32 hook;
	__u32 linefeed;
	__u32 ringing;
	__u32 faults;
};

struct en75xx_voice_ring {
	__u32 enable;
	__u32 cadence_on_ms;
	__u32 cadence_off_ms;
	__u32 reserved;
};

struct en75xx_voice_stats {
	__u64 rx_bytes;
	__u64 tx_bytes;
	__u64 rx_overruns;
	__u64 tx_underruns;
	__u64 hook_changes;
	__u64 dma_errors;
};

#define EN75XX_VOICE_GET_INFO       _IOR(EN75XX_VOICE_IOC_MAGIC, 0x00, struct en75xx_voice_info)
#define EN75XX_VOICE_GET_STATE      _IOR(EN75XX_VOICE_IOC_MAGIC, 0x01, struct en75xx_voice_line_state)
#define EN75XX_VOICE_SET_RING       _IOW(EN75XX_VOICE_IOC_MAGIC, 0x02, struct en75xx_voice_ring)
#define EN75XX_VOICE_SET_LINEFEED   _IOW(EN75XX_VOICE_IOC_MAGIC, 0x03, __u32)
#define EN75XX_VOICE_FLUSH          _IO(EN75XX_VOICE_IOC_MAGIC, 0x04)
#define EN75XX_VOICE_GET_STATS      _IOR(EN75XX_VOICE_IOC_MAGIC, 0x05, struct en75xx_voice_stats)

#endif /* _UAPI_LINUX_EN75XX_VOICE_H */
