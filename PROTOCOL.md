# Implemented iDotMatrix protocol subset

This document describes the protocol subset implemented by stable Release 0.8.2. The
BLE wire protocol is carried forward unchanged from the stable 0.8.1
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

### Compatibility/security note

The observed original-device profile uses writable GATT characteristics without
pairing or application authentication, and 0.8.2 compatibility mode preserves
that behaviour. Nearby BLE peers may therefore issue supported commands. CRC42
checks media integrity only; it is not an authentication mechanism. Mandatory
pairing/encryption is intentionally not added in 0.8.2 because that would change
the captured wire/client contract.

| Screen type | Logical resolution |
|---|---|
| `01` | 16x16 |
| `03` | 32x32 |
| `04` | 64x64 |

## Framing and ACK

FA02 normal commands start with a 16-bit little-endian total length. 0.8.2 queues
each complete ATT write unchanged from the NimBLE callback, then performs all
FA02 reassembly and protocol dispatch in the normal WLED loop. Logical packets
may span multiple ATT writes; one ATT write may also finish one packet and begin
the next. The logical-packet maximum is 8192 bytes. Audio/Rhythm frames keep
their captured fixed-family stream framing, but a recognized normal command
(reset, Carousel control, bulk, etc.) terminates audio-stream routing and is
processed normally.

Standard ACK on FA03:

```text
05 00 COMMAND SUBCOMMAND 01
```

Status `01` is confirmed for the commands below; it is not a universal bulk ACK.

## Device information

Request: `04 00 01 80`

16x16 response: `09 00 01 80 00 09 01 01 00`

Offsets 4 and 5 expose the public Usermod release major/minor to the official app.
For release `0.8.2` they are `00 08`; the internal build identifier is never encoded
in this response. Offset 7 becomes `03` or `04` for the other profiles. The final
byte remains the confirmed fixed `00`. Encoding WLED power there did not change the app switch and
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

WLED remains the primary clock authority whenever its `localTime` is valid, so
normal NTP, timezone, and DST configuration continues to apply. The last valid
application time synchronization is also retained and used as an offline fallback
while WLED local time is not yet valid.

## Program / Schedule ACK semantics

Hardware-informed behavior requires command-specific interpretation:

```text
07 80 -> 05 00 07 80 01
05 80 -> 05 00 05 80 03
```

`0x03` is a transaction-termination status here, not a universal success flag.
A recognized schedule-activity transaction therefore returns `0x03` even when
validation or persistent storage rejects the activity internally. This keeps the
wire transaction aligned with original-device behavior while failures remain
visible through Usermod diagnostics.

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

**WLED mapping:** fill the Usermod RGB canvas and select the
registered `iDotMatrix` effect. The command no longer rewrites WLED's
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
canvas used by other app content, and the single `iDotMatrix` WLED effect
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
framebuffer. The Usermod selects its registered `iDotMatrix` 2D
effect, which copies the canvas while WLED services the current segment and has
valid virtual XY state. A valid pixel packet also selects the effect. No
physical serpentine mapping is duplicated in this module.

Complete short packets use the 64-byte queue. Larger logical FA02 packets are
reassembled with 4112 bytes of permanent inline storage and temporary dynamic
storage up to the 8192-byte logical-packet maximum before protocol dispatch.

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
`iDotMatrix` effect. The shared effect reads WLED local time and draws into the
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
`iDotMatrix`.

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
remains under `iDotMatrix`. Stopwatch state also remains independent of
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
owned by `iDotMatrix`; no native WLED colour/effect state is modified.

When date display is enabled, the integration preserves the experimentally
verified emulator presentation: 30 seconds of `HH:MM`, followed by 5 seconds of
`DD/MM`. This timing is emulator behavior and is not claimed as a universal
original-device protocol requirement.

## Logical-to-physical mapping

**WLED integration decision:** `screenType` selects the advertised logical
profile while the physical output size comes from the selected WLED 2D segment.
On the 0.9 native-matrix path, unequal logical/physical 16x16, 32x32 and 64x64
sizes are mapped automatically. Upscale uses nearest-neighbour replication;
downscale uses box averaging; equal sizes are copied 1:1. WLED remains
responsible for panel layout, rotation, mirroring, grouping, and serpentine
wiring. The historical `rescale` option is retained for legacy low-memory
storage profiles rather than as a prerequisite for output scaling.

