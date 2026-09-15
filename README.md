# Development line 0.9

**Current development release: 0.9.0 / build 0.9.0-dev.12.**

This branch starts the ESP32-S3 / PSRAM / native WLED HUB75 generation. The first
target is the Adafruit MatrixPortal ESP32-S3 driving one 64x64 HUB75 panel on
WLED 0.17. Stable 0.8.2 remains the recommended ESP32-C3 / 16x16 release.

For the first 64x64 hardware build use
`platformio_override.ini.matrixportal-s3-hub75`.

# WLED iDotMatrix Usermod

> **Release 0.8.2 / build 0.8.2:** stable release for the hardware-qualified ESP32/ESP32-C3 line, with protocol convergence, persistent Carousel support, optional AudioReactive input, and the audited transport/storage fixes validated through the 0.8.2 release-candidate cycle.

> It is based on the hardware-qualified 0.8.1 code and keeps optional WLED
> AudioReactive input for the existing iDotMatrix Audio/Rhythm visualizers.

WLED Usermod for the ESP32 family that emulates an iDotMatrix BLE peripheral
and lets the official iDotMatrix app drive a WLED 2D matrix. WLED remains the
owner of normal LED output, effects, 2D segments, presets, playlists, brightness,
HTTP/JSON APIs, Home Assistant, mapping and network realtime protocols; the
Usermod adds the iDotMatrix-compatible BLE peripheral and renders app content
through the `iDotMatrix` WLED effect.

> **BLE compatibility/security:** the compatibility GATT profile is intentionally
> unauthenticated, matching the observed original-device/app exchange. A nearby
> BLE peer can therefore send iDotMatrix commands, including persistent/reset
> commands. CRC fields provide integrity checking, not authentication. Disable
> the Usermod/BLE service when this proximity-access model is not acceptable.

## Release status

**0.8.2** is the current stable release. It keeps the AudioReactive source integration, Device Info /
Schedule ACK alignment, the hardware-validated TEXT renderer and the persistent
12-slot Device Assets bank. The release also hardens the current ESP32-C3 path with single-owner FA02 reassembly, audio/normal-command routing,
Carousel bad-slot handling, reusable per-slot GIF caches, transactional cached
GIF replacement, strict framebuffer ownership during GIF staging (preventing
Clock/TEXT bleed into cold caches), filesystem recovery/capacity checks, reset
verification, and WLED-playlist termination when iDotMatrix explicitly takes
display ownership.
ESP32-S3, native HUB75, PSRAM and 16/32/64 logical-profile output scaling are hardware-validated on MatrixPortal S3. Build 0.9.0-dev.12 keeps the validated transfer artwork and Carousel diagnostics, and fixes the indeterminate left/right activity bar by explicitly scheduling periodic WLED redraws while an upload is active. Hardware diagnostics proved that the setup packet describes the full 12-slot bank rather than the real session asset count, so the firmware deliberately avoids presenting a false percentage. The bar fills completely only when the upload quiet-period confirms session completion.

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

A dedicated C3 AudioReactive profile is supplied as `platformio_override.ini.c3-audio`.
The normal `platformio_override.ini.c3` remains iDotMatrix-only so users who do
not need local audio keep the lower RAM footprint. The C3 audio profile uses the
same pinned WLED IDF5/shared-RMT base and NimBLE 2.5.1 as the validated 0.8.1
C3 build; it only adds `audioreactive` to `custom_usermods`.

### Persistent Device Assets / Carousel

The Usermod stores one 12-slot Device Assets bank in LittleFS. GIF and TEXT
slots may be mixed and each slot retains its app-provided dwell time. After a
page upload, playback starts automatically once the transfer stream becomes
quiet; no separate app-side "play Carousel" command is required.

WLED remains authoritative at boot. If `iDotMatrix` is configured as
the WLED boot effect and a valid stored Carousel exists, that Carousel becomes
the startup content. Selecting a native WLED effect suspends Carousel playback
without deleting the bank; selecting `iDotMatrix` again resumes it.

0.8.2 retains Carousel GIF playback so an unchanged stored slot can reuse its
per-slot frame cache instead of copying the GIF and rebuilding `/idot_cache.bin`
on every visit. Invalid slots are quarantined for the current bank generation
and later valid slots continue to play. Feature-owned temp/backup files are
reconciled at boot.

### Stable hardware baseline

Release 0.8.2 retains the two hardware families qualified in 0.8.1 and extends the ESP32-C3 validation with Carousel/cache, reset, boot, automation and local-microphone AudioReactive testing:

