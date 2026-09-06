# The SLIC shares the boot flash's SPI controller

`0x1fbc0000` is not a SLIC peripheral. It is `CR_SPI_BASE`, the SPI
master on the MIPS parts, and its primary job is the boot flash.

**This is a gen1 story.** On EN7523 and later the SPI controller moved:
the vendor device tree has `spi_controller@1fa10000`
(`econet,ecnt-spi_ctrl`), with NFI2SPI at `0x1fa11000` and the NAND ECC
block at `0x1fa12000`. There is no `0x1fbc0000` node in those trees at
all. The sharing-with-flash problem is the same in shape -- one master,
flash plus SLIC on different chip selects -- but the address is not.

```
boot/bootrom/bootram/include/asm/tc3162.h:  #define CR_SPI_BASE  0xBFBC0000
linux/arch/mips/include/asm/tc3162/tc3162.h: #define CR_SPI_BASE  0xBFBC0000
boot/uboot/u-boot-airoha/arch/arm/include/asm/tc3162.h: 0x1fbC0000
```

The ProSLIC and DUSLIC-XS parts hang off that same master on a different
chip select. That is why the vendor SLIC module programs a chip-select
field in register `0x28` rather than toggling a GPIO.

## The proof

`spiflash_tc3162.c` exports a semaphore whose comment says exactly what
it is for:

```c
DEFINE_SEMAPHORE(SPI_SEM);  // Make sure all related SPI operations are atomic
EXPORT_SYMBOL(SPI_SEM);
```

and the vendor SLIC module imports it. `readelf` on
`spi_si32192.ko` lists `SPI_SEM`, `down` and `up` among its undefined
symbols. Two drivers, one lock, one controller.

The flash driver also saves and restores `reg0x28` (`SPI_FLASH_MM`)
around every transaction, because the SLIC path changes the clock
divider in bits 27:16 and the chip select in bits 31:29 and leaves them
changed:

```c
reg0x28 &= 0xf000ffff;      /* keep 31:28, clear the divider */
reg0x28 |= (0x5 << 16);
*((__u32 *)(CR_SPI_BASE | SPI_FLASH_MM)) = reg0x28;
```

## Register map, corrected

An earlier reading of the vendor SLIC module in this project had `0x04`
as a data register. It is not:

| offset | flash driver name  | meaning                                          |
| ------ | ------------------ | ------------------------------------------------ |
| 0x00   | `SPI_FLASH_CTL`    | bit 8 = start, bit 16 = busy, 7:0 = tx/rx counts |
| 0x04   | `SPI_FLASH_OPCODE` | opcode, 8 bits                                   |
| 0x08   | `SPI_FLASH_DATA`   | data                                             |
| 0x28   | `SPI_FLASH_MM`     | chip select 31:29, clock divider 27:16           |

Only `0x00` and `0x28` were read correctly the first time. Any standalone
"SIF" driver built on the earlier reading is wrong twice over: wrong
offsets, and wrong to exist at all.

## What this means for this tree

**There is no `sif@1fbc0000` node.** It was in the device tree
fragments briefly and has been removed. A platform driver claiming that
address would take the boot flash controller away from MTD, or race with
it — on the device you are booting from.

**The ProSLIC is a plain `spi_device`.** `en75xx_slic_si3219x.c` is a
`spi_driver`, so it sits under whatever Linux driver owns the
controller, on its own chip select, and the SPI core's bus lock provides
the serialisation that `SPI_SEM` provided in the vendor stack. That is
the whole reason to go through the SPI subsystem rather than poking
registers: the locking is not optional and it is not ours to invent.

**On EN7523 the SLIC does not use the SPI controller at all.** The
vendor PCM module reaches it through wrappers inside the PCM window, at
`+0x1010` and `+0x4098` of `0x1fbd0000`. So on gen2 the flash and the
voice path are separate by construction.

**Le9642 boards are unaffected.** ZSI does not use this controller at
all — the SLIC control channel rides the PCM bus through the wrapper at
`0x1fbd1000`. On a Le9642 board the flash controller and the voice path
never touch.

## The operational hazard: flash erase versus voice

The vendor flash driver carries this, guarded by `TCSUPPORT_VOIP`:

```c
/* #11542: For voice affected by Flash action issue */
```

and behind it a whole kthread (`spiflash_wait_erase_ready`) plus a timer
that defers and chunks erase operations. The reason is structural: a
sector erase holds the controller for tens of milliseconds, the SLIC
cannot be reached while it is held, and 8 kHz audio does not tolerate
that.

So on a ProSLIC board, a firmware upgrade or any `mtd write` during a
call will disturb the audio, and nothing in this tree prevents it. The
mitigations, in order of preference:

1. Do not write flash during calls — config saves included.
2. If you must, chunk erases and yield between them, which is what the
   vendor kthread does.
3. Hook `SLIC_LF_OPEN` on the affected lines around long flash
   operations so the user gets silence rather than mangled audio.

None of this applies to the Le9642 path.

## Why the shared controller is easy to miss

The vendor SLIC module reads plausibly as a standalone serial engine:
it has its own probe, its own register accessors, its own chip-select
handling, and its own framing for three SLIC vendors. Nothing inside it
says "this belongs to the flash driver" except one imported symbol.
Checking the undefined-symbol list is what settles it, and it is worth
doing routinely on any vendor `.ko` that seems to own a peripheral —
`readelf -sW module.ko | awk '$7=="UND"{print $8}'`.
