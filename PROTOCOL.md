# Implemented iDotMatrix protocol subset

This document describes the protocol subset implemented by the stable 0.8.0
WLED iDotMatrix Usermod. It includes the validated media/profile baseline, seven
standalone light effects, source-isolated app Solid rendering, countdown,
stopwatch, scoreboard, persistent alarms and programs/schedules, active-buzzer
mapping, and five LEVEL plus five FFT Audio/Rhythm visualizers. The standalone
`IDotMatrix-ESP32-Emulator` repository remains the primary, complete protocol
reference.

- **Confirmed protocol**: derived from the reference and experiments.
- **WLED mapping**: an integration decision, not a wire-protocol fact.
- **Limitation**: unknown or unimplemented behavior.

## BLE transport

| Role | UUID | Properties |
|---|---|---|
| FA service | `000000fa-0000-1000-8000-00805f9b34fb` | Service |
| FA02 | `0000fa02-0000-1000-8000-00805f9b34fb` | Write, write without response |
| FA03 | `0000fa03-0000-1000-8000-00805f9b34fb` | Read, notify |
| AE service | `0000ae00-0000-1000-8000-00805f9b34fb` | Service |
| AE01 | `0000ae01-0000-1000-8000-00805f9b34fb` | Write, write without response |
| AE02 | `0000ae02-0000-1000-8000-00805f9b34fb` | Read, notify |

Manufacturer data: `54 52 00 70 SCREEN_TYPE`.

| Screen type | Logical resolution |
|---|---|
| `01` | 16x16 |
| `03` | 32x32 |
| `04` | 64x64 |

## Framing and ACK

FA02 commands start with a 16-bit little-endian total length. Version 0.6.1
requires one complete command per queued BLE write and validates declared length
against received length.

Standard ACK on FA03:

```text
05 00 COMMAND SUBCOMMAND 01
```

Status `01` is confirmed for the commands below; it is not a universal bulk ACK.

## Device information

Request: `04 00 01 80`

16x16 response: `09 00 01 80 04 0E 01 01 00`

Offset 7 becomes `03` or `04` for the other profiles. The final byte remains the
confirmed fixed `00`. Encoding WLED power there did not change the app switch and
was reverted.

For compatibility, the Usermod sends this response unsolicited at about 1.2 and
2.5 seconds after connection. The second identical send is a WLED timing
workaround, not a protocol variant.

## Time synchronization

Command:

```text
0B 00 01 80 YEAR MONTH DAY DOW HOUR MINUTE SECOND
```

Response: `05 00 01 80 01`

The current implementation acknowledges but does not apply the time fields. This is an
intentional WLED integration decision: WLED `localTime`, NTP, timezone, and DST
configuration remain the sole clock authority.

## Screen power

**Confirmed protocol:** `05 00 07 01 STATE`

- `00`: off;
- non-zero: on.

ACK: `05 00 07 01 01`

**WLED mapping:** use WLED's normal power path, preserving/restoring the previous
non-zero brightness. A new app connection also emits ON, matching the standalone
reference.

## Brightness

**Confirmed protocol:** `05 00 04 80 PERCENT`

Input is clamped to `0..100`. ACK: `05 00 04 80 01`.

**WLED mapping:** rounded conversion to master brightness:

```text
WLED_BRIGHTNESS = (PERCENT * 255 + 50) / 100
```

While logically OFF, a non-zero command updates the remembered WLED level but
does not power on. Zero produces black while retaining the last usable non-zero
level for a later OFF/ON cycle.

**Limitation:** synchronization is app to WLED only. No confirmed device-to-app
brightness-state message is known, so WLED changes do not update the app slider.

## Full-screen RGB

**Confirmed protocol:** `07 00 02 02 RED GREEN BLUE`

ACK: `05 00 02 02 01`

**WLED mapping (0.8.0):** fill the Usermod RGB canvas and select the
registered `iDotMatrix Display` effect. The command no longer rewrites WLED's
primary colour or exposes native WLED `Solid`; this deliberately keeps app state
and WLED state separate because the official app has no confirmed reverse state
synchronization path.

## Standalone light effects

**Confirmed protocol:**

```text
LENlo LENhi 03 02 EFFECT SPEED COUNT [R G B]...
```

ACK: `05 00 03 02 01`

- `EFFECT`: effect id `0..6` for the seven effects exposed by the app;
- `SPEED`: app speed value, observed/implemented over `0..100`;
- `COUNT`: number of palette colours that follow;
- each palette channel is encoded by the app on a `0..127` scale and is expanded
  by the protocol layer to `0..255`;
- the implementation stores at most 16 palette colours, matching the standalone
  reference emulator; extra colours in an otherwise complete packet are ignored;
