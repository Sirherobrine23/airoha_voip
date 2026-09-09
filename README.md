# EN75xx voice stack

FXS/VoIP support for the EcoNet/Airoha EN751221, EN7528 and EN7523 SoCs:
the on-die PCM/TDM engine plus drivers for the SLICs that hang off it.
Out-of-tree kernel modules and OpenWrt packages.

Nothing here loads vendor blobs. The vendor `.ko` files were used as
reference material only.

## Layout

```
include/                UAPI (/dev/en75xx-fxsN) and the internal API
src/en75xx_pcm.c        TDM engine: rings, DMA, timeslots, G.711
src/en75xx_voice_core.c line registry + character device
src/en75xx_zsi.c        ZSI transport (SLIC control over the PCM bus)
src/en75xx_slic_le9642.c  Microsemi Le9641/Le9642 over ZSI
src/en75xx_slic_si3219x.c Skyworks Si3218x/Si3219x over SPI
src/en75xx_proslic_fw.c ProSLIC patch firmware loader
tools/proslic-patch2fw.py  patch .c -> firmware blob converter
asterisk/chan_en75xx.c  Asterisk channel driver (type EN75XX)
vendor/proslic/         Skyworks ProSLIC API + Si3219x patch
vendor/vp886/           Microsemi VoicePath API-II (reference)
dts/                    SoC fragments (en7523/en751221/en7528) + examples
openwrt/                OpenWrt package
docs/                   what was reverse engineered, and how
```

## Status

This is a reverse-engineered porting prototype, not a production-ready
voice stack. The code now reflects the recovered register/descriptor
layouts and the current kernel pinctrl/reset/clock providers, but it
still needs per-board electrical and runtime validation.

| Piece | State |
|-------|-------|
| PCM register map | confirmed against the vendor `pcm1.ko` regMap; see `docs/07` |
| PCM gen1 (EN751221/EN7528) | implemented from vendor layout; bench validation required |
| PCM gen2 (EN7523) | corrected to 12-byte descriptors and `CHAN_ENABLE`; hardware validation required |
| G.711 and `/dev/en75xx-fxsN` | kernel-reference companding and endian paths corrected; integration test required |
| ZSI transport | EN751221 legacy sequence retained explicitly; modern EN7523 resources wired, runtime validation required |
| Le9642: profiles, slots, feed, ring cadence, hook | profiles and timeslot handling now match the vendor API; bench validation required |
| Le9642 converter topology | `bb` and `ib` profiles from the vendor sources; selection is required, not defaulted |
| Le9642 alarms | thermal, over-current and clock-fault bits reported from SIGREG; the converter is powered down on remove and shutdown |
| Si3219x adapter | experimental SPI adapter; Si32192/Si32193 require the missing ISI transport and their known compatibles are rejected |
| MaxLinear PEF32001/PEF32002 (DUSLIC-XS) | not started; firmware blobs identified, and the upstream packages now named, see `docs/09` |
| Asterisk channel driver | draft implementation; build/runtime testing against the target Asterisk version required |

## Two SLIC families, two transports

The Le9642 is **not** an SPI device. ZSI multiplexes its control channel
over the PCM bus, so it only answers while the PCM engine is clocking
PCLK/FSYNC, and every control byte needs a ~5 ms gap for the SLIC to
clock it over the 8 kHz bus. Driving it as plain SPI leaves it
electrically mute, which looks exactly like a dead board. The ProSLIC
parts are ordinary SPI devices and use the Linux SPI subsystem.

## The converter topology is not optional

`airoha,slic-power-type` has to be in the device tree, `"bb"` or `"ib"`,
and the Le9642 driver refuses to probe without it.

The SLIC sits behind a high-voltage converter that the board builds
around it, and the device profile programs that converter: switching
timing, regulator parameters, switcher configuration, output voltage
limits. Airoha ships two profiles for the Le9641/Le9642, a 47 uH
buck-boost at 12 V in and 100 V out, and a 500 kHz inductorless
inverting boost at 12 V in and 90 V out. Eighteen bytes differ, and they
are all in that group. Nothing in the chip reports which circuit it is
wired into, and the vendor picks with a `mode=IB` module parameter.

An earlier revision hardcoded the buck-boost profile. On an
inductorless-boost board that programs one converter's switching
behaviour into another, which is a way to destroy the switching
transistor or the inductor rather than merely produce silence. Failing
the probe is the right answer to a question the software cannot answer
for itself.

## The Le9642 transmit slot, and why G.711 is no longer the default

ZSI adds two PCLK cycles of delay on the transmit side. The Microchip
API compensates by programming the transmit timeslot one byte slot
*below* the wanted bus slot and letting the device profile's clock-slot
field add six clocks back, which nets the −2 that cancels the delay.

