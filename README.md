# WLED iDotMatrix Usermod

> **Stable 0.8.0:** adds seven standalone light effects, countdown, stopwatch,
> scoreboard, persistent alarms and programs/schedules, optional active-buzzer
> support, and five LEVEL plus five FFT Audio/Rhythm visualizers. It retains the
> validated 0.7.1 media/memory architecture and makes the settings page follow
> the decoder capacity compiled by each build profile.
>
> **Validation boundary:** classic-ESP32 16x16, 32x32 logical-to-16x16, and the
> no-PSRAM 64x64 logical-to-16x16 compact-cache path are validated. The release
> now also supplies WLED 16.0.1 build profiles for classic ESP32 8/16 MB,
> ESP32-WROVER/PSRAM, ESP32-S3 8/16 MB PSRAM targets, and WLED's native HUB75
> environments; those
> additional hardware combinations remain pending physical validation.

WLED Usermod for the ESP32 family that emulates an iDotMatrix BLE peripheral
and lets the official iDotMatrix app drive a WLED 2D matrix. The stable hardware
baseline is classic ESP32; PSRAM-capable WROVER/ESP32-S3 and HUB75 targets are
supplied for the next validation phase.

The project implements the peripheral/server side of the protocol: WLED
advertises the expected BLE services, accepts commands from the app, validates
bulk transfers, and renders supported content through one WLED effect named
`iDotMatrix Display`.

## Release status

Version **0.8.0** is the current stable release.

It keeps the Usermod runtime behavior validated through `0.8.0-dev.20` while
finalizing release metadata, documentation, tests, partitions, and the expanded
PlatformIO hardware-target matrix.
The release combines the proven 0.7.1 media/memory architecture with the complete
feature layer: source-isolated app rendering, seven light effects, timers and
scoreboard, persistent alarms and schedules, active-buzzer integration, ten
Audio/Rhythm visualizers, and build-aware resolution settings.

The earlier `0.8.0-dev.2` through `dev.8` entries were experimental larger-profile
work that ultimately became stable 0.7.1. They remain in `HISTORY.md` only as
engineering history.

### Hardware validation scope

| Configuration | GIF backend | Status |
|---|---|---|
| 16x16 logical / 16x16 physical, classic ESP32 | `animatedgif10` | Hardware-validated stable path, including the 0.8.0 feature set |
| 32x32 logical -> 16x16 physical, `rescale=true`, classic ESP32 | `animatedgif11` | Hardware-validated: clock/text and repeated GIF playback |
| 64x64 logical -> 16x16 physical, `rescale=true`, classic ESP32 without PSRAM, `64x64-lite` build | `compact12/cache` | Hardware-validated: static images, clocks, large/100-frame GIFs, repeated GIF replacement, WLED/clock/image transitions, responsive Web UI |
| 64x64 with PSRAM | `animatedgif12/psram` | Implemented; hardware validation still pending |
| Native physical 64x64 and HUB75 DMA | depends on build | Not yet release-validated |

### Compiled resolution and settings choices

The override determines the largest protocol/media profile compiled into the
firmware. The settings page never offers a profile larger than that capacity:

| Override / decoder | Available `ScreenType` values | `Rescale` |
|---|---|---|
| `platformio_override.ini.example` / LZW10 | 16x16 | hidden and forced off |
| `platformio_override.ini.32x32` / LZW11 | 16x16, 32x32 | available for tests |
| `.64x64` or `.64x64-lite` / LZW12 | 16x16, 32x32, 64x64 | available for tests |

For normal use, select the `ScreenType` matching the physical WLED matrix.
`Rescale` exists only to test a larger logical protocol/decoder profile on a
smaller panel. It is not a way to retain 32x32 or 64x64 image detail on 16x16.

When firmware with a smaller compiled maximum loads an older configuration,
the profile is reduced to the nearest supported value: 64x64 becomes 32x32 in
an LZW11 build, while 32x32 or 64x64 becomes 16x16 in the standard build.

The final classic-ESP32 64x64 validation included more than ten consecutive GIF
replacements, large animations, clock/date -> GIF, WLED effect -> GIF, static
image -> GIF, return to normal WLED effects, and live `/json/info` access while
media was active. No reboot, WLED Error 8/90, or manual Solid reset was required.

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

### Validated classic-ESP32 target

The release was developed and tested with:

- **classic ESP32** compatible with PlatformIO `esp32dev`;
- at least **4 MB flash**;
- WLED **16.0.1**;
- a WLED 2D matrix using a digital LED output supported by the **I2S** backend;
- Wi-Fi for normal WLED operation;
- BLE enabled by the ESP32 hardware;
- USB/serial access for the supplied no-OTA build layout.

PSRAM is **not required** for the validated 16x16 path, the 32->16 path, or the
validated classic-ESP32 64->16 `compact12/cache` path.

PSRAM is still recommended for native large physical matrices, full 64x64
framebuffers, HUB75 DMA, or builds that retain many additional WLED integrations.
When `IDOT_GIF_LZW12` is compiled and PSRAM is detected at runtime, the Usermod
selects the full AnimatedGIF direct-playback path automatically.

### Important ESP32/WLED constraints

The verified build uses WLED's I2S LED backend. **Do not use RMT** for a digital
LED bus in this BLE build: on the tested classic ESP32/framework combination,
RMT-HI conflicts with the Bluetooth controller and can cause a reboot loop. The
Usermod detects this condition and refuses to start BLE.

The BLE-enabled firmware also exceeds the normal OTA application slot used by
the tested 4 MB layout. All supplied iDotMatrix targets therefore use a
single-application partition table and define `WLED_DISABLE_OTA`. Matching
4/8/16 MB layouts are supplied for the standard targets, with a 32 MB layout
used by the Waveshare HUB75 wrapper. This is also an intentional safety policy:
an official WLED binary does not contain this Usermod and would replace the
customized firmware. Flash the supplied environments over USB/serial.

An advanced user may design an OTA-capable build, remove `WLED_DISABLE_OTA`, and
update over the network, but both OTA slots must fit the customized firmware and
the uploaded image must contain this Usermod and match the already-installed
partition layout. OTA variants are not part of the release-validation matrix.
See [`BUILD_PROFILES.md`](BUILD_PROFILES.md) for the partition and target policy.

The Usermod forces Wi-Fi modem sleep on (`noWifiSleep = false`) because it is
required for Wi-Fi/Bluetooth coexistence with the pinned ESP-IDF generation.

## Software requirements

| Component | Version / setting |
|---|---|
| WLED | 16.0.1 |
| PlatformIO environment | validated: `esp32dev_idotmatrix`; additional targets in `BUILD_PROFILES.md` |
| Platform | `espressif32@~6.13.0` |
| Arduino-ESP32 | 2.0.17 |
| ESP-IDF | 4.4.7 |
| BLE stack | `h2zero/NimBLE-Arduino@1.4.3` |
| GIF library | `bitbank2/AnimatedGIF@1.4.7` |
| LED driver | I2S |

Other WLED/framework/ESP32 variants may work, but they are not part of the
0.8.0 release-validation scope.

## GIF decoder profiles and memory model

The maximum GIF profile is selected at build time because AnimatedGIF 1.4.7
stores its LZW tables inside the decoder object:

- default: **10-bit / 16x16** low-RAM decoder;
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

Clone or extract this repository beside the WLED source directory. For example:

```text
D:\WLED source\WLED-16.0.1
D:\WLED source\wled-usermod-idotmatrix
```

The directory name `wled-usermod-idotmatrix` is assumed by the supplied example
overrides. If you use another directory name, update their paths accordingly.

### 2. Choose a media profile and hardware target

The override file selects the **media/decoder profile**:

- `platformio_override.ini.example` — standard 16x16 / LZW10;
- `platformio_override.ini.32x32` — maximum 32x32 / compact LZW11;
- `platformio_override.ini.64x64` — maximum 64x64 / complete LZW12, normal WLED feature set;
- `platformio_override.ini.64x64-lite` — complete LZW12 with selected optional WLED integrations removed to preserve classic-ESP32 internal RAM; the 4 MB target is the **hardware-validated no-PSRAM 64->16 configuration**;
- `platformio_override.ini.hub75` — wrappers around WLED 16.0.1's native HUB75 environments, with complete LZW12 support; experimental in this project.

The normal `example`, `32x32`, and `64x64` files each expose seven hardware targets:

- `esp32dev_idotmatrix` — classic ESP32, 4 MB;
- `esp32dev_8M_idotmatrix` — classic ESP32, 8 MB;
- `esp32dev_16M_idotmatrix` — classic ESP32, 16 MB;
- `esp32_wrover_idotmatrix` — classic ESP32-WROVER, 4 MB flash + PSRAM;
- `esp32s3dev_8MB_opi_idotmatrix` — ESP32-S3, 8 MB flash, OPI PSRAM;
- `esp32s3dev_8MB_qspi_idotmatrix` — ESP32-S3, 8 MB flash, QSPI PSRAM;
- `esp32s3dev_16MB_opi_idotmatrix` — ESP32-S3, 16 MB flash, OPI PSRAM.

