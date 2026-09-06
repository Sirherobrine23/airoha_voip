/* SPDX-License-Identifier: GPL-2.0 */
/*
 * G.711 companding used on the PCM wire.
 *
 * Needed because some SLICs on this platform drive only 8 bits per
 * timeslot: with a 16-bit linear codec the second byte of each slot is
 * always zero, so quiet speech quantises to silence and the audio
 * "cuts out on every word". Running the codec in u-law and companding
 * in the data path keeps the low-level detail; the character device
 * still presents 16-bit linear so userspace is unaffected.
 */
#ifndef _EN75XX_G711_H
#define _EN75XX_G711_H

#include <linux/types.h>

#define G711_BIAS	0x84

static inline int en75xx_ulaw_segment(int value)
{
	int segment = 0;

	value >>= 7;
	if (value & 0xf0) {
		value >>= 4;
		segment += 4;
	}
	if (value & 0x0c) {
		value >>= 2;
		segment += 2;
	}
	if (value & 0x02)
		segment++;

	return segment;
}

static inline int en75xx_alaw_segment(int value)
{
	static const u16 segment_end[8] = {
		0x00ff, 0x01ff, 0x03ff, 0x07ff,
		0x0fff, 0x1fff, 0x3fff, 0x7fff,
	};
	int segment;

	for (segment = 0; segment < 8; segment++)
		if (value <= segment_end[segment])
			break;

	return segment;
}

static inline s16 en75xx_ulaw_decode(u8 byte)
{
	int u = (~byte) & 0xff;
	int sign = u & 0x80;
	int exponent = (u >> 4) & 0x07;
	int mantissa = u & 0x0f;
	int sample = (((mantissa << 3) + G711_BIAS) << exponent) - G711_BIAS;

	return sign ? (s16)-sample : (s16)sample;
}

static inline u8 en75xx_ulaw_encode(s16 sample)
{
	int value = sample;
	int mask;
	int segment;
	u8 encoded;

	if (value < 0) {
		value = G711_BIAS - value;
		mask = 0x7f;
	} else {
		value += G711_BIAS;
		mask = 0xff;
	}
	if (value > 0x7fff)
		value = 0x7fff;

	segment = en75xx_ulaw_segment(value);
	encoded = (segment << 4) |
		  ((value >> (segment + 3)) & 0x0f);

	return encoded ^ mask;
}

static inline s16 en75xx_alaw_decode(u8 byte)
{
	int a = byte ^ 0x55;
	int sign = a & 0x80;
	int exponent = (a >> 4) & 0x07;
	int mantissa = a & 0x0f;
	int sample;

	if (exponent)
		sample = ((mantissa << 4) + 0x108) << (exponent - 1);
	else
		sample = (mantissa << 4) + 8;

	return sign ? (s16)sample : (s16)-sample;
}

static inline u8 en75xx_alaw_encode(s16 sample)
{
	int value = sample;
	int mask;
	int segment;
	u8 encoded;

	if (value >= 0) {
		mask = 0xd5;
	} else {
		mask = 0x55;
		value = -value;
	}
	/* -32768 has no positive s16 counterpart. */
	if (value > 0x7fff)
		value = 0x7fff;

	segment = en75xx_alaw_segment(value);
	encoded = (segment << 4) |
		  ((value >> (segment ? segment + 3 : 4)) & 0x0f);

	return encoded ^ mask;
}

#endif /* _EN75XX_G711_H */
