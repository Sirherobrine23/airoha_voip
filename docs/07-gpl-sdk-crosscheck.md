# Cross-check against the Airoha LTS SDK

Everything before this document was reverse engineered from vendor
binaries and from a running stock device. This one is different: TP-Link
published the GPL sources for the VB430, and the drop contains Airoha's
LTS SDK 7.3.283 with the VoIP BSP inside it. That BSP ships the vendor's
own SLIC integration as C source, and the PCM module as an **unstripped**
kernel object. Both can be read directly.

```
VB430_GPL_ITWIND3.tar.gz
└── VB430/bba_3_0_platform/sdk/an7551/
    ├── Airoha_Doc/VOIP/VOIP Reference Module En.pdf
    ├── airoha_feeds/target/linux/airoha/files/
    │   └── arch/{arm,arm64}/mach-econet/ecnt_pcm.c
    ├── openwrt-21.02/openwrt-21.02.1_dev/feeds/airoha/
    │   └── target/linux/airoha/files/arch/arm/boot/dts/en7523.dtsi
    └── tclinux_phoenix/release_bsp/<profile>/BSP/voip_bsp/voip_module/
        ├── DSP/MTK/mod-slic3/src/zarlink/    Microchip integration
        ├── DSP/MTK/mod-slic3/src/silab/      Skyworks integration
        └── ko/pcm1.ko                        not stripped
```

The SDK targets AN7551/AN7581, which are newer than the parts this
driver supports. The register table inside `pcm1.ko` still carries the
MIPS `0xbfbd0000` addresses, and the SLIC integration branches on
`isEN7523` and friends, so the lineage is continuous and the evidence
carries back.

## The PCM register map is now first-hand

`pcm1.ko` is not stripped, so its `regMap` symbol can be read straight
out of `.data`. Each entry is a name pointer, a flags word, a
writable-bit mask, an address, and a reset value. The full table is
quoted in `src/en75xx_pcm_regs.h`.

Every offset the driver already used is confirmed. Three things were
wrong:

**The exported register table does not list 0xa8.** The EN7523 start-up path wrote `0xa0`
there. The table has `txRxDMA` at 0x40 and `txRxChanEnable` at 0xac and
no intervening entry. This alone does not prove the hardware lacks other
registers; the write was removed because its meaning is unverified.

**The interrupt registers are eleven bits wide.** `ISR` and `INTMask`
both carry a writable mask of `0x000007ff`. The "OEM interrupt mask
0x5828" an earlier revision programmed sets bits 11, 12 and 14, which do
not exist, and the hook-status interrupt bits it claimed were at 11..16
are not there either. Both definitions were removed. `EN75XX_PCM_INT_ALL`
was already inside the valid range and is unchanged.

**The timeslot field is a bit offset, not a slot index.** The reset
values settle it: channel 0 resets to 0, channel 1 to 8, channel 2 to 16,
and so on to channel 31 at 248 — that is, channel *n* starts at bit
*n*·8. A byte timeslot *s* therefore lives at bit offset *s*·8. This is
what makes the slot-to-channel mapping a property of the configured
table rather than a formula (below).

One earlier finding is confirmed rather than corrected: `txrxRingSizeAndOff`
at 0x3c really does reset to `0xc0`, which is why the driver must program
it and why the reset value leaves the RX ownership bit stuck.

## The slot-to-channel mapping was inconsistent

An earlier revision mapped a bus timeslot to a DMA channel with
`channel = slot - 4`, so bus slots 4 and 6 became channels 0 and 2, and
the two-line examples asked for a DMA channel mask of `0x05`.

That disagrees with this driver's own default timeslot table. Decoding
`0x10301020, 0x10501040, ...` with the bit-offset reading above gives
channel *n* at bit offset 32 + *n*·16, i.e. at bus slot 4 + *n*·2. So
bus slot 6 is channel **1**, not channel 2 — and with mask `0x05`,
channel 1 was never enabled, which is exactly the silent second line the
device-tree document warns about.

