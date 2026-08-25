# Device tree bindings

Three SoC fragments plus board examples:

```
dts/en751221-voice.dtsi     gen1, MIPS big endian
dts/en7528-voice.dtsi       gen1, MIPS little endian
dts/en7523-voice.dtsi       gen2, ARM
dts/examples/*.dts          boards consuming them
```

Include a fragment after the SoC `.dtsi` and enable what the board
populates. Everything ships `disabled`: an unpopulated PCM block that
probes will arm its DMA against a SLIC that is not there.

## pcm@1fbd0000

| property | required | notes |
|----------|----------|-------|
| `compatible` | yes | `econet,en751221-pcm`, `econet,en7528-pcm`, `airoha,en7523-pcm` |
| `reg` / `reg-names` | `pcm` yes | plus optional `sys` (0x1fb00000) and `chip` (0x1fa20000) |
| `interrupts` | no | `GIC_SPI 27` on the ARM parts; omit to run the polling worker |
| `airoha,pcm-interface-control` | no | default `0xf5071306` |
| `airoha,pcm-reset-mask` | no | bit in SYS 0x834; default `0x800` |
| `airoha,tx-slot-config` | no | 4 × u32, two slots per word |
| `airoha,rx-slot-config` | no | same |
| `airoha,pcm-big-endian` | no | swap 16-bit samples in the DMA buffer |
| `airoha,configure-pcm-pins` | no | let the driver set the pinmux |

## Two device tree styles

Upstream and the vendor model the control blocks differently, and the
drivers accept both.

Upstream (`arch/arm64/boot/dts/airoha/en7523.dtsi`) exposes them as
syscons and expects phandles:

```
airoha,scu = <&scuclk>;         /* system-controller@1fb00000 */
airoha,chip-scu = <&chip_scu>;  /* syscon@1fa20000            */
```

The vendor trees instead put raw ranges in each consumer's `reg`:

```
reg = <0x1fbd0000 0x1000>, <0x1fb00000 0x1000>, <0x1fa20000 0x1000>;
reg-names = "pcm", "sys", "chip";
```

Prefer the phandles where the tree offers them: a regmap serialises
against the other users of the block, and there is no address to get
wrong. The raw form is the fallback.

The `sys` and `chip` ranges overlap the SoC's SCU node on purpose --
the vendor tree has `scu@1fb00000` covering both `0x1fb00000` and
`0x1fa20000`. The driver maps them with plain `devm_ioremap` rather
than `devm_ioremap_resource` so the regions are not claimed
exclusively; otherwise whichever driver probed second would fail with
`-EBUSY`. The same applies to the ZSI node, which needs the same two
ranges.

Without `sys` and `chip` the driver cannot reset the block or configure
the PCM pins. It will still probe, which makes for a confusing failure —
map both unless you know you do not need them.

The vendor trees model the whole thing as a single node --
`pcm@bfbd0000` with `reg = <0x1fbd0000 0x4fff>`, identical on EN7523,
EN7552 and EN7581 -- covering PCM1 at +0x0000, the ZSI wrapper at
+0x1000, PCM2 at +0x2000 and a second wrapper at +0x4098. This tree
splits that window into separate nodes so the PCM engine and the ZSI
transport can be separate modules. The ranges do not overlap, so both
views are valid; just do not mix them with a driver that claims the
whole window.

The compatible alone selects the descriptor layout, ring config and
timeslot register count. Nothing else in the node changes between
generations, which is why the three fragments look nearly identical.

## zsi@1fbd1000

Needs all three ranges (`zsi`, `sys`, `scu`): entering ZSI mode changes
the interface mode, the pinmux and the PCM clock, not just the wrapper.

`airoha,zsi-gap-us` defaults to 5000. The wrapper acknowledges a byte as
soon as it has sent it, but the SLIC still has to clock it over the
8 kHz bus; below roughly 2000 µs the SLIC mis-parses profile streams and
answers `0xf1`.

There is deliberately no node for `0x1fbc0000`. That address is
`CR_SPI_BASE`, the SoC's SPI master, shared between the boot flash and
the SLIC through chip selects — see `docs/06-spi-shared-with-flash.md`.
The ProSLIC goes under the existing SPI controller node as an ordinary
`spi_device`.

## Pin muxing

On an upstream tree the PCM pins belong to pinctrl, not to this driver.
Give the node a state and the pinctrl core applies it before probe:

```
pinctrl-names = "default";
pinctrl-0 = <&pcm1_pins>;
```

Both the PCM and the ZSI driver check for a `pinctrl-0` property and
skip their internal mux writes when they find one, so a tree that uses
pinctrl and a tree that uses `airoha,configure-pcm-pins` both work and
neither fights the other.

Groups and mux bits differ per SoC. All three keep them in `chip_scu`,
at different offsets:

**EN7523** — `chip_scu + 0x214` (`REG_GPIO_SPI_CS1_MODE`)

| group | pins | bit |
|-------|------|-----|
| `pcm1` | 24-27 | 12 |
| `pcm2` | 16-19 | 13 |
| `pcm_spi` | 16-19, 24-27 | 16 |
| `pcm_spi_rst` / `pcm_spi_int` | 14 / 15 | 8 / 9 |
| `pcm_spi_cs1..cs4` | 22, 39, 20, 23 | 17-21 |

**EN751221** — `chip_scu + 0x104` (`REG_IOMUX_CONTROL1`)

