# Descriptor layouts: the two PCM generations are not the same

This is the one place where the draft and the vendor binaries disagreed,
and it is worth writing down because the mistake is a very reasonable
one to make.

## What the draft assumed

```c
struct en75xx_pcm_desc {
	u32 status;
	u32 buf_addr[EN75XX_PCM_MAX_CHANNELS];	/* 36 bytes */
};
```

with `ring_count = 5` on EN7523. That is self-consistent: the EN7523
`pcm1.ko` allocates `pcmKmalloc(0xb4)` = 180 bytes, and 180 / 36 = 5.

## What the binaries actually do

The allocation size matches, but the **stride** does not. Both
generations index their rings by a fixed multiplier, and the two
multipliers are different:

| SoC                        | alloc                     | stride          | descriptors       |
| -------------------------- | ------------------------- | --------------- | ----------------- |
| EN751221, EN7528 (MIPS BE) | `pcmKmalloc(0x21c)` = 540 | `n * 0x24` = 36 | 540 / 36 = **15** |
| EN7523 (ARM LE)            | `pcmKmalloc(0xb4)` = 180  | `n * 0x0c` = 12 | 180 / 12 = **15** |

So it is 15 descriptors in both cases; what changes is how wide each one
is. Three independent places in the EN7523 decompile use the 12-byte
stride (`rxDescSet`, `pcmSend`, `descGet`), and `descGetAll` loops to
`0xf`, same as gen1.

### gen1 — 36 bytes, one buffer per channel

```
+0x00  status : OWN(31) | chvalid(23:16) | sample size(9:0)
+0x04  buf_addr[0]
...
+0x20  buf_addr[7]
```

`rxDescBufSet()` fills `buf_addr[i]` for `i < n_channels`, stepping the
address by `frame_samples * (2 if the slot is 16-bit else 1)`.
`descGet()` dumps `buf0..buf7`. The channel mask goes into byte 1 of the
status word, which on big-endian MIPS is bits 23:16 — exactly the
`GENMASK(23, 16)` the draft already had.

### gen2 — 12 bytes, mask in its own word, single buffer

```
+0x00  status   : OWN(31) | sample size(9:0)
+0x04  ch_valid : (1 << n_channels) - 1, as a full word
+0x08  buf_addr
```

`rxDescSet()` writes `~(-1 << n_channels)` as a **32-bit store at +4**,
not into the status word, and `descGet()` on this generation prints only
`buf0` at +8. EN7523 also gains a dedicated `txRxChanEnable` register at
`0xac` and 16+16 timeslot registers instead of 4+4, which fits: the
per-channel information moved out of the descriptor and into the block.

## "sample size", not "byte count"

The vendor's own debug string settles the units:

```
desc status:0x%08lx(ownership:%d,chvaild:0x%08x,sample size:%u)
```

The config node default is `0x50` = 80, and the buffer stride doubles
when the slot is 16-bit wide. So a frame is **80 samples = 160 bytes**,
which is what the draft's UAPI already said.

## Other values worth pinning

| Thing              | gen1                           | gen2                                | note                                                     |
| ------------------ | ------------------------------ | ----------------------------------- | -------------------------------------------------------- |
| `RING_CFG` (0x3c)  | `0x9f`                         | `0x3f`                              | reset is `0xc0`; with `0xc0` the RX OWN bit never clears |
| ring base encoding | `phys & 0x1fffffff`            | `(phys & 0x3fffffff) \| 0x80000000` |                                                          |
| `IFACE_CTRL`       | `0xf5071306` observed on stock | computed default `0x00051306`       | the two differ in bits 31..24 and bit 17                 |
| buffer stride      | `d * 8 + ch`                   | same                                | 8-channel layout even when the mask enables 4            |

The `IFACE_CTRL` gap is not resolved. `0xf5071306` is a read-back from a
running stock EN751221, while `0x00051306` is what the vendor's own
`pcmConfigSetup()` computes from its default config node. The extra bits
are the probe field (30:28), the config-valid bit (26), the soft-reset
bit (24) and bit 17 — all of which the OEM voice stack could plausibly
set after the initial config. The driver uses the observed value and
lets the device tree override it, which is the safer default.
