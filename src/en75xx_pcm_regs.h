/* SPDX-License-Identifier: GPL-2.0 */
/*
 * EcoNet/Airoha EN75xx PCM/TDM register and descriptor definitions.
 *
 * Sources of truth, in order of authority:
 *
 *  1. The "regMap" table in the vendor pcm1.ko. The copy shipped in the
 *     TP-Link VB430 GPL drop (Airoha AN7551/AN7581 LTS SDK, under
 *     tclinux_phoenix/release_bsp/<profile>/BSP/voip_bsp/voip_module/ko)
 *     is not
 *     stripped, so the table can be read directly: each entry is
 *     { const char *name; u32 flags; u32 writable_mask; u32 address;
 *       u32 reset; }. Every offset below is quoted from it.
 *  2. The vendor debug printk in descGet().
 *  3. Register values read back from a stock EN751221 with the OEM
 *     voice stack running.
 *
 * The vendor table, verbatim (addresses are the MIPS KSEG1 form of
 * 0x1fbd0000 + offset, and are identical on the ARM parts):
 *
 *   name                offset  writable mask  reset
 *   pcmCtrl             0x00    0x1f7f1f1f     0x0500040a
 *   txTimeSlotCfg0..3   0x04..0x10  0x13ff13ff 0x00080000, 0x00180010,
 *                                              0x00280020, 0x00380030
 *   rxTimeSlotCfg0..3   0x14..0x20  0x13ff13ff same resets as tx
 *   ISR                 0x24    0x000007ff     0x00000000
 *   INTMask             0x28    0x000007ff     0x00000000
 *   txPolling           0x2c    0xffffffff     0x00000000
 *   rxPolling           0x30    0xffffffff     0x00000000
 *   txRingBaseAddr      0x34    0xffffffff     0x00000000
 *   rxRingBaseAddr      0x38    0xffffffff     0x00000000
 *   txrxRingSizeAndOff  0x3c    0x000000ff     0x000000c0
 *   txRxDMA             0x40    0x0000000f     0x0f000000
 *   txTimeSlotCfg4..15  0x48..0x74  0x13ff13ff 0x00480040 .. 0x00f800f0
 *   rxTimeSlotCfg4..15  0x78..0xa4  0x13ff13ff 0x00480040 .. 0x00f800f0
 *   txRxChanEnable      0xac    0x0000000f     0x0000000f
 *
 * Three things follow from that table and are worth stating plainly,
 * because earlier revisions of this driver got them wrong:
 *
 *  - There is no register at 0xa8. An earlier draft wrote 0xa0 there
 *    during EN7523 start-up; that write went nowhere.
 *  - ISR and INTMask are eleven bits wide. Interrupt bits above 10 --
 *    and therefore the "OEM interrupt mask 0x5828" an earlier draft
 *    programmed -- do not exist.
 *  - The reset of txrxRingSizeAndOff really is 0xc0, which is why the
 *    driver must program it: with the reset value the RX descriptor
 *    ownership bit never clears.
 *
 * IMPORTANT: the descriptor layout is NOT the same across generations.
 *
 *   gen1 (EN751221, EN7528, MIPS32r2): stride 0x24 = 36 bytes.
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
 * Both readings are corroborated by the vendor modules' own descGet()
 * format strings. The AN7581 (gen2) module prints
 *
 *     desc status:0x%08lx(ownership:%d,sample size:%u)
 *
 * and the EN7528 (gen1) module prints
 *
 *     desc status:0x%08lx(ownership:%d,chvaild:0x%08x,sample size:%u)
 *
 * The EN7528 module carries its own regMap, which stops at txRxDMA:
 * gen1 has neither the twelve extra timeslot registers at 0x48..0xa4
 * nor txRxChanEnable at 0xac, and every register the two generations
 * share has the same offset, mask and reset. Note also that those
 * EN7528 modules are little-endian MIPS, so big-endian cannot be
 * assumed for the generation; see airoha,pcm-big-endian for the sample
 * byte order, which is the only place it matters.
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

/*
 * Timeslot configuration registers 4..15, and the per-channel enable.
 * The vendor regMap lists all of them unconditionally, but only the
 * gen2 parts are known to act on them; the driver programs the first
 * four registers, which already cover the eight channels the DMA
 * engine exposes.
 */
#define EN75XX_PCM_TX_SLOT4		0x48
#define EN75XX_PCM_RX_SLOT4		0x78
#define EN75XX_PCM_CHAN_ENABLE		0xac

/* Number of timeslot-config registers the driver programs (2 slots each). */
#define EN75XX_PCM_SLOT_REGS		4

/*
 * IFACE_CTRL. Bit positions come from the vendor pcmConfigSetup(), which
 * translates its 64-byte config node into this register. Only bit 12
 * (bit delay), bit 26 (config commit) and bit 24 (soft reset) have
 * dedicated vendor helpers confirming them; for the rest the driver
 * prefers the whole-register value observed on stock firmware.
 *
 * The regMap writable mask is 0x1f7f1f1f, so bits 31:29, 23, and 15:13
 * are not writable. The observed value below sets bits 31:28; only the
 * 0x15071306 part of it actually lands.
 */
#define EN75XX_PCM_CTRL_WRITABLE	0x1f7f1f1fu
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

/*
 * Timeslot config: two slots per register, writable mask 0x13ff13ff.
 *
 * The NUM field is a bit offset into the 8 kHz frame, not a byte-slot
 * index. The vendor reset values settle it: channel 0 resets to 0,
 * channel 1 to 8, channel 2 to 16 ... channel 31 to 248, i.e. channel
 * n starts at bit n * 8. A byte timeslot s therefore sits at bit
 * offset s * 8, and a 16-bit channel occupies two byte timeslots.
 */
#define EN75XX_PCM_TS_LO_WIDE		BIT(12)		/* 1 = 16-bit slot */
#define EN75XX_PCM_TS_LO_NUM		GENMASK(9, 0)
#define EN75XX_PCM_TS_HI_WIDE		BIT(28)
#define EN75XX_PCM_TS_HI_NUM		GENMASK(25, 16)

#define EN75XX_PCM_BITS_PER_SLOT	8

/* Bit offset in the frame for byte timeslot @slot. */
#define EN75XX_PCM_SLOT_TO_BIT(slot)	((slot) * EN75XX_PCM_BITS_PER_SLOT)

#define EN75XX_PCM_DMA_TX_EN		BIT(0)
#define EN75XX_PCM_DMA_RX_EN		BIT(1)
#define EN75XX_PCM_DMA_CH_MASK		GENMASK(31, 24)

/*
 * ISR/IMR. The regMap writable mask is 0x000007ff: the block implements
 * eleven interrupt bits and nothing above them. Bits 3 and 5 and the
 * error group 10:6 are confirmed by the vendor ISR.
 */
#define EN75XX_PCM_INT_TX_DESC		BIT(2)
#define EN75XX_PCM_INT_RX_DESC		BIT(3)
#define EN75XX_PCM_INT_TX_END		BIT(4)
#define EN75XX_PCM_INT_RX_END		BIT(5)
#define EN75XX_PCM_INT_TX_UNDERRUN	BIT(6)
#define EN75XX_PCM_INT_RX_OVERRUN	BIT(7)
#define EN75XX_PCM_INT_AHB_ERR		BIT(8)
#define EN75XX_PCM_INT_ERR		GENMASK(10, 6)
#define EN75XX_PCM_INT_ALL		GENMASK(10, 2)
#define EN75XX_PCM_ISR_VALID		GENMASK(10, 0)

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

#endif /* _EN75XX_PCM_REGS_H */
