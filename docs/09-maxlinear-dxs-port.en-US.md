# MaxLinear PEF32001/PEF32002 (DXS101/DXS102) port

## Confirmed material

DXS TAPI SP 2.0 contains `drv_tapi_dxs-1.0.0.0`, firmware 2.1.9,
coefficient package 0.0.9 and the line-testing library. The driver source is
dual-licensed under GPL-2.0/BSD-2-Clause and is preserved under
`vendor/maxlinear/drv_tapi_dxs-1.0.0.0`.

DXS uses SPI mode 3. The current reference permits up to 8 MHz; the older Voice
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

This identifies the blob, not every board fitted with a PEF32001/PEF32002.
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

## Minimal Device Tree node

```dts
dxs@0 {
	compatible = "maxlinear,pef32001"; /* or maxlinear,pef32002 */
	reg = <0>;
	spi-max-frequency = <2000000>;
	spi-cpol;
	spi-cpha;
	reset-gpios = <&gpio N GPIO_ACTIVE_HIGH>;
	reset-interval-ms = <10>;
};
```

The original driver uses the legacy `intel,reset-gpios` and
`intel,reset-interval` property names. The modern DT adaptation should accept
`reset-gpios` and `reset-interval-ms`, retaining the old names as fallback.
