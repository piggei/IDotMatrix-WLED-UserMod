# Architecture

This document describes the **0.9 architecture** used by release candidate
`0.9.0-rc.3`. It retains the qualified 0.8.2 ESP32/ESP32-C3 foundations and
extends them with the ESP32-S3 / PSRAM / native WLED HUB75 path, universal
16/32/64 logical-to-physical scaling, multi-packet Alarm/Program media, and the
volatile Preset / Default bank.

## Design goals

The Usermod keeps BLE transport, reverse-engineered protocol semantics, media
handling, rendering, and WLED state changes separate.

The main rules are:

- BLE callbacks stay short and bounded;
- filesystem access, CRC-heavy work, allocation, decoding, and WLED state changes run in normal WLED loop/effect context;
- WLED remains the authority for physical matrix mapping, power, brightness, time, and normal effects;
- large logical profiles must not permanently reserve large internal-DRAM buffers when a smaller physical canvas is sufficient;
- allocation failure must degrade to a media error/recovery path, not WLED Error 8, a reboot, or a dead Web UI;
- a legal 12-bit GIF stream must never be made unsafe by physically truncating the 4096-code LZW dictionary.

## Components

### `IDotMatrixBLEServer`

Owns NimBLE, the FA/AE GATT database, advertising, reconnection, delayed device
information notifications, complete-ATT-write queuing, and FA02 logical-packet
reassembly. NimBLE callbacks never own the FA02 assembler: they only enqueue
bounded immutable writes/connection events; the WLED loop owns assembler state,
allocation, timeout and transfer cancellation. It requests MTU 517 so the official app can use large ATT payloads.
It does not render pixels or directly change WLED state.

### `IDotMatrixFA02Assembler`

Owns one bounded FA02 logical-packet assembler with **4112 bytes of permanent
inline storage** (`4096 + 16`) and a maximum logical packet size of **8192
bytes**. Packets above the inline capacity use temporary dynamic storage only for
the lifetime of that reassembly. The first two little-endian bytes define the
complete FA02 packet length. ATT fragments are appended until the logical packet
is complete. No protocol ACK is emitted for an ATT fragment. An abandoned
partial transfer is cleared after five seconds so later commands cannot be
poisoned by stale reassembly state.

### `IDotMatrixBulkTransfer`

Validates the common bulk header, enforces sequential chunks, calculates CRC32,
and exposes decoded chunk spans. TEXT is retained in a bounded buffer; RAW and
GIF are streamed onward chunk by chunk.

### `IDotMatrixProtocol`

Owns WLED-independent command validation, typed command decoding, TEXT record
parsing, ACK generation, device-information replies, and the media-sink
boundary. It is host-testable.

### `IDotMatrixRenderer`

Owns RGB storage, app Solid colour, the seven standalone light effects, graffiti
pixels, clock artwork, text glyphs, RAW image assembly, and animation-frame
publication.

A pixel is exactly three bytes. If logical and storage dimensions match, the
canvas sizes are:

| Resolution | RGB storage |
|---:|---:|
| 16x16 | 768 B |
| 32x32 | 3,072 B |
| 64x64 | 12,288 B |

From the 0.7.1 low-memory rescale work, logical protocol dimensions and storage
dimensions can differ. With `screenType=64x64`, `rescale=true`, and a physical
16x16 WLED matrix, the renderer stores only a 16x16 canvas (768 B) while keeping
64x64 as the BLE/media source geometry.

RAW input is sampled as bytes arrive. GIF/source pixels are sampled while they
are decoded. The renderer reuses its normal canvas for animation rather than
keeping a second full animation framebuffer.

Large pixel allocations prefer PSRAM when it is present.

For the 0.9 native-matrix path, output scaling is automatic and bidirectional.
Logical 16/32/64 canvases may target physical 16/32/64 WLED matrices. Enlargement
uses nearest-neighbour replication; reduction uses box averaging. This output
policy is separate from the historical low-memory `rescale=true` storage mode.

### `IDotMatrixMedia`

Owns compact PNG decoding, GIF RX/PLAY files, decoder lifetime, and the optional
LittleFS frame cache.

The GIF build profile is selected at compile time. In Release 0.8.2 the
**supported 16x16 profiles compile `IDOT_GIF_LZW12`** and independently cap the
visible protocol/UI profile with `IDOT_SCREEN_MAX_DIM=16`:

- supported 16x16: LZW12 capability + 16x16 screen cap; on no-PSRAM targets the runtime backend is `compact12/cache`;
- `IDOT_GIF_LZW11`: compact 11-bit/32x32 AnimatedGIF profile used by the supplied 32x32 test profile;
- `IDOT_GIF_LZW12`: complete 12-bit/64x64 decoder capability used by 16x16 and 64x64 profiles, with the allowed screen size controlled separately.

For `IDOT_GIF_LZW12`, the final backend is selected at runtime:

- PSRAM detected -> full AnimatedGIF direct playback, allocation preferring PSRAM;
- no PSRAM -> `IDotMatrixCompactGif` + LittleFS frame-cache path.

### `IDotMatrixCompactGif`

A dedicated no-PSRAM 64x64 predecoder. It implements GIF87a/GIF89a parsing and
keeps the complete 4096-code 12-bit LZW space while packing the largest tables
more tightly than AnimatedGIF.

It is used only during cache construction and is destroyed before playback.

### `IDotMatrixWLEDAdapter`

Is the WLED boundary. It maps power/brightness and app-content ownership to WLED.
It registers one effect named `iDotMatrix`, reads WLED local time for
clocks, and applies WLED's configured 2D mapping/rescale when emitting pixels.
App Solid and light effects stay in the renderer rather than being translated
into native WLED effect/colour state. Explicit iDotMatrix ownership also terminates an active WLED playlist and clears
any playlist preset already queued for asynchronous application before selecting
the framebuffer effect. This mirrors WLED's direct-effect semantics while also
accounting for WLED's deferred `handlePresets()` pass after the Usermod loop.

It also owns GIF staging/recovery rules. On the classic-ESP32 frame-cache path,
WLED `Static` is used internally while the cache is prepared because it has a
small RAM footprint. The selected segment is temporarily blanked so this
internal staging state is not shown to the user.

### `usermod_idotmatrix.cpp`

Owns Usermod lifecycle, settings, the I2S/RMT safety guard, Wi-Fi modem-sleep
requirement, delayed BLE startup, reset/heap crash-snapshot diagnostics, and
compact runtime status under `/json/info`.

## Execution model

1. WLED initializes LEDs and Wi-Fi.
2. The Usermod allocates the renderer storage canvas and waits five seconds.
3. NimBLE starts and advertises the iDotMatrix-compatible GATT database.

### ESP32-C3 supported backend split

Release 0.8.2 retains the WLED 16.0.1/NimBLE 1.x path for classic ESP32 and
retains the separately qualified C3 path on pinned WLED commit `d55037f...`. Legacy
IDF4 RMT builds produced physical LED spikes both with and without BLE, with BLE
advertising making the fault much more visible. The IDF5 WLED backend uses
`WLED_USE_SHARED_RMT`; on the tested C3 this eliminated the spikes through BLE
advertising/connection, images, GIF playback and sustained WLED/media switching.

NimBLE-Arduino 2.x changes callback signatures and GATT start semantics, so the
BLE server contains a compile-time API bridge. Classic profiles remain on
NimBLE 1.4.3; the C3 profile pins 2.5.1. Protocol parsing and rendering are shared.

## State ownership

| State | Authority |
|---|---|
| Power / brightness | WLED |
| App Solid colour | `IDotMatrixRenderer` canvas |
| BLE logical profile | Usermod configuration |
| Renderer storage dimensions | physical WLED matrix when low-memory rescale is active |
| Graffiti / RAW / PNG / GIF visible pixels | `IDotMatrixRenderer` |
| Clock time, timezone, DST | WLED time subsystem |
| Clock style / text style / light-effect state | last valid app command |
| Countdown / stopwatch state | emulated-device state in `IDotMatrixWLEDAdapter` |
| Scoreboard values | last valid app command |
| Physical XY/serpentine mapping | WLED matrix configuration |
| WLED playlist lifecycle | WLED; terminated and any queued playlist preset cancelled when iDotMatrix explicitly reclaims display ownership |

### Control-source isolation and user experience

The official iDotMatrix app is effectively a one-way controller in this
integration: it sends commands, but there is no confirmed general mechanism for
WLED-side state changes to be pushed back into the app. Treating an iDotMatrix
light effect as the corresponding native WLED effect would therefore create a
misleading UI. For example, the app could still show a red strobe after the user
changed the WLED strobe to blue. Both programs would be internally consistent,
but the combined user experience would look desynchronized.

In stable 0.8.2, **all app-originated visual content stays under one
WLED effect: `iDotMatrix`**. That includes Solid colour and the seven
standalone light effects in addition to graffiti, clock, text, images and GIFs.
The WLED effect is only a framebuffer publisher; it does not expose the app
content as a native WLED Solid/Strobe/etc. state.

