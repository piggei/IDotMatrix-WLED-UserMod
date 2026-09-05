# Architecture

This document describes the **stable 0.7.1 architecture**, with special attention
to the RAM constraints that shaped the 32x32 and 64x64 media paths on classic
ESP32.

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
information notifications, short-command queuing, and FA02 logical-packet
reassembly. It requests MTU 517 so the official app can use large ATT payloads.
It does not render pixels or directly change WLED state.

### `IDotMatrixFA02Assembler`

Owns one bounded 4112-byte logical-packet buffer. The first two little-endian
bytes define the complete FA02 packet length. ATT fragments are appended until
the logical packet is complete. No protocol ACK is emitted for an ATT fragment.
An abandoned partial transfer is cleared after five seconds so later commands
cannot be poisoned by stale reassembly state.

### `IDotMatrixBulkTransfer`

Validates the common bulk header, enforces sequential chunks, calculates CRC32,
and exposes decoded chunk spans. TEXT is retained in a bounded buffer; RAW and
GIF are streamed onward chunk by chunk.

### `IDotMatrixProtocol`

Owns WLED-independent command validation, typed command decoding, TEXT record
parsing, ACK generation, device-information replies, and the media-sink
boundary. It is host-testable.

### `IDotMatrixRenderer`

Owns RGB storage, graffiti pixels, clock artwork, text glyphs, RAW image assembly,
and animation-frame publication.

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

### `IDotMatrixMedia`

Owns compact PNG decoding, GIF RX/PLAY files, decoder lifetime, and the optional
LittleFS frame cache.

The GIF build profile is selected at compile time:

- default: 10-bit/16x16 AnimatedGIF profile;
- `IDOT_GIF_LZW11`: compact 11-bit/32x32 AnimatedGIF profile;
- `IDOT_GIF_LZW12`: 12-bit/64x64 capability.

For `IDOT_GIF_LZW12`, the final backend is selected at runtime:

- PSRAM detected -> full AnimatedGIF direct playback, allocation preferring PSRAM;
- no PSRAM -> `IDotMatrixCompactGif` + LittleFS frame-cache path.

### `IDotMatrixCompactGif`

A dedicated no-PSRAM 64x64 predecoder. It implements GIF87a/GIF89a parsing and
keeps the complete 4096-code 12-bit LZW space while packing the largest tables
more tightly than AnimatedGIF.

It is used only during cache construction and is destroyed before playback.

### `IDotMatrixWLEDAdapter`

Is the WLED boundary. It maps power, brightness, RGB, and app-content ownership
to WLED. It registers one effect named `iDotMatrix Display`, reads WLED local
time for clocks, and applies WLED's configured 2D mapping/rescale when emitting
pixels.

It also owns GIF staging/recovery rules. On the classic-ESP32 frame-cache path,
WLED `Static` is used internally while the cache is prepared because it has a
small RAM footprint. The selected segment is temporarily blanked so this
internal staging state is not shown to the user.

### `usermod_idotmatrix.cpp`

Owns Usermod lifecycle, settings, the I2S/RMT safety guard, Wi-Fi modem-sleep
requirement, delayed BLE startup, reset/heap flight-recorder diagnostics, and
compact runtime status under `/json/info`.

## Execution model

1. WLED initializes LEDs and Wi-Fi.
2. The Usermod allocates the renderer storage canvas and waits five seconds.
3. NimBLE starts and advertises the iDotMatrix-compatible GATT database.
4. BLE callbacks only copy bounded writes or append FA02 fragments.
5. The WLED loop processes complete packets and sends replies.
6. Protocol events are applied through the WLED adapter.
7. `iDotMatrix Display` renders app-owned content from WLED effect context.
8. GIF filesystem/decoder/cache work runs in normal loop context, never a BLE callback.

For cached 64x64 GIFs without PSRAM, cache construction decodes **at most one
frame per WLED loop turn**. This deliberately yields between frames so Wi-Fi,
BLE, WebSocket, WLED, and LittleFS can continue servicing transient work.