The mapping is not a constant of the hardware; it is whatever the
timeslot registers were programmed to say. So the PCM driver now exports
`en75xx_pcm_channel_for_slot()`, which searches the configured table,
and the SLIC drivers call it instead of computing. A slot no channel
covers is now a probe failure with a message.

## ZSI adds two clocks of transmit delay

This is the important one. From `Vp886SetOptionTimeslot()` in the
Microchip VoicePath API-II:

```c
/* ZSI adds 2 clocks of delay on the transmit side.  This means that in
   order to transmit in the requested slot, we need to shift backwards 2
   clocks.  The clock slot register only allows forward shifting, so we
   must shift back one whole slot (8 clocks), and the clock slot register
   will be set for +6 clocks on TX. */
if (pDevObj->stateInt & VP886_ZSI_DETECTED) {
    if (txSlot == 0) {
        txSlot = MIN(127, pDevObj->devProfileData.pcmClkRate / 64 - 1);
    } else {
        txSlot--;
    }
}
```

Two halves, and this driver had neither.

The +6 clocks come from the device profile. Register 0x44 is `CLKSLOTS`:
bit 6 is the transmit clock edge and bits 2:0 are the transmit clock
slot. The vendor's `DEV_PROFILE_100V_BB_124_ZSI` sets it to `0x46`, and
`le9641_reset_slicParams()` patches byte 9 of whichever device profile is
in use to `0x46` for every ZSI board:

```c
if (INTERFACE_ZSI == spi_interface_type) {
    *(DEVICE_PRO + INTERFACE_OFFSET) = 0x46;   /* ZSI interface, XE = Pos */
}
```

The driver's copy had `0x06`. In ZSI mode the edge bit is hardwired —
the API detects ZSI precisely by flipping it and seeing the flip refused
— so that half was harmless, but the value now matches the source.

The −8 clocks are the missing half, and they are not harmless. Without
the slot decrement, transmit lands +6 (clock slot) +2 (ZSI) = eight
clocks late: exactly one byte. That is what the `tx_msb` parameter was
compensating for, and it is why 16-bit linear appeared not to work and
the driver moved to u-law. `le9642_program_slots()` now does the shift,
and the default codec is 16-bit linear, matching the vendor, which sets
`VP_OPTION_LINEAR` for these parts.

## Profile lengths come from the profile header

The API reads the raw-MPI length out of the profile header rather than
assuming it runs to the end:

```c
const uint8 *pData = pAcProfile + VP_PROFILE_DATA_START;   /* +6 */
uint8 mpiLength = pAcProfile[VP_PROFILE_MPI_LEN] - 1;      /* [5] - 1 */
uint8 cmd = *pData++;
VpSlacRegWrite(VP_NULL, pLineCtx, cmd, mpiLength, pData);
```

`AC_FXS_RF14_600R_DEF_LE9641` is 80 bytes with a header length of 76 + 4
and an MPI section of 0x49 = 73. The driver streamed
`sizeof(profile) - 6` = 74 bytes, so one trailing formatted-parameter
byte went onto the bus where the SLIC read it as an opcode. The profiles
are now stored whole and the stream helper honours byte 5 of the header.

The device, DC and ring profiles match the vendor sources byte for byte
and were already the right length.

## Timeslot assignment

`slic_adaptor_m.c` picks slots like this:

```c
/* ZSI interface timeslot 0 can not be used, change start time slot to 2 */
if (spi_interface_type == INTERFACE_ZSI) {
    timeSlotIdx = 2;
    if (isNewerThanEN7523)
        timeSlotIdx += 2 * deviceId;
}
...
timeSlots.tx = timeSlotIdx << 1;
timeSlots.rx = timeSlotIdx << 1;
...
timeSlotIdx += 1;
```

`isNewerThanEN7523` is `(isEN7581 || isEN7523 || isAN7552 || isAN7583)`,
so EN751221 and EN7528 keep a single device at index 2. A two-line part
therefore gets bus slots 4 and 6 in every case, which is what this
driver already defaults to.

## What the Skyworks path was missing

