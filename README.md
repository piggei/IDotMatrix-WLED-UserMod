# WLED iDotMatrix Usermod — 0.9.4

**Release: 0.9.4 / build: 0.9.4.**  
**Current stable release: 0.9.4.**

0.9.4 is the stable release promoted from hardware-qualified `0.9.4-rc.2` with no functional runtime changes. The rc.2 cycle closed two independently reproduced Automation/persistence defects from rc.1: Schedule quiet-period publication while a following Program multipart was still active, and non-atomic Alarm media/NVS replacement. It also added clock-field validation and kept the qualified Waveshare/MatrixPortal PSRAM/media policies unchanged. The WLED target remains **17.0.0-devV5**.

Before adding iDotMatrix, the physical Waveshare board was verified with the official **WLED 16.0.1** `ESP32-S3_Waveshare_HUB75` binary and one 64x64 HUB75 panel configured as **HUB75 (Half Scan), 64x64, one panel, 1x1**. The complete panel renders correctly; `/json/info` reports 4096 LEDs, 32 MB flash, 16 MB PSRAM and AudioReactive. The 0.9.4 development line is also qualified on real hardware through dev.8: native 64x64 output, TEXT, static image, GIF, Carousel, BLE reconnect, reboot/persistence, Alarm/Program and Matrix Auto Rotation all passed. Local AudioReactive input works after disabling UDP Sound Sync receive mode. The dev.8 PSRAM/telemetry soak completed at about 22 minutes of continuous Carousel/GIF activity with **266/266 successful PSRAM stagings, zero fallback**, all telemetry invariants valid, and the current largest PSRAM block recovering to 14,942,208 bytes after a temporary 256 KiB low-water excursion. 16x16 -> 64x64 and 32x32 -> 64x64 logical scaling have now both been hardware-validated on the Waveshare panel with the legacy Rescale option disabled.

For the Waveshare build use:

```text
overrides/waveshare-s3-hub75.ini
env:waveshare
```

The short `waveshare` name is intentional so this section can be merged into the same global `platformio_override.ini` used for the other local board aliases. It extends WLED 17.0.0-devV5's upstream `env:waveshare_esp32s3_32MB_hub75` hardware environment. Board definition, HUB75 pinout/backend, PSRAM configuration, 32 MB partition/OTA policy, ES8311 pins and SD flags remain inherited from WLED.

The upstream devV5 environment currently lists `SHTC3_v2` with a GitHub `/commit/` page URL. PlatformIO tries to clone that page as a repository and fails before compilation. Dev.2 therefore repeats the Waveshare `custom_usermods` list and changes only that entry to the cloneable pinned Git form `git+https://github.com/lost-hope/SHTC3_v2.git#1f6e3fc...`, while retaining `Internal_Temperature`, the pinned Waveshare AudioReactive Usermod, and iDotMatrix.

No protocol, rendering, automation or external-buzzer behavior changes in dev.16. Dev.7 added the Waveshare-only PSRAM staging path; dev.8 corrected the PSRAM snapshot semantics; dev.9 isolated source-stage ownership; dev.10 added bounded persistent reuse for durable Carousel GIF sources; dev.11 added one-item look-ahead and passed its hardware gate without visible delay. Dev.12 changed only `gifStage peak` accounting when a persistent/prefetched source becomes active and passed its hardware smoke. Dev.13 changed only the legacy low-memory Rescale capability/UI policy on native S3 HUB75 profiles. Dev.14 enabled MatrixPortal transient staging after real-device memory measurement; dev.15 qualified MatrixPortal persistent source reuse; dev.16 qualified the existing one-item / 250 ms MatrixPortal look-ahead with all stage/cache limits unchanged. The Waveshare 2 MiB per-GIF stage limit, 4 MiB pre-allocation reserve, 1 MiB persistent cache budget, 512 KiB per-entry limit, 12 metadata entries and LRU policy remain unchanged. Any staging/prefetch failure leaves the normal playback fallback path intact. Larger physical layouts remain deferred until hardware is available.

WLED remains the owner of LED output, effects, 2D mapping, presets, playlists, brightness, HTTP/JSON APIs, Home Assistant and network realtime protocols. The Usermod adds the iDotMatrix-compatible BLE peripheral and renders app content through the `iDotMatrix` WLED effect.

> **BLE compatibility/security:** the compatibility GATT profile is intentionally
> unauthenticated, matching the observed original-device/app exchange. A nearby
> BLE peer can therefore send iDotMatrix commands, including persistent/reset
> commands. CRC fields provide integrity checking, not authentication. Disable
> the Usermod/BLE service when this proximity-access model is not acceptable.

## Development status

`0.9.4` is the **stable release**, promoted from the regression-fix-only rc.2 candidate after host regression coverage and real-hardware qualification. `0.9.3` is now the previous stable release and remains the behavioral reference for features that were intentionally unchanged in 0.9.4.

The 0.9.3 line removed the internal buzzer backend and delegated sound to the optional standalone **WLED Buzzer Usermod** through the weak-link service bridge. That architecture is unchanged in 0.9.4. The final code delta relative to rc.1 is concentrated in Automation persistence/commit hardening and does not alter the BLE wire format. On native S3 HUB75 profiles the settings page now omits the obsolete Rescale control entirely because 0.9 output scaling is automatic; classic low-memory profiles that still expose the storage optimization label it explicitly as `Low-memory canvas downscale:`.