| Target | WLED base | Arduino / ESP-IDF | LED/BLE path | NimBLE |
|---|---|---|---|---|
| classic ESP32 (`esp32dev`) | WLED 16.0.1 | Arduino 2.0.17 / IDF 4.4.7 in supplied overrides | I2S LED output + BLE | 1.4.3 |
| ESP32-C3 4 MB | pinned WLED commit `d55037f7510541eddc390c8f3d01afc5787aa44a` | Arduino 3.3.8 / IDF 5.5.4 | WLED shared-RMT + BLE | 2.5.1 |

The 0.8.2 C3 qualification additionally covered persistent mixed Carousel playback and boot restore, Carousel-to-Carousel replacement, verified reset cleanup, alarms/programs, clock artwork, and WLED AudioReactive input from an external microphone. The pinned C3 WLED/IDF5/shared-RMT base remains unchanged from the qualified 0.8.1 platform baseline.

### Hardware validation scope

| Configuration | GIF backend | Status |
|---|---|---|
| 16x16 logical / 16x16 physical, classic ESP32 | `compact12/cache` | **supported and hardware-validated** |
| 16x16 logical / 16x16 physical, ESP32-C3 4 MB | `compact12/cache` | **supported and hardware-validated on IDF5/shared-RMT** |
| 32x32 logical -> 16x16 physical, `rescale=true`, classic ESP32 | `animatedgif11` | hardware-validated |
| 64x64 logical -> 16x16 physical, `rescale=true`, classic ESP32 without PSRAM, `64x64-lite` | `compact12/cache` | hardware-validated |
| 64x64 logical / 64x64 physical, MatrixPortal ESP32-S3 + PSRAM | `animatedgif12/psram` | **hardware-validated on WLED 0.17 beta / native HUB75** |
| 32x32 logical -> 64x64 physical, MatrixPortal ESP32-S3 | `animatedgif12/psram` | **hardware-validated; automatic 2x nearest-neighbour upscale** |
| 16x16 logical -> 64x64 physical, MatrixPortal ESP32-S3 | `animatedgif12/psram` | **hardware-validated; automatic 4x nearest-neighbour upscale** |

### Compiled resolution and settings choices

The override determines the largest protocol/media profile compiled into the
firmware. The settings page never offers a profile larger than that capacity:

| Override / decoder | Available `ScreenType` values | `Rescale` |
|---|---|---|
| `platformio_override.ini.example` / LZW12 + `IDOT_SCREEN_MAX_DIM=16` | 16x16 | hidden and forced off |
| `platformio_override.ini.c3` / LZW12 + `IDOT_SCREEN_MAX_DIM=16` | 16x16 | hidden and forced off |
| `platformio_override.ini.c3-audio` / same media profile + AudioReactive | 16x16 | hidden and forced off |
| `platformio_override.ini.32x32` / LZW11 | 16x16, 32x32 | available for tests |
| `.64x64` or `.64x64-lite` / LZW12 | 16x16, 32x32, 64x64 | available for tests |
| `platformio_override.ini.matrixportal-s3-hub75` / LZW12 + PSRAM | 16x16, 32x32, 64x64 | automatic physical-output scaling; primary 0.9 target |

On the 0.9 native-matrix path, `ScreenType` is the logical iDotMatrix profile and may differ from the physical WLED matrix. Output scaling is automatic in both directions for 16x16, 32x32 and 64x64 logical/physical combinations. `Rescale` is retained for backward compatibility with the older low-memory 0.8.x storage path, not as a requirement for 0.9 output scaling.

## Supported functionality

