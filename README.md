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
dts/                    device tree nodes
openwrt/                OpenWrt package
docs/                   what was reverse engineered, and how
```

## Status

| Piece | State |
|-------|-------|
| PCM engine, both descriptor generations | implemented |
| G.711 companding, `tx_msb` byte placement | implemented |
| `/dev/en75xx-fxsN` character device | implemented |
| ZSI transport | implemented |
| Le9642: profiles, feed, ring cadence, hook, audio | implemented, follows a path proven on hardware |
| Si3219x over SPI | implemented against the ProSLIC API, untested on hardware |
| MaxLinear PEF32001/PEF32002 (DUSLIC-XS) | not started; firmware blobs identified |
| Asterisk channel driver | implemented, untested on hardware |

## Two SLIC families, two transports

The Le9642 is **not** an SPI device. ZSI multiplexes its control channel
over the PCM bus, so it only answers while the PCM engine is clocking
PCLK/FSYNC, and every control byte needs a ~5 ms gap for the SLIC to
clock it over the 8 kHz bus. Driving it as plain SPI leaves it
electrically mute, which looks exactly like a dead board. The ProSLIC
parts are ordinary SPI devices and use the Linux SPI subsystem.

## Why the Le9642 runs G.711 and not 16-bit linear

The chip drives only 8 bits per timeslot. With a linear codec the other
byte of every 16-bit slot is zero, so quiet speech quantises to silence
and the audio cuts out between words. Running the codec in u-law and
companding in the PCM data path fixes it; the character device stays
16-bit linear, so userspace never sees the difference.

Capture always carries the code in the low byte of the slot. Playback
does not necessarily use the same byte, because the SLIC uses different
clock-slot offsets in each direction — hence the `tx_msb` parameter.

## Timeslots and DMA channels

The SLIC's `TXSLOT`/`RXSLOT` is a PCM **bus** timeslot. The RX DMA lands
that audio on a DMA channel offset from it: `dma_channel = bus_slot - 4`
on EN751221. Two simultaneous lines need their own slots (4 and 6, i.e.
DMA channels 0 and 2). The per-descriptor buffer stride always uses the
8-channel layout even when the channel-valid mask only enables four.

## Interrupts

On EN751221 the PCM interrupt (hwirq 12) does not map cleanly — the INTC
reports it as a shadow interrupt. The driver requests the IRQ if the
device tree provides one and otherwise falls back to a polling worker,
so the DTS ships with `interrupts` commented out.

## Building

```
make -C /path/to/linux M=$PWD/src modules
```

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
