# DXS: polling and HOST register observations

Apply after stage-isolation and oem-hostcfg. This patch adds diagnostics;
it does not fix or bypass calibration.

The latest run confirms HOST_CFG=0x07 and firmware 2.1.1 immediately before
opmode=6. Calibration and the later mailbox version query both time out.
No subsequent BOOT_FIN or positive ROM identification appears in that run.
The interrupt-polarity alignment did not resolve this attempt.

The old trace only printed nonzero INT1 or register-read errors, so silence
could mean stopped polling or active polling reading zero. With event_trace=1,
the new observer counts progress per device and prints snapshots before
calibration and after a failed calibration wait:

| Counter | Observation |
|---|---|
| poll | Timer callback found the device |
| eligible | Polling mode and basic initialization permit handling |
| handler | Handler passed the SPI-active/STOP filters |
| int_reads | Normal interrupt register read returned, including errors |
| outbox=A/B | Outbox handler entries/returns |
| done | Active handler path returned |

Counters are atomic but the multi-field snapshot is not globally atomic.
They accumulate while tracing is enabled, until backend unload.
Each snapshot also reads DXS HOST_CFG (0x06), HOST_IEN1 (0x08), HOST_IEN2
(0x0a), HOST_LEN (0x18), and HOST_BINF (0x24) through the existing transport.
These are not SoC MMIO addresses. It does not read self-clearing INT1/INT2
or consume DATA; normal event handling retains responsibility for those.
It sends no extra reset, calibration, recovery or linefeed command.

`cached_fw` is the host cache, not proof of the live firmware after failure.
A `reading reg=` marker precedes every register transaction, followed by
its result or error. The extra reads/console messages alter observation timing;
the original calibration timeout and acceptance conditions remain unchanged.

Rebuild/install drv_tapi_dxs.ko for the running kernel, keeping firmware,
BBD, PCM timing and HOST_CFG unchanged from the preceding run. With both
modules currently loaded, after replacing the backend on disk:

```sh
rmmod en75xx_slic_dxs &&
rmmod drv_tapi_dxs &&
modprobe drv_tapi_dxs &&
echo 1 > /sys/module/drv_tapi_dxs/parameters/event_trace &&
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" bringup_mode=voice

dmesg | tail -n 200
```

Compare the two snapshots. Advancing poll/handler/read/done counters show
host progress even when INT1 logs are absent. If poll advances but eligible
does not, inspect mode/state. Handler progress without completed reads points
toward a transaction in progress or a transport stall. Outbox entries without
matching returns point toward its processing/locks. Coherent HOST responses
with no mailbox reply narrow the firmware/event investigation but do not
prove electrical health; SPI success alone does not prove SLIC presence.
Compare all-zero/all-one values and failures against the baseline.

The host test compiles the actual observation/handler/timer code with mocked
register access in both 8-bit and 16-bit paths. It passed locally. Full
OpenWrt module build and board validation remain necessary.