| Function | Protocol / source | WLED mapping | Status |
|---|---|---|---|
| Discovery | FA/AE GATT + manufacturer data | BLE Usermod | Verified |
| Screen power | FA02 | WLED power | Verified |
| Brightness | FA02 | WLED master brightness | Verified |
| Full-screen RGB | FA02 | `iDotMatrix` framebuffer | Verified; isolated from native WLED state |
| Standalone light effects (7) | FA02 `03 02` | locally rendered by `iDotMatrix` | Hardware-validated, including one-pixel scrolling for effects 3/4/5 |
| Audio/Rhythm (5 LEVEL + 5 FFT) | FA02 stream `06 00 00 02` / `21 00 01 02`; optional WLED AudioReactive source | locally rendered by `iDotMatrix` | Phone/BLE path retained; local AudioReactive/external-microphone path hardware-validated in 0.8.2 |
| Countdown | FA02 `08 80` | local timer icon + `MM:SS` under `iDotMatrix` | Hardware-validated; async finish status on FA03 |
| Stopwatch | FA02 `09 80` | local timer icon + `MM:SS` under `iDotMatrix` | Hardware-validated |
| Scoreboard | FA02 `0A 80` | locally rendered blue/white/red score under `iDotMatrix` | Hardware-validated |
| Alarms | FA02 `00 80` | persistent time/day/media trigger under `iDotMatrix` | Implemented and hardware-tested with the official app |
| Programs / schedules | FA02 `07 80` + `05 80` | persistent weekday/time-window GIF/PNG/TEXT activities | Implemented and hardware-tested; finite activation sound |
| DIY/Graffiti | FA02 | `iDotMatrix` | Verified on 16x16; larger logical coordinates supported |
| Clock | FA02 | `iDotMatrix`, WLED local time | Verified on 16x16, 32x32 and 64->16 rescale |
| Text | bulk type `0x03` | app bitmaps rendered by `iDotMatrix` | Verified at matching logical/physical resolution |
| RAW/cloud image | bulk type `0x02` | atomic/downscaled RGB framebuffer | Verified on 16x16 and 64->16 rescale |
| Compact PNG | inline type `0x00` | decoded RGB/RGBA framebuffer | Verified on 16x16; larger-profile coverage remains partial |
| GIF animation | bulk type `0x01` | direct decoder or LittleFS frame cache | Verified on 16x16, 32->16 and 64->16 no-PSRAM path |
| 32x32 profile | profile `0x03` | logical profile + optional rescale | Hardware-validated with physical 16x16 |
| 64x64 profile | profile `0x04` | logical profile + optional low-memory rescale | Hardware-validated with physical 16x16/no PSRAM |

Alarms and programs/schedules include persistent metadata/media and active-buzzer
integration. Display rotation and energy-saving remain owned by WLED; the verified `03 80` protocol reset clears Usermod-owned Carousel/alarm/program state without rebooting WLED
rather than duplicated in the BLE emulator.

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
- NimBLE-Arduino 2.5.1 and AnimatedGIF 1.4.7 as pinned by `platformio_override.ini.c3`.

The direct active-buzzer output was not part of the successful C3 hardware qualification. The tested 5 V active buzzer was too weak when driven directly from C3 GPIO; use an external transistor/MOSFET driver if that buzzer hardware is required. This does not affect matrix/BLE support.

PSRAM is not required for either supported 16x16 target.

### Important ESP32/WLED constraints

The classic ESP32 and C3 must not be treated as interchangeable build targets.
On classic ESP32 the verified BLE build requires I2S LED output and retains the
RMT safety block. On C3, legacy IDF4 RMT was experimentally shown to produce
pixel spikes; the supported C3 profile is therefore compile-time guarded so it
only builds on ESP-IDF 5 with `WLED_USE_SHARED_RMT` and NimBLE 2.x.

All supplied release profiles use a single-application no-OTA partition layout
and define `WLED_DISABLE_OTA`. Flash them by USB/serial. An official WLED OTA
image does not contain this out-of-tree Usermod and would replace the customized
firmware.

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

### 1. Place the Usermod beside WLED

The directory name is significant because the supplied overrides use a relative
symlink:

```text
<workdir>/WLED/
<workdir>/wled-usermod-idotmatrix/
```

### 2. Choose the matching release profile

- classic ESP32 16x16: `platformio_override.ini.example` with WLED 16.0.1;
- ESP32-C3 16x16: `platformio_override.ini.c3` with the pinned WLED IDF5 commit;
- ESP32-C3 16x16 + AudioReactive: `platformio_override.ini.c3-audio` on the same
  pinned WLED base;
- larger logical classic/S3/HUB75 profiles remain in the repository for the
  previously documented validation/development cases and are not promoted by
  this 0.8.2 release.

Normal overrides keep only iDotMatrix in `custom_usermods`. The C3 audio profile
explicitly lists `audioreactive` plus iDotMatrix; no profile inherits WLED's
default Usermod list implicitly.

### 2a. ESP32-C3: pinned WLED source

A reproducible C3 checkout is:

```bash
git clone https://github.com/wled/WLED.git WLED-idot-c3
cd WLED-idot-c3
git checkout d55037f7510541eddc390c8f3d01afc5787aa44a
cp ../wled-usermod-idotmatrix/platformio_override.ini.c3 platformio_override.ini
```

Use Node.js 20+ and PlatformIO. Under WSL/Linux, the normal C3 build is:

```bash
pio run -e esp32c3dev_idotmatrix_16x16 -t clean
pio run -e esp32c3dev_idotmatrix_16x16
```

