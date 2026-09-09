<!--
Origin: this file lives in the sirherobrine23/airoha_docs repository, not
here. It is parked in this repo because the GitHub authorisation for that
repository stopped working mid-session and the commit could not be pushed.
Treat airoha_docs as the home for it; this copy is reference material.
-->

# Airoha / EcoNet VoIP: the PCM engine, ZSI and the SLICs

Notes taken from Airoha's LTS SDK 7.3.283, published by TP-Link as the
GPL sources for the VB430 (`VB430_GPL_ITWIND3.tar.gz`, ~5.3 GB). The
drop targets AN7551/AN7581, but the VoIP block is the same lineage all
the way back to the MIPS EN751221, so most of it carries backwards.

Applies to EN751221, EN7523, EN7528, EN7552, EN7581 and AN7583.

## Where the VoIP code lives in the drop

```
VB430/bba_3_0_platform/sdk/an7551/
├── Airoha_Doc/VOIP/VOIP Reference Module En.pdf      222 pages
├── airoha_feeds/target/linux/airoha/files/
│   └── arch/{arm,arm64}/mach-econet/ecnt_pcm.c       resource shim
├── openwrt-21.02/.../feeds/airoha/target/linux/airoha/files/
│   └── arch/arm/boot/dts/en7523.dtsi                 PCM node
└── tclinux_phoenix/release_bsp/<profile>/BSP/voip_bsp/voip_module/
    ├── DSP/MTK/mod-slic3/src/zarlink/                Microchip, source
    ├── DSP/MTK/mod-slic3/src/silab/                  Skyworks, source
    └── ko/pcm1.ko                                    not stripped
```

Three profiles ship the same BSP: `UNION_AN7552_MT7916_JEDI_KERNEL_5_4_demo`,
`UNION_AN7581_EAGLE_LOGAN_KERNEL_5_4_demo` and
`UNION_AN7581_KITE_LOGAN_KERNEL_5_4_demo`.

The SLIC layer is real C. The PCM engine, the DSP and the SPI/ZSI
wrapper are binary-only (`pcm1.ko`, `DSPCore.ko`, `spi.ko`, `ovdsp.ko`,
`foip.ko`, `lec.ko`, `acodec_x.ko`, `fxs3.ko`), but `pcm1.ko` keeps its
symbol table, which is enough to read its register map.

## The PCM block

Base 0x1fbd0000. In the device tree it is one node with one memory
resource and one interrupt:

```
pcm@bfbd0000 {
	compatible = "econet,ecnt-pcm";
	reg = <0x1fbd0000 0x4fff>;
	interrupts = <GIC_SPI 27 IRQ_TYPE_LEVEL_HIGH>;
};
```

identical for EN7523, EN7552, EN7581 and AN7583. `ecnt_pcm.c` is only a
shim: it maps the window and exports `GET_PCM_REG`, `SET_PCM_REG`,
`get_pcm_irq` and `get_pcm_dev` for the proprietary module to use.

The vendor claims the whole 0x5000 window. PCM1 sits at +0x0000, the ZSI
wrapper at +0x1000 and PCM2 at +0x2000.

### Register map

From the `regMap` symbol in `pcm1.ko`. Each entry is a name pointer, a
flags word, a writable-bit mask, an address and a reset value. Addresses
are given in the MIPS KSEG1 form and are the same on the ARM parts.

| register | offset | writable mask | reset |
| --- | --- | --- | --- |
| `pcmCtrl` | 0x00 | 0x1f7f1f1f | 0x0500040a |
| `txTimeSlotCfg0..3` | 0x04..0x10 | 0x13ff13ff | see below |
| `rxTimeSlotCfg0..3` | 0x14..0x20 | 0x13ff13ff | see below |
| `ISR` | 0x24 | 0x000007ff | 0 |
| `INTMask` | 0x28 | 0x000007ff | 0 |
| `txPolling` | 0x2c | 0xffffffff | 0 |
| `rxPolling` | 0x30 | 0xffffffff | 0 |
| `txRingBaseAddr` | 0x34 | 0xffffffff | 0 |
| `rxRingBaseAddr` | 0x38 | 0xffffffff | 0 |
| `txrxRingSizeAndOff` | 0x3c | 0x000000ff | 0xc0 |
| `txRxDMA` | 0x40 | 0x0000000f | 0x0f000000 |
| `txTimeSlotCfg4..15` | 0x48..0x74 | 0x13ff13ff | see below |
| `rxTimeSlotCfg4..15` | 0x78..0xa4 | 0x13ff13ff | see below |
| `txRxChanEnable` | 0xac | 0x0000000f | 0x0000000f |

