# MaxLinear PEF32001/PEF32002 (DXS101/DXS102) port

## Confirmed material

DXS TAPI SP 2.0 contains `drv_tapi_dxs-1.0.0.0`, firmware 2.1.9,
coefficient package 0.0.9 and the line-testing library. The driver source is
dual-licensed under GPL-2.0/BSD-2-Clause and is preserved under
`vendor/maxlinear/drv_tapi_dxs-1.0.0.0`.

DXS uses SPI mode 3 at the logical protocol level. On the EN7523 Askey
RTF8225VW and MitraStar GPT-2742GX4X5v6 boards, however, that protocol is
carried by the PCM-shared CSI wrapper rather than an independent SPI
controller. The OEM firmware explicitly detects `Lantiq CSI` and loads
`slic3_intel.ko type=CSI slic=pef32001 mode=IB`.

The Nokia G-240G-E stock rootfs shows a different EN751221 integration:
`slic3_sep.ko type=ZSI fxsNum=2 devNum=1 pcm1ChNum=2`, followed by
`drv_ifxos.ko`, `drv_tapi.ko`, and `drv_dxs.ko major=93`. `drv_dxs.ko` imports
`SPI_cfg`, `SPI_Reset`, and `SPI_bytes_{read,write}`. Thus, "SPI" is the DXS
logical API here, while stock `spi.ko` carries those bytes over ZSI. The
`econet,en751221-zsi-spi` compatible reproduces that bridge through the modern
SPI subsystem instead of incorrectly treating this board as CSI.

The current reference permits up to 8 MHz; the older Voice
System Package 4.44 documentation limited its integration to 2 MHz. Initial
bring-up should use 2 MHz and increase it only after register, firmware and
mailbox transactions run without errors.

The register protocol has a two-byte header. A write uses `0x7e, offset`; a
read uses `0xbe, offset`. Bit 0 of the second byte enables auto-increment for
multi-word transfers, except for the `DXS_HOST_DATA` mailbox register. Normal
registers are 16-bit words and the mailbox carries 32-bit words. Chip select
must remain active across the header and payload.

## Electrical topology

The BBD controls line coefficients, ringing, DC feed and converter topology.
The historical `firmware/dxs/DXS_BBD.bin` is byte-identical to the official
`dcdc_CIBB12/R600.bin` profile:

| Field     | Confirmed value                                                    |
| --------- | ------------------------------------------------------------------ |
| DC/DC     | CIBB12                                                             |
| Impedance | 600 ohms                                                           |
| Size      | 498 bytes                                                          |
| SHA-256   | `e0c84932582aa8a2931c59d910896f9b1184d189f59af1574ef8fc586354261d` |

Both repository blobs are byte-identical to the Nokia G-240G-E production
rootfs files:

| File          | SHA-256                                                            |
| ------------- | ------------------------------------------------------------------ |
| `DXS_FW.bin`  | `708f929decc967544c03295dbc7601ec6eda24c7197166d1d757afcabf3dd882` |
| `DXS_BBD.bin` | `e0c84932582aa8a2931c59d910896f9b1184d189f59af1574ef8fc586354261d` |

This confirms CIBB12/R600 for that specific board, not every board fitted with a PEF32001/PEF32002.
Before enabling line feed or ringing on another board, confirm the topology
from its schematic, BOM or stock firmware.

## Selected architecture

The MaxLinear driver already contains TAPI, BBD parsing, firmware download,
mailbox handling, ring, hook, PCM and line tests. Reimplementing these state
machines in a small driver would discard important validation. The first
integration keeps `drv_tapi_dxs` as an independent module. A later bridge uses
the TAPI Kernel API (`ifx_tapi_kopen`, `ifx_tapi_kioctl`,
`ifx_tapi_kclose`) to register lines with `en75xx_voice` without duplicating
electrical control.

Do not autoload the module until the following are available:

1. explicit per-board DC/DC topology selection;
2. firmware and BBD matching the silicon and circuit;
3. a mode-3 SPI node with the correct reset polarity;
4. PCM slots matching those programmed into the EN75xx controller;
5. a TAPI-to-`/dev/en75xx-fxsN` bridge or equivalent userspace support.

## Confirmed board matrix

| SoC      | Board                     | SLIC               | Transport status                                          |
| -------- | ------------------------- | ------------------ | --------------------------------------------------------- |
| EN751221 | Nokia G-240G-E            | 1× PEF32002, 2 FXS | ZSI; stock stack, clock/route, firmware and BBD confirmed |
| EN7580   | Nokia XS-2426G-A          | PEF32002           | CSI; SoC routing/clock still pending                      |
| EN7523   | MitraStar GPT-2742GX4X5v6 | PEF32001           | CSI and CIBB12 OEM build confirmed                        |
| EN7523   | Askey RTF8225VW           | PEF32001           | CSI and CIBB12 OEM build confirmed                        |

In both EN7523 root filesystems, `voip_loader` supports either one or two
devices and only appends `devNum=2` when discovery finds the second SLIC.
Consequently the number of lines must not be inferred from PEF32001 alone.

On the G-240G-E the hardware file declares two POTS ports using PEF32002, but
the loader confirms `devNum=1` and `pcm1ChNum=2`: one dual-channel DXS102 using
two PCM1 slots.

## Minimal EN751221/ZSI Device Tree node

```dts
zsi0: spi@1fbd1000 {
	compatible = "econet,en751221-zsi-spi";
	reg = <0x1fbd1000 0x1000>;
	reg-names = "zsi";
	airoha,scu = <&scuclk>;
	airoha,chip-scu = <&chip_scu>;
	airoha,legacy-scu-programming;
	resets = <&scuclk EN751221_PCM1_ZSI_ISI_RST>;
	reset-names = "zsi";
	pinctrl-names = "default";
	pinctrl-0 = <&pcm_spi_pins>;
	airoha,zsi-gap-us = <5000>;
	#address-cells = <1>;
	#size-cells = <0>;

	dxs@0 {
		compatible = "maxlinear,pef32002";
		reg = <0>;
		spi-max-frequency = <2000000>;
		spi-cpol;
		spi-cpha;
		reset-gpios = <&gpio N GPIO_ACTIVE_HIGH>;
		reset-interval-ms = <10>;
	};
};
```

The reset GPIO number still has to come from the board DTS or schematic. The
stock `gpio_tool write 254 1,0,1` sequence appears to address an OEM logical
GPIO framework and is not enough evidence for putting GPIO 254 in Linux DT.

## Minimal EN7523/CSI Device Tree node

```dts
csi: spi@1fbd1000 {
	compatible = "airoha,en7523-csi-spi";
	reg = <0x1fbd1000 0x100>;
	/* clocks, resets and SCU phandles match the SoC ISI node */

	dxs@0 {
		compatible = "maxlinear,pef32001";
		reg = <0>;
		spi-max-frequency = <2000000>;
		spi-cpol;
		spi-cpha;
		reset-gpios = <&gpio N GPIO_ACTIVE_HIGH>;
		reset-interval-ms = <10>;
	};
};
```

Reset GPIO numbers and polarity still need board-specific confirmation. CSI
transport support alone does not authorize BBD download or linefeed/ringing.

The original driver uses the legacy `intel,reset-gpios` and
`intel,reset-interval` property names. The modern DT adaptation should accept
`reset-gpios` and `reset-interval-ms`, retaining the old names as fallback.
