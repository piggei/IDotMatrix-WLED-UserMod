# WLED iDotMatrix Usermod

> **Release 0.8.1 / build 0.8.1-audit-fix1:** final public source release.
> This is the exact corrective build that completed the final ESP32-C3 hardware
> qualification after the independent release audit. It keeps the validated
> ESP32-C3 IDF5/shared-RMT path and the established classic-ESP32/WLED 16.0.1 path.

WLED Usermod for the ESP32 family that emulates an iDotMatrix BLE peripheral
and lets the official iDotMatrix app drive a WLED 2D matrix. WLED remains the
owner of normal LED output, effects, 2D segments, presets, playlists, brightness,
HTTP/JSON APIs, Home Assistant, mapping and network realtime protocols; the
Usermod adds the iDotMatrix-compatible BLE peripheral and renders app content
through the `iDotMatrix Display` WLED effect.

## Release status

Release **0.8.1** is the current public release. The released source remains
identified internally as build **0.8.1-audit-fix1**, because that is the exact
corrective revision that passed the final hardware qualification. Keeping the
validated build identifier makes the published source unambiguous without
changing the public release number.

Two stable build families are intentionally maintained because the proven LED/BLE
backend differs by MCU:

| Target | WLED base | Arduino / ESP-IDF | LED/BLE path | NimBLE |
|---|---|---|---|---|
| classic ESP32 (`esp32dev`) | WLED 16.0.1 | Arduino 2.0.17 / IDF 4.4.7 in supplied overrides | I2S LED output + BLE | 1.4.3 |
| ESP32-C3 4 MB | pinned WLED commit `d55037f7510541eddc390c8f3d01afc5787aa44a` | Arduino 3.3.8 / IDF 5.5.4 | WLED shared-RMT + BLE | 2.5.1 |

The C3 support decision is based on physical testing, not compilation alone. The
legacy IDF4 RMT path reproduced visible LED spikes, strongly amplified by BLE.
The pinned IDF5/shared-RMT path completed BLE advertising/connection, images,
animated GIFs, WLED effects, WebSocket use, program/schedule operation and
extended switching stress without reported LED instability or reboot. The final
`0.8.1-audit-fix1` qualification performed about 50 WLED effect changes, 50
iDotMatrix content/effect changes, and another 50 WLED effect changes. Its final
`/json/info` reported 75,296 bytes free heap, a 65,536-byte largest contiguous
block, `gifProbe=79444`, and a 69,632-byte probe largest block. The IDF5 reset
reason diagnostic reported `unknown`; no reset was observed during the run.

The C3 WLED base identifies itself as `17.0.0-devV5`, so WLED's Web UI displays
its normal development-build warning. That warning is expected; 0.8.1 pins the
exact tested WLED commit rather than an arbitrary nightly.

### Hardware validation scope

| Configuration | GIF backend | Status |
|---|---|---|
| 16x16 logical / 16x16 physical, classic ESP32 | `compact12/cache` | **supported and hardware-validated** |
| 16x16 logical / 16x16 physical, ESP32-C3 4 MB | `compact12/cache` | **supported and hardware-validated on IDF5/shared-RMT** |
| 32x32 logical -> 16x16 physical, `rescale=true`, classic ESP32 | `animatedgif11` | hardware-validated |
| 64x64 logical -> 16x16 physical, `rescale=true`, classic ESP32 without PSRAM, `64x64-lite` | `compact12/cache` | hardware-validated |
| 64x64 with PSRAM | `animatedgif12/psram` | implemented; hardware validation pending |
| ESP32-S3 / native physical 64x64 / HUB75 | depends on build | development work for the next release line |

### Compiled resolution and settings choices

The override determines the largest protocol/media profile compiled into the
firmware. The settings page never offers a profile larger than that capacity:

| Override / decoder | Available `ScreenType` values | `Rescale` |
|---|---|---|
| `platformio_override.ini.example` / LZW12 + `IDOT_SCREEN_MAX_DIM=16` | 16x16 | hidden and forced off |
| `platformio_override.ini.c3` / LZW12 + `IDOT_SCREEN_MAX_DIM=16` | 16x16 | hidden and forced off |
| `platformio_override.ini.32x32` / LZW11 | 16x16, 32x32 | available for tests |
| `.64x64` or `.64x64-lite` / LZW12 | 16x16, 32x32, 64x64 | available for tests |

For normal use select the `ScreenType` matching the physical WLED matrix.
`Rescale` exists for deliberate larger-logical-profile tests, not to preserve
32x32/64x64 detail on a 16x16 panel.

## Supported functionality