Nothing exists at 0xa8, and nothing above 0xac.

### Timeslot configuration

Sixteen registers each side, two channels per register, packed as
`hi16 << 16 | lo16`. Within a half, bit 12 marks a 16-bit slot and bits
9:0 hold the slot position.

That position is a **bit offset into the 8 kHz frame**, not a byte-slot
index. The reset values say so:

```
txTimeSlotCfg0  0x00080000   ->  channel 0 at bit 0,   channel 1 at bit 8
txTimeSlotCfg1  0x00180010   ->  channel 2 at bit 16,  channel 3 at bit 24
txTimeSlotCfg2  0x00280020   ->  channel 4 at bit 32,  channel 5 at bit 40
txTimeSlotCfg3  0x00380030   ->  channel 6 at bit 48,  channel 7 at bit 56
...
txTimeSlotCfg15 0x00f800f0   ->  channel 30 at bit 240, channel 31 at bit 248
```

Channel *n* resets to bit *n*·8, one byte timeslot apart. A byte
timeslot *s* therefore lives at bit offset *s*·8, and a 16-bit channel
takes two byte timeslots.

This is what makes "which DMA channel carries bus slot 4?" a question
about the programmed table rather than a fixed formula.

### Interrupts

`ISR` and `INTMask` implement eleven bits. There is no hook-status
interrupt in this block at bits 11 and up.

`pcm1.ko` exposes `/proc/pcm1/pcmisrinfo`, which reports
`PCM_0 isr_error=%d isr_delay=%d`, plus `pcmIsrDebug` and
`pcmIsrJitterTest`. The reference manual says to pin the PCM interrupt
and the DSP tasks (`fxs_task`, `pcmreinit_task`, `slicint_task`,
`cid_task`) to the last CPU and keep everything else off it.

### Descriptors

`pcm1.ko` on AN7581 prints

```
desc status:0x%08lx(ownership:%d,sample size:%u)
```

with no channel-mask field. The older MIPS build also prints `chvaild`.
Two descriptor layouts, then: the MIPS parts carry the channel mask in
the status word and one buffer pointer per channel (36 bytes), the ARM
parts move the mask to its own word and keep a single buffer pointer
(12 bytes). Fifteen descriptors per ring on both.

## ZSI, ISI and CSI

The SLIC control channel can ride the PCM bus instead of a separate
serial bus. `mod-slic3` calls the choice `spi_interface_type`:

| value | name | used by |
| --- | --- | --- |
| 0 | SPI | plain four-wire SPI plus PCM |
| 1 | ZSI | Microchip / Microsemi |
| 2 | ISI | Skyworks / Silicon Labs |
| 3 | CSI | MaxLinear |

`/proc/spi/spi_ctrl_test` reads and writes SLIC registers by hand:

```
echo "0 <slic_type> <interface_type> <slic_num>" > /proc/spi/spi_ctrl_test   # init
echo "1 <slic_type> <interface_type> <slic_id> <ctrl> <reg> <len>" > ...     # read
echo "2 <slic_type> <interface_type> <slic_id> <ctrl> <reg> <len> <data...>" # write
```

`slic_type` is 0 Microchip, 1 Silicon Labs, 2 MaxLinear. For Microchip
the `ctrl` byte is unused and register parity carries the direction:
odd addresses read, even addresses write. For Silicon Labs `ctrl` is
0x60/0x70 to read channel 0/1 and 0x20/0x30 to write. For MaxLinear it
is 0xbe to read and 0x7e to write.

### Timeslot assignment

From `slic_adaptor_m.c`:

```c
/* ZSI interface timeslot 0 can not be used, change start time slot to 2 */
if (spi_interface_type == INTERFACE_ZSI) {
    timeSlotIdx = 2;
    if (isNewerThanEN7523)
        timeSlotIdx += 2 * deviceId;
}
...
timeSlots.tx = timeSlotIdx << 1;   /* 16-bit codec: two byte slots */
timeSlots.rx = timeSlotIdx << 1;
...
timeSlotIdx += 1;                   /* next line */
```

`isNewerThanEN7523` is `(isEN7581 || isEN7523 || isAN7552 || isAN7583)`,
so EN751221 and EN7528 keep a single device at index 2. A two-line part
gets bus slots 4 and 6 either way.

