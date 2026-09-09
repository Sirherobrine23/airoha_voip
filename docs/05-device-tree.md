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

| property                       | required | notes                                                                                  |
| ------------------------------ | -------- | -------------------------------------------------------------------------------------- |
| `compatible`                   | yes      | `econet,en751221-pcm`, `econet,en7528-pcm`, `airoha,en7523-pcm`                        |
| `reg` / `reg-names`            | yes      | one PCM range named `pcm`                                                              |
| `interrupts`                   | no       | vendor EN7523 candidate is `GIC_SPI 27`; validate on the board, or omit to use polling |
| `resets` / `reset-names`       | no       | use the SoC reset provider and the name `pcm`                                          |
| `airoha,pcm-interface-control` | no       | default `0xf5071306`                                                                   |
| `airoha,tx-slot-config`        | no       | 4 × u32, two slots per word                                                            |
| `airoha,rx-slot-config`        | no       | same                                                                                   |
| `airoha,dma-channel-mask`      | no       | gen1 supports `0xff`, EN7523 supports `0x0f`; configure all channels a board may open  |
| `airoha,pcm-big-endian`        | no       | swap 16-bit samples in the DMA buffer                                                  |

## Ownership of SCU resources

The current kernel already exposes the shared SCU resources through
pinctrl, reset and clock providers. The PCM driver therefore maps only
its own register window. It does not map the NP/chip SCUs, write
`0x834`, or change the pinmux behind the corresponding framework.

The reset IDs used by the supplied fragments are:

| block         | EN751221                    | EN7528                    | EN7523                                 |
| ------------- | --------------------------- | ------------------------- | -------------------------------------- |
| PCM1 engine   | `EN751221_PCM1_RST`         | `EN7528_PCM1_RST`         | `EN7523_PCM1_RST`                      |
| PCM2 engine   | `EN751221_PCM2_RST`         | `EN7528_PCM2_RST`         | not exposed as a separate engine reset |
| ZSI wrapper 1 | `EN751221_PCM1_ZSI_ISI_RST` | `EN7528_PCM1_ZSI_ISI_RST` | `EN7523_PCM1_ZSI_ISI_RST`              |
| ZSI wrapper 2 | `EN751221_PCM2_ZSI_ISI_RST` | `EN7528_PCM2_ZSI_ISI_RST` | `EN7523_PCM2_ZSI_ISI_RST`              |

These are binding IDs, not the raw register bit numbers. In particular,
the old draft's `0x1000` for PCM2 was wrong: the current MIPS reset
provider maps PCM2 to RST_CTRL1 bit 4. Bit 25 is named `SFC2_PCM_RST`;
it must not be treated as a generic SPI or SLIC reset.

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

The modern form has one `zsi` register range, a `zsi` reset, pinctrl and,
where exposed, a clock named `slic`. EN7523 provides `EN7523_CLK_SLIC`;
this is the SLIC control clock and must not be attached to the PCM node
as if it were the PCM bit/frame clock.

EN751221 currently has no SLIC clock in its clock provider. The supplied
EN751221 fragment therefore opts into `airoha,legacy-scu-programming`,
which retains the vendor-derived route/clock sequence. That sequence is
verified only on EN751221 and the driver rejects the property on EN7528
and EN7523. EN7528 ZSI clock/routing still needs board-level validation.

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

The PCM driver never writes pinmux registers. The ZSI driver also leaves
them to pinctrl, except inside the explicit EN751221 legacy mode when no
`pinctrl-0` state is present.

Groups and mux bits differ per SoC. All three keep them in `chip_scu`,
at different offsets:

**EN7523** — `chip_scu + 0x214` (`REG_GPIO_SPI_CS1_MODE`)

| group                         | pins           | bit   |
| ----------------------------- | -------------- | ----- |
| `pcm1`                        | 24-27          | 12    |
| `pcm2`                        | 16-19          | 13    |
| `pcm_spi`                     | 16-19, 24-27   | 16    |
| `pcm_spi_rst` / `pcm_spi_int` | 14 / 15        | 8 / 9 |
| `pcm_spi_cs1..cs4`            | 22, 39, 20, 23 | 17-21 |

**EN751221** — `chip_scu + 0x104` (`REG_IOMUX_CONTROL1`)

| group                         | pins    | bit     |
| ----------------------------- | ------- | ------- |
| `pcm1`                        | 25-28   | 13      |
| `pcm2`                        | 17-20   | 14      |
| `pcm_spi`                     | 17-20   | 12      |
| `pcm_spi_rst` / `pcm_spi_int` | 15 / 16 | 10 / 11 |
| `pcm_spi_cs3` / `cs4`         | 16 / 22 | 8 / 9   |

**EN7528** — `chip_scu + 0x15c` (`REG_PON_I2C_MODE`), chip selects 2-7 in
`REG_FORCE_GPIO22_EN`

| group                         | pins                  | bit               |
| ----------------------------- | --------------------- | ----------------- |
| `pcm1`                        | 12-15                 | 19                |
| `pcm2`                        | 24-27                 | 20                |
| `pcm_spi`                     | 4-7                   | 18                |
| `pcm_spi_rst` / `pcm_spi_int` | 2 / 1                 | 16 / 17           |
| `pcm_spi_cs1`                 | 3                     | 14                |
| `pcm_spi_cs2..cs7`            | 10, 23, 21, 9, 28, 29 | 10-15 (other reg) |

### Whether `pcm_spi` steals PCM pins depends on the SoC

This is the part that does not generalise:

