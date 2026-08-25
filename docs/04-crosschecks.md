# Cross-checks against other vendors' ProSLIC integrations

Independent confirmations of things this tree had to infer. Useful
because they come from a different SoC family and a different vendor,
so agreement is not just the same mistake twice.

## MediaTek, mtk-openwrt-feeds

MediaTek ships a ProSLIC integration for MT7986/MT7988 as an ASoC codec:
`sound/soc/codecs/si3218x-spi.c` plus the machine driver
`sound/soc/mediatek/mt7986/mt7986-si3218x.c`.

**No patch or firmware data is in that feed.** The ProSLIC API
subdirectory it would need is commented out in both places that would
build it:

```
#config SND_SOC_SI3218X
#	tristate
...
#obj-$(CONFIG_SND_SOC_SI3218X)	+= si3218x/
```

What ships is the hook -- an SPI shim calling `si3218x_spi_probe()` from
a `si3218x/si3218x.h` that is not in the tree. The DSP patches still
have to come from the ProSLIC API sources, which is what
`tools/proslic-patch2fw.py` converts.

### What it does confirm

**SPI mode 3 at 10 MHz.** Their device tree sets `spi-cpha = <1>`,
`spi-cpol = <1>`, `spi-max-frequency = <10000000>`. This tree's driver
already asks for `SPI_MODE_3` in `spi_setup()`, so the two agree; the
DTS snippet in `dts/en751221-voice.dtsi` now uses the same numbers.

**The bit delay is real.** Their DAI link uses:

```
SND_SOC_DAIFMT_DSP_A | SND_SOC_DAIFMT_IB_NF | SND_SOC_DAIFMT_CBS_CFS
```

`DSP_A` means data starts one bit clock *after* frame sync. That is the
same convention as bit 12 of `IFACE_CTRL`, which the EcoNet vendor
driver sets unconditionally and has a dedicated helper for
(`pcmBitDelaySet`). Two unrelated vendors landing on the same delay is a
good sign the bit is what it looks like.

**The SoC is the clock master.** `CBS_CFS` puts the codec in bit-clock
and frame slave. That matches the ZSI requirement on our side: the SLIC
only answers while the PCM engine is clocking PCLK/FSYNC, because it
never generates them itself.

**Property names.** Their ProSLIC node uses `channel_count`,
`reset_gpio` and `debug_level` -- the ProSLIC API's own conventions
rather than standard bindings. Worth knowing when reading vendor DTS,
though this tree uses `reset-gpios` and `econet,lines` to stay closer to
upstream style.

## The EN7523 pinctrl driver

`pinctrl-en7523.c` confirms the ZSI story from the hardware side. Its
`pcm_spi` group is:

```c
static const int pcm1_pins[]    = { 24, 25, 26, 27 };
static const int pcm2_pins[]    = { 16, 17, 18, 19 };
static const int pcm_spi_pins[] = { 16, 17, 18, 19, 24, 25, 26, 27 };
```

On this part the multiplexed SLIC control channel is not on a bus of
its own: it is the same eight pins as the two PCM buses, selected by a
different mux bit.

**That does not generalise across the family**, which is worth stating
plainly because the EN7523 evidence alone reads like it does:

| SoC | `pcm_spi` pins | overlap with PCM buses |
|-----|----------------|------------------------|
| EN7523 | 16-19, 24-27 | both |
| EN751221 | 17-20 | pcm2 only |
| EN7528 | 4-7 | none |

EN751221 is the part the ZSI work was proven on, and there the control
channel does share the pcm2 pins -- consistent with the register-side
finding that the SLIC only answers while the PCM engine is clocking.
EN7528 gives it dedicated pins and seven chip selects, so whether the
same timing constraint applies there is an open question rather than a
settled one.

The group list also names the rest of the SLIC wiring the vendor
expects: `pcm_spi_rst` (gpio 14), `pcm_spi_int` (gpio 15) and four chip
selects. So a SLIC reset line and an interrupt line are provided for by
the SoC, even though the drivers here poll instead.

### What it does not tell us

Nothing about slot width or companding. The ASoC path negotiates format
through `hw_params`, so the machine driver never states whether the
wire carries 16-bit linear or G.711 -- which is exactly the open
question on the Le9642 side (`tx_msb`, `big_endian_samples`). No help
there.