The resulting UX rule is deliberately simple:

```text
use iDotMatrix app -> iDotMatrix publishes app framebuffer
select a WLED effect -> WLED takes the display as an explicit source change
next supported iDotMatrix content command -> iDotMatrix takes it back
```

No attempt is made to synthesize a hybrid state such as "iDot effect + WLED
colour + iDot speed". Power and master brightness remain WLED-level device
controls because they affect the physical output independently of the content
renderer. The only intentional internal exception is no-PSRAM GIF precache: it
briefly uses WLED `Static` for RAM headroom while forcing the physical output
black, then restores the previous WLED primary colour before the framebuffer
effect is shown again.

### Standalone light-effect renderer

The seven effect algorithms are ported from the standalone ESP32 emulator rather
than mapped to native WLED effects. The renderer keeps only a bounded palette
(maximum 16 RGB colours), effect id, speed, and timing metadata. The existing
RGB canvas is reused for every frame. There is no second framebuffer, no
per-pixel effect-state array, and no effect-specific heap allocation.

For scrolling effects 3, 4 and 5, the animation position is deliberately a
frame counter rather than a value derived from elapsed wall-clock time. Each
accepted renderer update advances the pattern by exactly one physical canvas
pixel; app speed changes only the minimum interval between updates. If WLED,
Wi-Fi, BLE or filesystem work delays a loop iteration, the effect may momentarily
slow down but it cannot catch up by visibly jumping several pixels.

On the host C++11 ABI used by the regression suite, adding the complete effect
state increases `sizeof(IDotMatrixRenderer)` from 96 to 160 bytes. The exact ESP32
alignment may differ, but the important property is architectural: persistent
RAM growth is tens of bytes, not kilobytes, and does not touch the GIF decoder
or frame-cache budgets that dominated 0.7.1.

### Timers and scoreboard

Countdown, stopwatch, and scoreboard are also rendered locally under `iDotMatrix`; they never map to native WLED effects. Countdown and stopwatch now use separate reconstructed original-device 16x16 artworks with stacked `MM` / `SS` digits, while scoreboard draws two full-width three-digit rows with leading zeroes. The artwork is scaled through the same logical/physical path already used by the clock, so no additional framebuffer is allocated.

Timer **display ownership** and timer **device state** are deliberately separate.
If the user selects a native WLED effect while a countdown or stopwatch is
running, WLED takes the panel but the emulated timer keeps advancing in the
background. This matches the one-way-controller model better than freezing the
timer invisibly: the iDotMatrix app has not been told that WLED took over and
therefore still expects its timer state to be valid. A later pause/resume/start
command from the app reclaims `iDotMatrix` using that retained state.

Countdown completion is the first implemented asynchronous app-facing event: the
adapter latches a one-bit completion flag, the protocol converts it to
`05 00 08 80 03`, and the BLE server emits it on FA03 from the normal WLED loop
context. No NimBLE callback performs rendering or timer-state mutation.

The timer/scoreboard feature adds only bounded scalar state (booleans, timestamps,
and two 16-bit scores) and uses no heap allocation, decoder workspace, filesystem
cache, or per-pixel persistent state.

### PlatformIO hardware targets and Usermod inheritance

The PlatformIO layer is deliberately orthogonal to the media/protocol profile.
`platformio_override.ini.example`, `.32x32`, and `.64x64` each expose the same
classic-ESP32 and ESP32-S3 hardware target matrix while changing only the maximum
iDotMatrix decoder profile. `platformio_override.ini.hub75` instead wraps WLED's
board/pinout-specific HUB75 environments. See `BUILD_PROFILES.md` for the full
matrix and validation status.

The normal supplied overrides intentionally **replace** the base environment's
`custom_usermods` list with only:

```ini
custom_usermods =
  symlink://../wled-usermod-idotmatrix
```

They do not inherit `${env:<base>.custom_usermods}`. The explicit exception in
0.8.2 is `platformio_override.ini.c3-audio`, which deliberately contains:

```ini
custom_usermods =
  audioreactive
  symlink://../wled-usermod-idotmatrix
```

This keeps the standard C3 memory budget unchanged while providing a separate
test build for local microphone data. No profile silently inherits arbitrary
base Usermods.

## Current memory rules