`slic_adaptor_s.c` applies three register fix-ups after `ProSLIC_Init()`
that the API presets do not cover, plus a calibration step:

- `ProSLIC_LBCal()` — longitudinal balance calibration, run once the
  batteries are up;
- `PCMMODE` bits 1:0 forced to `0x3`, 16-bit linear;
- `PCMTXHI` bit 4 cleared, "set DTX data to be driven on positive edge
  of PCLK";
- `IRQEN1/2/3` set to `0 / 2 / 0`, hook interrupt only.

All four are now in `en75xx_slic_si3219x.c`.

The same file also shows the vendor using slot 0 for ISI on parts older
than EN7523, and `chanId * 16` otherwise. This driver's `pcm_channel * 16`
matches the SPI case.

## The Si32192 is an ISI part

Airoha's own SLIC support matrix, in section 2.6.1.4 of the VoIP Module
Reference Manual, lists the interface for each supported part:

| vendor | lines | part | interface |
|---|---|---|---|
| Microchip | 1 | Le9641/Le9643 | SPI/PCM & ZSI |
| Microchip | 2 | **Le9642**/Le9662 | **ZSI only** |
| Microchip | 2 | Le9652 | ZSI only |
| Skyworks | 1 | Si32184/Si32185 | SPI/PCM |
| Skyworks | 2 | Si32282/Si32283 | SPI/PCM |
| Skyworks | 1 | **Si32192**/Si32193 | **ISI** |
| MaxLinear | 2 | PEF32002 (DXS102) | SPI/PCM & CSI |

ISI is the Skyworks equivalent of ZSI: the control channel rides the PCM
bus. `en75xx_slic_si3219x.c` is an ordinary Linux SPI driver, but the
supplied evidence does not establish a conventional SPI mode for
Si32192/Si32193. Their known compatibles are now rejected before bus setup
or reset; see `08-proslic-review.en-US.md`. That is now stated in the file header.
These boards need an ISI transport in front of the
driver, in the shape of `en75xx_zsi.c`.

The manual also confirms the MPI addressing this driver uses for the
Microchip parts: "for the Microchip SLIC, odd addresses represent reads"
and even addresses writes.

## The EN7523 PCM interrupt is confirmed

`docs/05` listed GIC SPI 27 as a vendor candidate. The SDK ships the
device tree:

```
pcm@bfbd0000 {
	compatible = "econet,ecnt-pcm";
	reg = <0x1fbd0000 0x4fff>;
	interrupts = <GIC_SPI 27 IRQ_TYPE_LEVEL_HIGH>;
};
```

identically for EN7523, EN7552, EN7581 and AN7583. `ecnt_pcm.c` shows
what that node is for: it is a shim that maps the window and exports
`GET_PCM_REG`, `SET_PCM_REG`, `get_pcm_irq` and `get_pcm_dev` to the
proprietary `pcm1.ko`. One memory resource, one interrupt, nothing else.

## Still not settled

- The EN751221 PCM interrupt. The SDK covers the ARM parts only, and
  says nothing about the MIPS INTC shadow-interrupt problem.
- Whether `chipScuReg` in `pcm1.ko` uses chip-SCU offset 0x0cc or 0x0d8
  for the PCM clock output on the older parts. The AN7581 table says
  0x0cc; the observed EN751221 sequence used 0x0d8. The raw SCU path
  stays opt-in and EN751221-only, so nothing changed.
- Everything electrical. None of this replaces an oscilloscope on
  PCLK/FSYNC, and none of it has been run on hardware.

## Second pass: nineteen more TP-Link GPL drops

The VB430 drop is not the only one TP-Link has published. Nineteen more
were surveyed, covering seven EcoNet/Airoha SoCs. Each was downloaded,
its file list kept, the VoIP-relevant paths extracted, and the archive
deleted.

