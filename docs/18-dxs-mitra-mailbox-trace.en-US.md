# MitraStar: OEM comparison and DXS calibration tracing

## New evidence from the Mitra folder

Source: https://drive.google.com/drive/u/0/folders/1Kx1MO_VIQWr7DnYbmFSX9uI8CIbnR3yG

Files read: lib/modules/spi.ko, lib/modules/slic3_intel_latam_25_BR.ko,
userfs/bin/voip_loader, usr/script/voip_load.sh, lib_voip, lib_modules and
userfs/profile.cfg. Analysis was static; OEM executables were not run.

| File | SHA-256 |
|---|---|
| spi.ko, 57,852 bytes | 8ed3cf418fffedbbcb7f97ea73d58cbe148d060460338f9fdafb05421f1debed |
| slic3_intel_latam_25_BR.ko, 306,640 bytes | 602b6658e40c0cfec1518aa9c62c3256d5628871216b78beca79016b1fafb19f |

Both are relocatable, little-endian ARM ELF files with symbols. Instructions
and relocations were checked using pyelftools/Capstone.

### SPI/CSI wrapper

spi.ko directly confirms the decompiled STATUS sequence:

| Function | Evidence: offsets relative to .text |
|---|---|
| spi_wraper_write_cmd | 0x1858 calls regReadArm; 0x185c ORs with 2; 0x1868 branches to regWriteArm |
| spi_wraper_write_data | 0x18c4 calls regReadArm; 0x18c8 ORs with 2; 0x18d4 branches to regWriteArm |
| spi_wraper_read_data | 0x193c reads RXDATA; 0x1948 rereads STATUS; 0x194c ORs with 4; 0x1958 writes STATUS |

Accesses use OEM alias addresses 0xbfbd1004/1008/100c and a 0x2000 stride.
The port's physical resource remains 0x1fbd1000. The command byte arrives in
r1, despite the missing argument in Ghidra's spi_wraper_write_cmd prototype.
The strange indirect calls have arm_delay_ops relocations, not logger calls.
This resolves the earlier limitation that STATUS acknowledgment was verified
only against decompiled output. It does not establish the calibration root cause.

In the BR Intel module, DXS_RegWriteMulti() branches to .text+0x222d4. CSI
selects a six-byte frame, subtracts the two-byte header, and sends four payload
bytes per fragment. Mailbox DWORDs are serialized MSB first.
DXS_RegReadMulti() branches to .text+0x21fd0: CSI requests ten bytes and shifts
the useful data start by one byte. These characteristics match the port.

DXS_ALM_Calibration() passes 0x1f4 (500) to IFXOS_EventWait at .text+0x211ac.
The approximately 500 ms timeout was not introduced by this port.

### BBD, firmware and SLIC selection

The BR ELF contains byte-identical copies of:

| Object | ELF file offset | Bytes | SHA-256 |
|---|---|---|---|
| Previously recovered BBD | 0x281b8 | 498 | e0c84932582aa8a2931c59d910896f9b1184d189f59af1574ef8fc586354261d |
| Previously recovered Mitra firmware | 0x2d2bc | 20,168 | 779afa0255e079319ccb264acba714816505c7bcda60c73b967b0c2259dae824 |

These are file offsets, not memory addresses for devmem. The recent test uses
firmware reporting 2.1.1; the identity above describes the previously recovered
OEM Mitra firmware. This patch does not replace firmware.

profile.cfg enables PI_AUTO_DETECT_SLIC. voip_load.sh calls voip_loader and
contains a conditional VOIP_LOADER_SUCCESS=yes block with a le9642/ZSI fallback.
voip_loader contains PEF32001/CSI/mode=IB command formats and Intel normal,
LATAM and BR module names. The fallback does not establish which SLIC is
installed, and having all three modules does not prove which one was loaded.
Do not copy Zarlink fallback settings into a PEF32001 board DTS.

## What the patch observes

The latest log passes DEV_START and BBD blocks, with a valid version query
before calibration. After opmode=6, polling continues but outbox events stop;
a subsequent version query times out. HOST registers remain readable. This
does not determine whether DXS accepted the command.

