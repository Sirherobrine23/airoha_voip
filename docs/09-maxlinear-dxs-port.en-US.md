# MaxLinear PEF32001/PEF32002: DXS/TAPI port

## Port status

Base: `airoha_voip` main commit a9a6ede. This patch implements CSI transport,
adapts the MaxLinear backend, and adds the `en75xx-slic-dxs` bridge to the
`/dev/en75xx-fxsN` ABI. This port has not been tested on DXS hardware.
The DTS examples start in **identify** mode: register access testing and a DXS
controller reset, without firmware/BBD download or linefeed activation.
This changes clocks, muxes and registers; it is not a read-only operation.

| Selected board | SoC | SLIC | Expected channels | DTS |
|---|---|---|---|---|
| Nokia G-240G-E | EN751221 | one PEF32002 | 2 | `dts/nokia-g-240g-e-dxs.dtsi` |
| MitraStar GPT-2742GX4X5v6 | EN7523 | PEF32001 | 1 in the example | `dts/mitrastar-gpt2742gx4x5v6-dxs.dtsi` |

The MitraStar loader can discover another SLIC. Check actual board population
before adding a second device. This patch does not enable Askey RTF8225VW or
Nokia XS-2426G-A.

## Correction: Nokia also uses CSI

The previous ZSI conclusion came from `type=ZSI` passed to `slic3_sep.ko`.
That does not describe the final DXS transport selection: the Nokia-specific
`dxs_init_spi()` decompilation forces `slicInterfType=3` and calls
`SPI_cfg()`/`SPI_Reset(3)`. Value 3 selects **CSI** in the matching `spi.ko`.
The logical protocol and framing differ from ZSI, although both select 5 in
the SYS SCU + 0x94 interface field.

The correct compatible is `econet,en751221-csi-spi`. The previous
`econet,en751221-zsi-spi` path was removed; the real Zarlink ZSI driver remains.
EN7523 uses `airoha,en7523-csi-spi`.

## Recovered protocol

DXS uses `7e offset` write headers and `be offset` read headers, with big-endian
word serialization. The mailbox register is 0x14. Both Nokia and MitraStar use
the following CSI framing:

| Operation | Implemented CSI framing |
|---|---|
| Single 16-bit register write | two header bytes followed by the word twice; example `7e 06 12 34 12 34` |
| Read | two header bytes, followed by a 10-byte read; discard the first byte and consume up to 4 bytes |
| Mailbox | fragments of at most 4 bytes, without word duplication or auto-increment |
| Wrapper configuration | `(CFG & 0xe0ffffef) \| 0x0f000010`; 10 µs inter-byte gap |

CSI quirks apply only to the two CSI controller compatibles. Conventional SPI
retains its original framing and auto-increment behavior.
Recovered EN751221 `pcm1.ko` gates are chip SCU + 0xd8 bit 0 and + 0x148 bits
2/3. Pinctrl owns the mux; no absolute divider value was invented.
EN7523 retains the existing wrapper clock/APLL sequence; CSI clock and timing
still require measurements on the board.

## Per-board firmware and BBD

| Image | Evidence | Bytes | SHA-256 |
|---|---|---|---|
| Nokia `DXS_FW.bin` | stock file; version 2.1.0 header | 16984 | `708f929decc967544c03295dbc7601ec6eda24c7197166d1d757afcabf3dd882` |
| MitraStar `DXS_FW_mitrastar.bin` | embedded in all three recovered Intel modules; version 2.1.3 header | 20168 | `779afa0255e079319ccb264acba714816505c7bcda60c73b967b0c2259dae824` |
| `DXS_BBD.bin` | stock Nokia, embedded in all three MitraStar modules, identical to official `coef_dxs-0.0.9/dcdc_CIBB12/R600.bin` | 498 | `e0c84932582aa8a2931c59d910896f9b1184d189f59af1574ef8fc586354261d` |

MitraStar `IntelSlicInit()` passes `.data + 0x94` and size 0x4ec8 to
`DXS_DwldAndStartFW()`; ARM relocations confirm the `.LANCHOR2` symbol.
The blob starts at file offset 183884 in `slic3_intel.ko` and
`slic3_intel_latam_25.ko`, and 185020 in `slic3_intel_latam_25_BR.ko`.
The extractor searches for the header and verifies the complete hash rather
than relying on these offsets alone.

Extract from a locally available OEM module **before** building the package:

```sh
python3 tools/extract-dxs-mitrastar.py /path/to/slic3_intel.ko \
    firmware/dxs/DXS_FW_mitrastar.bin
```

The OpenWrt package includes this image when it exists. The bridge does not
silently fall back to Nokia firmware on MitraStar. CIBB12/R600 is circuit
specific; its presence does not establish regulatory suitability for every
country or compatibility with another PEF3200x board. The original BBD parser
checks the image and compiled topology. The official 2.1.9 firmware recovered
from the SDK does not automatically replace either OEM image.

## Implemented integration

The backend retains MaxLinear TAPI, firmware download, mailbox handling,
BBD parsing, power control and ringing. The bridge uses the TAPI3 Kernel API
for line operations. Firmware and BBD have dedicated kernel-buffer entry
points: `request_firmware()` pointers are not sent through `copy_from_user()`.