| SoC      | `pcm_spi` pins | overlap            |
| -------- | -------------- | ------------------ |
| EN7523   | 16-19, 24-27   | **both** PCM buses |
| EN751221 | 17-20          | **pcm2** only      |
| EN7528   | 4-7            | **none**           |

So on EN7523 the multiplexed SLIC control channel costs you both PCM
buses, on EN751221 it costs you the second one, and on EN7528 it costs
nothing -- that part has dedicated pins and seven chip selects, which
says it was meant for many more FXS lines than the others.

On EN751221 there is a second trap: `pcm_spi_int` and `pcm_spi_cs3` are
both gpio 16. Pick one.

### The legacy EN751221 register sequence

The old draft wrote `chip_scu + 0x104` from both PCM and ZSI code. On
EN751221 this is exactly `REG_IOMUX_CONTROL1`, owned by pinctrl, so that
write was removed from the PCM driver. The only retained raw sequence is
the opt-in ZSI compatibility path described above.

For the record, the vendor-derived constants decode cleanly against the
EN751221 pinctrl: the mask `0x7d00` is bits 8, 10, 11, 12 and 14 --
`pcm_spi_cs3`, `pcm_spi_rst`, `pcm_spi_int`, `pcm_spi` and `pcm2` -- and
the value `0x2000` is bit 13, `pcm1`. "Enable the pcm1 mux, clear the
SLIC control bits."

## SLIC nodes

Le9642, child of the ZSI node:

| property           | notes                                                      |
| ------------------ | ---------------------------------------------------------- |
| `compatible`       | `microsemi,le9642`, `microsemi,le9641`, `microchip,le9642` |
| `airoha,zsi`       | phandle to the transport                                   |
| `airoha,pcm`       | phandle to the PCM instance                                |
| `airoha,lines`     | 1 or 2                                                     |
| `airoha,bus-slots` | one PCM **bus** timeslot per line                          |
| `airoha,slic-power-type` | **required**: `"bb"` or `"ib"`, see below              |
| `airoha,a-law`     | 16-bit linear is the default; see below                    |
| `airoha,no-zsi-tx-shift` | only for a board wiring this part over plain SPI/PCM |

The wire codec defaults to 16-bit linear, which is what the vendor
integration uses and what the PCM engine's 16-bit timeslots expect. Set
`airoha,a-law`, or the module parameter `codec=ulaw`, to run G.711
instead; the driver then compands in the PCM data path and the character
device is unchanged either way.

`airoha,slic-power-type` names the high-voltage converter around the
SLIC, and it has no default. `"bb"` is a 47 uH buck-boost running 12 V
to 100 V; `"ib"` is a 500 kHz inductorless inverting boost running 12 V
to 90 V. Eighteen bytes of the device profile differ between them, and
they are the switching-regulator timing, the regulator parameters, the
regulator control byte, the switcher configuration and the output
voltage limits. Streaming one topology's profile at the other programs
the wrong switching behaviour into a real power converter, so the driver
refuses to probe rather than guess. The vendor makes the same choice
through a `mode=IB` module parameter. Read it off the schematic; the
SLIC datasheet will not tell you, because this describes the circuit
around the chip rather than the chip.

`airoha,no-zsi-tx-shift` turns off the one-slot transmit shift that
compensates for the two PCLK cycles ZSI adds on transmit. It exists only
for a board that does not use ZSI. On a ZSI board, leaving the shift out
puts transmit audio one byte late — see `docs/07`.

ProSLIC, child of an SPI controller:

| property               | notes                                  |
| ---------------------- | -------------------------------------- |
| `compatible`           | `silabs,si32192`, `silabs,si3219x`     |
| interface              | SPI only; Airoha lists the Si32192 as ISI, see `docs/07` |
| `spi-cpha`, `spi-cpol` | mode 3; the driver asks for it anyway  |
| `spi-max-frequency`    | 10 MHz, per MediaTek's own integration |
| `airoha,pcm`           | phandle to the PCM instance            |
| `airoha,pcm-channel`   | DMA channel, not a bus slot            |
| `reset-gpios`          | optional                               |

The SPI controller these nodes attach to is the same one the boot flash
uses. Give the SLIC its own chip select, and expect flash erases to
disturb audio — see `docs/06`.

## The slot-versus-channel trap

`airoha,bus-slots` takes PCM **bus** timeslots. `airoha,pcm-channel`
takes a **DMA channel**. They are not the same number, and the relation
between them is not a constant of the hardware: it is whatever
`airoha,tx-slot-config` and `airoha,rx-slot-config` were programmed to
say. The slot field in those words is a bit offset into the 8 kHz frame,
so byte timeslot *s* is the channel whose configured offset is *s*·8.

With the default table channel *n* sits at bit offset 32 + *n*·16, i.e.
at bus slot 4 + *n*·2. Bus slots 4 and 6 are therefore DMA channels 0
and 1, and a two-line board wants `airoha,dma-channel-mask = <0x03>`.

An earlier revision used `channel = slot - 4` here, which contradicted
that table and left the second line on a channel the mask never enabled.
The SLIC drivers now ask the PCM driver to resolve a slot instead of
computing it, and a slot no channel covers fails the probe with a
message. The PCM driver separately rejects a line whose DMA channel is
outside `airoha,dma-channel-mask` or the SoC limit.

The configured DMA mask stays constant while the engine runs. Opening
or closing an additional FXS line only changes which FIFO is consumed;
it no longer tears down and restarts a call already in progress.

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