The dev.8 PSRAM telemetry/staging gate, dev.9 source-stage refactor smoke, dev.10 persistent source-reuse gate and dev.11 one-item look-ahead gate are complete. Dev.11 cold-cache testing showed one normal stage plus successful speculative prefetches with no visible delay; later mixed Carousel/Preset activity reached `gifStage attempts=9 ok=9 fallback=0`, `gifPrefetch attempts=12 ok=12 cached=19 fail=0`, and a full source-cache lifecycle of `stores=21 invalid=21 entries=0 bytes=0`. After invalidation, the current largest PSRAM block recovered to **14155776 bytes**, matching the pre-cache baseline and showing no persistent fragmentation in the reported run. Dev.12 fixed the `gifStage peak` diagnostic semantics exposed by cached playback and passed its real-device smoke (`bytes=49799 peak=49799`, no staging/prefetch failure). The independent 16x16 -> 64x64 and 32x32 -> 64x64 Waveshare scaling gates also passed with Rescale disabled. Native 64x64, BLE, TEXT, static image, GIF, Carousel, reboot/persistence, Alarm/Program and auto-rotation remain the qualified baseline. AudioReactive qualification is independent of the external Buzzer Usermod and must be evaluated with a non-conflicting audio backend.


### PSRAM telemetry, GIF staging, source reuse and look-ahead

Build 0.9.4 retains the dev.6 `/json/info` PSRAM/GIF frame-cache telemetry, the `gifStage=...` telemetry introduced in dev.7, the dev.8 self-consistent PSRAM snapshots, the dev.9 isolated `IDotMatrixGifSourceStage` ownership and the dev.10 persistent source cache. On the Waveshare target, direct AnimatedGIF playback still stages eligible GIF source files up to 2 MiB in PSRAM. The guard requires enough free PSRAM for the source plus a 4 MiB reserve before allocation; subsequent decoder allocation may consume a small additional amount. Any size, reserve, allocation or copy failure still falls back automatically to the pre-dev.7 filesystem path.

On MatrixPortal S3, dev.14 qualified the same transient source-stage implementation with target-specific limits: **256 KiB maximum** and **1 MiB reserve**. Dev.15 qualified a **384 KiB persistent source cache** for durable Carousel GIFs, with a **256 KiB per-entry limit** and up to **8 metadata entries**; the complete seven-GIF test set occupies 249267 bytes and remains resident. Dev.16 qualified the existing one-item / 250 ms Carousel look-ahead, so `gifPrefetch attempts` should become non-zero during a cold Carousel while `fail` remains zero. App-upload/transient GIFs remain one-play only, and any source that is too large or cannot satisfy the reserve guard continues through the original LittleFS callbacks.

Dev.10 adds bounded persistent reuse for **durable Carousel GIF sources only**. The first play of an eligible stored GIF follows the existing stage path, then transfers ownership of that allocation into a source cache without a second copy. Later plays of the unchanged stored path reuse the same PSRAM image. App-upload/transient `/idot_play.gif` content remains one-play staging and is never admitted to the persistent cache. The default Waveshare cache policy is 1 MiB total, 512 KiB maximum per admitted source, up to 12 metadata entries, and least-recently-used eviction. Resident entries are expendable under PSRAM pressure and are retired before the normal one-play stage path is allowed to fail its reserve/allocation guard. A larger GIF may still use the normal 2 MiB transient stage even when it is not eligible for persistent admission.

Carousel storage remains authoritative. Bank reset/reconfiguration clears resident source copies, and a committed slot replacement invalidates that slot explicitly, including same-size replacements. An entry invalidated while AnimatedGIF is still using it is retired only after the decoder closes, preserving callback lifetime safety.

The existing `gifStage=...` counters make reuse measurable: cache hits do not increment `attempts` or `ok` because no filesystem source copy occurs. Dev.10 added `gifSourceCache=state:<off|idle|active> entries:<n> bytes:<n> hits:<n> misses:<n> stores:<n> evict:<n> invalid:<n> maxEntries:<n> maxBytes:<bytes> entryMax:<bytes>`. Dev.11 added `gifPrefetch=attempts:<n> ok:<n> cached:<n> fail:<n> bytes:<n>`. A prefetch request is issued only after the current item is visible and only for the immediately following playable GIF; a 250 ms delay keeps that copy away from the transition itself. Prefetch never changes the active AnimatedGIF source pointer, and failure is advisory only: normal dev.10 playback staging/reuse remains authoritative. Dev.12 also makes `gifStage peak` track the largest source image that has actually become active for AnimatedGIF, including cache/prefetch hits; therefore every emitted media snapshot again satisfies the intuitive relationship `gifStage peak >= gifStage bytes`. Cache hits still do not increment normal `gifStage attempts/ok`.

`psram=total` is a property of the complete linked WLED firmware image, not only the physical PSRAM chip. When comparing optimization builds, use the same WLED/Usermod composition and focus on free/low-water/largest-block behavior in addition to the absolute total.

## Preset / Default

The official app's **Preset / Default** page is implemented separately from Device Assets / Carousel. It uses protocol media slots `14..19` (maximum six entries), uploads objects through the existing Bulk transport, and activates the ordered list with command `06/02`. Uploading Preset media never changes the display by itself; the new playlist becomes active only when the activation command arrives.

Preset media are deliberately volatile. They are stored in temporary LittleFS files, are not written to NVS, and are not restored at boot. A new Preset may be uploaded while an older Preset continues to play; the pending bank is promoted only on the next activation. Preset activation is transactional within the current session: a filesystem failure rolls the active bank back to its previous complete state. Transaction backup files are temporary and are deliberately removed at boot rather than recovered across reboot. Image/GIF entries use an approximately 3000 ms visible dwell, while TEXT uses the existing renderer timing so scrolling content can complete before the next entry.

The new audio-source setting has three modes:

| Setting | Behaviour |
|---|---|
| **Phone / BLE** | Original 0.8.1 behaviour. Level/FFT data sent by the iDotMatrix app drives the visualizer. This remains the default. |
| **WLED AudioReactive** | Uses WLED AudioReactive's processed local audio data. BLE Audio/Rhythm frames still select LEVEL/FFT and visualizer mode, but phone amplitude/spectrum data is ignored. If AudioReactive data is unavailable, the visualizer receives silence. |
| **Auto** | Prefers WLED AudioReactive data when available and falls back to the original Phone / BLE stream otherwise. |