The bridge configures linear 16-bit/8 kHz PCM, hook, linefeed, cadence and
ring trip, querying events every 20 ms. The ABI fault field records the most
recent observed TAPI fault event ID. No external DXS IRQ is assumed: the
backend runs in polling mode. The PCM IRQ still comes from DTS.
Line registration reuses `en75xx_voice`, allowing the existing PCM tests and
`chan_en75xx` after successful initialization.

TAPI device numbering follows chip select, with optional `airoha,tapi-device`
override. Fixes include logical GPIO reset polarity, probe-defer propagation,
single SPI driver registration, mutex cleanup after failure, and dynamic
major numbers without 8-bit truncation.

## DTS and test sequence

Include only the selected board fragment after the base SoC DTS. The fragments
expect `scuclk`, `chip_scu`, `pinctrl`, `pcm0`, and the kernel clock/reset bindings.
Each fragment imports the repository voice definition; do not separately include
that same definition again in the board DTS. CSI replaces the alternative ZSI
node at the same address. Keep other ISI/SLIC clients disabled on this wrapper.

No physical reset GPIO was assigned: stock `gpio_tool write 254` addresses an
OEM logical interface and does not prove Linux GPIO 254. Add `reset-gpios`
and its polarity only when confirmed. Without it, the backend relies on the
external/boot reset state; the access attempt may fail.

Select `kmod-en75xx-slic-dxs-cibb12` in OpenWrt. It includes `drv_tapi_dxs.ko`
and `en75xx-slic-dxs.ko`, with PCM/serial and firmware dependencies. The package
does not autoload DXS. For the first test, leave DTS in identify mode:

```sh
modprobe drv_tapi_dxs
modprobe en75xx-slic-dxs
dmesg | grep -Ei 'dxs|csi|en75xx'
```

Expect `DXS access test passed`. Identify mode does **not** create
`/dev/en75xx-fxs*`. If access fails, check reset, mux, clocks and framing before
proceeding.

To enable voice, change `airoha,bringup-mode` to `"voice"` in `&dxs_fxs` and
set `clock-frequency`, `maxlinear,pcm-tx-edge` and `maxlinear,pcm-rx-edge`.
Edges are from the DXS perspective: 0=rising, 1=falling. The frequency is PCM
DCL, not `spi-max-frequency`. This adapter supports 1.024/2.048/4.096/8.192 MHz.
Use frequency and edges verified on hardware; identify DTS examples deliberately
leave them unset. Firmware, BBD and `maxlinear,dcdc-topology = "CIBB12"` are
already explicit.

TAPI slots count bytes; the SoC table counts bits. The bridge sets
**DXS RX = SoC TX / 8** and **DXS TX = SoC RX / 8**. The MitraStar example uses
TX/RX bits 24/32, following `IntelSlicInit()` RX/TX slots 3/4.
The Nokia example uses bits 32/48 for its two channels in both directions.
That is internally consistent but still requires validation of the board's
PCM framing and clock.

After voice initialization registers the lines, test info/hook, then low-level
audio, and finally ringing. Successful CSI access does not establish correct
audio or analog operation. This unvalidated port makes no claim of zero
electrical risk.

## Build and validation performed

`tools/build-dxs.sh` builds the backend first and passes its `Module.symvers`
to the bridge Kbuild. It requires a prepared kernel and toolchain and accepts
additional configure arguments. OpenWrt uses the same build order.

Validation in this session:

- 11 host tests passed, including actual DXS C access functions with mocked I/O:
  both CSI compatibles, prefix handling, padding, mailbox, transport errors,
  and preservation of conventional SPI.
- The extractor recovered the same image from all three MitraStar modules and
  rejected truncated/corrupted input.
- The backend and EN75xx modules compiled against Linux 6.18.55, commit
  `21c8344c030504a8e85dfb1c84cf4970e754302a`, using an x86_64 compilation
  configuration with OF/SPI/GPIOLIB/REGMAP enabled.
- The kernel was not fully built: its `Module.symvers` was unavailable, so
  modpost allowed warnings for kernel exports. Backend/bridge/EN75xx exports
  were checked separately. This is not a full OpenWrt build, ARM/MIPS cross-build,
  or module loading test.
- Both DTS fragments compiled with dtc against that kernel's actual SoC DTS
  files. Existing I²C/switch/watchdog warnings remain; dt-schema and full
  per-board DTS validation were not performed.

## Sources consulted

From recovered `sdk_base`: `docs/maxlinear/617580_dxs_api_device_driver_pr_rev3.0.pdf`
and `docs/maxlinear/PEF32001VSV13.pdf`; Nokia modules/decompilations under
`reverse/en751221/nokia-g_240g_e/` and MitraStar under
`reverse/en7523/ghidra/mitrastar/`. The DxsInit/DxsBBD API PDF does not replace
the TAPI headers used by this port. Ghidra output was treated as behavioral
evidence and checked against ELF relocations/bytes, not as directly rebuildable
C source. The MitraStar rootfs supplied the modules used for firmware/BBD comparison.