For C3 + AudioReactive, replace the copied override and build its separate
environment:

```bash
cp ../wled-usermod-idotmatrix/platformio_override.ini.c3-audio platformio_override.ini
pio run -e esp32c3dev_idotmatrix_audio_16x16 -t clean
pio run -e esp32c3dev_idotmatrix_audio_16x16
```

Do not substitute WLED 16.0.1 or a random nightly for the C3 release build. The
source intentionally fails compilation if IDF5/shared-RMT/NimBLE 2.x are absent.

### 3. Clean and build

Classic ESP32 / WLED 16.0.1:

```bash
cp ../wled-usermod-idotmatrix/platformio_override.ini.example platformio_override.ini
pio run -e esp32dev_idotmatrix_16x16 -t clean
pio run -e esp32dev_idotmatrix_16x16
```

C3 uses the commands in the previous section.

### 4. Upload

```bash
# classic ESP32
pio run -e esp32dev_idotmatrix_16x16 -t upload

# ESP32-C3
pio run -e esp32c3dev_idotmatrix_16x16 -t upload

# ESP32-C3 + AudioReactive
pio run -e esp32c3dev_idotmatrix_audio_16x16 -t upload
```

With WSL2, attach the USB device to WSL using `usbipd-win` before upload. The
one-time `usbipd bind` persists; `usbipd attach --wsl --busid <BUSID>` normally
must be repeated after reconnect/reboot. PlatformIO can then use `/dev/ttyACM*`.

### 5. Configure WLED

For the supported 16x16 setups:

1. configure a **16x16 2D matrix**;
2. classic ESP32: select an **I2S** digital LED output, not RMT;
3. C3: use the normal WLED C3 digital output on the pinned shared-RMT stack;
4. configure Wi-Fi/timezone/NTP as required;
5. keep `screenType=16x16`; `Rescale` is hidden in the supported 16x16 profiles;
6. optionally change the BLE name suffix;
7. reboot/reconnect the iDotMatrix app after BLE-name/profile changes;
8. buzzer support is optional and is not required for target validation.

### 6. Pair from the iDotMatrix app

Open the official iDotMatrix app and scan for the configured `IDM-...` device.
A successful connection should let the app control the WLED matrix directly.

## Configuration options

- `enabled`: enables the BLE emulator;
- `screenType`: logical profile (`16x16`, `32x32`, `64x64`);
- `deviceName`: editable BLE-name suffix shown after the fixed `IDM-` prefix;
  when no name is saved, a stable six-digit default (`IDM-xxxxxx`) is derived
  from the ESP32 eFuse MAC;
- `rescale`: test-only nearest-neighbour mapping from a deliberately mismatched logical profile to the selected WLED 2D segment/storage canvas; hidden and forced off in the standard 16x16 build;
- `buzzer-pin`: optional GPIO for an active buzzer; leave unassigned to disable it;
- `buzzerActiveHigh`: selects active-high or active-low buzzer polarity;
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
release=0.8.2
build=0.8.2
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
release=0.8.2
build=0.8.2
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

### Brightness direction

Brightness is synchronized from the iDotMatrix app to WLED. The app does not
query the current WLED brightness from the emulated peripheral, so changing
brightness elsewhere in WLED does not necessarily move the app slider.

### Clock source

WLED remains the primary clock authority whenever its local time is valid.
Configure WLED NTP, timezone, and daylight-saving settings normally. The last
valid app time-synchronization packet is retained and used as an offline fallback
while WLED local time is not yet valid.

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

The PSRAM/direct backend, native physical 64x64 output and WLED native HUB75 DMA remain outside the 0.8.2 stable release matrix, but are hardware-validated in the 0.9 development line on Adafruit MatrixPortal ESP32-S3 with one physical 64x64 HUB75 panel. Validation includes native 64x64 operation plus logical 32x32 -> 64x64 and 16x16 -> 64x64 automatic upscale, BLE, direct AnimatedGIF/PSRAM playback, Carousel, TEXT including the 32x64 glyph path, and WLED/iDotMatrix ownership transitions.

## Repository layout

