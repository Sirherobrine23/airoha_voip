# MitraStar EN7523: OEM DXS comparison

## Inputs and scope

The three supplied Ghidra exports were inspected: `slic3_intel.ko.c`,
`slic3_intel_latam_25.ko.c`, and `slic3_intel_latam_25_BR.ko.c`.
They identify the OEM DXS driver as 1.3.1.0. The port uses the standalone
`drv_tapi_dxs-1.0.0.0` bundle, which includes DXS and TAPI; these package
numbers alone do not establish implementation equivalence or upgrade order.

The first two exports contain 556 parsed functions each. Only the body of
`dxs_proc_VersionGet` differs textually, reporting different build timestamps.
This does not establish equality of the ELF binaries or their data sections.
The BR export contains 557 functions and additional BBD/mailbox/event tracing.
Across all three, 422 of 517 common named functions have identical C text.
Other differences include relocated symbols, Ghidra temporary variables,
tracing and changes in other routines. Full semantic/data equivalence is
not claimed.

Ghidra C is useful evidence, not directly buildable source. Untyped globals,
inferred void returns, `extraout_*`, and opaque DAT/UNK/LANCHOR references
must not be copied literally. Data arrays require ELF inspection or verified
extractions. The Portuguese companion contains the function/line reference table.

## Confirmed critical sequence

The OEM performs basic access/reset, ROM version query, firmware download,
DEV_START, BBD download including calibration, PCM configuration and linefeed.
Calibration precedes PCM configuration in the OEM as well as in the port.

Calibration requests opmode 6 and waits 500 ms. The operating-mode event path
must observe entry into calibration and the subsequent transition out of it;
the completion handler reads/checks results and wakes the caller. Pending
calibration must not be treated as success or bypassed to expose an FXS line.

Polling uses a 10 ms timer. INT1 bit 0x800 indicates an available outbox.
The version query uses a mailbox command, the same polling/outbox chain and
a host FIFO; it is not an independent test of register-level CSI access.

The reconstructed channel-zero calibration command matches the current trace:
header `0x01000704`, payload `0x00060000`. Expected CSI fragments are:

```text
7e 14 01 00 07 04
7e 14 00 06 00 00
```

These are reconstructed bytes, not an on-board bus capture. The OEM reset
routine also sends disabled-mode commands to channels 0 and 1, so the
reported rejection of `0x01010704` does not establish a port-only defect.

## Concrete HOST_CFG difference

All three OEM exports end DXS_FW_Start by writing DXS register 0x06:
0x07 for CSI (`slicInterfType == 1`), 0x03 otherwise. The previously available
OEM ELF has the same decompiled function; ARM opcodes confirm MOVEQ r2,#7
at .text+0x1488c, MOVNE r2,#3 at .text+0x14890 and the DXS_RegWrite call
relocation at .text+0x14894.

The port instead wrote 0x06 for CSI. The register definitions identify:

| Bit | Value | Meaning |
|---|---:|---|
| INT_MD_INTPOS | 0x01 | Interrupt output polarity |
| SC_MD_SCON | 0x02 | Interrupt self-clearing mode |
| 8BIT_EN | 0x04 | 8-bit SPI access |
| RST_RSTCORE | 0x80 | Core reset |

This patch selects 0x07 after firmware download for the explicit EN7523 CSI
controller compatible and verifies bits 0..2 by register readback. A failed
read or mismatch aborts initialization. The access-test configuration remains
separate. Other controllers retain the port's existing 0x06; blindly copying
OEM 0x03 would also change the access-width configuration.

This is an OEM alignment, not a proven calibration fix. The port uses polling,
so interrupt output polarity alone does not explain the observed failures.
No power setting, coefficient, timeout, clock, opmode command or calibration
acceptance criterion is changed. HOST_CFG is a DXS internal register accessed
through CSI/SPI, not EN7523 MMIO for devmem writes.

## Current hardware evidence

DEV_START-only and BBD-without-calibration diagnostics preserve firmware 2.1.1
after fresh queries following 3-second waits. The latest voice attempt still
reports 2.1.1 immediately before opmode=6, then times out with pending=1;
the subsequent version query also times out. There is no new BOOT_FIN or
positive ROM identification in that attempt. Earlier attempts did report
BOOT_FIN followed by ROM 2.0.0 before host cleanup. Keep these observations
distinct when diagnosing the next run.

## Apply and test

Apply after the CSI and stage-isolation patches:

```sh
git apply --check /path/airoha-voip-dxs-oem-hostcfg.patch
git apply /path/airoha-voip-dxs-oem-hostcfg.patch
```

Rebuild/install `drv_tapi_dxs.ko` for the running kernel. This patch does not
require a new adapter API. Keep the previous firmware, BBD and timing settings.
With both modules currently loaded, replace the backend on disk, then:

```sh
rmmod en75xx_slic_dxs &&
rmmod drv_tapi_dxs &&
modprobe drv_tapi_dxs &&
echo 1 > /sys/module/drv_tapi_dxs/parameters/event_trace &&
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" bringup_mode=voice

dmesg | tail -n 160
```

Expect `runtime HOST_CFG: requested=0x7 read=0x7 status=0x0` before calibration.
The host C test covers controller selection, missing device/node, failed
register writes/reads and mismatched readback. It passed with cc -Wall -Werror;
full OpenWrt module build and hardware validation remain outstanding.

If calibration still fails, verify polling continuity and non-destructive
HOST register responses after opmode=6. Physical CSI/reset/power captures
can then distinguish protocol, clock and supply problems; the decompilations
alone cannot determine those electrical conditions.
