# ProSLIC port review — 2026-09-08

Base: commit `7a966fd`, fetched from the user's repository. Working branch:
`codex/proslic-lifecycle`. This review preserves the existing PCM/ZSI corrections
and changes the firmware loader and ProSLIC lifecycle. It does not implement
ISI or complete the VoIP port.

## Implemented corrections

| Finding | Change | Local evidence |
|---|---|---|
| The converter uses `binascii.crc32`, but the loader used an uncomplemented Linux accumulator | Match the converter's initial and final CRC complements | `tools/proslic-patch2fw.py:build_blob`, `src/en75xx_proslic_fw.c:proslic_fw_parse` |
| `n_psaddr=0` reached the last-element access | Reject before allocation and terminator lookup | `proslic_fw_parse` |
| Header chipset, revision and BOM were merely logged | Check requested identity, including the fallback file | `proslic_fw_parse` |
| Reset manually inverted the logical GPIO value | Use `in_reset` as the logical value; the descriptor supplies polarity | `si3219x_reset`, GPIO API in the available kernel |
| Changing `bom` implied support for other electrical topologies | Accept only `lcqc`, the patch used with the compiled LCCB configuration | `src/Makefile`, `vendor/proslic/custom/si3219x_LCCB_constants.c` |
| Initialization published per-device buffers in global patch symbols | Lock publication and consumption; clear symbols before unlocking | `en75xx_si3219x_api_init`, patch selection in `vendor/proslic/src/si3219x_intf.c` |
| Invalid PCM channels were detected after converter startup | Validate the bit offset before API initialization | `en75xx_si3219x_api_init` |
| LBCal failure only produced a warning | Abort initialization and run cleanup | `ProSLIC_LBCal`, `en75xx_si3219x_api_init` |
| Initialization errors and removal could leave the converter running | Attempt PCM stop and converter shutdown, request open linefeed and assert reset before freeing resources | `ProSLIC_PowerDownConverter` in `vendor/proslic/src/proslic.c` |
| Line registration failure leaked firmware buffers | Shared API/firmware cleanup | `en75xx_si3219x_probe` |
| Reboot/power-off had no SLIC callback | A shutdown callback cancels work and attempts converter shutdown | `en75xx_si3219x_shutdown` |
| Examples enabled Si32192 under SPI despite the ISI support matrix | Disable Si32192 nodes; reject Si32192/Si32193 compatibles before this driver's reset and SPI operations | `docs/07-gpl-sdk-crosscheck.md`, supplied `Airoha_Doc/VOIP/VOIP Reference Module En.pdf`, page 188 |

The LCQC patch name does not establish that the LCCB electrical configuration
matches a board. Electrical profile, chip revision and external circuitry must
still agree. The generic `silabs,si3219x` compatible remains a prototype;
renaming a Si32192 node does not implement ISI.

Power-down is a software attempt, not a guarantee of zero voltage. The vendor
routine itself transitions through FWD_OHT before OPEN and waits for VBAT to
discharge. Transport errors or external supplies can prevent shutdown. This
review includes no physical measurement. The probe rejection also cannot
control earlier parent-driver or bootloader activity.

## PCM, ZSI and interrupts

Timeslot, ZSI TX offset and MPI length corrections were already in the base
commit and were not reimplemented. Documentation now qualifies the absence of
`0xa8` from the exported table: it does not prove that the register does not
exist. Its purpose must be established before writing it.

| Family | PCM1 / PCM | PCM2 | Other peripheral | Provenance |
|---|---:|---:|---|---|
| EN751221 | 11 | 33 | — | User-supplied information |
| EN751627 / EN7528 | 11 | 32, PCM2 label still to confirm | — | The user's second entry repeated the PCM1 label |
| EN7523 | 27 | Not established here | I2S: 48 | User; PCM SPI 27 also recorded in the SDK DTS cross-check |

Hardware interrupt numbers are not Linux dynamically allocated virtual IRQs.
I2S 48 is not PCM2. MIPS interrupt-controller mapping still needs checking
against the target kernel's DTS and irqchip.

## Validation performed

- `python3 tests/test_proslic_fw.py -v`: six tests passed. The actual C loader is
  compiled with host shims and UBSan. Tests accept converter output and reject
  mismatched identity, zero counts, truncation, CRC corruption and missing
  terminators.
- Objects for all five modules compiled against the local Linux
  `6.18.41-g8f909dac6c5a` x86-64 configuration, including ProSLIC/vendor code and
  relocatable linking. This is not a MIPS/ARM cross-build.
- The complete `modules` target stopped at MODPOST because the kernel's
  `Module.symvers` is missing. That error was not bypassed; no board-loadable
  validated module set was produced.
- `git diff --check` passed.
- No module loading, MMIO access or electrical testing was performed.

## Remaining work

1. Implement ISI using BSP symbols/relocations and initialization flow while
   preserving family differences. ZSI protocol and configuration cannot simply
   be assumed to apply to ISI.
2. Separate digital diagnostics from power/ringing initialization. The generic
   path still performs `ProSLIC_Init`, calibration and line feeding; it is not
   passive identification.
3. Propagate transport failures throughout the API, including compound RAM
   operations, fixups and ringing work. Several return values remain ignored.
4. Review `/dev/en75xx-fxsN` lifetime during unbind with open files and serialize
   complete SLIC operations. This review's mutex protects firmware symbols
   during Init, not all these operations.
5. Validate profile/BOM, clocks, reset polarity/sharing and timing on the actual
   board before enabling FXS. DMA/IRQ/audio, protection behavior and physical
   shutdown still require testing.

Apply the delivered patch on a clean `7a966fd` checkout with
`git apply --check driver-changes.patch`, then `git apply driver-changes.patch`.