- `usermod_idotmatrix.cpp` — Usermod lifecycle, configuration, startup guards, runtime status;
- `IDotMatrixBLEServer.*` — NimBLE GATT server, reassembly, notifications;
- `IDotMatrixFA02Assembler.*` — bounded fragmented FA02 reconstruction;
- `IDotMatrixBulkTransfer.*` — bulk framing, CRC42, TEXT/RAW/GIF chunk state;
- `IDotMatrixProtocol.*` — protocol validation and command decoding;
- `IDotMatrixBuildProfile.h` — compile-time ScreenType/Rescale capability policy;
- `IDotMatrixRenderer.*` — RGB storage canvas and all local visual rendering;
- `IDotMatrixMedia.*` / `IDotMatrixMediaSink.h` — PNG/GIF/RAW/TEXT media boundary, RX files, direct playback, and frame-cache orchestration;
- `IDotMatrixCompactGif.*` — compact-safe full-code-space LZW12 predecoder for no-PSRAM 64x64;
- `IDotMatrixWLEDAdapter.*` — protocol-to-WLED state, ownership, staging, timers, scoreboard, audio, and display effect;
- `IDotMatrixAutomation.*` — persistent alarms and program/schedule execution;
- `IDotMatrixBuzzer.*` — non-blocking active-buzzer pattern engine;
- `patch_animatedgif_profiles.py` — selects 10/11/12-bit AnimatedGIF build profile;
- `WLED_ESP32_*MB_IDOT_NO_OTA.csv` — 4/8/16/32 MB single-app partition tables;
- `platformio_override.ini.*` — media profiles and WLED hardware-target wrappers;
- `tests/` — host regression tests and compact-GIF fixtures.

Further documentation:

- [`BUILD_PROFILES.md`](BUILD_PROFILES.md) — media profiles, 4/8/16 MB and ESP32-S3 targets, HUB75 wrappers, partitions, and build commands;
- [`PROTOCOL.md`](PROTOCOL.md) — implemented wire-protocol subset;
- [`ARCHITECTURE.md`](ARCHITECTURE.md) — component boundaries, current memory model, and RAM-engineering history;
- [`TESTING.md`](TESTING.md) — host/build/hardware regression procedure;
- [`HARDWARE_TEST_CHECKLIST_0.8.2.md`](HARDWARE_TEST_CHECKLIST_0.8.2.md) — final ESP32-C3 0.8.2 hardware qualification record/checklist;
- [`HISTORY.md`](HISTORY.md) — release/development history;
- [`TODO.md`](TODO.md) — post-0.8.2/new-hardware roadmap;
- [`RELEASE_NOTES_0.8.2.md`](RELEASE_NOTES_0.8.2.md) — stable 0.8.2 release notes;
- [`TEST_REPORT_0.8.2.md`](TEST_REPORT_0.8.2.md) — stable 0.8.2 qualification report;
- [`RELEASE_NOTES_0.8.1.md`](RELEASE_NOTES_0.8.1.md) — stable Release 0.8.1 notes;
- [`AUDIT_REMEDIATION_0.8.1.md`](AUDIT_REMEDIATION_0.8.1.md) — corrective audit pass and deferred items;
- [`TEST_REPORT_0.8.1-audit-fix1.md`](TEST_REPORT_0.8.1-audit-fix1.md) — stable 0.8.1 host regression and sanitizer results.

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

## Optional active buzzer

Version 0.8.2 retains the optional active-buzzer support from 0.8.1. Choose the buzzer GPIO in
**Config → Usermods → iDotMatrix** and set `buzzerActiveHigh` to match the module
polarity. Leaving the pin unassigned disables buzzer hardware. After saving,
**Test buzzer** emits one finite three-short-beep trill so wiring and polarity
can be checked immediately.

The driver is fully non-blocking. Alarms use the repeating trill for their
configured duration when the app requests sound; program/schedule sound is a
finite activation notice of three groups of three short trills. Passive/PWM
buzzers are not enabled yet.

On WLED 0.16.x an out-of-tree Usermod cannot register its own unique `PinOwner`
without modifying the WLED core. This Usermod therefore refuses GPIOs already
owned by WLED and exposes the configured pin to the Usermods settings pin scanner,
but deliberately does not reuse another Usermod's owner ID.

## License

This project is licensed under the **European Union Public Licence (EUPL) v1.2**.
See [`LICENSE`](LICENSE).

The licence was chosen to align this WLED usermod with the current licensing of
WLED, which is distributed under EUPL v1.2 or later. WLED remains copyright of
Christian Schwinne and the individual WLED contributors. Third-party dependencies
used by this project remain subject to their respective licences.

### Automatic native-matrix upscale

In the 0.9 line, an iDotMatrix logical profile smaller than the selected WLED 2D matrix is automatically enlarged at output using nearest-neighbour sampling. This makes 16x16 -> 32x32, 16x16 -> 64x64 and 32x32 -> 64x64 normal supported display paths. The legacy `rescale` switch remains for deliberate logical-downscale tests.
