# Asterisk integration

`chan_en75xx` registers each `/dev/en75xx-fxsN` as an Asterisk channel of
type `EN75XX`, so a plain analog telephone behaves like a telephone:
dial tone, digits, ring, answer, hangup.

## Why a channel driver and not DAHDI

DAHDI would give the analog state machine for free through `chan_dahdi`,
but it wants its own kernel module, its own span/channel model and a
1 ms chunk cadence, all of which would sit on top of a character device
that already exists and already carries 10 ms frames. The state machine
`chan_dahdi` provides is about 400 lines of the file below; the rest of
DAHDI would be pure overhead on a router with 2 FXS ports.

The split is: anything timing critical stays in the kernel — hook
polling, ring cadence and its silent gaps, G.711 companding — and
Asterisk only sees 8 kHz signed-linear audio plus four ioctls.

## Threads

| Thread    | Owns                                                                      |
| --------- | ------------------------------------------------------------------------- |
| monitor   | polls every line for `POLLPRI` and turns hook changes into channel events |
| ss_thread | one per outgoing call: dial tone, digit collection, `ast_pbx_run`         |
| channel   | Asterisk's own; does `read()`/`write()` on the same fd                    |

The monitor and the channel thread share one fd deliberately. The
monitor only ever polls `POLLPRI` and issues ioctls; the channel thread
only ever moves audio. `EN75XX_VOICE_GET_STATE` both reads the hook and
acknowledges the event, so `POLLPRI` clears when it is handled.

## A kernel-side change came with this

`poll()` used to report `EPOLLIN` unconditionally. That is fine for a
program that just calls `read()` and blocks, which is what the original
consumer did, but it makes a poll-driven consumer spin or block inside
its I/O loop. `poll()` now waits on the PCM channel's own wait queues
and reports readability only when a whole 160-byte frame is queued, and
drops `EPOLLOUT` when the TX FIFO cannot take one. Three new
`line_ops` entries carry that information up from the PCM driver:
`poll_wait`, `rx_avail`, `tx_space`.

## DTMF

Neither the Le9642 nor the ProSLIC parts report digits out of band here,
so the tones are pulled out of the audio with `ast_dsp` in the read
path: `ast_dsp_process()` rewrites a voice frame into `AST_FRAME_DTMF`
when it finds one, which is what `ast_waitfordigit()` in the switch
thread consumes. `hardware_dtmf=yes` in `en75xx.conf` turns the detector
off, for a SLIC that grows out-of-band reporting later.

## Tones

Tones are looked up in the channel's tone zone rather than hardcoded, so
setting the zone in `indications.conf` gives the right dial, busy and
congestion tones for the country. For Brazil:

```
[general]
country = br
```

## Digit collection timing

Matches the `chan_dahdi` defaults: 16 s for the first digit, 8 s between
digits, and 3 s after a match when a longer extension could still match,
so a dialplan with both `1` and `100` behaves sanely.

## Dial tone and DTMF detection

The active Asterisk tone zone supplies the dial-tone frequencies. If the
line driver advertises a tone generator and the zone specifies a continuous
single or dual tone, the SLIC plays it; otherwise Asterisk plays the zone's
full tone pattern. `hardware_tones=no` forces software playback. Optional
frequency and level overrides remain available in `en75xx.conf`.

Dial tone can reflect through the hybrid into the receive path and mask
the first DTMF digit. During digit collection, the channel driver derives
up to two notch filters from the frequencies it actually plays. The filters
are removed before the conversation begins. Software tone volume defaults
to Asterisk's own value; `dialtone_volume` and `callprogress_volume` can
override it for a particular line if measurements require that.

## FXS status LED

The base Asterisk package drives LEDs whose device-tree function is `voip`.
It leaves the LED off while Asterisk is disabled or SIP is unavailable, on
while SIP is ready, and blinking while a line is dialing, ringing or in a
call. If outbound registrations exist, one must be `Registered`; otherwise
an available inbound PJSIP contact is accepted. Boards with a differently
named phone LED can set `asterisk.general.fxs_led` to its sysfs LED name.
`asterisk.general.fxs_led_registration` may be set to `outbound` or
`contacts` to choose a specific SIP topology. No LED is required for the
channel driver to operate.

## Installing

```
cp -r openwrt/package/asterisk-chan-en75xx <openwrt>/package/
make menuconfig      # Network -> Telephony -> asterisk-chan-en75xx
```

Then on the device:

```
asterisk -rx 'module load chan_en75xx.so'
asterisk -rx 'en75xx show lines'
```

`en75xx show lines` prints hook state, channel state and the PCM
counters (overruns, underruns, DMA errors) per line — the first place to
look when audio is wrong.

`en75xx ring <line> on|off` rings a line directly, without a call, which
is the quickest bench check that the SLIC and the cadence work.

## First call to try

Extension `600` in the sample dialplan is `Echo()`. If you hear yourself,
both directions of the PCM path work. If you hear nothing, check
`en75xx show lines` for rx bytes climbing: audio arriving but silence in
the earpiece is a different fault from no audio at all.
