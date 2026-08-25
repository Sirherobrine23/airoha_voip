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
#define G711_CLIP	32635

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
	static const u8 seg_end[8] = { 0x1f, 0x3f, 0x7f, 0xff,
				       0x1ff, 0x3ff, 0x7ff, 0xfff };
	int sign = (sample >> 8) & 0x80;
	int value = sign ? -sample : sample;
	int seg;

	if (value > G711_CLIP)
		value = G711_CLIP;
	value += G711_BIAS >> 2;
	value >>= 2;

	for (seg = 0; seg < 8; seg++)
		if (value <= seg_end[seg])
			break;

	if (seg >= 8)
		return (u8)(0x7f ^ sign);

	return (u8)~(sign | (seg << 4) |
		     ((value >> (seg + 1)) & 0x0f));
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

	return sign ? (s16)-sample : (s16)sample;
}

static inline u8 en75xx_alaw_encode(s16 sample)
{
	static const u8 seg_end[8] = { 0x1f, 0x3f, 0x7f, 0xff,
				       0x1ff, 0x3ff, 0x7ff, 0xfff };
	int sign = ((~sample) >> 8) & 0x80;
	int value = sign ? sample : -sample;
	int seg, out;

	if (value > G711_CLIP)
		value = G711_CLIP;

	for (seg = 0; seg < 8; seg++)
		if (value <= seg_end[seg])
			break;

	if (seg >= 8)
		return (u8)((0x7f ^ 0x55) | sign);

	out = (seg << 4);
	if (seg < 2)
		out |= (value >> 4) & 0x0f;
	else
		out |= (value >> (seg + 3)) & 0x0f;

	return (u8)((out | sign) ^ 0x55);
}

#endif /* _EN75XX_G711_H */