| group | pins | bit |
|-------|------|-----|
| `pcm1` | 25-28 | 13 |
| `pcm2` | 17-20 | 14 |
| `pcm_spi` | 17-20 | 12 |
| `pcm_spi_rst` / `pcm_spi_int` | 15 / 16 | 10 / 11 |
| `pcm_spi_cs3` / `cs4` | 16 / 22 | 8 / 9 |

**EN7528** — `chip_scu + 0x15c` (`REG_PON_I2C_MODE`), chip selects 2-7 in
`REG_FORCE_GPIO22_EN`

| group | pins | bit |
|-------|------|-----|
| `pcm1` | 12-15 | 19 |
| `pcm2` | 24-27 | 20 |
| `pcm_spi` | 4-7 | 18 |
| `pcm_spi_rst` / `pcm_spi_int` | 2 / 1 | 16 / 17 |
| `pcm_spi_cs1` | 3 | 14 |
| `pcm_spi_cs2..cs7` | 10, 23, 21, 9, 28, 29 | 10-15 (other reg) |

### Whether `pcm_spi` steals PCM pins depends on the SoC

This is the part that does not generalise:

| SoC | `pcm_spi` pins | overlap |
|-----|----------------|---------|
| EN7523 | 16-19, 24-27 | **both** PCM buses |
| EN751221 | 17-20 | **pcm2** only |
| EN7528 | 4-7 | **none** |

So on EN7523 the multiplexed SLIC control channel costs you both PCM
buses, on EN751221 it costs you the second one, and on EN7528 it costs
nothing -- that part has dedicated pins and seven chip selects, which
says it was meant for many more FXS lines than the others.

On EN751221 there is a second trap: `pcm_spi_int` and `pcm_spi_cs3` are
both gpio 16. Pick one.

### The register the driver used to poke

`airoha,configure-pcm-pins` makes the driver write `chip_scu + 0x104`
itself. On EN751221 that is exactly `REG_IOMUX_CONTROL1`, the register
upstream pinctrl manages -- the two would fight. Both the PCM and the
ZSI driver check for a `pinctrl-0` property and skip their internal mux
writes when they find one, so an upstream tree and a vendor tree both
work.

For the record, the vendor-derived constants decode cleanly against the
EN751221 pinctrl: the mask `0x7d00` is bits 8, 10, 11, 12 and 14 --
`pcm_spi_cs3`, `pcm_spi_rst`, `pcm_spi_int`, `pcm_spi` and `pcm2` -- and
the value `0x2000` is bit 13, `pcm1`. "Enable the pcm1 mux, clear the
SLIC control bits."

## No PCM reset in the SCU binding

`econet,en751221-scu.h` and `econet,en7528-scu.h` define FE, GSW, GDMA
and XPON resets; neither has a PCM line. So the block is reset through
the raw bit in NP SCU `0x834` (`airoha,pcm-reset-mask`) rather than
`resets = <&scuclk ...>`. If a PCM reset ID appears in those headers
later, switching is the obvious cleanup.

## SLIC nodes

Le9642, child of the ZSI node:

| property | notes |
|----------|-------|
| `compatible` | `microsemi,le9642`, `microsemi,le9641`, `microchip,le9642` |
| `airoha,zsi` | phandle to the transport |
| `airoha,pcm` | phandle to the PCM instance |
| `airoha,lines` | 1 or 2 |
| `airoha,bus-slots` | one PCM **bus** timeslot per line |
| `airoha,a-law` | u-law is the default |

ProSLIC, child of an SPI controller:

| property | notes |
|----------|-------|
| `compatible` | `silabs,si32192`, `silabs,si3219x` |
| `spi-cpha`, `spi-cpol` | mode 3; the driver asks for it anyway |
| `spi-max-frequency` | 10 MHz, per MediaTek's own integration |
| `airoha,pcm` | phandle to the PCM instance |
| `airoha,pcm-channel` | DMA channel, not a bus slot |
| `reset-gpios` | optional |

The SPI controller these nodes attach to is the same one the boot flash
uses. Give the SLIC its own chip select, and expect flash erases to
disturb audio — see `docs/06`.

## The slot-versus-channel trap

`airoha,bus-slots` takes PCM **bus** timeslots. `airoha,pcm-channel`
takes a **DMA channel**. They are not the same number: the RX DMA lands
a slot's audio on channel `slot - 4`, so bus slot 4 is DMA channel 0 and
bus slot 6 is DMA channel 2.

Two lines sharing a slot is the failure worth naming, because it does
not look like a failure: both probe fine and the second line is simply
silent.

## Timeslot words

`airoha,tx-slot-config` packs two slots per word:

```
bits 12      slot width: 1 = 16-bit           [even slot]
bits 9:0     bit offset of the slot           [even slot]
bit  28      slot width                       [odd slot]
bits 25:16   bit offset                       [odd slot]
```

The default `<0x10301020 ...>` is eight 16-bit slots at bit offsets 32,
48, 64, 80, 96, 112, 128, 144. Offsets accumulate by the slot width, so
narrowing a slot to 8 bits shifts everything after it.

## Checking a tree without the SoC sources

```
dtc -I dts -O dtb -o /dev/null board.dts
```

A `Missing interrupt-parent` warning on the PCM node when compiling a
fragment in isolation is expected — the real SoC `.dtsi` supplies it.