`screenType` is also bounded by the decoder profile compiled into the firmware:

- LZW10 profile accepts only protocol profile `0x01` (16x16);
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
| 9 | 4 | complete-payload CRC42, little-endian |
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

One dedicated FA02 assembler provides **4112 bytes of permanent inline storage**
for the observed 4096-byte payload chunk plus 16-byte header. The maximum logical
FA02 packet is **8192 bytes**; packets above 4112 bytes use temporary dynamic
reassembly storage. The NimBLE callback only copies a complete ATT write into a bounded queue;
assembler ownership, allocation, timeout, logical-packet dispatch, CRC42, and
notification all happen in the normal Usermod loop. TEXT is
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

The current implementation replies `05 00 00 00 03`. No outer CRC was observed.
The decoder validates the PNG structure required by its supported subset and
validates/decompresses the embedded zlib payload; it is not a general-purpose PNG
validator and does not claim full chunk-CRC validation. This layout is a new
inference from the WLED trace, not yet a confirmed BUILD 80 or original-device
fact. The decoder accepts non-interlaced 8-bit RGB/RGBA and exact logical-profile
dimensions. The complete compact PNG envelope must fit the **8192-byte maximum
logical FA02 packet size**; 4112 bytes is only the permanent inline capacity.

## GIF payload

**Confirmed by the reference:** common type `0x01` contains a standard
`GIF87a` or `GIF89a` stream. CRC42 covers the complete compressed payload. The
WLED integration writes chunks to an RX file and starts playback only after a
valid completion and deferred RX-to-PLAY promotion.

Release 0.8.2 accepts GIF dimensions up to both the active logical screen profile
and the compiled decoder capability. The supported 16x16 classic/C3 profiles
compile `IDOT_GIF_LZW12` but set `IDOT_SCREEN_MAX_DIM=16`, so only 16x16 is
advertised/accepted there while the complete legal 4096-code LZW12 space remains
available to the decoder. The supplied 32x32 test profile uses `IDOT_GIF_LZW11`;
the 64x64 profiles use `IDOT_GIF_LZW12`. With PSRAM, LZW12 can use full
AnimatedGIF/direct playback; without PSRAM it uses the compact12/LittleFS
frame-cache backend. Backend choice is a WLED integration and memory-management
detail, not a wire-protocol difference.

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
logical image is published only after valid CRC42, then the existing display
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
and selects the existing `iDotMatrix` effect. The global colour,
background, speed, movement, and effects are rendered locally. SimSun and
SimHei require no WLED font library because the official app rasterizes them
before transmission.

## Audio / Rhythm stream

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

Every valid audio frame selects `iDotMatrix`, cancels the previous
app-originated visual mode, and renders into the existing RGB canvas. Selecting
a normal WLED effect releases audio state; a later audio frame reclaims the
display. No audio feedback is sent from WLED to the app.

### Optional local AudioReactive source

The **wire protocol is unchanged**. The app continues to send the same LEVEL and
FFT frames, and those frames continue to select the visualizer family and mode.
The new `audioSource` Usermod setting changes only where the live amplitude and
spectrum values used by the renderer come from:

- `Phone / BLE` (`0`) uses the frame values exactly as in 0.8.1;
- `WLED AudioReactive` (`1`) ignores the frame's amplitude/bands and substitutes
  processed data exported by WLED AudioReactive; if that data is unavailable,
  the renderer receives silence rather than silently changing source;
- `Auto` (`2`) prefers AudioReactive data when available and otherwise uses the
  original BLE values.

The bridge consumes AudioReactive's smoothed volume and 16 GEQ bands through
WLED's inter-Usermod data API. Adjacent GEQ pairs are averaged to produce the
eight legacy iDotMatrix bands, then mapped from `0..255` to `0..12`. This is a
renderer-input substitution only: it adds no GATT characteristic, command, ACK,
or packet format and does not sample the microphone independently.

## Deferred / unsupported

- unconfirmed TEXT marker aliases `0x03` and `0x06`;
- interlaced PNG, other PNG colour types, and PNG dimensions differing from
  the logical profile;
- GIF dimensions larger than the active logical profile or larger than the
  active logical screen profile or maximum compiled GIF decoder capability;