`64x64-lite` intentionally contains only the three classic-ESP32 targets. The
HUB75 file instead wraps board/pinout-specific WLED environments; do not select
one solely by flash size. See [`BUILD_PROFILES.md`](BUILD_PROFILES.md) for the
complete target/HUB75 matrix, partition layout, PSRAM notes, and validation status.

The normal profiles preserve the custom Usermods inherited from the selected
WLED base environment before adding `wled-usermod-idotmatrix`. The `64x64-lite`
profile deliberately omits inherited Usermods to protect the contiguous
internal-DRAM margin used by the no-PSRAM compact12 GIF cache path.

Copy the appropriate file to WLED's `platformio_override.ini`, then build the
environment matching the actual controller.

Do **not** define `IDOT_GIF_LZW11` and `IDOT_GIF_LZW12` together.
Do **not** add `esp-nimble-cpp` or the registry package `ESP32 BLE Arduino`.

### 3. Clean and build

From the WLED source directory, for the validated classic 4 MB baseline:

```powershell
pio run -e esp32dev_idotmatrix -t clean
pio run -e esp32dev_idotmatrix
```

For example, after copying `platformio_override.ini.64x64`, an ESP32-S3 with
16 MB flash and OPI PSRAM is built with:

```powershell
pio run -e esp32s3dev_16MB_opi_idotmatrix -t clean
pio run -e esp32s3dev_16MB_opi_idotmatrix
```

The AnimatedGIF patch banner should match the selected build profile, for
example:

```text
[iDotMatrix] AnimatedGIF 1.4.7 profile: 12-bit LZW / max 64x64 / dict=4096 / filebuf=1024
```

### 4. Upload

```powershell
pio run -e esp32dev_idotmatrix -t upload
```

Optional serial monitor:

```powershell
pio device monitor -e esp32dev_idotmatrix
```

### 5. Configure WLED

For the validated physical 16x16 setup:

1. configure the panel as a **16x16 2D matrix**;
2. use an **I2S** digital LED output, not RMT;
3. configure Wi-Fi, timezone, and NTP if you want the clock to be correct;
4. choose an available Usermod `screenType`, normally matching the physical
   matrix; the compiled override determines whether 16x16, 32x32, and/or 64x64
   are offered;
5. enable `rescale` only for a deliberate profile/matrix test;
6. optionally edit the `deviceName` suffix shown after the fixed `IDM-` prefix;
7. reboot and reconnect the iDotMatrix app after changing the BLE device name or logical profile;
8. save the Usermod configuration before pressing **Test buzzer**.

For the validated classic-ESP32 64x64 logical test, use
`platformio_override.ini.64x64-lite`, set `screenType=64x64`, enable
`rescale=true`, and keep the physical WLED matrix at 16x16.

### 6. Pair from the iDotMatrix app

Open the official iDotMatrix app and scan for the configured `IDM-...` device.
A successful connection should let the app control the WLED matrix directly.

## Configuration options

- `enabled`: enables the BLE emulator;
- `screenType`: logical profile (`16x16`, `32x32`, `64x64`);
- `deviceName`: editable BLE-name suffix shown after the fixed `IDM-` prefix;
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
build=0.8.0
gifDecoder=compact12/cache
gifDecoderBytes=16128
gifProbe=... largest=... reserve=10240
gifCachedFrames=...
gifCacheWaits=... low=... guard=9216
reset=poweron
heap=... min=... largest=...
content=gif
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

The app time-synchronization packet is acknowledged for compatibility, but WLED
remains the clock authority. Configure WLED NTP, timezone, and daylight-saving
settings normally.

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
outside the 0.8.0 release-validation matrix.

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
- [`TODO.md`](TODO.md) — roadmap after 0.8.0;
- [`RELEASE_NOTES_0.8.0.md`](RELEASE_NOTES_0.8.0.md) — stable 0.8.0 summary, validation scope, and upgrade notes;
- [`RELEASE_NOTES_0.7.1.md`](RELEASE_NOTES_0.7.1.md) — previous stable media/memory milestone.

The individual `RELEASE_NOTES_0.8.0-dev.*.md` files are retained as engineering
history; normal users should follow the stable 0.8.0 notes and the current README.

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

Stable 0.8.0 provides an optional active buzzer. Choose the buzzer GPIO in
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