- a truncated colour list is recognized and ACKed like the standalone reference,
  but no partial effect state is published.

**WLED mapping:** all seven effects are rendered locally into the same bounded RGB
canvas used by other app content, and the single `iDotMatrix Display` WLED effect
publishes that framebuffer. No native WLED effect is selected or modified. This
is intentional: a WLED-side effect selection is treated as a source change, not
as an edit to the app's unsynchronized light-effect state.

The seven renderers were ported from the validated standalone ESP32 emulator.
They use only the existing canvas plus a bounded palette/state block; no second
framebuffer and no effect-specific heap allocation are introduced.

## Graffiti / DIY mode

**Confirmed protocol:** `05 00 04 01 STATE`

- `00`: leave the DIY editing session;
- non-zero: enter the DIY editing session.

ACK: `05 00 04 01 01`

Entering a new session clears the logical canvas. Matching the reference
implementation, leaving the editing session does not erase or hide the last
image; another display-content command must replace it.

## Graffiti pixel updates

**Confirmed protocol:**

```text
LENlo LENhi 05 01 UNKNOWN R G B X0 Y0 X1 Y1 ...
```

- byte 4 remains semantically unknown and is ignored;
- RGB is at offsets 5..7;
- coordinate pairs begin at offset 8;
- coordinates are native to the selected 16x16, 32x32, or 64x64 profile;
- invalid coordinates are ignored;
- an unmatched trailing coordinate byte is ignored, matching the reference;
- the reference sends no FA03 acknowledgement for these pixel packets.

**WLED mapping:** accepted pixels update a three-byte-per-pixel logical RGB
framebuffer. The Usermod selects its registered `iDotMatrix Display` 2D
effect, which copies the canvas while WLED services the current segment and has
valid virtual XY state. A valid pixel packet also selects the effect. No
physical serpentine mapping is duplicated in this module.

Complete short packets use the 64-byte queue. Larger logical FA02 packets are
reassembled in the dedicated 4112-byte slot before protocol dispatch.

## Clock

**Confirmed protocol:**

```text
08 00 06 01 FLAGS RED GREEN BLUE
```

- `FLAGS & 0x3F`: clock-style value;
- `FLAGS & 0x40`: use 24-hour time;
- `FLAGS & 0x80`: enable date display;
- bytes 5..7: selected RGB colour.

ACK: `05 00 06 01 01`

**WLED mapping:** the command stores the display options and selects the custom
`iDotMatrix Display` effect. The shared effect reads WLED local time and draws into the
same logical RGB canvas used by other iDotMatrix content. The eight currently
known styles use the hand-tuned 16x16 artwork from the standalone reference and
are scaled to a 32x32 or 64x64 logical profile.

## Countdown

**Confirmed protocol:**

```text
07 00 08 80 MODE MINUTES SECONDS
```

ACK: `05 00 08 80 01`

`MODE` semantics from the standalone reference:

- `0`: reset/stop and clear the remaining time;
- `1`: start or restart from `MINUTES:SECONDS`;
- `2`: pause, preserving the current remaining time;
- `3`: resume the preserved remaining time when it is non-zero.

The display is a 16x16 `MM:SS` layout scaled through the normal renderer canvas.
Values above 99 minutes wrap visually modulo 100, matching the reference artwork.
The final five displayed seconds are red; earlier time is white.

When the countdown reaches zero the original device reports completion
asynchronously on FA03:

```text
05 00 08 80 03
```

The Usermod keeps countdown state independent of native WLED display ownership.
Selecting a WLED effect hides the timer but does not rewrite or stop its emulated
device state; a later pause/resume/start command from the app reclaims
`iDotMatrix Display`.

## Stopwatch

**Confirmed protocol:**

```text
05 00 09 80 MODE
```

ACK: `05 00 09 80 01`

`MODE` semantics:

- `0`: reset to zero and stop;
- `1`: reset to zero and start;
- `2`: pause while preserving elapsed time;
- `3`: resume from the preserved elapsed time.

The visible output uses the same white 16x16 `MM:SS` renderer as countdown and
remains under `iDotMatrix Display`. Stopwatch state also remains independent of
native WLED effect selection.

## Scoreboard

**Confirmed protocol:**

```text
08 00 0A 80 A_lo A_hi B_lo B_hi
```

ACK: `05 00 0A 80 01`

Both scores are little-endian 16-bit values on the wire. The verified reference
artwork renders only the last two decimal digits of each value (`score % 100`):
team A in blue, a white separator, and team B in red. The framebuffer remains
owned by `iDotMatrix Display`; no native WLED colour/effect state is modified.

