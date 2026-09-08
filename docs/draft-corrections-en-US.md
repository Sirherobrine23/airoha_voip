# EN75xx FXS/VoIP draft corrections

## Scope and confidence

This revision cross-checks the draft against the decompiled vendor
modules and the target Linux tree. It fixes objective incompatibilities,
but it does not turn the project into hardware-validated production
support. Findings below are classified as:

- **confirmed in the current kernel**: pinctrl, reset IDs, and SLIC clock;
- **confirmed in vendor modules**: PCM map, descriptor stride/fields,
  ring count, and DMA address encoding;
- **board validation required**: PCM IRQ, external polarities, ZSI
  clock/routing outside EN751221, and SLIC electrical stability.

## Corrections applied

| Area             | Draft problem                                                                | Correction                                                                                             |
| ---------------- | ---------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------ |
| gen2 DMA         | duplicated `ch_valid` in status                                              | EN7523 status contains `OWN + sample_size`; the mask exists only in the `+0x04` word                   |
| EN7523           | did not program `CHAN_ENABLE`                                                | writes register `0xac`, limited to the observed `0x0f` channel mask                                    |
| Descriptors      | active clients changed ring geometry during a call                           | descriptors use a stable configured DMA mask                                                           |
| Concurrent lines | opening/closing line two restarted PCM                                       | hardware starts on the first open and stops on the last; other opens only activate FIFOs               |
| IFACE_CTRL       | single write                                                                 | commits with a bit-26 `CFG_VALID` clear-to-set edge                                                    |
| Interrupts       | omitted error bits 9/10 and enabled IMR without an IRQ                       | uses bits 2–10; polling keeps IMR zero and acknowledges observed ISR bits                              |
| Endianness       | native `s16` casts violated the UAPI                                         | UAPI stays S16 little-endian; unaligned LE access and symmetric TX/RX swapping are used                |
| G.711            | encoders used reduced segment limits and the A-law decoder reversed the sign | uses the kernel reference implementations' 16-bit segmentation and preserves the sign correctly        |
| DMA address      | silently truncated `dma_addr_t`                                              | applies the SoC DMA mask and rejects unencodable ring/buffer ranges                                    |
| SCU/reset        | PCM wrote `0x834` directly                                                   | uses `reset_control_reset()` and current provider IDs                                                  |
| Pinmux           | PCM reused EN751221 offsets on every SoC                                     | pinctrl is the normal path; the PCM driver performs no SCU mux writes                                  |
| ZSI              | bit 25 was called an SPI reset                                               | the current name is `SFC2_PCM_RST`; ZSI consumes its dedicated reset                                   |
| ZSI              | EN751221 clock offsets were global                                           | raw setup is opt-in and rejected on non-EN751221 compatibles                                           |
| ZSI robustness   | regmap/populate errors were ignored and lookup raced removal                 | errors propagate, list insertion is rolled back, and the device reference is taken under the list lock |
| Le9642 slots     | invalid/duplicate slots produced a silent line                               | probe rejects slots below 4, outside the known range, or duplicated                                    |

## Second revision: cross-check against the Airoha LTS SDK

The corrections above came from decompiling vendor binaries. A later
revision re-checked the same ground against Airoha's own C sources,
published as part of the TP-Link VB430 GPL drop, and against the
unstripped `pcm1.ko` it ships. `docs/07-gpl-sdk-crosscheck.md` has the
evidence; the changes are:

| Area | Problem | Correction |
| ---- | ------- | ---------- |
| PCM register 0xa8 | EN7523 start-up wrote `0xa0` to a register that does not exist | write removed; the vendor `regMap` has nothing between 0x40 and 0xac |
| Interrupt mask | `INT_HOOK` (bits 11..16) and the "OEM mask 0x5828" set bits the block does not implement | removed; `ISR` and `INTMask` have a writable mask of `0x7ff` |
| Timeslot field | read as a byte-slot index | it is a bit offset into the frame; channel *n* resets to offset *n*·8 |
| Slot to channel | `channel = slot - 4`, contradicting this driver's own default slot table | `en75xx_pcm_channel_for_slot()` searches the configured table; an unmatched slot fails the probe |
| Le9642 transmit slot | programmed equal to the receive slot | ZSI adds two PCLK of transmit delay; the transmit slot is now one below the bus slot, as the Microchip API does |
| Le9642 codec | u-law plus a `tx_msb` byte selector, working around the missing slot shift | defaults to 16-bit linear, matching the vendor; G.711 stays selectable |
| Le9642 device profile | `CLKSLOTS` operand `0x06` | `0x46`, the value the vendor patches in for every ZSI board |
| Le9642 AC profile | streamed `sizeof - 6` = 74 bytes | streams the 73 the profile header declares; the 74th was read as an opcode |
| Le9642 ring | ignored the caller's cadence | rounds the requested cadence to whole ticks |
| ProSLIC timeslot | `channel * 16` | the frame bit offset the PCM engine assigned to that channel |
| ProSLIC init | no LB calibration, no PCM format, edge or interrupt fix-ups | all four added, matching the vendor integration |
| Si32192 interface | documented as an ordinary SPI part | Airoha lists it as ISI; the file header now says so |
| EN7523 PCM IRQ | "a vendor candidate" | confirmed: `GIC_SPI 27`, level high, in the SDK device tree |