- four 517-byte queue slots for complete ATT writes (matching the configured maximum local MTU);
- one 4112-byte inline FA02 reassembly buffer, with temporary dynamic storage for logical packets above 4112 bytes and an 8192-byte hard maximum;
- one persistent RGB renderer canvas, sized to storage dimensions rather than automatically to logical dimensions;
- standalone light effects reuse that canvas and add only a bounded 16-colour palette plus small timing/state fields;
- countdown, stopwatch, and scoreboard reuse the same canvas and add only scalar timer/score state;
- no second permanent GIF animation framebuffer;
- RAW prefers a temporary atomic-publication buffer but can receive in-place/hidden if a second canvas cannot be allocated;
- TEXT storage is bounded and allocated outside BLE callbacks;
- compressed GIF bytes are streamed to LittleFS instead of being buffered in RAM;
- supported 16x16 profiles compile LZW12 while `IDOT_SCREEN_MAX_DIM=16` limits the visible logical profile; on no-PSRAM hardware the runtime backend is compact12/cache;
- the 11-bit/32x32 decoder is allocated on demand and released when WLED retakes display ownership;
- the validated compact 11-bit profile uses a 1282-entry physical dictionary and measured `gifDecoderBytes=9372` on the tested classic ESP32;
- 64x64 semantics always retain all 4096 legal LZW codes;
- PSRAM-capable 64x64 builds use the full AnimatedGIF path; no-PSRAM 64x64 builds use the compact predecoder and frame cache;
- PNG/miniz scratch memory is temporary;
- dynamic media resources are released on WLED takeover and on replacement/recovery paths.

## Low-memory logical-to-physical storage

The largest persistent win in 0.7.1 is separating **logical profile** from
**storage canvas**.

For a 64x64 logical source shown on a 16x16 physical matrix:

```text
logical RGB canvas if stored at full size: 64 * 64 * 3 = 12,288 B
physical storage canvas:                  16 * 16 * 3 =    768 B
persistent saving:                                      11,520 B
```

RAW packets still contain the complete 64x64 row-major RGB source and are
validated at their real logical byte count. Only source pixels selected by the
nearest-neighbour destination grid are retained.

GIF decoders likewise see a legal 64x64 stream. Downsampling changes storage,
not protocol semantics or LZW code validity.

## GIF backend selection

### 16x16

Release 0.8.2 does not use the old 10-bit decoder as the standard 16x16
profile. The supported classic-ESP32 and ESP32-C3 16x16 overrides compile
`IDOT_GIF_LZW12` and set `IDOT_SCREEN_MAX_DIM=16`. Decoder capability and
advertised screen size are therefore separate: the logical/UI profile remains
16x16 while no-PSRAM playback uses the validated `compact12/cache` backend.

### 32x32

The 11-bit profile uses a compact physical dictionary sized for the maximum code
creation possible within a 32x32 frame. Its decoder is allocated only while GIF
playback owns the display, and it is released immediately when normal WLED
effects retake ownership.

Hardware validation measured `gifDecoderBytes=9372`, with ten consecutive GIFs
passing on classic ESP32 and heap recovering after return to WLED.

### 64x64 with PSRAM

When PSRAM is detected, the full AnimatedGIF 12-bit path is selected. The
allocator first requests PSRAM for the decoder object. This preserves direct
playback and avoids LittleFS frame-cache writes.

This path is hardware-validated for 0.9 on Adafruit MatrixPortal ESP32-S3 with 2 MB PSRAM and WLED 0.17 native HUB75 output. Repeated animated GIF transfers and Carousel playback use `animatedgif12/psram` without the no-PSRAM frame-cache path.

### 64x64 without PSRAM

The final classic-ESP32 path is:

```text
BLE GIF transfer
    -> CRC/length validation
    -> promote RX file to /idot_play.gif
    -> blank physical segment + stage WLED Static
    -> allocate compact12 workspace
    -> decode one frame per WLED loop
    -> downscale into physical renderer canvas
    -> append delay + RGB frame to /idot_cache.bin
    -> release compact decoder/workspace
    -> open cache for reading
    -> activate iDotMatrix
    -> cached playback
```

The transient cache is capped at 512 KiB. Carousel GIFs instead use per-slot
persistent cache files (`/idot_cN.bin`) that are reused across normal rotation
and invalidated only when the slot changes, reset removes the bank, or validation
fails.

## Compact-safe LZW12 workspace

`IDotMatrixCompactGif` keeps the complete 4096-code LZW space with this layout:

