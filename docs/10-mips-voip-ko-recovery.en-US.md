# Verifiable recovery of the MIPS VoIP modules

## Scope and input material

The SDK preserves unstripped modules for the
`UNION_EN7528_LE_7592_7613_MAP_R2_demo` profile. They are little-endian ELF32
MIPS32r2 modules built for Linux 3.18.21 SMP. There is no DWARF, but the files
retain `.symtab`, `.strtab`, relocations, `.pdr`, `.reginfo`, local names and
function sizes.

The proprietary modules are not copied into this project. Instead,
`tools/voip-ko-recovery.py` records hashes, ABI metadata, symbols, relocations
and per-function hashes. Recovered C functions can therefore be replaced and
measured incrementally without treating Ghidra output as the acceptance test.

| Module       | Functions | Imports | Exports | Relocations |
| ------------ | --------: | ------: | ------: | ----------: |
| `pcm1.ko`    |        58 |      46 |       6 |       1,696 |
| `pcm2.ko`    |        57 |      48 |       0 |       1,665 |
| `pcmDump.ko` |        37 |      33 |       0 |         596 |
| `spi.ko`     |        68 |      27 |      10 |       2,060 |
| `sys_mod.ko` |        46 |      19 |      38 |         346 |
| `slic3.ko`   |       424 |      23 |       1 |      12,455 |
| `fxs3.ko`    |       701 |      95 |      93 |      18,450 |

## Recovered ABI

`pcm1.ko` exports `getPcmConfig`, `pcmChipScuQuery`,
`pcmSlicResetRegQuery`, `pcm_reinit_slic`, `pcm_setFreeRunMode` and
`getGponUpStatus`. `spi.ko` imports both SCU/reset query functions, proving
that SLIC bus setup depends on PCM and must not keep a second address table.

`spi.ko` exports `SPI_cfg`, `SPI_Reset`, `pSLIC_Reset`, byte read/write
operations, ZTE variants, chip-select GPIO setup, SLIC validation and the
ZSI/CSI clock test. Its internal symbols confirm four separate transports:
SPI, ZSI, ISI and CSI. Dedicated `ZSI_bytes_*`, `ISI_bytes_*` and
`CSI_bytes_*` pairs mean ISI must not be implemented as a blind ZSI alias.
Separate Zarlink, Silicon Labs and Lantiq/MaxLinear SPI paths also exist.

The Ghidra C can now be constrained by the original module's symbols and
relocations. That cross-check places the serial wrapper at
`0x1fbd1000 + id*0x2000`, with TX/RX at `+0x04`, control/status at `+0x08` and
RX at `+0x0c`. The recovered framing is:

| Transport | Prefix                | Read operation                          | explicit delay in the old module |
| --------- | --------------------- | --------------------------------------- | -------------------------------- |
| ISI       | `control`, `register` | read bytes after both commands          | none; polling uses 2 us          |
| ZSI       | `register`            | additionally send `0x06` before reading | 10 us after every byte           |
| CSI       | `control`, `register` | read after both commands                | 10 us after every byte           |

The legacy mode register at `0x1fb00094` uses low nibble `0xf` for ISI and
`0x5` for ZSI/CSI. This confirms that ISI can be exposed as a virtual
`spi_controller` to the ProSLIC driver: a three-byte SPI write becomes
`control, register, data`, and `spi_write_then_read(control, register)` matches
the ISI sequence. The old module's 10-us ZSI delay does not override the longer
gap measured on live hardware; timing remains transport-specific.

`sys_mod.ko` is mostly an OS abstraction for mutexes, tasks, lists and logging.
It should not be ported literally; current kernel primitives should replace it.

## PCM1 versus PCM2

The modules share 57 named functions. Eighteen have identical function bytes;
39 differ, although most keep the same size. This is consistent with a common
source built with different constants, globals and initialization paths.

`pcm2.ko` imports `pcm_reinit_slic` and `pcm_setFreeRunMode` from PCM1 and does
not export an independent ABI. `getGponUpStatus` exists only in PCM1. The new
implementation should remain one driver class parameterized by SoC/DT
resources rather than two duplicated drivers.

## Registers confirmed by the MIPS headers

The SDK `pcmdriver.h` confirms the following legacy-generation layout:

| Item                            | PCM1         | PCM2         |
| ------------------------------- | ------------ | ------------ |
| legacy virtual base             | `0xbfbd0000` | `0xbfbd2000` |
| physical base                   | `0x1fbd0000` | `0x1fbd2000` |
| principal registers             | 17           | 17           |
| TX/RX descriptors               | 15 / 15      | 15 / 15      |
| channels/buffers per descriptor | 8            | 8            |
| legacy reset mask               | `0x800`      | `0x10`       |

It also confirms `IMR=0x28`, TX/RX polling at `0x2c/0x30`, DMA control at
`0x40`, ring configuration at `0x3c`, and the interrupt bits already used by
the new driver.

The header uses `PCM_INT=12` and `PCM2=34` with `CONFIG_MIPS_TC3262`. These
must not be copied directly into DT because they may be legacy Linux IRQ
numbers after the cascaded INTC offset. Previous evidence names hwirq 11/32,
while a live EN7528 boot reported mapped `irq=34`. Keep IRQ resources in DT and
log both hwirq and virq during bring-up.

## Verifier usage

```sh
tools/voip-ko-recovery.py inventory out \
    /path/pcm1.ko /path/pcm2.ko /path/spi.ko

tools/voip-ko-recovery.py compare out/reference.json \
    pcm1.ko /path/rebuilt-pcm1.ko
```

Exact final-module reproduction additionally needs the original Linux 3.18.21
tree, configuration, `Module.symvers`, toolchain and `modpost`. Until those are
available, per-function hashes provide an incremental criterion. Binary
equality does not replace hardware testing, especially for reset, line feed,
ringing and the SLIC DC/DC converter.

## Next targets

1. recover `pcmConfigSetup`, `descInit`, `rxDescSet` and `pcmSend`;
2. recover all four transport pairs from `spi.ko`;
3. map recovered behavior into `en75xx_pcm.c` and `en75xx_zsi.c`;
4. cross-check the XC220-G3v test's `en75xx-isi-spi` against the recovered
   framing and enable Si32192/Si32193 only below that controller;
5. use `pcmDump.ko` only as a reference for RX/TX taps, not as a production
   dependency.