The Usermod does **not** open a second I2S input and does not run a second FFT.
It consumes the data already exported by WLED AudioReactive. Its 16 GEQ bins are
reduced to the eight legacy iDotMatrix bands by averaging adjacent bin pairs,
then mapping the 0..255 values to the renderer's 0..12 range.

A dedicated C3 AudioReactive profile is supplied as `overrides/esp32c3-16x16-audio.ini`.
The normal `overrides/esp32c3-16x16.ini` remains iDotMatrix-only so users who do
not need local audio keep the lower RAM footprint. The C3 audio profile uses the
same pinned WLED IDF5/shared-RMT base and NimBLE 2.5.1 as the validated 0.8.1
C3 build; it only adds `audioreactive` to `custom_usermods`.


### Large 64-pixel TEXT payloads

The original 64x64 device can send up to 64 glyphs at the 32x64 font size. That is a 16654-byte TEXT object: a 14-byte global header plus 64 records of 260 bytes each. The 0.9 implementation accepts that complete size end-to-end for live Bulk TEXT, Carousel and Preset playback. The large payload is not stored in a permanent 16 KiB array: temporary buffers are allocated only while receiving or replaying TEXT and prefer PSRAM on ESP32-S3 when available.

### Persistent Device Assets / Carousel

The Usermod stores one 12-slot Device Assets bank in LittleFS. GIF and TEXT
slots may be mixed and each slot retains its app-provided dwell time. After a
page upload, playback starts automatically once the transfer stream becomes
quiet; no separate app-side "play Carousel" command is required.

WLED remains authoritative at boot. If `iDotMatrix` is configured as
the WLED boot effect and a valid stored Carousel exists, that Carousel becomes
the startup content. Selecting a native WLED effect suspends Carousel playback
without deleting the bank; selecting `iDotMatrix` again resumes it.

Carousel GIF reuse is backend-specific. On no-PSRAM 12-bit profiles, an unchanged stored slot can reuse its per-slot filesystem frame cache instead of rebuilding `/idot_cache.bin`. On the Waveshare direct-decoder profile, dev.10 retains the compressed whole-file source in the bounded PSRAM source cache and dev.11 can warm the immediately following GIF before its transition. Invalid slots are quarantined for the current bank generation and later valid slots continue to play. Feature-owned temp/backup files are reconciled at boot, and Carousel storage mutations invalidate any resident PSRAM source generation.

### Historical qualified hardware baseline

The 0.9.x line inherits its classic ESP32 and initial ESP32-C3 foundation from the qualified 0.8.2 baseline, which extended the 0.8.1 hardware work with Carousel/cache, reset, boot, automation and local-microphone AudioReactive testing:

| Target | WLED base | Arduino / ESP-IDF | LED/BLE path | NimBLE |
|---|---|---|---|---|
| classic ESP32 (`esp32dev`) | WLED 16.0.1 | Arduino 2.0.17 / IDF 4.4.7 in supplied overrides | I2S LED output + BLE | 1.4.3 |
| ESP32-C3 4 MB | pinned WLED commit `d55037f7510541eddc390c8f3d01afc5787aa44a` | Arduino 3.3.8 / IDF 5.5.4 | WLED shared-RMT + BLE | 2.5.1 |

That 0.8.2 C3 qualification covered persistent mixed Carousel playback and boot restore, Carousel-to-Carousel replacement, verified reset cleanup, alarms/programs, clock artwork, and WLED AudioReactive input from an external microphone. The current 0.9.x C3 profiles build on that qualified IDF5/shared-RMT foundation and add the separately documented 0.9.x features and OTA profile.

### Hardware validation scope

| Configuration | GIF backend | Status |
|---|---|---|
| 16x16 logical / 16x16 physical, classic ESP32 | `compact12/cache` | **supported and hardware-validated** |
| 16x16 logical / 16x16 physical, ESP32-C3 4 MB | `compact12/cache` | **supported and hardware-validated on IDF5/shared-RMT** |
| 32x32 logical -> 16x16 physical, `rescale=true`, classic ESP32 | `animatedgif11` | hardware-validated |
| 64x64 logical -> 16x16 physical, `rescale=true`, classic ESP32 without PSRAM, `64x64-lite` | `compact12/cache` | hardware-validated |
| 64x64 logical / 64x64 physical, MatrixPortal ESP32-S3 + PSRAM | `animatedgif12/psram` | **hardware-validated on WLED 17.0.0-devV5 / `06ae26db67107cb3f6a3d107a92340035991a063` / native HUB75** |
| 64x64 logical / 64x64 physical, Waveshare ESP32-S3-RGB-Matrix + 16 MB PSRAM | `animatedgif12/psram` | **0.9.4 functional/media path through dev.12 hardware-qualified; 16x16/32x32 -> 64x64 automatic scaling PASS on WLED 17.0.0-devV5** |
| 32x32 logical -> 64x64 physical, MatrixPortal ESP32-S3 | `animatedgif12/psram` | **hardware-validated; automatic 2x nearest-neighbour upscale** |
| 16x16 logical -> 64x64 physical, MatrixPortal ESP32-S3 | `animatedgif12/psram` | **hardware-validated; automatic 4x nearest-neighbour upscale** |

### Compiled resolution and settings choices

The override determines the largest protocol/media profile compiled into the
firmware. The settings page never offers a profile larger than that capacity:

| Override / decoder | Available `ScreenType` values | `Rescale` |
|---|---|---|
| `overrides/16x16.ini` / LZW12 + `IDOT_SCREEN_MAX_DIM=16` | 16x16 | hidden and forced off |
| `overrides/esp32c3-16x16.ini` / LZW12 + `IDOT_SCREEN_MAX_DIM=16` | 16x16 | hidden and forced off |
| `overrides/esp32c3-16x16-audio.ini` / same media profile + AudioReactive | 16x16 | hidden and forced off |
| `overrides/esp32c3-16x16-audio-ota.ini` / AudioReactive + dual-slot OTA | 16x16 | hidden and forced off |
| `overrides/32x32.ini` / LZW11 | 16x16, 32x32 | low-memory canvas downscale available |
| `.64x64` or `.64x64-lite` / LZW12 | 16x16, 32x32, 64x64 | low-memory canvas downscale available |
| `overrides/matrixportal-s3-hub75.ini` / LZW12 + PSRAM | 16x16, 32x32, 64x64 | legacy Rescale hidden/forced off; automatic physical-output scaling |
| `overrides/waveshare-s3-hub75.ini` / LZW12 + PSRAM | 16x16, 32x32, 64x64 | legacy Rescale hidden/forced off; automatic physical-output scaling |

