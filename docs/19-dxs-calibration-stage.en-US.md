# DXS: isolated calibration before audio configuration

## Nokia G-240G-E result

On October 19, 2026, EN751221/PEF32002 passed identify, firmware, DEV_START
and BBD download without calibration. Fresh queries after 3000 ms returned
HW 0xb, FW 0x02010000 (2.1.0), two FXS channels, including after BBD blocks.
HOST_CFG requested/read was 0x6. The 498-byte BBD SHA-256 is
`e0c84932582aa8a2931c59d910896f9b1184d189f59af1574ef8fc586354261d`.
These tests do not establish calibration or PCM/analog voice operation.

## New calibrate mode

The identification DTS lacks PCM frequency/edge properties. Voice validates
those properties before chip startup and would fail before calibration.

`bringup_mode=calibrate` boots the declared firmware, runs DEV_START, waits
and queries firmware identity, then invokes the existing full BBD/calibration
routine used by voice. It checks identity immediately afterward and again
after `stage_wait_ms`, and returns before audio PCM setup/activation and FXS
line registration. No new `/dev/en75xx-fxs*` nodes are expected in this mode.

BBD/topology validation and calibration error handling are retained. The
calibration timeout is unchanged. Voice still requires PCM properties and
normal calibration. Calibrate performs real analog calibration and may
operate internal supply circuitry; it is not a read-only test. The bridge
does not request operating linefeed or ringing in this mode.

The device remains initialized until bridge removal. Existing DEV_STOP/reset
cleanup handles removal. No backend API or exported symbol changes are needed.

## Apply and run

This is incremental over the bridge providing identify/firmware/start/bbd/
voice. The reviewed source blob is
`6ceb155e343c36c4a37b1344911e1f68f39dcdee` for `src/en75xx_slic_dxs.c`.
Apply from the airoha_voip root:

```sh
git apply --check /path/airoha-voip-dxs-calibrate.patch
git apply /path/airoha-voip-dxs-calibrate.patch
```

Rebuild the bridge through the OpenWrt package and install the resulting
`en75xx-slic-dxs.ko` for the board's running kernel before using the new mode.
Existing firmware files and backend patches are still required.

```sh
echo 1 > /sys/module/drv_tapi_dxs/parameters/event_trace
rmmod en75xx_slic_dxs &&
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" \
    bringup_mode=calibrate stage_wait_ms=3000
dmesg | grep -Ei 'en75xx|duslic|dxs|csi'
```

Expected on this Nokia: successful calibration of both channels, fresh
queries still reporting HW 0xb/FW 0x02010000/two FXS, followed by
`DXS calibration test passed`. Preserve the complete event log if it fails,
including the pre-cleanup version query. Resolve failures before voice.
After success, validate Nokia PCM frequency, edges and offsets before enabling
voice; do not inherit MitraStar audio settings.

## Validation

Three host tests passed using compiled real C snippets with mocked I/O/time.
They cover existing one/two-channel calibration, stopping before voice,
calibrate without PCM settings, BBD/topology/wait validation, calibration
errors, return to ROM and post-calibration query timeouts. Voice retains its
PCM validation and error propagation. Patch application and whitespace were
checked. No OpenWrt/MIPS cross-build or hardware test of this mode was run.
