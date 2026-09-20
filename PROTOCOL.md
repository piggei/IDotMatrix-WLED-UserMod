# Implemented iDotMatrix protocol subset

This document describes the protocol subset implemented by Release 0.9.1 / build 0.9.1-rc.1. Stable 0.9.0 remains the previous release baseline. The
BLE wire protocol is carried forward from the stable 0.9.0 WLED iDotMatrix Usermod and extended only where new original-app traffic has been confirmed. It includes the validated media/profile baseline, seven
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
pairing or application authentication, and the compatibility mode preserves
that behaviour. Nearby BLE peers may therefore issue supported commands. CRC32
checks media integrity only; it is not an authentication mechanism. Mandatory
pairing/encryption is intentionally not added because that would change
the captured wire/client contract.

| Screen type | Logical resolution |
|---|---|
| `01` | 16x16 |
| `03` | 32x32 |
| `04` | 64x64 |

## Framing and ACK

FA02 normal commands start with a 16-bit little-endian total length. The implementation queues
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
For the legacy 0.8.x device identity they are `00 08`; the internal build identifier is never encoded
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

Hardware validation established command-specific flow control:

```text
07 80 -> 05 00 07 80 01
05 80 accepted but incomplete -> 05 00 05 80 01
05 80 complete + CRC-valid + committed -> 05 00 05 80 03
05 80 rejected/failed -> 05 00 05 80 02
```

For Schedule activity uploads, `0x01` is the continuation status required by the official 64x64 app. Sending `0x03` after the first 4096-byte chunk prematurely terminates the transfer. `0x03` is therefore reserved for the final successfully committed media object.

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

## Graffiti full-raster multipart transport

**Confirmed original-app protocol:** Bluetooth capture from the 64x64 app and the
standalone emulator B171 establish a second Graffiti path for publishing a full
canvas. This format is distinct from both compact inline PNG and generic Bulk RAW.
The WLED implementation of this path is also hardware-validated on the physical
64x64 MatrixPortal/HUB75 target using the official app and several complex
photographic images.

Each Graffiti raster chunk is one complete FA02 logical packet with a 9-byte header:

| Offset | Size | Field |
|---:|---:|---|
| 0 | 2 | logical packet length, little-endian |
| 2 | 1 | content type `0x00` |
| 3 | 1 | fixed `0x00` in the confirmed capture |
| 4 | 1 | chunk marker: `0x00` first, `0x02` continuation |
| 5 | 4 | complete raster size, little-endian |
| 9 | remaining | raw row-major RGB bytes for this chunk |

The declared size is the size of the **complete raster object**, repeated in every
chunk. It is not the current chunk length. Expected complete sizes are:

| Screen type | Resolution | Complete raster bytes |
|---:|---:|---:|
| `0x01` | 16x16 | 768 |
| `0x03` | 32x32 | 3072 |
| `0x04` | 64x64 | 12288 |

The captured 64x64 transfer is exactly:

```text
packet 1: 9-byte header + 4096 RGB bytes, marker 00
          -> FA03 05 00 00 00 02
packet 2: 9-byte header + 4096 RGB bytes, marker 02
          -> FA03 05 00 00 00 02
packet 3: 9-byte header + 4096 RGB bytes, marker 02
          -> FA03 05 00 00 00 01

4096 + 4096 + 4096 = 12288 = 64 * 64 * 3
```

For this command family, status `0x02` means **accepted but incomplete** and
status `0x01` is the final completion ACK. These values are command-specific and
must not be confused with generic Bulk's `0x01` continue / `0x03` terminate rule.
No CRC field is present in the confirmed 9-byte Graffiti raster header; completion
is determined by the accumulated byte count reaching the declared full-raster size.

There are two independent fragmentation layers:

1. BLE ATT writes are reassembled into one complete FA02 logical packet by the
   normal FA02 assembler (hard maximum 8192 bytes);
2. multiple complete Graffiti FA02 packets are then accumulated into one logical
   raster object.

Therefore the existing 8192-byte FA02 maximum does **not** need to increase: the
confirmed logical packets are 4105 bytes each. The WLED implementation streams
each RGB chunk directly into the renderer's raw-image staging sink instead of
allocating another 12288-byte protocol buffer. The transfer is cancelled after a
5-second inactivity timeout and on BLE disconnect, protocol reset, or replacement
by an incompatible media transfer. A successful completion publishes the result
as Graffiti/DIY display ownership.

A continuation marker without an active matching transfer is consumed without a
completion ACK. A new first marker starts a fresh transaction and discards any
incomplete previous Graffiti raster.

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