When date display is enabled, the integration preserves the experimentally
verified emulator presentation: 30 seconds of `HH:MM`, followed by 5 seconds of
`DD/MM`. This timing is emulator behavior and is not claimed as a universal
original-device protocol requirement.

## Logical-to-physical mapping

**WLED integration decision:** `screenType` selects the advertised logical
profile. Renderer storage may use the smaller physical matrix dimensions when
the test-only rescale option is enabled. The physical output size comes from the selected
WLED 2D segment. With `rescale=false`, unequal sizes are blocked and produce black. With
`rescale=true`, nearest-neighbour sampling maps the complete logical canvas to
the segment. WLED remains responsible for panel layout, rotation, mirroring,
grouping, and serpentine wiring.

`screenType` is also bounded by the decoder profile compiled into the firmware:

- LZW10/default accepts only protocol profile `0x01` (16x16);
- LZW11 accepts `0x01` and `0x03` (16x16/32x32);
- LZW12 accepts `0x01`, `0x03` and `0x04` (16x16/32x32/64x64).

The settings UI applies the same boundary. Loading a larger value from an old
configuration selects the nearest supported profile instead of advertising a
size that its media decoder cannot handle. Rescale is absent and forced off in
the 16x16-only build.

## FA02 bulk transport

**Confirmed protocol:** every bulk packet has a 16-byte header:

| Offset | Size | Field |
|---:|---:|---|
| 0 | 2 | packet length, little-endian |
| 2 | 1 | content type |
| 3 | 1 | fixed `00` in confirmed packets |
| 4 | 1 | unknown |
| 5 | 4 | complete payload size, little-endian |
| 9 | 4 | complete-payload CRC32, little-endian |
| 13 | 3 | unknown header fields |
| 16 | remaining | payload chunk |

Type `0x03` identifies TEXT. While more payload is required, FA03 returns:

```text
05 00 03 00 01
```

When the transaction terminates, FA03 returns:

```text
05 00 03 00 03
```

`0x01` is the confirmed continue status. `0x03` means that the transaction has
terminated; it must not be interpreted as universal success because the
reference also uses it after CRC or storage failure.

**Channel correction from hardware testing:** BUILD 80 calls
`processBulkPacket()` after reassembling FA02 writes. Its AE01 callback only
logs received bytes. Version 0.6.3-dev.1 incorrectly assigned bulk to AE01 and
therefore observed no chunks; 0.6.3-dev.2 follows the source implementation.

One dedicated FA02 assembler accepts an observed 4096-byte payload chunk plus
the 16-byte header. The callback only performs bounded copies; logical-packet
dispatch, CRC32, and notification happen in the normal Usermod loop. TEXT is
bounded to 4096 payload bytes; RAW is bounded to the 12288 bytes required by a
64x64 RGB frame. GIF is streamed to LittleFS and capped at 2 MiB here.

Matching BUILD 80, an otherwise unknown complete short FA02 command is
tolerated and receives `05 00 COMMAND SUBCOMMAND 01`.

The routed common types are `0x01` GIF, `0x02` RAW RGB, and `0x03` TEXT. Their
ACK retains the type and uses `0x01` while incomplete and `0x03` when the
transaction terminates.

## Compact PNG envelope (experimental)

A physical app trace produced this 140-byte packet prefix:

```text
8c 00 00 00 00 83 00 00 00 89 50 4e 47 ...
```

Packet length 140, size 131, and the PNG signature at offset 9 imply:

| Offset | Size | Field |
|---:|---:|---|
| 0 | 2 | packet length, little-endian |
| 2 | 1 | content type `00` |
| 3..4 | 2 | observed `00 00`, semantics unknown |
| 5 | 4 | PNG byte length, little-endian |
| 9 | remaining | PNG beginning `89 50 4E 47...` |

The current implementation replies `05 00 00 00 03`. No outer CRC was observed; PNG/zlib
structure is validated. This layout is a new inference from the WLED trace,
not yet a confirmed BUILD 80 or original-device fact. The decoder accepts
non-interlaced 8-bit RGB/RGBA and exact logical-profile dimensions. The complete
compact PNG envelope must fit the 4112-byte FA02 slot.

## GIF payload

**Confirmed by the reference:** common type `0x01` contains a standard
`GIF87a` or `GIF89a` stream. CRC32 covers the complete compressed payload. The
WLED integration writes chunks to an RX file and starts playback only after a
valid completion and deferred RX-to-PLAY promotion.

Stable 0.8.0 accepts GIF dimensions up to the active logical profile and the
compiled decoder limit. The default build supports 16x16, `IDOT_GIF_LZW11`
supports 32x32, and `IDOT_GIF_LZW12` supports 64x64 with the complete legal
4096-code LZW12 space. On ESP32 with PSRAM the 64x64 build selects full
AnimatedGIF/direct playback; on classic ESP32 without PSRAM it selects the
compact12/LittleFS frame-cache backend. Backend choice is a WLED integration
and memory-management detail, not a wire-protocol difference.

