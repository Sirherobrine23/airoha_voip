/* SPDX-License-Identifier: GPL-2.0 */
/*
 * EcoNet/Airoha EN75xx PCM/TDM register and descriptor definitions.
 *
 * Sources of truth, in order of authority:
 *
 *  1. The vendor pcm1.ko "regMap" table (name/mask/address/reset per
 *     register) recovered from .data on EN7523, EN751221 and EN7528.
 *  2. The vendor debug printk in descGet(), which literally names the
 *     status fields: "ownership", "chvaild", "sample size".
 *  3. Register values read back from a stock EN751221 with the OEM
 *     voice stack running.
 *
 * IMPORTANT: the descriptor layout is NOT the same across generations.
 *
 *   gen1 (EN751221, EN7528, MIPS BE): stride 0x24 = 36 bytes.
 *       pcmKmalloc(0x21c) = 540 = 15 * 36, rxDescSet() indexes
 *       ring + n * 0x24, descGet() dumps buf0..buf7 and the channel
 *       mask lives in status byte 1 (bits 23:16).
 *       -> u32 status; u32 buf_addr[8];
 *
 *   gen2 (EN7523, ARM LE): stride 0x0c = 12 bytes.
 *       pcmKmalloc(0xb4) = 180 = 15 * 12, rxDescSet() indexes
 *       ring + n * 0xc, writes the channel mask as a full word at +4
 *       and a single buffer pointer at +8; descGet() dumps only buf0.
 *       -> u32 status; u32 ch_valid; u32 buf_addr;
 *
 * Both generations have 15 descriptors per ring: the vendor index
 * arithmetic is "% 0xf" in both, and both allocations divide evenly.
 */
#ifndef _EN75XX_PCM_REGS_H
#define _EN75XX_PCM_REGS_H

#include <linux/bits.h>
#include <linux/types.h>

#define EN75XX_PCM_IFACE_CTRL		0x00
#define EN75XX_PCM_TX_SLOT0		0x04
#define EN75XX_PCM_RX_SLOT0		0x14
#define EN75XX_PCM_ISR			0x24
#define EN75XX_PCM_IMR			0x28
#define EN75XX_PCM_TX_POLL		0x2c
#define EN75XX_PCM_RX_POLL		0x30
#define EN75XX_PCM_TX_DESC_BASE		0x34
#define EN75XX_PCM_RX_DESC_BASE		0x38
#define EN75XX_PCM_RING_CFG		0x3c
#define EN75XX_PCM_DMA_CTRL		0x40

/* gen2 only: timeslot cfg 4..15 and the per-channel enable register */
#define EN7523_PCM_TX_SLOT4		0x48
#define EN7523_PCM_RX_SLOT4		0x78
#define EN7523_PCM_V2_CFG		0xa8
#define EN7523_PCM_CHAN_ENABLE		0xac

/*
 * IFACE_CTRL. Bit positions come from the vendor pcmConfigSetup(), which
 * translates its 64-byte config node into this register. Only bit 12
 * (bit delay), bit 26 (config commit) and bit 24 (soft reset) have
 * dedicated vendor helpers confirming them; for the rest the driver
 * prefers the whole-register value observed on stock firmware.
 */
#define EN75XX_PCM_CTRL_PROBE		GENMASK(30, 28)
#define EN75XX_PCM_CTRL_CFG_VALID	BIT(26)	/* commit: clear, then set */
#define EN75XX_PCM_CTRL_LOOPBACK	BIT(25)
#define EN75XX_PCM_CTRL_SOFT_RESET	BIT(24)
#define EN75XX_PCM_CTRL_SLOT_NUM	GENMASK(22, 18)
#define EN75XX_PCM_CTRL_BIT_DELAY	BIT(12)
#define EN75XX_PCM_CTRL_IFACE		GENMASK(9, 8)
#define EN75XX_PCM_CTRL_CLK_RATE	GENMASK(3, 1)

/* Read back from stock EN751221 with the OEM voice stack running. */
#define EN75XX_PCM_CTRL_OEM		0xf5071306

/* Timeslot config: two slots per register. */
#define EN75XX_PCM_TS_LO_WIDE		BIT(12)		/* 1 = 16-bit slot */
#define EN75XX_PCM_TS_LO_NUM		GENMASK(9, 0)
#define EN75XX_PCM_TS_HI_WIDE		BIT(28)
#define EN75XX_PCM_TS_HI_NUM		GENMASK(25, 16)

#define EN75XX_PCM_DMA_TX_EN		BIT(0)
#define EN75XX_PCM_DMA_RX_EN		BIT(1)
#define EN75XX_PCM_DMA_CH_MASK		GENMASK(31, 24)

/*
 * ISR/IMR. Bits 3 and 5 and the error group 10:6 are confirmed by the
 * vendor ISR; bits 11..16 carry SLIC hook-status changes, which is why
 * the OEM mask is 0x5828 and not just the DMA bits.
 */
#define EN75XX_PCM_INT_TX_DESC		BIT(2)
#define EN75XX_PCM_INT_RX_DESC		BIT(3)
#define EN75XX_PCM_INT_TX_END		BIT(4)
#define EN75XX_PCM_INT_RX_END		BIT(5)
#define EN75XX_PCM_INT_TX_UNDERRUN	BIT(6)
#define EN75XX_PCM_INT_RX_OVERRUN	BIT(7)
#define EN75XX_PCM_INT_AHB_ERR		BIT(8)
#define EN75XX_PCM_INT_ERR		GENMASK(10, 6)
#define EN75XX_PCM_INT_HOOK		(BIT(11) | BIT(12) | BIT(14) | \
					 BIT(15) | BIT(16))
#define EN75XX_PCM_INT_ALL		GENMASK(8, 2)
#define EN75XX_PCM_INT_OEM_MASK		0x5828

/* Descriptor status word, identical in both generations. */
#define EN75XX_PCM_DESC_OWN		BIT(31)
#define EN75XX_PCM_DESC_CH_VALID	GENMASK(23, 16)
#define EN75XX_PCM_DESC_SAMPLE_SIZE	GENMASK(9, 0)

#define EN75XX_PCM_RING_COUNT		15
#define EN75XX_PCM_MAX_CHANNELS		8
#define EN75XX_PCM_FRAME_SAMPLES	80
#define EN75XX_PCM_FRAME_BYTES		160	/* 80 samples, 16-bit */
#define EN75XX_PCM_FIFO_BYTES		4096

/*
 * The per-descriptor buffer stride always uses the 8-channel layout
 * (index d*8+ch) even when the channel-valid mask only enables four.
 * A 4-channel stride reads the wrong slab from the 2nd descriptor on.
 */
#define EN75XX_PCM_FRAME_STRIDE \
	(EN75XX_PCM_MAX_CHANNELS * EN75XX_PCM_FRAME_BYTES)

/* gen1: EN751221, EN7528 */
struct en75xx_pcm_desc_v1 {
	u32 status;
	u32 buf_addr[EN75XX_PCM_MAX_CHANNELS];
};

/* gen2: EN7523 */
struct en75xx_pcm_desc_v2 {
	u32 status;
	u32 ch_valid;
	u32 buf_addr;
};

/*
 * The SLIC's TXSLOT/RXSLOT is a PCM *bus* timeslot; the RX DMA lands
 * that audio on a DMA channel offset from it. Determined empirically on
 * EN751221: bus slot 4 -> DMA channel 0, bus slot 6 -> DMA channel 2.
 */
#define EN75XX_PCM_SLOT_TO_DMA_CH(slot)	((slot) - 4)

#endif /* _EN75XX_PCM_REGS_H */