This diagnostic patch is incremental on the previous corrections, including
airoha-voip-en7523-csi-status-ack.patch. It changes no firmware, BBD, DTS,
clocks, timeouts, line mode or calibration success criteria.

1. drv_tapi_dxs gains mailbox_trace, off by default. The exact channel-0
   request header=0x01000704/payload=0x00060000 arms up to 32 DATA/LEN/CFG
   fragment records. Firmware and repeated INT1 polls are not dumped.
2. Writes save actual prepared bytes for the first two fragments and print
   only after the logical write ends. Calibration has exactly two fragments.
   Reads dump raw bytes before removing the leading CSI byte. Failed RX
   buffers are never dumped.
3. en75xx_isi_spi gains csi_trace, off by default. For the pair below it caches
   header completion and prints only after the payload. It reports completed
   bytes, return code, last TX polling STATUS and last written ACK. No extra
   MMIO reads are performed for this trace.
4. The probe message identifies read/OR/write versus mask-only ACK, making
   the updated controller distinguishable from an older loaded module.

Expected frames:

```text
7e 14 01 00 07 04   command header
7e 14 00 06 00 00   calibration payload
```

Controller tracing matches complete frames. Different backend bytes may prevent
CSI calibration lines from appearing; their absence alone does not prove the
controller was not called.

A done=6/6 and ret=0 means the wrapper signaled completion for all six bytes.
It does not prove DXS reception, decoding or execution. A suitable physical
CSI capture is still needed to establish bus signaling. This filter targets
the Mitra's channel 0, not every channel or SLIC.

## Apply

From the repository root, after the previous patches:

```sh
git apply --check /path/airoha-voip-dxs-mitra-mailbox-trace.patch
git apply /path/airoha-voip-dxs-mitra-mailbox-trace.patch
python3 -m unittest discover -s tests -p 'test_*trace.py' -v
python3 -m unittest discover -s tests -p test_csi_status_ack.py -v
```

Rebuild and install the packages providing **drv_tapi_dxs.ko and
en75xx_isi_spi.ko** for the device's exact kernel. Updating only
en75xx-slic-dxs.ko does not install this diagnostic. Reboot after installation
to ensure the new controller is running.

## Board test

Keep DTS, firmware and BBD identical to the last test. After boot:

```sh
modprobe drv_tapi_dxs
cat /sys/module/en75xx_isi_spi/parameters/csi_trace
cat /sys/module/drv_tapi_dxs/parameters/mailbox_trace

echo 1 > /sys/module/en75xx_isi_spi/parameters/csi_trace
echo 1 > /sys/module/drv_tapi_dxs/parameters/mailbox_trace
echo 1 > /sys/module/drv_tapi_dxs/parameters/event_trace

# Remove the adapter if loaded. On first load, rmmod reporting
# that it is not loaded is expected.
rmmod en75xx_slic_dxs
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" bringup_mode=voice

dmesg > /tmp/dxs-mailbox-trace.log
dmesg | tail -n 240
```

A missing parameter means the corresponding module lacks this change. The
updated EN7523/CSI controller also prints at boot:
`CSI SPI controller ready (status ACK read/OR/write, calibration trace available)`.

Disable the dumps without unloading modules:

```sh
echo 0 > /sys/module/en75xx_isi_spi/parameters/csi_trace
echo 0 > /sys/module/drv_tapi_dxs/parameters/mailbox_trace
echo 0 > /sys/module/drv_tapi_dxs/parameters/event_trace
```

## Interpretation and validation

- Unexpected TX: inspect serialization, header and fragmentation.
- done below six or nonzero ret: inspect wrapper completion.
- Correct frames and done=6/6 without an event: the wrapper send stage passes,
  but this still does not separate CSI reception from firmware/calibration failure.
- New BOOT_FIN: correlate with a fresh version query; cached_fw is not proof
  firmware remains running.

Four local tests passed, compiling actual extracted C functions with
-Wall -Wextra -Werror. Coverage includes CSI/SPI frames, padding, raw RX with
its leading byte, failures, bounded logging, invalid device index, disabled
tracing, partial completion, fresh STATUS reads, and output after the payload
outside the spinlock. No complete OpenWrt module build or hardware test was
performed. Tracing adds execution/console time and should be disabled outside
diagnostic runs.