- device-level rotation, energy-saving, and reset commands, intentionally left
  to WLED's own configuration and control paths.

The unconfirmed TEXT aliases remain documented for protocol archaeology, but
are not planned for implementation unless a real app capture requires them.
Device-level display policy is intentionally not duplicated by the emulator.

### Timer rendering note

The countdown (`08 80`) and stopwatch (`09 80`) wire formats are unchanged. The WLED renderer now preserves millisecond state internally so the BUILD80 timer-hand phase can be reproduced above the MM:SS display; this is a rendering change only, not a protocol change.


## Alarm / program sound mapping

The wire protocol is unchanged. Alarm packets retain their per-alarm buzzer request. Program global flags retain bit 1 as the sound request. The WLED mapping intentionally differentiates them: alarms repeat the non-blocking trill for their configured duration, while a program activity emits three groups of three short trills once when the activity becomes active and does not sound continuously for the full time window.


## Device reset (`03 80`)

The recognized reset frame is:

```text
04 00 03 80
```

It is a **live iDotMatrix protocol reset**, not an ESP32/WLED reboot. The Usermod:

- erases all persistent Device Assets / Carousel slots and their manifest;
- erases all persisted alarms and alarm media;
- erases all persisted schedules/programs, staging data and schedule media;
- stops active alarm/program/buzzer ownership and clears transient iDotMatrix display content;
- preserves WLED configuration, Wi-Fi/BLE operation, system time and the last valid app time synchronization.

The normal acknowledgement is `05 00 03 80 01`. If `iDotMatrix` remains selected after reset, the normal no-Carousel standalone policy may subsequently show the Clock fallback.

## Device Assets / Carousel

Original-hardware observations confirm one persistent Device Assets bank with 12 playable slots (`0..11`). The official app may show multiple 12-item pages, but those are alternative app-side sets rather than additional physical device slots.

### Slot setup / clear

```text
11 00 02 01 0C 00 01 02 03 04 05 06 07 08 09 0A 0B
```

`CMD=0x02`, `SUB=0x01`. Byte 4 is the number of configured slots and the following bytes are the playback order. A new setup clears the existing Usermod-managed Device Assets bank before the replacement assets are uploaded.

### Bulk metadata

The normal 16-byte Bulk header is also used for Device Assets. In addition to data type, total length and CRC42:

```text
bytes 13..14  timeSign   uint16 little-endian dwell time in seconds
byte  15      imageIndex Device Assets slot index
```

`imageIndex=0..11` selects a persistent Carousel slot. `imageIndex=12` is the normal transient/show-now path and `13` is treated as transient preview content. The Usermod does not reinterpret indices above the physical 12-slot Carousel bank as additional Carousel slots.

The Usermod supports persistent GIF (`type=0x01`) and TEXT (`type=0x03`) slots, matching the mixed-content behavior observed on original 64x64 hardware. Slot data is streamed to LittleFS rather than accumulated as one complete compressed-media buffer in RAM.

### Carousel activation

The first dev.4 hardware test showed that the official app can complete a
Device Assets page upload without sending a separate reliable "enter Carousel"
command. Therefore the Usermod treats successful persistent slot transfers as
the authoritative signal: after a short quiet period following the last slot,
autonomous Carousel playback starts automatically. A following slot transfer
cancels and rearms that quiet-period timer.

`04 00 0A 01` is still accepted as an explicit compatibility enter command if
it is observed, but playback does not depend on it.

Bulk `option`, `timeSign` and `imageIndex` are latched from the first chunk of a
transfer. Continuation chunks are required to keep type, total size and CRC
stable, but are not required to repeat those Device Assets metadata bytes.

Playback continues without a BLE connection. A later transient display command
suspends runtime Carousel playback but does not delete the stored bank. WLED
controls boot behavior: a stored Carousel starts at boot when `iDotMatrix
Display` is the selected WLED boot effect.

On the no-PSRAM frame-cache backend, persistent GIF slots use a per-slot cache
(`/idot_cN.bin`). A cache survives normal Carousel rotation and is invalidated
when its slot is replaced or reset, avoiding repeated flash rewrites for
unchanged content. A slot that cannot be started is quarantined for the current
bank generation so later valid slots are not starved.