| Structure | Bytes |
|---|---:|
| packed 4096-entry prefix table, 12 bits/entry | 6,144 |
| suffix table | 4,096 |
| reverse stack | 4,096 |
| global RGB565 palette | 512 |
| local RGB565 palette | 512 |
| disposal-3 backup | one physical RGB frame |

The fixed portion is 15,360 bytes. On a 16x16 physical canvas the disposal backup
is 768 bytes, for a **16,128-byte** workspace.

The full AnimatedGIF 12-bit object measured about **20,660 bytes** with the
validated toolchain, so the compact backend recovers roughly **4.5 KiB** at the
critical predecode moment without sacrificing any legal LZW code.

## RAM admission and runtime guards

Two different memory checks are intentional.

### Predecode admission

Before allocating the compact workspace, the no-PSRAM path requires:

```text
free internal heap >= workspace + 10 KiB reserve
largest free block  >= workspace
```

The 10 KiB reserve is a **total free-heap reserve**; it does not need to be
contiguous with the decoder. The largest-block check exists only because the
workspace itself is one contiguous allocation.

### Cache-build runtime guard

While the decoder is alive, short-lived Wi-Fi/BLE/WebSocket/LittleFS allocations
can temporarily push free heap below the normal margin. A single low sample is
not treated as OOM.

If free heap falls below **9 KiB**:

1. do not decode another frame in that loop turn;
2. return control to WLED/network processing;
3. retry later;
4. fail with `gif-ram-reserve` only if the low-heap condition remains continuous for about **2 seconds**.

This wait/retry rule eliminated nondeterministic cases where the same GIF could
succeed once and fail on the next attempt solely because a transient network
buffer happened to exist at the instant of the guard check.

## GIF replacement as a transaction

A new GIF does not destroy a currently playing GIF while bytes are still being
received. The replacement becomes a transaction boundary only after length and
CRC validation succeed.

For the frame-cache path, 0.8.2 keeps the previous committed GIF recoverable until
the candidate is fully prepared:

1. receive the candidate into an alternating RX slot and validate length + CRC;
2. preserve the currently committed source/cache pair and build the candidate
   cache as `/idot_cache.new`;
3. only after the candidate cache is complete, move the current
   `/idot_play.gif` and `/idot_cache.bin` to short-lived `.bak` files;
4. promote the candidate source/cache pair;
5. reopen the promoted cache successfully before deleting the backups;
6. on any preparation or commit failure, discard the candidate and reopen the
   previous source/cache pair.

Thus a CRC-valid but undecodable GIF, a cache-build failure, or a commit failure
does not destroy known-good playback. Carousel assets use their own persistent
per-slot source/cache pair and do not pass through `/idot_play.gif`; a slot that
fails asynchronously during decoder/cache preparation is quarantined for the
current Carousel session and later valid slots are tried.

This lifecycle also fixes the hardware pattern where repeated A/B GIF swaps
would eventually stop working until the user manually selected Solid.

## Blank staging

`Static` is useful during no-PSRAM precache because it minimizes WLED effect-side
RAM. Showing the actual Static primary colour, however, produced a distracting
flash between the previous content and the GIF.

0.7.1 therefore:

1. saves the selected segment's primary colour;
2. temporarily sets the staging colour to black and blanks the segment;
3. prepares the cache under WLED `Static`;
4. restores the saved primary colour before `iDotMatrix` playback or failure recovery.

Global WLED power/brightness is not toggled, so this is a transient presentation
detail rather than a persistent OFF state.

## Memory engineering history: failures, causes, and fixes

The final design came from repeated hardware failures on a classic ESP32 running
WLED, Wi-Fi, NimBLE, filesystem I/O, and GIF decoding concurrently. These
experiments are worth preserving because several apparently reasonable RAM
optimizations were unsafe or simply moved the failure elsewhere.

