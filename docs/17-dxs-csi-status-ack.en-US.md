# Additional comparison: spi.ko.c and slic3.ko.c

## Scope and evidence

Static analysis of the attachments; no hardware test at this stage. This patch
aligns the wrapper sequence with OEM decompiled output. It remains experimental
on hardware and is not a confirmed fix for the calibration failure.

| File | SHA-256 | Finding |
|---|---|---|
| spi.ko.c | 718c1c5067fa4c7b5cc5f9b8a591034433df1ff77dba2bd55eed208db7729ade | Identical to the previously compared mitra-spi.ko.c |
| slic3.ko.c | 8526ff84791b0e284a562d205b69b495be7ba874b70a7b23d94e9075e7ab0bb0 | Zarlink/Microchip VoicePath driver, not slic3_intel |

Source references below are function names present in the attachments. The
matching spi.ko ELF is unavailable in this batch, so instructions and relocations
were not checked. Incomplete prototypes, extraout_* variables and apparent
_GET_Log_moudle_switch calls may be Ghidra artifacts, not compilable source.

## Shared transport

spi_wraper_cfgtime_cmd() sets CFG to
`(old & 0xe0ffffef) | 0x0f000010`. CSI_bytes_read()/CSI_bytes_write() call
this preparation once before the two header-send calls and the data loop.
The current controller already prepares CFG once per SPI message, avoiding
reconfiguration between the TX header and RX data.

For wrapper physical base 0x1fbd1000, with a 0x2000 stride per instance:

| Offset | Observed role | OEM operation |
|---|---|---|
| +0x00 | CFG | CSI transaction configuration |
| +0x04 | TXDATA | Byte transmission |
| +0x08 | STATUS | Bit 0 starts RX; wait for bit 1 TX or bit 2 RX |
| +0x0c | RXDATA | Received byte |

spi_wraper_write_cmd()/spi_wraper_write_data() wait for `STATUS & 2`,
**read STATUS again** and write `status | 2`. spi_wraper_read_data() starts
with `status | 1`, waits for `STATUS & 4`, reads RXDATA, **reads STATUS again**
and writes `status | 4`.

Previously the port wrote only 2 or 4 when acknowledging completion. This patch
adds the fresh read and OR for TX/RX acknowledgments only on
`airoha,en7523-csi-spi`. Other compatibles retain their existing sequence.
Polling time limits remain; OEM infinite loops are not copied.

This difference could matter for control bits sharing STATUS, but the complete
register semantics have not been established from documentation. Read/OR/write
also writes back other active status bits. Matching OEM behavior does not
establish the root cause of the calibration failure.

## slic3.ko.c contents

init_module() selects SPI=0, ZSI=1, ISI=2 and CSI=3. This shared enumeration
does not establish that every listed chip supports every interface.
get_slic_type() recognizes zl88601, zl88801, le89116, le89156, le89316,
le9641, le9651, le9652, le9622, le9642 and le9662.
slic_init_device() uses VpMakeDeviceObject(), VpMakeLineObject(), VpInitDevice()
and VpSetOption(). Its SLICMGR routines help recover hook, ring and line-state
integration for a future Zarlink backend.

No DXS, PEF or Intel identifiers were found in this file. Its VoicePath profiles
and power selections are not PEF32001 BBD/DC/DC settings. It provides no new
DXS calibration sequence.

## Current failure

In the latest log, polling and INT1-read counters keep advancing during the
calibration wait, while the outbox delivers no new events. CFG/IEN1/IEN2/LEN/BINF
remain readable. This argues against a stopped callback in that interval but
does not prove firmware received or processed the ALM command. cached_fw is
not a live firmware query. Unlike earlier runs, the latest test did not show
a new BOOT_FIN event.

Status 0x706d remains a calibration wait timeout, wrapped as 0x7055 by BBD
loading. Do not force success, skip calibration or change the DC/DC profile
to work around that error.

## Apply and test

Incremental patch against the tree containing the ISI/CSI controller and the
previous corrections. From the repository root:

```sh
git apply --check /path/airoha-voip-en7523-csi-status-ack.patch
git apply /path/airoha-voip-en7523-csi-status-ack.patch
python3 -m unittest discover -s tests -p test_csi_status_ack.py -v
```

Rebuild the package providing en75xx_isi_spi for the same kernel and target,
install it and reboot to ensure the new controller is loaded. Updating only
en75xx-slic-dxs.ko does not install this change. Keep firmware, BBD and DTS
unchanged for the comparison.

Start with the firmware test after boot:

```sh
modprobe drv_tapi_dxs
echo 1 > /sys/module/drv_tapi_dxs/parameters/event_trace
# If the adapter is already loaded, remove it before the following insmod.
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" bringup_mode=firmware
```

If it passes, remove the adapter and repeat the existing voice test:

```sh
rmmod en75xx_slic_dxs &&
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" bringup_mode=voice
dmesg | tail -n 200
```

Success requires completed calibration and FXS registration, not merely a
loaded module or absence of Unknown symbol errors. If the timeout persists,
compare the new events and existing observer snapshots. This analysis provides
no demonstrated reason to increase the timeout.

Local validation compiled the actual extracted C byte functions with
-Wall -Wextra -Werror against an MMIO model. Checks cover TX/RX, a fresh STATUS
read after RXDATA, timeout without success acknowledgment and quirk scope.
This is not an OpenWrt module build or electrical/functional hardware validation.