On the 0.9 native-matrix path, `ScreenType` is the logical iDotMatrix profile and may differ from the physical WLED matrix. Output scaling is automatic in both directions for 16x16, 32x32 and 64x64 logical/physical combinations. `Rescale` is retained only on profiles that intentionally support the older low-memory 0.8.x storage path. It is hidden and forced off on the native Waveshare and MatrixPortal S3 HUB75 profiles because it is not required for 0.9 output scaling.

## Supported functionality

| Function | Protocol / source | WLED mapping | Status |
|---|---|---|---|
| Discovery | FA/AE GATT + manufacturer data | BLE Usermod | Verified |
| Screen power | FA02 | WLED power | Verified |
| Brightness | FA02 | WLED master brightness | Verified |
| Full-screen RGB | FA02 | `iDotMatrix` framebuffer | Verified; isolated from native WLED state |
| Standalone light effects (7) | FA02 `03 02` | locally rendered by `iDotMatrix` | Hardware-validated, including one-pixel scrolling for effects 3/4/5 |
| Audio/Rhythm (5 LEVEL + 5 FFT) | FA02 stream `06 00 00 02` / `21 00 01 02`; optional WLED AudioReactive source | locally rendered by `iDotMatrix` | Phone/BLE path retained; local AudioReactive/external-microphone path hardware-validated in 0.8.2 |
| Countdown | FA02 `08 80` | original-device hourglass + stacked `MM` / `SS` under `iDotMatrix` | Hardware-validated; async finish status on FA03 |
| Stopwatch | FA02 `09 80` | original-device stopwatch + stacked `MM` / `SS` under `iDotMatrix` | Hardware-validated |
| Scoreboard | FA02 `0A 80` | two original-device 3-digit score rows under `iDotMatrix` | Hardware-validated |
| Alarms | FA02 `00 80` | persistent time/day/media trigger under `iDotMatrix` | Implemented and hardware-tested with the official app |
| Programs / schedules | FA02 `07 80` + `05 80` | persistent weekday/time-window GIF/PNG/TEXT activities | Implemented and hardware-tested; finite activation sound |
| DIY/Graffiti | FA02 pixel updates + type-0 multipart raster | `iDotMatrix` | Pixel mode verified; 64x64 multipart full-raster path hardware-validated with the official app and complex photographic images |
| Clock | FA02 | `iDotMatrix`, WLED local time | Verified on 16x16, 32x32 and 64->16 rescale |
| Text | bulk type `0x03` | app bitmaps rendered by `iDotMatrix` | Verified at matching logical/physical resolution; non-scrolling effects use multi-row pages (64x64: 1x64 / 2x32 / 4x16, 32x32: 1x32 / 2x16, 16x16: 1x16) |
| RAW/cloud image | bulk type `0x02` | atomic/downscaled RGB framebuffer | Verified on 16x16 and 64->16 rescale |
| Compact PNG | inline type `0x00` | decoded RGB/RGBA framebuffer | Verified on 16x16; larger-profile coverage remains partial |
| GIF animation | bulk type `0x01` | direct decoder or LittleFS frame cache | Verified on 16x16, 32->16 and 64->16 no-PSRAM path |
| 32x32 profile | profile `0x03` | logical profile + optional rescale | Hardware-validated with physical 16x16 |
| 64x64 profile | profile `0x04` | logical profile + optional low-memory rescale | Hardware-validated with physical 16x16/no PSRAM |

Alarms and programs/schedules include persistent metadata/media and preserve their protocol-level sound requests. Sound playback is delegated to the optional standalone WLED Buzzer Usermod. Display rotation and energy-saving remain owned by WLED; the verified `03 80` protocol reset clears Usermod-owned Carousel/Device Assets, Preset/Default, alarm, program/schedule and transient iDotMatrix state without rebooting WLED. WLED configuration, connectivity, system time and unrelated filesystem content are preserved.
These responsibilities remain in WLED rather than being duplicated in the BLE emulator.

## Hardware requirements

### Supported 16x16 targets

**Classic ESP32**

- PlatformIO-compatible `esp32dev`, at least 4 MB flash;
- WLED 16.0.1 source tree;
- 16x16 WLED 2D matrix;
- digital LED output using WLED's **I2S** backend; the Usermod deliberately blocks BLE when a classic-ESP32 digital RMT bus is detected;
- NimBLE-Arduino 1.4.3 and AnimatedGIF 1.4.7 as pinned by the supplied override.

**ESP32-C3**

- 4 MB ESP32-C3 compatible with WLED `esp32c3dev`;
- the exact WLED commit `d55037f7510541eddc390c8f3d01afc5787aa44a`;
- 16x16 WLED 2D matrix; the release hardware test used GPIO4;
- WLED IDF5 `WLED_USE_SHARED_RMT` backend;
- NimBLE-Arduino 2.5.1 and AnimatedGIF 1.4.7 as pinned by `overrides/esp32c3-16x16.ini`.

Buzzer hardware is no longer owned by iDotMatrix. Install and configure the standalone WLED Buzzer Usermod when sound output is required; it owns the GPIO, hardware type, trigger polarity, LEDC resources and playback timing.

PSRAM is not required for either supported 16x16 target.

### Important ESP32/WLED constraints