The codec is `VP_OPTION_LINEAR`, 16-bit, not G.711.

### The two clocks ZSI costs you

From the Microchip VoicePath API-II, which the SDK ships as the
integration's dependency:

```c
/* ZSI adds 2 clocks of delay on the transmit side.  This means that in
   order to transmit in the requested slot, we need to shift backwards 2
   clocks.  The clock slot register only allows forward shifting, so we
   must shift back one whole slot (8 clocks), and the clock slot register
   will be set for +6 clocks on TX. */
```

So a ZSI board must program the transmit timeslot one byte slot *below*
the one it wants, and the device profile supplies the +6 through the
clock-slot register (0x44, bits 2:0). Miss the shift and transmit audio
lands exactly one byte late.

The API detects ZSI by flipping bit 6 of register 0x45 and reading it
back: in ZSI mode the bit is hardwired and the flip is refused.

## SLIC support matrix

From section 2.6.1.4 of the VoIP Module Reference Manual.

| vendor | lines | part | interface |
| --- | --- | --- | --- |
| Microchip | 1 | Le9641 / Le9643 | SPI/PCM or ZSI |
| Microchip | 2 | Le9642 / Le9662 | ZSI |
| Microchip | 1 | Le9651 / Le9653 | SPI/PCM or ZSI |
| Microchip | 2 | Le9652 | ZSI, high voltage |
| Microchip | 2 | Le9622 | SPI/PCM, replaces ZL88601 |
| Silicon Labs | 1 | Si32182 / Si32183 | ISI |
| Silicon Labs | 1 | Si32184 / Si32185 | SPI/PCM |
| Silicon Labs | 2 | Si32280 / Si32281 | ISI |
| Silicon Labs | 2 | Si32282 / Si32283 | SPI/PCM |
| Silicon Labs | 2 | Si32284 / Si32285 | ISI |
| Silicon Labs | 2 | Si32286 / Si32287 | SPI/PCM |
| Silicon Labs | 1 | Si32192 / Si32193 | ISI, ProSLIC API 9.1.0 |
| MaxLinear | 1 | PEF31001 (DXC101) | CSI |
| MaxLinear | 2 | PEF31002 (DXC102) | CSI |
| MaxLinear | 1 | PEF32001 (DXS101) | SPI/PCM or CSI |
| MaxLinear | 2 | PEF32002 (DXS102) | SPI/PCM or CSI |

The `/dtmfdet` variants add hardware DTMF detection. MaxLinear is marked
"the customer maintains the SDK": Airoha ships no driver for it.

The Microchip integration carries full profiles for Le89116, Le89156,
Le89316, ZL88601, ZL88801, Le9622, Le9641, Le9651, Le9652 and Le9662.
The Skyworks side carries the ProSLIC API 9.1.0 with patches for
Si3217x, Si3218x, Si3219x, Si3226x and Si3228x, plus the MLT line-test
API 4.0.1.

## What the Skyworks path does after init

`slic_adaptor_s.c` applies four things the ProSLIC API presets do not:

```c
ProSLIC_LBCal(...);                              /* longitudinal balance */
PCMMODE  = (PCMMODE & ~0x3) | 0x3;               /* 16-bit linear */
PCMTXHI &= ~(1 << 4);                            /* DTX on rising PCLK */
IRQEN1 = 0; IRQEN2 = 2; IRQEN3 = 0;              /* hook interrupt only */
```

and picks the timeslot as `chanId * 16` for SPI and for parts newer than
EN7523, or 0 for ISI on the older MIPS parts.

## Userspace

Three character devices, created at build time:

| device | major | used for |
| --- | --- | --- |
| `/dev/slic` | 231 | SLIC control and events |
| `/dev/vdsp` | 232 | DSP control and events |
| `/dev/spi` | 233 | raw SLIC register access |

The application API is ADAM (`libadam.so`, headers `adam.h`,
`eva_constant.h`, `eva_struct.h`), driven by `evcom` on the command
line. `voip_loader` probes the SLIC and loads the matching modules, but
only for ZSI, ISI and CSI: an SPI board has to load them by hand.

Country tone and ring profiles ship as text under
`voip_app/CountrySetting/<ISO3>/toneSetting.txt`, covering 34 countries.

## Building the vendor stack

```
package/feeds/airoha/voip_driver/compile MSDK=1     # kernel modules
package/feeds/airoha/voip_app/compile MSDK=1        # libadam and tools
```