| Experiment / symptom | Root cause | Final lesson / fix |
|---|---|---|
| Stock/full AnimatedGIF allocated late failed despite reasonable total heap | heap fragmentation; largest contiguous block was too small | track both total heap and largest block; reduce decoder footprint |
| Full decoder placed permanently in static DRAM overflowed `.dram0.bss` | too much permanent internal RAM removed from WLED/network | historically keep only the small decoder fixed; the current LZW12 standard instead allocates compact/full decoder workspace lazily |
| Reserving the full large decoder too early caused reset loops | WLED/Wi-Fi/BLE lost working RAM before they initialized normally | allocate media resources lazily, only after a valid transfer needs them |
| 11-bit decoder remained allocated after returning to normal WLED effects, causing WLED Error 8 | display ownership changed but dynamic decoder lifetime did not | release media/decoder immediately when WLED retakes the segment |
| 64x64 logical framebuffer plus animation buffering consumed too much RAM | 12,288 B per RGB64 canvas, duplicated by naive animation buffering | reuse one canvas and, with rescale, store only the physical canvas |
| Experimental 12-bit dictionaries truncated to 2560/2304 entries appeared to save RAM, then produced panic/corrupt heap values | a valid 12-bit stream can reference dictionary codes up to 4095; physical arrays were too small | **never truncate the legal 4096-code LZW12 space**; compress representation instead |
| Safe full LZW12 (~20.6 KiB) could play a GIF but the Web UI stopped responding | decoder stayed resident during playback and transient network allocations had too little internal heap | predecode to LittleFS, release decoder, then play cached frames |
| Full 20.6 KiB decoder was still too large during the cache-build phase | frame caching fixed playback RAM, not predecode RAM | implement compact-safe 4096-code decoder (16,128 B on 16x16 storage) |
| Decoder allocated before WLED effect staging could pass a RAM probe and then reboot when WLED allocated effect memory | allocation order did not include future WLED effect cost | stage the low-RAM WLED state first, then measure/allocate decoder resources |
| Instantaneous 9 KiB guard caused identical GIFs to alternate between success/failure | short-lived Wi-Fi/BLE/WebSocket/LittleFS allocations crossed the threshold | treat the guard as a scheduling threshold and wait/retry for up to ~2 s |
| Repeated GIF replacement eventually required manual Solid to recover | previous cache/ownership resources were not retired at the correct transaction boundary | retire prior playback only after CRC-valid replacement; make failure recovery explicit |
| Low-RAM `Static` staging was technically correct but visually annoying | the required internal WLED state was exposed to the panel | blank only the physical presentation during staging, then restore colour atomically |

### Why the unsafe truncated LZW12 experiments matter

The dangerous dev.4/dev.5 approach retained 12-bit code-width handling but made
the physical dictionary smaller than the legal code space. Complex GIFs could
then index beyond the arrays. Hardware symptoms included panics and impossible
heap diagnostics, which are classic signs of memory corruption rather than a
normal OOM.

The final compact decoder solves the same RAM problem differently: **all 4096
entries still exist**, but the prefix table is stored at 12 bits per entry and
other storage is sized to the actual physical frame.

## Representative hardware observations

These numbers are diagnostic examples from the validated classic ESP32, not
hard minimum requirements; WLED configuration and transient network activity
change them.

| Test | Representative result |
|---|---|
| 32x32 logical -> 16x16, repeated GIFs | `gifDecoderBytes=9372`; recorded minimum heap 7904 B; return to WLED recovered about 33.5 KiB free / 28.6 KiB largest block |
| 64x64 logical -> 16x16, final compact backend | `gifDecoderBytes=16128`; large and 100-frame GIFs cached successfully |
| repeated 64x64 A/B replacement | 10+ swaps passed without manual Solid reset; Web UI remained responsive |
| representative compact-cache GIF sample | `heap=33076 min=7468 largest=26612`, `gifCachedFrames=20`, `content=gif` |
| transition stress ending in WLED | cache wait diagnostics observed (`guard=9216`) while WLED remained responsive and recovered |

The `min` value is historical since boot. It may be much lower than current free
heap because predecode intentionally creates the highest transient pressure.
Current `heap`/`largest` during cached playback are more useful for judging
whether WLED/network headroom has recovered.

## Compile-time resolution capability (0.8.1)

`patch_animatedgif_profiles.py` exports `IDOT_GIF_MAX_DIM` together with the
selected LZW profile. `IDotMatrixBuildProfile.h` is the single C++ policy layer
that converts that value into supported `screenType` choices, rescale availability
and migration of older configuration values.

The normal 16x16 build omits the rescale key from generated Usermod configuration
and forces its runtime value off. LZW11 exposes 16x16/32x32; LZW12 exposes
16x16/32x32/64x64. This prevents BLE from advertising a logical display whose
GIF decoder was not compiled into the firmware. It also keeps UI visibility,
stored configuration and runtime behavior governed by the same capability.

## Validation boundaries and future work

The 0.8.1 hardware matrix covers the classic-ESP32 no-PSRAM 64x64 **logical**
profile when rescaled to a physical 16x16 matrix, the complete feature set on
the classic 16x16 platform, and the supported ESP32-C3 16x16 IDF5/shared-RMT
path with BLE active. It does not yet prove:

- the automatic PSRAM/direct path on real PSRAM hardware;
- a native physical 64x64 RGB canvas;
- HUB75 DMA plus BLE/media memory pressure;
- every optional WLED integration combined with the 64x64 profile.

Those tests should preserve the invariants above rather than reintroducing
unsafe dictionary truncation or permanent large DRAM allocations.

## Buzzer hardware boundary

The optional buzzer is owned by the iDotMatrix usermod rather than the WLED effect engine. `IDotMatrixBuzzer` is a hardware-agnostic non-blocking pattern state machine; the usermod maps its logical ON/OFF output to the configured GPIO and active-high/active-low polarity. This keeps alarm/schedule semantics separate from the physical backend and leaves a clean path for a future passive/PWM implementation.

The Usermod settings page can request a **one-shot test trill** through a small same-origin POST endpoint. The endpoint never changes persistent configuration and only operates on the GPIO/polarity already applied by WLED, so testing cannot silently drive an unsaved or conflicting pin. The one-shot path stops after three beeps; alarms use the repeating pattern, while programs use a finite multi-group activation notice.

The active pattern matches the standalone emulator: three 90 ms pulses, 70 ms gaps, then a 550 ms pause. No `delay()` is used. Direct GPIO drive of a particular buzzer is an electrical hardware property rather than a target-support requirement; the C3 release validation does not claim a specific 5 V buzzer can be driven directly from a 3.3 V GPIO.

WLED 0.16.x requires a compile-time `PinOwner` enum value for true PinManager ownership, which an out-of-tree library cannot add safely. The module therefore does not borrow another usermod's owner. The configuration key ends in `pin` so WLED's Usermods settings page includes it in its pin-use scan, and runtime setup rejects GPIOs already allocated by WLED. If WLED later adds external PinOwner registration, only the hardware setup/teardown boundary needs to change.

## Countdown, stopwatch and scoreboard artwork

Dev.27 ports the reconstructed original-device B154 visuals without changing protocol state. Countdown uses a 7x10 hourglass with ten 200 ms frames, white minutes, gray seconds and red seconds during the final ten seconds; at `00:00` the last hourglass frame remains visible. Stopwatch uses an independent 7x9 face with orange button, gray/lilac case, white dial, red hand and orange seconds. Its eight hand positions advance every 100 ms from elapsed time, which naturally freezes the hand while paused. Scoreboard uses two 4x7 three-digit rows (`000..999`) with player A at the top in `#7858F8` and player B at the bottom in `#F82078`. All three are composed on the legacy 16x16 canvas and then use the normal logical/physical scaling path.


## Alarm and program buzzer semantics

The buzzer has two deliberately different automation semantics:

- an alarm with its buzzer flag set owns the buzzer for the alarm lifetime and repeats the three-pulse trill until the alarm ends;
- a program/schedule with global sound enabled emits only an activation notice: three groups of three short trills, then remains silent for the rest of the activity window.

Program sound is started only by a real `startScheduleActivity()` transition. The normal automation loop never restarts the finite notice simply because the schedule remains active. If an alarm fires while the finite program notice is still playing, the alarm takes priority and switches the buzzer to its repeating pattern. The implementation remains non-blocking and does not change display ownership.

## Audio/Rhythm stream, source selection and rendering

Audio FA02 traffic bypasses the ordinary length-prefixed command assembler.
LEVEL frames are six bytes; FFT uses a continuous sequence of 21-byte logical
frames even when ATT writes split a logical frame. A 21-byte carry buffer in
`IDotMatrixProtocol` performs resynchronisation and publishes only complete,
valid frames from WLED's main loop. The wire protocol is unchanged in
0.8.2.

`IDotMatrixAudioSource` adds source selection **after** BLE parsing. `Phone /
BLE` preserves the 0.8.1 data path. `WLED AudioReactive` asks the registered
AudioReactive Usermod for its exported `um_data_t`; slot 0 supplies smoothed
volume and slot 2 supplies the 16-byte GEQ/FFT array. `Auto` prefers that local
data when available and otherwise falls back to the original BLE samples. An
explicit AudioReactive selection does not silently fall back: if the Usermod is
absent or disabled, it supplies silence until local data becomes available.

The local path does not initialize I2S and does not perform an additional FFT.
WLED AudioReactive owns acquisition and signal processing. The bridge averages
adjacent pairs of its 16 GEQ bands to retain the full frequency range, then
scales those eight values and the smoothed level into the existing iDotMatrix
0..12 renderer domain. BLE Audio/Rhythm frames continue to select LEVEL versus
FFT and visualizer mode, so no new BLE command or application behaviour is
required.