The classic ESP32 and C3 must not be treated as interchangeable build targets.
On classic ESP32 the verified BLE build requires I2S LED output and retains the
RMT safety block. On C3, legacy IDF4 RMT was experimentally shown to produce
pixel spikes; the supported C3 profile is therefore compile-time guarded so it
only builds on ESP-IDF 5 with `WLED_USE_SHARED_RMT` and NimBLE 2.x.

Legacy classic/C3 profiles retain the single-application no-OTA layout. The
MatrixPortal profile inherits upstream WLED OTA support. `overrides/esp32c3-16x16-audio-ota.ini`
adds a separately validated 4 MB C3 OTA layout with two `0x1A0000` application
slots, 640 KiB LittleFS and a 64 KiB coredump partition. OTA images must be built
from this customized source tree so the out-of-tree iDotMatrix Usermod is retained.

The Usermod forces Wi-Fi modem sleep on (`noWifiSleep = false`) while BLE is
active because Wi-Fi/Bluetooth coexistence requires it on the supported ESP32
stacks.

## Software requirements

| Component | classic ESP32 | ESP32-C3 |
|---|---|---|
| WLED | 16.0.1 | commit `d55037f7510541eddc390c8f3d01afc5787aa44a` (`17.0.0-devV5`) |
| PlatformIO environment | `esp32dev_idotmatrix_16x16` | `esp32c3dev_idotmatrix_16x16` |
| Arduino-ESP32 | 2.0.17 | 3.3.8 |
| ESP-IDF | 4.4.7 | 5.5.4 |
| BLE | NimBLE-Arduino 1.4.3 | NimBLE-Arduino 2.5.1 |
| GIF | AnimatedGIF 1.4.7 | AnimatedGIF 1.4.7 |

For the C3 build, Linux/WSL is recommended. The tested WLED IDF5 tree can exceed
Windows process-command-line limits during PlatformIO compilation even from a
very short source path. WLED's UI build also requires **Node.js 20 or newer**.

`library.json` intentionally does not impose a single NimBLE version. The two
supported build families require different major APIs, and the supplied
PlatformIO profiles are authoritative: classic ESP32 pins NimBLE-Arduino 1.4.3
while ESP32-C3 pins 2.5.1.

## GIF decoder profiles and memory model

The maximum GIF profile is selected at build time because AnimatedGIF 1.4.7
stores its LZW tables inside the decoder object:

- standard 16x16 profile: **12-bit / compact12/cache**, UI capped by `IDOT_SCREEN_MAX_DIM=16`;
- `IDOT_GIF_LZW11`: **11-bit / 32x32** compact decoder;
- `IDOT_GIF_LZW12`: **12-bit / 64x64** support.

The 64x64 build then makes a second decision **at runtime**:

- **PSRAM detected:** select the full 4096-entry AnimatedGIF decoder and direct playback; allocation first prefers PSRAM;
- **no PSRAM:** select `compact12/cache`, which retains all 4096 legal LZW codes, downsamples during predecode, writes physical frames to LittleFS, destroys the decoder workspace, and only then starts playback.

On the validated 64x64 logical -> 16x16 physical setup, the compact workspace is
**16,128 bytes**, versus about **20,660 bytes** for the full AnimatedGIF object
with the current toolchain. The frame cache is capped at **512 KiB** and is
removed when GIF playback ends.

In the 0.9 native-matrix path, logical iDotMatrix resolution and physical WLED
matrix resolution are independent. Every 16x16, 32x32 and 64x64 combination is
scaled automatically: nearest-neighbour for enlargement, box averaging for
reduction, and a direct path when dimensions match. This allows, for example,
16x16 or 32x32 app profiles to fill a 64x64 HUB75 panel and a 64x64 logical
profile to drive a future 32x32 physical panel.

The historical `rescale` setting is retained for compatibility with low-memory
0.8-era profiles. When enabled there, logical protocol dimensions and renderer
storage may be separated so a 64x64 logical source can be sampled directly into
a smaller physical canvas without allocating a second full logical framebuffer.

The full rationale, failed experiments, memory measurements, and safety rules are
documented in [`ARCHITECTURE.md`](ARCHITECTURE.md).

## Installation

### 0.9.x reference platform: MatrixPortal S3 / HUB75 64x64

The reference 0.9 build is qualified against:

```text
Hardware : Adafruit MatrixPortal ESP32-S3 (8 MB flash / 2 MB PSRAM)
Display  : one 64x64 HUB75 RGB panel
WLED     : 17.0.0-devV5
Commit   : 06ae26db67107cb3f6a3d107a92340035991a063
Profile  : adafruit_matrixportal_esp32s3_idotmatrix_64x64
```

**MatrixPortal PSRAM optimization policy:** dev.14 qualified transient staging and dev.15 qualified persistent source reuse on the 2 MiB MatrixPortal target. The seven-GIF hardware corpus occupies **249267 bytes** and reached `hits:25` with only seven normal cold stages, `fallback:0` and `evict:0`. Dev.16 retains the **256 KiB per-GIF stage maximum**, **1 MiB reserve**, **384 KiB persistent source-cache budget**, **256 KiB per entry**, and **8 metadata entries**, and enables the same one-item / 250 ms Carousel look-ahead already qualified on Waveshare. Waveshare keeps its separate 2 MiB / 4 MiB / 1 MiB policy unchanged.

Keep the repositories beside one another because the supplied override uses a relative symlink:

```text
<workdir>/WLED/
<workdir>/wled-usermod-idotmatrix/
<workdir>/wled-usermod-buzzer/        # optional; required only for sound
```

Prepare the exact qualified WLED base and copy the MatrixPortal override:

```bash
git clone https://github.com/wled/WLED.git WLED
cd WLED
git checkout 06ae26db67107cb3f6a3d107a92340035991a063
cp ../wled-usermod-idotmatrix/overrides/matrixportal-s3-hub75.ini platformio_override.ini
```

Clean and build:

```bash
pio run -e adafruit_matrixportal_esp32s3_idotmatrix_64x64 -t clean
pio run -e adafruit_matrixportal_esp32s3_idotmatrix_64x64
```