The SLIC driver is meant to be maintained by the integrator: Airoha
provides a hook interface and the shipped driver is only there to prove
the hardware. `mod-slic3/src/{zarlink,silab,intel}/readme.txt` says to
drop the vendor's own SDK into `vp_api_ii/`, `proslic_api/` or
`proslic_mlt_api/` and rename `Makefile_*.release`.

## A free-software driver

`https://sirherobrine23.com.br/airoha/pcm_asterisk` carries an
out-of-tree GPL implementation of the PCM engine, the ZSI transport, the
Le9642 and the Si3219x, plus an Asterisk channel driver. Its
`docs/07-gpl-sdk-crosscheck.md` records what this SDK confirmed and what
it contradicted.

## Survey of twenty TP-Link GPL drops

Twenty drops were downloaded and searched for VoIP content. Only two
carry the VoIP BSP as source, and they carry the same one.

| drop | SoC | VoIP BSP source |
| --- | --- | --- |
| XN020-G3 v1, US1 v2, XN021-G3, XZ000-G3 v2, XZ001-G3 | EN7526G (EN751221 family) | no |
| XN020-G3 US1 v3, XN020-G3v 2.0, XC220-G3 BR v1 | EN7528 | no |
| XX231v | EN7529 | no |
| XGZ030 v1 | EN7580 | no |
| VX830v ITWIND | EN7516 | no |
| XB430v | AN7551 | partial |
| **VB430 ITWIND3** | AN7551 / AN7581 | **yes** |
| **XX532v** | AN7551 / AN7583 | **yes** |

The drops without the BSP still carry the platform code: `mach-econet`,
`asm/tc3162` and the vendor device trees. That is where the interrupt
numbering and the PCM node live.

XX532v adds an `AN7583_KITE_LOGAN` BSP variant to the two in VB430, but
`mod-slic3` is byte-identical content: the same Microchip families
(ZL88601, ZL88801, Le89116, Le89156, Le89316, Le9622, Le9641, Le9651,
Le9652, Le9662) and the same Skyworks families (Si3217x, Si3218x,
Si3219x, Si3226x, Si3228x).

### The PCM interrupt

`asm/tc3162/tc3182_int_source.h` is identical in the AN7551, EN7516,
EN7529 and EN7580 SDKs and names the PCM sources:

| source | MIPS 1004K | older MIPS |
| --- | --- | --- |
| PCM1 | 11 | 11 (IPL20) |
| PCM2 | 32 | 33 |

The sibling `int_source.h`, used by some configurations, leaves line 11
as `RESERVED1` at the same IPL20 on builds where voice is not wired, so
searching the wrong header of the pair looks like the interrupt is
absent.

On the ARM parts the node is the same everywhere, now confirmed on
EN7523, EN7529, EN7552, EN7580, EN7581 and AN7583:

```
pcm@bfbd0000 {
	compatible = "econet,ecnt-pcm";
	reg = <0x1fbd0000 0x4fff>;
	interrupts = <GIC_SPI 27 IRQ_TYPE_LEVEL_HIGH>;
};
```

### Le9641/Le9642 converter topologies

`ZLR964124_Le9641_BB_profiles.c` and `ZLR964124_Le9641_IB_profiles.c`
carry two device profiles, and `le9641_reset_slicParams()` picks between
them from `slic_power_type`:

| | `DEV_PROFILE_100V_BB_124_ZSI` | `DEV_PROFILE_90V_IB_124` |
| --- | --- | --- |
| switcher | 47 uH buck-boost | 500 kHz inductorless boost |
| rails | 12 V in, 100 V out | 12 V in, 90 V out |
| limits | 98 V, 98 V | 92 V, 92 V |

Eighteen bytes differ, all of them switching-regulator fields. The DC
profiles differ in the ground-key absolute bit; the ring profiles are
identical. `slic3_main.c` knows four topologies in total: BB, IB, CIBB
and TB.

### MaxLinear DUSLIC-XS

Airoha does not ship the MaxLinear driver, only a readme naming the
packages the integrator must obtain: `lib_ifxos-1.6.6`,
`drv_tapi-4.16.3.0` and `drv_dxs-1.3.1.0`. The built `slic3_intel.ko` is
433 KB and **not stripped**, statically linking all three, and retains
133 `DXS_*` symbols, the `drv_dxs` source file names, the BBD container
magic `!bbdDXS!`, and the `configure` line of each package. The DUSLIC-XS
on that board is configured `--enable-dcdc-hw=CIBB12`.