The display uses the reconstructed original-device 16x16 artwork: a 7x10 animated hourglass on the left and stacked `MM` / `SS` digits on the right. Minutes are white, seconds are orange and turn red during the final ten seconds. The hourglass uses ten 200 ms frames and freezes on its final frame at `00:00`. Values above 99 minutes wrap visually modulo 100.

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

The visible output uses the reconstructed original-device 16x16 stopwatch artwork: white face, gray/lilac case, orange top button, red hand, white minutes and orange seconds. The red hand has eight positions at 100 ms intervals and derives its phase from elapsed time, so pause freezes the artwork naturally. Stopwatch state remains independent of native WLED effect selection.

## Scoreboard

**Confirmed protocol:**

```text
08 00 0A 80 A_lo A_hi B_lo B_hi
```

ACK: `05 00 0A 80 01`

Both scores are little-endian 16-bit values on the wire. The reconstructed original-device artwork renders each side as a three-digit row with leading zeroes (`score % 1000`): player A on rows 0..6 in `#7858F8`, player B on rows 9..15 in `#F82078`, with two blank scanlines between them. The framebuffer remains owned by `iDotMatrix`; no native WLED colour/effect state is modified.

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
logs received bytes. An early reverse-engineering build incorrectly assigned Bulk to AE01 and therefore observed no chunks; the current implementation follows the verified source behavior.

One dedicated FA02 assembler provides **4112 bytes of permanent inline storage**
for the observed 4096-byte payload chunk plus 16-byte header. The maximum logical
FA02 packet is **8192 bytes**; packets above 4112 bytes use temporary dynamic
reassembly storage. The NimBLE callback only copies a complete ATT write into a bounded queue;
assembler ownership, allocation, timeout, logical-packet dispatch, CRC32, and
notification all happen in the normal Usermod loop. TEXT accepts up to **16654 payload bytes**, matching the original-device 64-glyph 32x64 format (14-byte TEXT header + 64 x 260-byte glyph records). The TEXT payload buffer is allocated only for the active transfer, preferring PSRAM on ESP32-S3 and falling back to internal heap when required; it is not a permanent 16 KiB reservation. RAW is bounded to the 12288 bytes required by a 64x64 RGB frame. GIF is streamed to LittleFS and capped at 2 MiB here.

Matching BUILD 80, an otherwise unknown complete short FA02 command is
tolerated and receives `05 00 COMMAND SUBCOMMAND 01`.

The routed common types are `0x01` GIF, `0x02` RAW RGB, and `0x03` TEXT. Their
ACK retains the type and uses `0x01` while incomplete and `0x03` when the
transaction terminates.

## Compact PNG envelope (separate type-0 format)

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
validator and does not claim full chunk-CRC validation. The decoder accepts
non-interlaced 8-bit RGB/RGBA and exact logical-profile dimensions. The complete
compact PNG envelope must fit the **8192-byte maximum logical FA02 packet size**;
4112 bytes is only the permanent inline capacity.

This compact PNG envelope must not be confused with the confirmed Graffiti
full-raster transport above. Both use type `0x00`, but compact PNG is a complete
single logical packet whose declared size equals the bytes at offset 9 and whose
payload begins with the PNG signature `89 50 4E 47`. Graffiti full-raster instead
uses marker `0x00`/`0x02`, repeats the complete raw-RGB raster size, and may span
multiple FA02 logical packets.

## GIF payload

**Confirmed by the reference:** common type `0x01` contains a standard
`GIF87a` or `GIF89a` stream. CRC32 covers the complete compressed payload. The
WLED integration writes chunks to an RX file and starts playback only after a
valid completion and deferred RX-to-PLAY promotion.

The current implementation accepts GIF dimensions up to both the active logical screen profile
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
logical image is published only after valid CRC32, then the existing display
effect lets WLED apply its configured matrix mapping or the optional rescale.

## TEXT payload and glyph records

**Confirmed payload header:**

| Offset | Field |
|---:|---|
| 0 | glyph count |
| 1..3 | not yet documented |
| 4 | movement/effect |
| 5 | speed (`0..100`; base cadence mapped to 500..15 ms per logical pixel; native 64x64 positional TEXT may use two pixels per accepted render in the final 10% to exceed the WLED frame-rate ceiling) |
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
| `0x08` / `0x09` | supported 64-pixel family | 32x64 | 256 bytes | 260 bytes |

