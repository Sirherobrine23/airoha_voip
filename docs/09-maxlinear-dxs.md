# MaxLinear DUSLIC-XS: what the GPL drops give us

The status table has listed the PEF32001/PEF32002 (DXS101/DXS102) as
"not started; firmware blobs identified" since the beginning. The
XX532v drop moves that on. It does not hand over the source, but it
narrows the work from "reverse engineer a SLIC family" to "obtain three
named upstream packages and write the glue".

## What Airoha ships, and what it does not

`mod-slic3/src/intel/` contains exactly two files, the same in every
drop that has it:

```
intel/Makefile_i.release
intel/readme.txt
```

and the readme says why:

```
Current SLIC SDK version(confirm with SLIC vendor MaxLinear):
  lib_ifxos-1.6.6
  drv_tapi-4.16.3.0
  drv_dxs-1.3.1.0

For Lantiq SLIC solution, please follow the steps:
1. Get SLIC SDK from SLIC vendor;
2. Copy lib_ifxos-1.6.6 to the mod-slic3/src/intel/
3. Copy drv_tapi-4.16.3.0 to the mod-slic3/src/intel/
4. Copy drv_dxs-1.3.1.0 to mod-slic3/src/intel/
5. Rename Makefile_i.release to be Makefile_i
6. Make target voip_module and ouput slic_intel.ko in dir mod-slic3
```

That is the same arrangement as the Microchip and Skyworks paths: Airoha
provides the integration, the SLIC vendor provides the driver. The
difference is that for those two the vendor SDK also ended up in the
drop, and for MaxLinear it did not.

`lib_ifxos` and `drv_tapi` are Lantiq/Intel packages that have been
distributed under the GPL for years, and the exact versions are now
known. `drv_dxs` is the DUSLIC-XS low-level driver and is the one to
chase.

## The built module is not stripped

`voip_module/ko/slic3_intel.ko` is a 433 KB aarch64 object with its
symbol table intact, statically linking all three packages. It carries
133 distinct `DXS_*` symbols, the source file names they came from, and
the `configure` line each package was built with.

The `drv_dxs-1.3.1.0` source layout, from the retained paths:

```
drv_dxs_access.c    drv_dxs_alm.c       drv_dxs_alm_aclm.c
drv_dxs_alm_lt.c    drv_dxs_bbd.c       drv_dxs_debug.c
drv_dxs_dtmf.c      drv_dxs_dtmf_sm.c   drv_dxs_dwld.c
drv_dxs_gpio.c      drv_dxs_init.c      drv_dxs_ioctl.c
drv_dxs_mbx.c       drv_dxs_pcm.c       drv_dxs_tg.c
```

The chip access layer is small and its shape is legible from the names
alone:

```
DXS_ChipAccessInit  DXS_AccessTest
DXS_RegRead         DXS_RegWrite
DXS_RegReadMulti    DXS_RegWriteMulti
DXS_CmdRead         DXS_CmdWrite
DXS_ObxRead         DXS_WaitForCmdMbxData
DXS_TAPI_LL_GetCmdMbxSize
```

A register interface plus a command mailbox, which is what the
`/proc/spi/spi_ctrl_test` documentation in the VoIP reference manual
already implied: MaxLinear parts read with control byte `0xbe` and write
with `0x7e`, against `0x60`/`0x20` for Skyworks and register parity for
Microchip.

## Firmware download

This project already carries `firmware/dxs/DXS_FW.bin` and
`DXS_BBD.bin`, recovered from a Nokia G-240G-E, with a note that the
PRAM patch is optional and the BBD is not. The module confirms the
shape of both paths:

```
DXS_DwldFirmwareSelect   DXS_DwldAndStartFW   DXS_DwldAndStartFW_Ext
DXS_DwldPatch            DXS_FW_Start
DXS_BBD_Download         DXS_BBD_DcDcStringTranslate
bbd_check_integrity      bbd_get_block
dxs_bbd_DcBasicCfg_Ext   dxs_bbd_RingCfg_Ext
```

and the BBD container's magic, `!bbdDXS!`, appears as a literal. The
`bbd_get_block` / `bbd_check_integrity` pair says the BBD is a
block-structured file with a checksum, and `DcBasicCfg` and `RingCfg`
are the two blocks that matter: DC feed and ringing, exactly the two
things the Microchip path gets from its DC and ring profiles.

## The converter topology again

`drv_dxs` was configured with

```
--enable-dcdc-hw=CIBB12
```

That is the same class of decision as `airoha,slic-power-type` on the
Le9642: a build-time statement about the DC-DC converter the board put
around the SLIC. `SLIC_POWER_CIBB` is one of the four topologies
`slic3_main.c` knows about, alongside BB, IB and TB.

Whoever implements this family should assume from the start that the
topology is a required per-board input and not a default, for the same
reason it is required on the Le9642: it programs a real power converter.

The full `configure` lines are worth keeping, because they say which
features the shipped module was built with:

```
drv_tapi: --enable-pcm --disable-dect --disable-kpi --disable-qos
          --disable-fxo --enable-metering --enable-nlt
          --enable-cont-measurement --enable-tapi4 --enable-trace

drv_dxs:  --with-max-devices=2 --enable-lt --enable-metering
          --enable-gpio --enable-direct-chip-access --enable-proc
          --enable-cap-measurement --enable-power-save
          --enable-dcdc-hw=CIBB12 --enable-interrupts --disable-cid
          --enable-cont-measurement --enable-trace
```

`--disable-dect` and `--disable-fxo` are worth noting: this build is FXS
only, with no DECT and no FXO.

## Suggested order of work

1. Obtain `lib_ifxos-1.6.6`, `drv_tapi-4.16.3.0` and `drv_dxs-1.3.1.0`.
   The first two circulate publicly; the third is the question mark.
2. If `drv_dxs` cannot be obtained, the unstripped module is a
   serviceable specification for the access layer and the BBD format,
   but the result would be a clean-room driver rather than a port, and
   that is a much larger piece of work than the Le9642 was.
3. Either way the PCM side is already done: the DUSLIC-XS is a
   `SPI/PCM & CSI` part in Airoha's matrix, and CSI is described in the
   reference manual as "similar to ZSI", needing a SLIC reset. The
   existing `en75xx_zsi.c` is the closest thing to a starting point.

## What is still missing

No drop surveyed so far contains `drv_dxs` source, an ISI transport, or
the `spi.ko` wrapper source that would explain ZSI, ISI and CSI framing
from the SoC side. Those three remain the gaps.