`IDotMatrixWLEDAdapter` stores only the current family, mode, level and eight
FFT bands. With local override enabled, incoming BLE frames update family/mode
but cannot overwrite the locally supplied level/bands. All ten renderers draw a
temporary 16x16 legacy canvas on the stack and scale it immediately into the
existing renderer canvas. There is no persistent second framebuffer. Animated
visualizers are refreshed at an 80 ms cadence while `iDotMatrix` owns
the selected segment; local AudioReactive data is sampled at a 40 ms cadence.

## 0.9 hardware transition

The 0.9 line keeps the stable 0.8.2 protocol and ownership architecture
but moves the primary hardware target to Adafruit MatrixPortal ESP32-S3, WLED's
native HUB75 backend, a 64x64 logical/physical matrix and PSRAM-backed direct GIF
playback. HUB75 remains a WLED output backend; the Usermod continues to render into
WLED's pixel/segment model rather than driving HUB75 pins directly.

## 0.9 native-matrix scaling

The iDotMatrix profile defines the logical protocol/rendering canvas, while WLED defines the physical 2D output. Output scaling is automatic and bidirectional at the final WLED segment emission stage for the 16x16, 32x32 and 64x64 profile family: enlargement uses nearest-neighbour replication and reduction uses box averaging. This preserves one renderer/protocol path for Clock, TEXT, images, GIF and procedural content and keeps WLED responsible for physical panel mapping. The historical `rescale` setting is retained for compatibility with the older low-memory storage path; it is not required for 0.9 native output scaling.

## Transfer status rendering

Carousel replacement uses a procedural status layer owned by `IDotMatrixWLEDAdapter`. `IDotMatrixCarousel` reports transfer activity to the adapter. Hardware validation showed that the Device Assets setup count is the physical 12-slot bank/order count, not the number of assets in the current upload, so it must not be used as a percentage denominator. The adapter renders a canonical 16x16 red downward arrow and blue receiving tray, with a dedicated native 64x64 rendition and normal scaling for other supported profiles.

The activity bar is intentionally **indeterminate for its entire visible lifetime**. The protocol provides the length of the current asset only and does not announce the number or byte lengths of all future assets in the upload session, so no global percentage or synthetic 100% state is displayed. Completion is represented by the indicator disappearing when the upload session ends and playback resumes.

The layer remains delayed by 250 ms to avoid flashes on short transfers. The adapter API is shared by Carousel and Preset / Default uploads so both features use the same visual language without duplicating status-rendering code.

## Preset / Default temporary playlist

Preset / Default is intentionally independent from the persistent Carousel subsystem. Bulk objects addressed to protocol slots 14..19 are staged into a six-slot volatile bank. Upload does not acquire display ownership. The `06/02` activation command promotes the selected pending objects and starts a cyclic player from the first requested slot. This keeps replacement atomic from the user's point of view and prevents a partially uploaded Preset from appearing on screen.

The player reuses the normal GIF/TEXT render paths and Bulk CRC/flow-control implementation. Preset files are temporary LittleFS objects only; there is no NVS metadata and no boot restore.

### Preset upload feedback
Preset Bulk uploads reuse the Carousel transfer indicator. The UI is kept active across consecutive slot uploads and is ended by the `06/02` activation command. A 5 s idle timeout prevents an abandoned upload from leaving the indicator on-screen indefinitely. This is presentation-only and does not alter the pending/active Preset transaction model.

## RC2 filesystem transactions and TEXT scratch RAM

Preset / Default remains intentionally volatile. RC2 makes activation transactional only within the current boot/session: active files are moved to temporary `.bak` names, every pending replacement is promoted, and metadata is committed only after the full filesystem operation succeeds. On any intermediate failure the previous active bank is restored. At boot, Preset active/pending/cache/backup files are removed; no Preset journaling or cross-reboot recovery is performed.

The current implementation also contains three independent 4096-byte TEXT scratch areas: BulkTransfer, Carousel playback and Preset playback (about 12 KiB total), in addition to the FA02 inline assembler, BLE queue, renderer/media storage and WLED/NimBLE allocations. RC2 documents this cost but deliberately does not merge those buffers immediately before release, because their lifetimes/ownership differ and the primary MatrixPortal target has PSRAM. A future memory-focused release may consolidate them after dedicated regression testing.
