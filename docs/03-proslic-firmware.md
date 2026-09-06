# ProSLIC patches as firmware

The Skyworks API ships its DSP patches as C files that get compiled into
the driver. That has two costs: every chipset, revision and BOM variant
is linked in whether the board uses it or not, and supporting a new
variant means rebuilding the module. `tools/proslic-patch2fw.py` turns
those C files into blobs that `request_firmware()` loads at probe.

## Where the patches came from

All branches of the ProSLIC API repository, 48 source files in total.
They overlap heavily -- the same patch appears in up to eight files --
so the converter groups by payload and writes one blob per distinct
identity. `firmware/proslic/INDEX` maps each blob back to the API symbol
names and the source files it was found in.

Result: **19 blobs** covering Si3217x rev B and C, Si3218x rev A,
Si3219x rev A, Si3226x rev C and Si3228x rev A, in the LCQC, FB, BB, TSS
and TSS_ISO variants, including older serials kept alongside the current
ones.

## Two things the conversion turned up

**The Si3219x rev A patch is byte-identical to the Si3218x rev A patch.**
Not a bug in the tool: the Silicon Labs file header says the Si3219x
patch was generated from `si3218x_patch_A_2017MAY25.dsp_prom`. Both
names are still written out, since the driver asks for the one matching
its chipset.

**Some patch objects share a payload across revisions and BOMs.** In
`si3217x_patch_C_FB_2014JUN18.c`, `si3217xPatchRevCFlbk` and
`si3217xPatchRevBBkbt` are both initialised from the same arrays, so
rev C flyback and rev B buck-boost get the same image in that drop.
Likewise `si3226x_1_patch_C_FB_2014JUN18.c` matches the TSS patch word
for word. The converter writes a file per identity rather than picking
one, because the API asks by symbol name.

## Container format

Field order follows the draft header in the `proslic_drivers2` branch --
serial, then the four sizes, then `patchData`, `psRamData`, `psRamAddr`,
`patchEntries`. What is added is what a loader needs to refuse a bad
file: a magic, a version, a CRC32, and the chipset/revision/BOM the
patch belongs to.

```
offset  size  field
0       8     "PROSLICP"
8       2     version (2)
10      2     header size (56)
12      4     patchSerial
16      2     patchData words, terminator included
18      2     psRamData words
20      2     psRamAddr words, terminator included
22      2     real jump-table entries: 8 or 16
24      4     CRC32 of the payload
28      8     chipset, e.g. "si3219x"
36      4     revision, e.g. "A"
40      12    BOM, e.g. "LCQC"
52      4     reserved
56      -     payload
```

Everything is little-endian so one set of files works on both the ARM
and the big-endian MIPS parts; the loader converts into freshly
allocated native-endian arrays rather than pointing at the blob.

Two details that would bite otherwise:

- `ProSLIC_LoadPatchData()` walks `patchData` until it hits a zero, and
  `ProSLIC_LoadSupportRAM()` walks `psRamAddr` the same way. The
  terminators are preserved and the loader verifies they survived.
- Si3217x rev B ships only the 8 low jump-table entries, but the API
  indexes `patchEntries[8]` unconditionally. The payload is padded to 16
  and the real count is kept in the header.

## How the driver gets the patch

The API resolves patches through fixed symbols -- `SI3219X_PATCH_A` maps
to `si3219xPatchRevALCQC`, `SI3219X_PATCH_A_DEFAULT` to `RevAPatch`.
`en75xx_proslic_fw.c` defines those symbols empty and fills them from
firmware before `ProSLIC_Init()` runs. They are deliberately not `const`:
a const definition would land in `.rodata`, where it could not be
populated at probe.

Lookup order:

```
en75xx/proslic/<chipset>_<rev>_<bom>.fw
en75xx/proslic/<chipset>_<rev>.fw
```

The BOM cannot be detected -- the chip has no way to report how its
DC-DC converter was wired -- so it comes from the `bom` module
parameter, default `lcqc`. When a chipset+revision has exactly one
patch, the converter also publishes it under the plain name so the
fallback works without knowing the BOM.

## Regenerating

```
tools/proslic-patch2fw.py -o firmware/proslic path/to/patch_files/*.c
tools/proslic-patch2fw.py --dump firmware/proslic/si3219x_a_lcqc.fw
```

`--dump` prints the header and verifies the CRC, which is the quick
check that a blob survived a build system or a flash write.