## RAW RGB image payload

**Confirmed protocol:** bulk type `0x02` contains row-major RGB triplets. Its
declared size must match the active logical profile:

| Profile | Resolution | RAW bytes |
|---:|---:|---:|
| `0x01` | 16x16 | 768 |
| `0x03` | 32x32 | 3072 |
| `0x04` | 64x64 | 12288 |

Each pixel is `R G B`; pixels advance left-to-right and rows top-to-bottom.
The WLED mapping is intentionally separate from the protocol order: the
logical image is published only after valid CRC32, then the existing display
effect lets WLED apply its configured matrix mapping or the optional rescale.

## TEXT payload and glyph records

**Confirmed payload header:**

| Offset | Field |
|---:|---|
| 0 | glyph count |
| 1..3 | not yet documented |
| 4 | movement/effect |
| 5 | speed (`0..100`; mapped to 500..15 ms per pixel) |
| 6 | colour mode |
| 7..9 | text RGB |
| 10 | background enabled/mode |
| 11..13 | background RGB |

Each glyph begins with a four-byte metadata prefix followed by its bitmap:

| Marker | Status | Glyph | Bitmap | Complete record |
|---:|---|---:|---:|---:|
| `0x02` | confirmed | 8x16 | 16 bytes | 20 bytes |
| `0x05` | confirmed | 16x32 | 64 bytes | 68 bytes |
| `0x03` | reference compatibility alias, unconfirmed | 8x16 | 16 bytes | 20 bytes |
| `0x06` | reference compatibility alias, unconfirmed | 16x32 | 64 bytes | 68 bytes |

The current implementation accepts only the experimentally confirmed `0x02` and
`0x05` markers. Bitmap rows are consecutive, and the least-significant bit is
the leftmost pixel within each byte. Mixed glyph sizes in one payload have not
been observed and are not supported.

**WLED mapping:** the app-supplied bitmap is stored by the independent renderer
and selects the existing `iDotMatrix Display` effect. The global colour,
background, speed, movement, and effects are rendered locally. SimSun and
SimHei require no WLED font library because the official app rasterizes them
before transmission.

## Audio / Rhythm stream (0.8.0)

The official app emits two families directly on FA02:

| Family | Logical frame | Modes |
|---|---|---:|
| LEVEL | `06 00 00 02 LEVEL MODE` | `MODE=1..5` |
| FFT | `21 00 01 02 MODE B0..B7 B7..B0` | `MODE=0..4` |

LEVEL and the first eight FFT bands are clamped to `0..12`. The mirrored FFT
half is consumed as part of the 21-byte logical frame but is not stored twice.
Observed FFT BLE writes are a continuous stream rather than one ATT write per
logical frame: a 33-byte write can contain one full frame followed by 12 bytes
of the next. The audio parser therefore carries at most one 21-byte partial
frame across writes and can publish multiple complete frames from a stream.

Every valid audio frame selects `iDotMatrix Display`, cancels the previous
app-originated visual mode, and renders into the existing RGB canvas. Selecting
a normal WLED effect releases audio state; a later audio frame reclaims the
display. No audio feedback is sent from WLED to the app.

## Unsupported in 0.8.0

- unconfirmed TEXT marker aliases `0x03` and `0x06`;
- interlaced PNG, other PNG colour types, and PNG dimensions differing from
  the logical profile;
- GIF dimensions larger than the active logical profile or larger than the
  maximum compiled GIF decoder profile (16x16 default, 32x32 LZW11, 64x64 LZW12);
- device-level rotation, energy-saving, and reset commands, intentionally left
  to WLED's own configuration and control paths.

The unconfirmed TEXT aliases remain documented for protocol archaeology, but
are not planned for implementation unless a real app capture requires them.
Device-level display policy is intentionally not duplicated by the emulator.

### Timer rendering note (0.8.0)

The countdown (`08 80`) and stopwatch (`09 80`) wire formats are unchanged. The WLED renderer now preserves millisecond state internally so the BUILD80 timer-hand phase can be reproduced above the MM:SS display; this is a rendering change only, not a protocol change.


## Alarm / program sound mapping (0.8.0)

The wire protocol is unchanged. Alarm packets retain their per-alarm buzzer request. Program global flags retain bit 1 as the sound request. The WLED mapping intentionally differentiates them: alarms repeat the non-blocking trill for their configured duration, while a program activity emits three groups of three short trills once when the activity becomes active and does not sound continuously for the full time window.