| drop | SoC | VoIP BSP source |
| --- | --- | --- |
| XN020-G3 v1 | EN7526G (EN751221 family), kernel 3.18 | no |
| XN020-G3 US1 v2 | EN7526G | no |
| XN020-G3 US1 v3, XN020-G3v 2.0 | EN7528 | no |
| XC220-G3 BR v1 | EN7528 | no |
| XN021-G3, XZ000-G3 v2, XZ001-G3 | EN7526G | no |
| XX231v | EN7529 | no |
| XGZ030 v1 | EN7580 | no |
| VB430 (first pass) | AN7551 / AN7581 | **yes** |

Only the VB430 drop ships the VoIP BSP as source. The rest carry the
platform code (`mach-econet`, `asm/tc3162`, the vendor device trees) and,
in the older ones, prebuilt `slic3.ko` and `voip.ko` for the Ralink-era
parts. That makes VB430 the reference for the SLIC layer and the others
useful mainly for platform detail.

### The PCM interrupt, settled

`asm/tc3162/tc3182_int_source.h` carries the MIPS interrupt map in two
enumerations, one for 1004K parts and one for everything older:

| source | 1004K | older |
| --- | --- | --- |
| PCM1 | 11 | 11 |
| PCM2 | 32 | 33 |
| SI_PC1 | — | 12 |

PCM1 is hardware line 11 in both. An earlier note in this project put the
EN751221 PCM interrupt at hwirq 12 and called it a shadow interrupt that
did not map cleanly; line 12 is `SI_PC1_INT`, an unrelated source. The
EN751221 fragment already carries 11 and 33, so this confirms values that
were previously uncertain rather than changing them.

The header is byte-identical in the AN7551, EN7516, EN7529 and EN7580
SDKs, which is four independent drops agreeing.

One caveat on where to read it. These SoCs ship two interrupt headers,
and only `tc3182_int_source.h` names the PCM sources. The sibling
`int_source.h` used by some configurations leaves the same slot as
`RESERVED1` at line 11, IPL20 — the same position and priority level,
just unnamed on a build where voice is not wired. So the numbers hold,
but a grep for `PCM1_INT` in the wrong header of the pair comes back
empty, which is easy to misread as the interrupt not existing.

On the ARM parts the node is identical across every drop seen:

```
pcm@bfbd0000 {
	compatible = "econet,ecnt-pcm";
	reg = <0x1fbd0000 0x4fff>;
	interrupts = <GIC_SPI 27 IRQ_TYPE_LEVEL_HIGH>;
};
```

confirmed now on EN7523, EN7529, EN7552, EN7580, EN7581 and AN7583.

### The converter topology

`ZLR964124_Le9641_IB_profiles.c` sits beside the BB profiles in the same
VB430 drop and had not been read. It carries the second device profile
the vendor selects between:

| | buck-boost | inverting boost |
| --- | --- | --- |
| profile | `DEV_PROFILE_100V_BB_124_ZSI` | `DEV_PROFILE_90V_IB_124` |
| switcher | 47 uH buck-boost | 500 kHz inductorless |
| rails | 12 V in, 100 V out | 12 V in, 90 V out |
| limits | 98 V, 98 V | 92 V, 92 V |

Eighteen bytes differ: device mode, the switching-regulator timing and
parameter blocks, the regulator control byte, the switcher configuration
nibble, the free-run and low-power timing, and the voltage limits. The
DC profiles differ in one formatted byte, the ground-key absolute bit,
and the ring profiles are identical.

`le9641_reset_slicParams()` chooses from `slic_power_type`, a module
parameter, because nothing in the chip reports the circuit around it.
This driver hardcoded buck-boost. It now requires
`airoha,slic-power-type` and refuses to probe without it, because
streaming one topology's switching parameters at the other is a way to
damage a power converter rather than a way to get silence.

### Alarms

`VP886_R_SIGREG_*` in the API headers documents three bits the cadence
worker was already reading and discarding: `TEMPA` thermal alarm,
`OCALM` switcher over-current, and `CFAIL` clock fault in byte 0. The
API's own comment on the auto-shutdown configuration says the
protections are "the power up default", so the chip defends itself
whether or not the driver programs `SSCFG`. What was missing was any way
to learn that it had. Those bits are now logged and reported.