## PCM layout implemented by the driver

| Item                 | EN751221 / EN7528                 | EN7523                      |
| -------------------- | --------------------------------- | --------------------------- |
| generation           | gen1                              | gen2                        |
| descriptor           | `0x24` bytes                      | `0x0c` bytes                |
| descriptors per ring | 15                                | 15                          |
| status               | `OWN`, `ch_valid[23:16]`, samples | `OWN`, samples              |
| `+0x04` word         | channel-0 buffer                  | `ch_valid` mask             |
| buffers              | 8 addresses per descriptor        | 1 address per descriptor    |
| `RING_CFG`           | `0x9f`                            | `0x3f`                      |
| ring address         | `phys & 0x1fffffff`               | `(phys & 0x3fffffff)        | 0x80000000` |
| extra enable         | not observed                      | `0xac`, maximum mask `0x0f` |

A frame remains 80 samples, or 160 bytes for PCM16. Each descriptor's
buffer slab retains an eight-channel stride even on EN7523, whose
observed hardware mask enables four channels.

## Reset, pinctrl, and clock ownership

The current reset provider maps the voice blocks. DTS values are
binding IDs, not raw register bits:

| Function | EN751221                    | EN7528                    | EN7523                              |
| -------- | --------------------------- | ------------------------- | ----------------------------------- |
| PCM1     | `EN751221_PCM1_RST`         | `EN7528_PCM1_RST`         | `EN7523_PCM1_RST`                   |
| PCM2     | `EN751221_PCM2_RST`         | `EN7528_PCM2_RST`         | no separate engine reset is exposed |
| ZSI1     | `EN751221_PCM1_ZSI_ISI_RST` | `EN7528_PCM1_ZSI_ISI_RST` | `EN7523_PCM1_ZSI_ISI_RST`           |

On EN751221/EN7528, PCM2 maps to RST_CTRL1 bit 4; the draft's old
`0x1000` selected bit 12 and was wrong. Pinctrl is also SoC-specific:
`+0x104` on EN751221, `+0x15c/+0x224` on EN7528, and `+0x214` on
EN7523. The PCM driver therefore no longer reuses raw offsets.

EN7523 exposes `EN7523_CLK_SLIC`, consumed by the ZSI node. This clock
does not prove that the PCM engine uses the same source. The MIPS clock
provider still has no SLIC output; raw compatibility is restricted to
EN751221, where the sequence was observed. EN7528 ZSI route/clock setup
still requires validation.

## Corrected DTS behavior

- PCM uses `resets` and `reset-names = "pcm"`.
- ZSI uses `resets` and `reset-names = "zsi"`.
- EN7523 ZSI consumes `EN7523_CLK_SLIC` as clock `slic`.
- Two-line examples use `airoha,dma-channel-mask = <0x05>` for DMA
  channels 0 and 2, derived from bus slots 4 and 6.
- EN7523 PCM2 does not invent a reset missing from the binding.
- EN7523 IRQ 27 remains a vendor candidate, not a board-validated fact.

## Validation performed in this revision

- All five modules (`en75xx-pcm`, `en75xx-voice`, `en75xx-zsi`,
  `en75xx-slic-le9642`, and `en75xx-slic-si3219x`) compile and link with
  no compiler warning under `W=1` against the target Linux 6.18 tree,
  commit `8f909dac6c5aca0a9c936ce0966a7dce56837427`.
- All five DTS examples pass the preprocessor and the `dtc` built from
  that same tree.
- `proslic-patch2fw.py` passes `py_compile`, the OpenWrt installer passes
  `sh -n`, and both copies of `chan_en75xx.c` are identical.

This is an API and module-link check using an x86_64 `modules_prepare`
configuration; it does not replace the ARM/MIPS cross-build. Without the
kernel's complete `Module.symvers`, `modpost` reports the kernel's own
symbols as unresolved. The Asterisk channel still must be built against
the exact Asterisk headers shipped in the target firmware.

The SDK cross-check revision repeated the build against a stock
`linux-6.18.50` release tarball, this time with a full `vmlinux` so that
`Module.symvers` is complete and `modpost` resolves every symbol. All
five modules build with no warning under `W=1` and link with nothing
undefined, the inter-module symbols included. The four Le9642 profiles
were also compared byte for byte against the vendor sources, and each
one's declared length and raw-MPI length agree with its contents.

Two caveats on that run. It is still x86_64, so it does not exercise the
big-endian MIPS paths. And the EN7523 device-tree examples do not build
against a stock 6.18 tree, because the fragment expects a `pinctrl`
label the upstream `en7523.dtsi` does not yet carry; this is unchanged
from the previous revision.

## Required work before deployment

1. Verify BCLK/FSYNC and the SLIC clock rate on an oscilloscope.
2. Validate the PCM IRQ; use polling until then.
3. Determine whether `pcm_spi_rst` resets the external component or only
   selects a hardware function.
4. Stress TX/RX rings for at least 30 minutes while tracking OWN,
   underrun, overrun, and AHB errors.
5. Test two simultaneous lines, independent open/close, and G.711 on
   big-endian MIPS.
6. Build `chan_en75xx` against the firmware's exact Asterisk version and
   test module load/unload, DTMF, ring, answer, and hangup.

Until these checks pass, treat this as an auditable porting base rather
than production support.