Upload over USB/serial with PlatformIO, or use WLED OTA if the installed MatrixPortal firmware uses the same upstream-compatible partition layout:

```bash
pio run -e adafruit_matrixportal_esp32s3_idotmatrix_64x64 -t upload
```

The MatrixPortal profile intentionally inherits the upstream WLED partition and OTA policy and also inherits `${common.default_usermods}` from this exact pinned WLED revision. That inheritance is therefore part of the qualification baseline rather than an unbounded dependency on future WLED revisions.

For first boot, configure the physical WLED matrix as 64x64 with the MatrixPortal/native HUB75 setup, configure Wi-Fi/timezone/NTP as desired, then leave the iDotMatrix logical profile at 64x64 for native operation. Logical 16x16 and 32x32 profiles are automatically upscaled to the physical 64x64 panel.

Open the official iDotMatrix app and scan for the configured `IDM-...` peripheral. `/json/info` should report `release=0.9.4`, `build=0.9.4`, `target=MatrixPortal-S3` and `wledBase=06ae26d`.

### Legacy qualified profiles

The source also retains the previously qualified pre-0.9 profiles:

- classic ESP32 16x16: `overrides/16x16.ini` with WLED 16.0.1;
- ESP32-C3 16x16: `overrides/esp32c3-16x16.ini` with pinned WLED commit `d55037f7510541eddc390c8f3d01afc5787aa44a`;
- ESP32-C3 16x16 + AudioReactive: `overrides/esp32c3-16x16-audio.ini` on the same pinned C3 base.
- ESP32-C3 16x16 + AudioReactive + OTA: `overrides/esp32c3-16x16-audio-ota.ini`; hardware-validated with three consecutive OTA cycles on a 4 MB C3.

These are retained compatibility/qualification baselines; they are not the 0.9.x S3 reference platform or the 0.9.4 Waveshare optimization/qualification target. See [`BUILD_PROFILES.md`](BUILD_PROFILES.md) for their exact build commands and partition policy.


### Validated ESP32-C3 4 MB OTA profile

Use the pinned C3 WLED base `d55037f7510541eddc390c8f3d01afc5787aa44a` and copy the OTA override:

```bash
cp ../wled-usermod-idotmatrix/overrides/esp32c3-16x16-audio-ota.ini platformio_override.ini
pio run -e esp32c3dev_idotmatrix_audio_16x16_ota -t clean
pio run -e esp32c3dev_idotmatrix_audio_16x16_ota
```

The first installation of the dual-slot partition layout must be performed over
USB/serial. Subsequent WLED updates can use the generated `firmware.bin` through
the WLED OTA page. Hardware qualification completed three consecutive OTA cycles
with Carousel, Preset and Schedule content already populated.

## Configuration options

- `enabled`: enables the BLE emulator. Changing this setting after boot requires a reboot; the 0.9 implementation deliberately does not implement a partial hot start/stop lifecycle;
- `screenType`: logical profile (`16x16`, `32x32`, `64x64`);
- `deviceName`: editable BLE-name suffix shown after the fixed `IDM-` prefix;
  when no name is saved, a stable six-digit default (`IDM-xxxxxx`) is derived
  from the ESP32 eFuse MAC;
- `rescale`: legacy low-memory canvas downscale for selected classic profiles; hidden and forced off on 16x16-only builds and on the native Waveshare/MatrixPortal S3 HUB75 profiles. It is not required for normal 0.9 output scaling;
- `audioSource`: `Phone / BLE` (default), `WLED AudioReactive`, or `Auto`. The
  AudioReactive choices consume WLED's existing processed audio data when the
  AudioReactive Usermod is compiled and enabled.

## Runtime status

`/json/info` exposes compact diagnostics under `u.iDotMatrix`. A 64x64
classic-ESP32/no-PSRAM build may report:

```text
BLE connected
profile=64x64
canvas=16x16
name=IDM-123456
release=0.9.4
build=0.9.4
audioSource=phone active=phone
audioReactive=absent
gifDecoder=compact12/cache
gifDecoderBytes=16128
gifProbe=... largest=... reserve=10240
gifCachedFrames=...
gifCacheWaits=... low=... guard=9216
gifCacheStats=build:... reuse:...
bleRx=drop:... oversize:... malformed:... faTimeout:... bulkTimeout:...  # emitted when nonzero
bootReset=poweron code=1
heap=... min=... largest=...
content=gif
```

For the supported C3 profile, the same section additionally reports:

```text
BLE connected
RMT+BLE=ESP32-C3 shared-RMT
framework=WLED IDF5/shared-RMT
wledBase=d55037f
nimble=2.x API
release=0.9.4
build=0.9.4
audioSource=phone active=phone
audioReactive=absent
```

`gifCacheWaits` is diagnostic, not automatically an error. On the no-PSRAM
cache path, a free-heap sample below 9 KiB pauses decoding and yields to
WLED/Wi-Fi/BLE; only a continuously low condition for roughly two seconds causes
`mediaError=gif-ram-reserve`.

Possible content owners are `WLED`, `solid`, `light`, `audio`, `graffiti`,
`clock`, `countdown`, `stopwatch`, `scoreboard`, `text`, `image`, and `gif`.
When a light effect is active, `/json/info` also exposes its
effect id, speed, and palette size. Other media errors include `gif-invalid`, `gif-decoder-oom`,
`gif-decoder-open`, `gif-canvas-oom`, `gif-cache-io`, and `gif-cache-full`.

## Behavior and limitations


### Graffiti full-raster multipart uploads

The original 64x64 app does not send a large Graffiti canvas as one oversized
PNG. It sends a dedicated type-0 raw-RGB object in multiple complete FA02 logical
packets. Each packet repeats a 9-byte header; marker `0x00` starts the object and
`0x02` continues it. The declared size is the complete raster size, not the current
chunk size. For 64x64 the captured sequence is three 4096-byte RGB chunks. WLED
ACKs `05 00 00 00 02` while incomplete and `05 00 00 00 01` on completion.
See `PROTOCOL.md` for the exact format and its distinction from compact PNG and
generic Bulk RAW. The multipart path is hardware-validated on the physical
64x64 MatrixPortal/HUB75 target with several complex photographs sent from
the official app.