| Function | Protocol / source | WLED mapping | Status |
|---|---|---|---|
| Discovery | FA/AE GATT + manufacturer data | BLE Usermod | Verified |
| Screen power | FA02 | WLED power | Verified |
| Brightness | FA02 | WLED master brightness | Verified |
| Full-screen RGB | FA02 | `iDotMatrix Display` framebuffer | Verified; isolated from native WLED state |
| Standalone light effects (7) | FA02 `03 02` | locally rendered by `iDotMatrix Display` | Hardware-validated, including one-pixel scrolling for effects 3/4/5 |
| Audio/Rhythm (5 LEVEL + 5 FFT) | FA02 stream `06 00 00 02` / `21 00 01 02` | locally rendered by `iDotMatrix Display` | Hardware-validated |
| Countdown | FA02 `08 80` | local timer icon + `MM:SS` under `iDotMatrix Display` | Hardware-validated; async finish status on FA03 |
| Stopwatch | FA02 `09 80` | local timer icon + `MM:SS` under `iDotMatrix Display` | Hardware-validated |
| Scoreboard | FA02 `0A 80` | locally rendered blue/white/red score under `iDotMatrix Display` | Hardware-validated |
| Alarms | FA02 `00 80` | persistent time/day/media trigger under `iDotMatrix Display` | Implemented and hardware-tested with the official app |
| Programs / schedules | FA02 `07 80` + `05 80` | persistent weekday/time-window GIF/PNG/TEXT activities | Implemented and hardware-tested; finite activation sound |
| DIY/Graffiti | FA02 | `iDotMatrix Display` | Verified on 16x16; larger logical coordinates supported |
| Clock | FA02 | `iDotMatrix Display`, WLED local time | Verified on 16x16, 32x32 and 64->16 rescale |
| Text | bulk type `0x03` | app bitmaps rendered by `iDotMatrix Display` | Verified at matching logical/physical resolution |
| RAW/cloud image | bulk type `0x02` | atomic/downscaled RGB framebuffer | Verified on 16x16 and 64->16 rescale |
| Compact PNG | inline type `0x00` | decoded RGB/RGBA framebuffer | Verified on 16x16; larger-profile coverage remains partial |
| GIF animation | bulk type `0x01` | direct decoder or LittleFS frame cache | Verified on 16x16, 32->16 and 64->16 no-PSRAM path |
| 32x32 profile | profile `0x03` | logical profile + optional rescale | Hardware-validated with physical 16x16 |
| 64x64 profile | profile `0x04` | logical profile + optional low-memory rescale | Hardware-validated with physical 16x16/no PSRAM |

Alarms and programs/schedules include persistent metadata/media and active-buzzer
integration. Display rotation, energy-saving, and reset policy remain owned by WLED
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

`rescale` is intended only for testing and protocol/decoder diagnostics when the
selected logical iDotMatrix profile does not match the physical WLED matrix.
For normal use, choose a logical profile matching the physical display and leave
`rescale` disabled: deliberately loading 64x64 content onto a 16x16 panel cannot
preserve the original detail.

With `rescale=true`, logical protocol dimensions and renderer storage are
separate. A 64x64 logical profile driving a 16x16 WLED matrix stores a 16x16 RGB
canvas (**768 bytes**) instead of a 64x64 RGB canvas (**12,288 bytes**). RAW data
and GIF scanlines are sampled directly into the smaller storage canvas, avoiding
a second full logical framebuffer.

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
- larger logical classic/S3/HUB75 profiles remain in the repository for the
  previously documented validation/development cases and are **not** newly
  promoted by 0.8.1.

Every supplied override sets `custom_usermods` to only
`symlink://../wled-usermod-idotmatrix`; do not inherit WLED's default Usermod
list implicitly.

### 2a. ESP32-C3: pinned WLED source

A reproducible C3 checkout is:

```bash
git clone https://github.com/wled/WLED.git WLED-idot-c3
cd WLED-idot-c3
git checkout d55037f7510541eddc390c8f3d01afc5787aa44a
cp ../wled-usermod-idotmatrix/platformio_override.ini.c3 platformio_override.ini
```

Use Node.js 20+ and PlatformIO. Under WSL/Linux:

```bash
pio run -e esp32c3dev_idotmatrix_16x16 -t clean
pio run -e esp32c3dev_idotmatrix_16x16
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
- `buzzerActiveHigh`: selects active-high or active-low buzzer polarity.

## Runtime status

`/json/info` exposes compact diagnostics under `u.iDotMatrix`. A 64x64
classic-ESP32/no-PSRAM build may report:

```text
BLE connected
profile=64x64
canvas=16x16
name=IDM-123456
release=0.8.1
build=0.8.1-audit-fix1
gifDecoder=compact12/cache
gifDecoderBytes=16128
gifProbe=... largest=... reserve=10240
gifCachedFrames=...
gifCacheWaits=... low=... guard=9216
reset=poweron
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
release=0.8.1
build=0.8.1-audit-fix1
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

Every app-originated visual mode selects the single `iDotMatrix Display` WLED
effect, including full-screen RGB and the seven standalone light effects. WLED
therefore exposes app content as a framebuffer source instead of pretending the
WLED and iDotMatrix apps share a synchronized Solid/effect state. Selecting a
normal WLED effect manually is an explicit source change and replaces app content
until the app sends another supported content command.

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

The automatic PSRAM direct backend is implemented but has not yet been tested on
the pending PSRAM hardware. Native physical 64x64 output and HUB75 DMA are also
outside the 0.8.1 release-validation matrix.

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
- [`HISTORY.md`](HISTORY.md) — release/development history;
- [`TODO.md`](TODO.md) — roadmap after 0.8.1;
- [`RELEASE_NOTES_0.8.1.md`](RELEASE_NOTES_0.8.1.md) — Release 0.8.1 notes;
- [`AUDIT_REMEDIATION_0.8.1.md`](AUDIT_REMEDIATION_0.8.1.md) — corrective audit pass and deferred items;
- [`TEST_REPORT_0.8.1-audit-fix1.md`](TEST_REPORT_0.8.1-audit-fix1.md) — host regression and sanitizer results for this build.

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

Version 0.8.1 retains the optional active-buzzer support from 0.8.0. Choose the buzzer GPIO in
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