## State ownership

| State | Authority |
|---|---|
| Power / brightness | WLED |
| Solid colour | WLED segment state |
| BLE logical profile | Usermod configuration |
| Renderer storage dimensions | physical WLED matrix when low-memory rescale is active |
| Graffiti / RAW / PNG / GIF visible pixels | `IDotMatrixRenderer` |
| Clock time, timezone, DST | WLED time subsystem |
| Clock style / text style | last valid app command |
| Physical XY/serpentine mapping | WLED matrix configuration |

## Current memory rules

- four 64-byte queue slots for complete short commands;
- one 4112-byte FA02 reassembly buffer for fragmented/large packets;
- one persistent RGB renderer canvas, sized to storage dimensions rather than automatically to logical dimensions;
- no second permanent GIF animation framebuffer;
- RAW prefers a temporary atomic-publication buffer but can receive in-place/hidden if a second canvas cannot be allocated;
- TEXT storage is bounded and allocated outside BLE callbacks;
- compressed GIF bytes are streamed to LittleFS instead of being buffered in RAM;
- default 16x16 keeps the proven low-RAM 10-bit decoder in fixed DRAM;
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

The 0.7.0-derived 10-bit decoder remains the default. Keeping this path small and
fixed avoids making every build pay for larger-profile memory.

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

This path is implemented in 0.7.1 but awaits hardware validation on the pending
PSRAM board.

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
    -> activate iDotMatrix Display
    -> cached playback
```

The cache is capped at 512 KiB and is removed when GIF playback ends.

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

For the frame-cache path:

1. receive new GIF into an alternating RX slot;
2. validate length + CRC;
3. only then retire the old cached playback/resources;
4. promote and precache the new GIF from a clean media state.

If the new validated replacement later fails to prepare, the old cache has
already been retired and cannot safely be resurrected. Recovery therefore lands
on WLED `Static` rather than an empty `iDotMatrix Display`. Restorable non-GIF
content (clock/text/image/DIY) keeps its renderer state and can be restored.

This lifecycle change fixed the hardware pattern where repeated A/B GIF swaps
would eventually stop working until the user manually selected Solid.

## Blank staging

`Static` is useful during no-PSRAM precache because it minimizes WLED effect-side
RAM. Showing the actual Static primary colour, however, produced a distracting
flash between the previous content and the GIF.

0.7.1 therefore:

1. saves the selected segment's primary colour;
2. temporarily sets the staging colour to black and blanks the segment;
3. prepares the cache under WLED `Static`;
4. restores the saved primary colour before `iDotMatrix Display` playback or failure recovery.

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
| Full decoder placed permanently in static DRAM overflowed `.dram0.bss` | too much permanent internal RAM removed from WLED/network | keep only the small 16x16 decoder fixed; allocate larger decoders on demand or in PSRAM |
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
| final dev.14 GIF sample | `heap=33076 min=7468 largest=26612`, `gifCachedFrames=20`, `content=gif` |
| transition stress ending in WLED | cache wait diagnostics observed (`guard=9216`) while WLED remained responsive and recovered |

The `min` value is historical since boot. It may be much lower than current free
heap because predecode intentionally creates the highest transient pressure.
Current `heap`/`largest` during cached playback are more useful for judging
whether WLED/network headroom has recovered.

## Validation boundaries and future work

0.7.1 hardware validation covers the classic-ESP32 no-PSRAM 64x64 **logical**
profile when rescaled to a physical 16x16 WLED matrix. It does not yet prove:

- the automatic PSRAM/direct path on real PSRAM hardware;
- a native physical 64x64 RGB canvas;
- HUB75 DMA plus BLE/media memory pressure;
- every optional WLED integration combined with the 64x64 profile.

Those tests should preserve the invariants above rather than reintroducing
unsafe dictionary truncation or permanent large DRAM allocations.