### Brightness direction

Brightness is synchronized from the iDotMatrix app to WLED. The app does not
query the current WLED brightness from the emulated peripheral, so changing
brightness elsewhere in WLED does not necessarily move the app slider.

### Alarm / Program multipart media

On 64x64 profiles the official app can split one Alarm or Program media asset across multiple complete logical FA02 packets. Each packet repeats the full Alarm/Program metadata header; `mediaSize` and `mediaCRC` describe the complete asset, while the bytes after that packet's header are only the current chunk. The current 0.9 implementation assembles chunks by stable media identity, total size and CRC. For Schedule, byte 10 is the one-byte content type and byte 11 is a transport chunk marker (`0x00` first, observed `0x02` continuation); the marker is not part of media identity. Schedule returns ACK status `0x01` for accepted incomplete media and `0x03` only after complete CRC-valid commit. An incomplete, mismatched, oversized, timed-out or CRC-invalid transfer is discarded without replacing the previously committed Alarm/Program. The current transaction timeout is 5 seconds and the defensive per-asset limit is 512 KiB.

### Clock source

For iDotMatrix Alarm and Program/Schedule compatibility, the most recent valid
app time-synchronization packet is authoritative once received, matching the
standalone emulator and original-device behavior. WLED local time/NTP remains
the fallback when the app has not synchronized time in the current boot/session.
Configure WLED NTP, timezone, and daylight-saving settings normally for WLED's
own clock and for operation before the app supplies its time synchronization.

### Display ownership

Every app-originated visual mode selects the single `iDotMatrix` WLED
effect, including full-screen RGB and the seven standalone light effects. WLED
therefore exposes app content as a framebuffer source instead of pretending the
WLED and iDotMatrix apps share a synchronized Solid/effect state. Selecting a
normal WLED effect manually is an explicit source change and replaces app content
until the app sends another supported content command. Conversely, when an
iDotMatrix content command explicitly reclaims the display, the Usermod terminates the active WLED playlist
and clears any playlist preset already queued for WLED's deferred preset handler
before selecting the `iDotMatrix` framebuffer effect. This matches WLED's own
direct-effect behavior while accounting for the asynchronous preset queue that
runs after the Usermod loop. The playlist is terminated rather than paused;
restart it from WLED when you want WLED playlist playback again.

For no-PSRAM 64x64 GIF preparation, WLED internally uses `Static` because it has
a small RAM footprint. The physical segment is deliberately blanked during this
short staging period and the previous WLED primary colour is restored before
playback/recovery.

### LittleFS frame-cache tradeoff

On classic ESP32 without PSRAM, 64x64 GIF playback trades temporary flash I/O
for RAM headroom. Each valid GIF is predecoded to `/idot_cache.bin`; the cache is
removed when playback ends. Very frequent GIF replacement therefore performs
more flash writes than the PSRAM/direct backend. The cache has a 512 KiB limit.

### Validation boundaries

The PSRAM/direct backend, native physical 64x64 output and WLED native HUB75 DMA remain outside the 0.8.2 stable release matrix, but are hardware-validated for 0.9 on Adafruit MatrixPortal ESP32-S3 with one physical 64x64 HUB75 panel. Validation includes native 64x64 operation plus logical 32x32 -> 64x64 and 16x16 -> 64x64 automatic upscale, BLE, direct AnimatedGIF/PSRAM playback, Carousel, TEXT including the 32x64 glyph path, and WLED/iDotMatrix ownership transitions.

## Repository layout

- `usermod_idotmatrix.cpp` — Usermod lifecycle, configuration, startup guards, runtime status;
- `IDotMatrixBLEServer.*` — NimBLE GATT server, reassembly, notifications;
- `IDotMatrixFA02Assembler.*` — bounded fragmented FA02 reconstruction;
- `IDotMatrixBulkTransfer.*` — bulk framing, CRC32, TEXT/RAW/GIF chunk state;
- `IDotMatrixProtocol.*` — protocol validation and command decoding;
- `IDotMatrixBuildProfile.h` — compile-time ScreenType/Rescale capability policy;
- `IDotMatrixRenderer.*` — RGB storage canvas and all local visual rendering;
- `IDotMatrixMedia.*` / `IDotMatrixMediaSink.h` — PNG/GIF/RAW/TEXT media boundary, RX files, direct playback, and frame-cache orchestration;
- `IDotMatrixCompactGif.*` — compact-safe full-code-space LZW12 predecoder for no-PSRAM 64x64;
- `IDotMatrixWLEDAdapter.*` — protocol-to-WLED state, ownership, staging, timers, scoreboard, audio, and display effect;
- `IDotMatrixAutomation.*` — persistent alarms and program/schedule execution;
- `IDotMatrixPreset.*` — volatile six-slot Preset / Default staging and transactional activation;
- `IDotMatrixAudioSource.*` — Phone/BLE vs WLED AudioReactive source selection and band mapping;
- `patch_animatedgif_profiles.py` — selects 10/11/12-bit AnimatedGIF build profile;
- `partitions/` — custom no-OTA tables plus the validated 4 MB dual-slot OTA table;
- `overrides/` — PlatformIO media/hardware target templates;
- `tests/` — host regression tests and compact-GIF fixtures.

Further documentation:

- [`BUILD_PROFILES.md`](BUILD_PROFILES.md) — media profiles, 4/8/16 MB and ESP32-S3 targets, HUB75 wrappers, partitions, and build commands;
- [`PROTOCOL.md`](PROTOCOL.md) — implemented wire-protocol subset;
- [`ARCHITECTURE.md`](ARCHITECTURE.md) — component boundaries, current memory model, and RAM-engineering history;
- [`TESTING.md`](TESTING.md) — host/build/hardware regression procedure;
- [`HISTORY.md`](HISTORY.md) — release/development history;
- [`TODO.md`](TODO.md) — current 0.9.4 qualification/optimization plan plus deferred work;
- [`RELEASE_NOTES_0.9.4.md`](RELEASE_NOTES_0.9.4.md) — stable 0.9.4 release notes;
- [`RELEASE_QUALIFICATION_0.9.4.md`](RELEASE_QUALIFICATION_0.9.4.md) — final promotion and hardware-qualification record;
- [`PRE_RELEASE_AUDIT_0.9.4-rc.2.md`](PRE_RELEASE_AUDIT_0.9.4-rc.2.md) — rc.2 audit response and regression-fix rationale;
- [`RELEASE_NOTES_0.9.4-dev.16.md`](RELEASE_NOTES_0.9.4-dev.16.md) — MatrixPortal one-item look-ahead qualification notes;
- [`RELEASE_NOTES_0.9.4-dev.15.md`](RELEASE_NOTES_0.9.4-dev.15.md) — MatrixPortal persistent-cache qualification notes;
- [`RELEASE_NOTES_0.9.4-dev.14.md`](RELEASE_NOTES_0.9.4-dev.14.md) — MatrixPortal transient-staging qualification notes;
- [`RELEASE_NOTES_0.9.4-dev.13.md`](RELEASE_NOTES_0.9.4-dev.13.md) — native-S3 Rescale cleanup notes;
- [`RELEASE_NOTES_0.9.4-dev.12.md`](RELEASE_NOTES_0.9.4-dev.12.md) — telemetry-consolidation build notes;
- [`RELEASE_NOTES_0.9.4-dev.11.md`](RELEASE_NOTES_0.9.4-dev.11.md) — qualified one-item Carousel GIF look-ahead build notes;
- [`RELEASE_NOTES_0.9.4-dev.10.md`](RELEASE_NOTES_0.9.4-dev.10.md) — persistent GIF source-reuse build notes and qualified baseline;
- [`RELEASE_NOTES_0.9.4-dev.9.md`](RELEASE_NOTES_0.9.4-dev.9.md) — qualified GIF source-stage refactor notes;
- [`RELEASE_NOTES_0.9.4-dev.8.md`](RELEASE_NOTES_0.9.4-dev.8.md) — qualified PSRAM telemetry-correction build notes;
- [`RELEASE_NOTES_0.9.4-dev.7.md`](RELEASE_NOTES_0.9.4-dev.7.md) — first Waveshare whole-file GIF PSRAM staging build;
- [`RELEASE_NOTES_0.9.4-dev.6.md`](RELEASE_NOTES_0.9.4-dev.6.md) — measurement-only PSRAM/cache baseline;
- [`RELEASE_NOTES_0.9.3.md`](RELEASE_NOTES_0.9.3.md) — current stable-release notes;
- [`RELEASE_NOTES_0.9.2.md`](RELEASE_NOTES_0.9.2.md) — previous stable 0.9.2 release notes;
- [`RELEASE_NOTES_0.9.0.md`](RELEASE_NOTES_0.9.0.md) — previous stable 0.9.0 release notes;
- [`RELEASE_NOTES_0.8.2.md`](RELEASE_NOTES_0.8.2.md) — stable pre-0.9 release notes;

Older release/development history is consolidated in `HISTORY.md`; this source
archive does not rely on release-note files that are not actually packaged.

## Related projects

Complementary iDotMatrix projects and protocol clients:

- [dallanwagz/idotmatrix-ha](https://github.com/dallanwagz/idotmatrix-ha)
- [markusressel/idotmatrix-api-client](https://github.com/markusressel/idotmatrix-api-client)
- [derkalle4/python3-idotmatrix-client](https://github.com/derkalle4/python3-idotmatrix-client)
- [8none1/idotmatrix](https://github.com/8none1/idotmatrix)
- [nj-designs/go-idot](https://github.com/nj-designs/go-idot)
- [whybutter/idotmatrix](https://github.com/whybutter/idotmatrix)

Most are clients/controllers for real iDotMatrix hardware. This repository makes
WLED behave as the BLE peripheral expected by the official app.

## Optional external Buzzer Usermod

Starting with 0.9.3, iDotMatrix contains no buzzer hardware backend. The optional
sound path is provided by **WLED Buzzer Usermod release 0.1.0 / build final** or newer. The final 0.1.0 service exposes the optional weak-link bridge used by iDotMatrix and retains the validated `triple_beep` one-shot with a 550 ms repeat gap. It owns GPIO allocation, Active/Passive hardware,
polarity, LEDC, volume and playback scheduling.

To enable sound, compile both repositories into the same WLED firmware:

```ini
custom_usermods =
  symlink://../wled-usermod-buzzer
  symlink://../wled-usermod-idotmatrix
```

The iDotMatrix settings page then exposes only **Buzzer → Enable**. Directly
under it the orange note states **Requires the WLED Buzzer Usermod.** If the
external service is not compiled into the firmware, the Enable checkbox is
shown disabled and no buzzer request is emitted.

iDotMatrix requests logical sound IDs rather than electrical waveforms:

| Event | Request |
|---|---|
| Alarm with buzzer flag | `triple_beep`, looped until Alarm stop |
| Program / Schedule sound flag | `notification`, once on activation |
| Natural Countdown completion | `triple_beep`, once |
| BLE application connection | `connect`, once |
| BLE application disconnection | `disconnect`, once |

The external Buzzer Usermod is therefore the only place that should configure or
drive the physical buzzer. iDotMatrix does not allocate a buzzer pin and does
not contain a local Test buzzer action.

## License

This project is licensed under the **European Union Public Licence (EUPL) v1.2**.
See [`LICENSE`](LICENSE).

The licence was chosen to align this WLED usermod with the current licensing of
WLED, which is distributed under EUPL v1.2 or later. WLED remains copyright of
Christian Schwinne and the individual WLED contributors. Third-party dependencies
used by this project remain subject to their respective licences.