Leave the shift out and transmit audio is exactly one byte late. That
looks like the SLIC using a different byte of the 16-bit slot in each
direction, and it makes the 16-bit linear codec produce nonsense, which
is why an earlier revision here ran the codec in u-law with a `tx_msb`
byte selector. Both were symptoms. The driver now applies the shift and
defaults to 16-bit linear, the same as the vendor's own integration.

The G.711 modes remain available (`codec=ulaw`, `codec=alaw`, or
`airoha,a-law` in the device tree); companding then happens in the PCM
data path and the character device stays 16-bit linear either way.

## Timeslots and DMA channels

The SLIC's `TXSLOT`/`RXSLOT` is a PCM **bus** timeslot; the DMA engine
numbers channels. The two are related only by the PCM engine's timeslot
table, whose slot field is a *bit offset into the frame*, so there is no
fixed formula. With this driver's default table channel n sits at bit
offset 32 + n·16, that is at bus slot 4 + n·2, so slots 4 and 6 are DMA
channels 0 and 1.

The SLIC drivers ask the PCM driver to resolve a slot rather than
computing it, and a slot no channel covers fails the probe with a
message instead of producing a silent line. The per-descriptor buffer
stride always uses the 8-channel layout even when the channel-valid
mask only enables four.

## Interrupts

The vendor's own interrupt tables settle the numbering. On the MIPS
parts, `asm/tc3162/tc3182_int_source.h` puts PCM1 on hardware line 11 in
both the 1004K and the older enumeration; PCM2 is line 32 on 1004K and
33 otherwise. An earlier note here said the EN751221 PCM interrupt was
hwirq 12 and did not map cleanly. Line 12 is `SI_PC1_INT`, a different
source entirely, which is the likelier explanation of the "shadow
interrupt" than anything wrong with the controller. The EN751221
fragment already ships 11 and 33.

On the ARM parts it is GIC SPI 27, level high, in every vendor device
tree seen so far: EN7523, EN7529, EN7552, EN7580, EN7581 and AN7583 all
carry the identical node.

The driver requests the IRQ when the device tree provides one and falls
back to a polling worker when it does not.

## Building

```
make -C /path/to/linux M=$PWD/src modules
```

The kernel must be configured and prepared first (`make modules_prepare`).
For OpenWrt, use `tools/install-openwrt.sh /path/to/openwrt`. It installs
both package definitions and the kernel package's required `src`,
`include`, `vendor` and `firmware` trees.

or through OpenWrt, with `openwrt/package/kernel/en75xx-voip` copied
into the tree and `kmod-en75xx-pcm` plus one `kmod-en75xx-slic-*`
selected.

## Firmware

`firmware/dxs/` holds `DXS_FW.bin` and `DXS_BBD.bin`, recovered from a
Nokia G-240G-E. The PRAM patch is optional (the DUSLIC-XS falls back to
its ROM firmware); the BBD is not.

`firmware/proslic/` holds 19 ProSLIC DSP patches converted from the API
C sources by `tools/proslic-patch2fw.py`, covering Si3217x/18x/19x/26x/28x
across their BOM variants. The driver loads one at probe instead of
having it compiled in, so a single module build serves every variant.
`firmware/proslic/INDEX` maps each blob back to its API symbol names and
source files.

## Reading order for the docs

1. `docs/01-pcm-descriptor-layouts.md` — the descriptor formats and the
   register values that matter, with the evidence behind each one.
2. `docs/02-asterisk.md` — how the FXS lines reach the dialplan, and why
   this is a channel driver rather than a DAHDI span.
3. `docs/03-proslic-firmware.md` — the patch blob format, and what the
   conversion turned up about the vendor's own patch sources.
4. `docs/04-crosschecks.md` — independent confirmation of the bit delay,
   SPI mode and clock mastering from MediaTek's ProSLIC integration.
5. `docs/05-device-tree.md` — the bindings, and the slot-versus-channel
   trap that makes a second line go silent instead of failing.
6. `docs/06-spi-shared-with-flash.md` — why the ProSLIC shares the boot
   flash's SPI controller, and what that costs during flash writes.
7. `docs/correcoes-do-draft-pt-BR.md` / `docs/draft-corrections-en-US.md`
   — what was corrected, what is confirmed, and what remains to prove.
8. `docs/07-gpl-sdk-crosscheck.md` — what the Airoha LTS SDK sources
   shipped in the TP-Link VB430 GPL drop confirmed, and what they
   contradicted, plus a survey of nineteen further drops.
9. `docs/09-maxlinear-dxs.md` — what the drops give us towards the
   DUSLIC-XS family, and what is still missing.

Lifecycle and firmware review: [English](docs/08-proslic-review.en-US.md) / [Português](docs/08-proslic-review.pt-BR.md).