The 0.9 implementation accepts the confirmed/reference 8x16 and 16x32 marker families
and the 32x64 cell required by the app's 64-pixel TEXT size. For the 32x64
case it also accepts a marker variant only when the complete payload has an exact
260-byte-per-glyph record structure; this avoids silently treating arbitrary
unknown TEXT formats as valid. Bitmap rows are consecutive, and the
least-significant bit is the leftmost pixel within each byte. Mixed glyph sizes
in one payload have not been observed and are not supported.

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

The countdown (`08 80`) and stopwatch (`09 80`) wire formats are unchanged. The renderer uses millisecond state only for the original-device animation phase; the current renderer uses separate reconstructed countdown/hourglass and stopwatch artwork instead of the legacy shared timer icon. This is a rendering change only, not a protocol change.


## Alarm / Program multipart media

64x64 captures established that `mediaSize` in Alarm (`00 80`, 24-byte header) and Program activity (`05 80`, 23-byte header) packets is the **total media size**, not the payload size of the current logical packet. The app may send multiple complete logical packets, each repeating its metadata header. An observed Alarm carrying 6706 media bytes arrived as 4096 bytes followed by 2610 bytes.

For Schedule activity packets, offset 10 is an **8-bit content type** and offset 11 is a separate per-chunk marker. Observed markers are `0x00` for the first chunk and `0x02` for continuation chunks. The marker is transport framing and is not part of media identity; interpreting bytes 10..11 as LE16 would incorrectly transform type `0x01` into `0x0201` on continuation packets and reset the transfer.

The receiver keeps independent Alarm and Program media transactions. Schedule chunks are associated by activity index + content type + total `mediaSize` + full-media CRC. A transaction is committed only when `received == mediaSize` and CRC32 over the complete assembled asset matches `mediaCRC`. Metadata mismatch, overflow, allocation failure, 5-second inactivity timeout or final CRC failure aborts only the in-progress transaction; the previously committed Alarm/Program remains intact. A defensive 512 KiB per-asset limit prevents unbounded allocation.

This assembly is above FA02 transport reassembly: BLE fragmentation first produces one complete logical packet, then the automation multipart layer joins multiple such logical Alarm/Program packets into one media asset. Generic FA02 framing is unchanged.

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

The normal 16-byte Bulk header is also used for Device Assets. In addition to data type, total length and CRC32:

```text
bytes 13..14  timeSign   uint16 little-endian dwell time in seconds
byte  15      imageIndex Device Assets slot index
```

`imageIndex=0..11` selects a persistent Carousel slot. `imageIndex=12` is the normal transient/show-now path and `13` is treated as transient preview content. The Usermod does not reinterpret indices above the physical 12-slot Carousel bank as additional Carousel slots.

The Usermod supports persistent GIF (`type=0x01`) and TEXT (`type=0x03`) slots, matching the mixed-content behavior observed on original 64x64 hardware. Slot data is streamed to LittleFS rather than accumulated as one complete compressed-media buffer in RAM.

### Carousel activation

Hardware testing showed that the official app can complete a
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

## Preset / Default (`06 02`)

The app's Preset / Default page is a dedicated temporary playlist, not a second persistent Carousel. Media objects are uploaded through the normal 16-byte Bulk header using protocol slots `14..19`. Types observed and supported are `0x01` (GIF/image media) and `0x03` (TEXT). The Bulk `timeSign` field is retained as opaque metadata; the value `5` observed in Preset traffic is **not** interpreted as a five-second dwell.

Large Preset objects use normal repeated-header Bulk continuation: marker byte 4 is `0x00` on the first packet and `0x02` on continuation packets, total size/CRC describe the complete object, ACK status is `0x01` while incomplete and `0x03` when the complete CRC-valid object has been staged. The continuation marker is not part of object identity.

Activation format:

```text
[lengthLE16] 06 02 <count> <slot1> ... <slotN>
```

`count` is `1..6`; every slot must be in `14..19`. The command contains only playback order. Uploads are staged without changing the active display. On `06/02`, pending media for the selected slots are promoted and playback restarts from the first requested entry. The playlist loops cyclically. Image/GIF entries use ~3000 ms visible dwell; TEXT duration is derived from the existing text renderer so horizontal scrolling completes before advancing and static/page-like presentation retains the final hold. Preset files are volatile and are cleared at boot/reset.

### Preset upload indicator
The visual upload indicator is local UI only. It starts on the first accepted Bulk object routed to slot 14..19, remains indeterminate across subsequent Preset objects, and ends on `06/02` activation (or the local interrupted-upload timeout). It does not change Bulk ACK `0x01/0x03`, CRC, marker, or slot semantics.
