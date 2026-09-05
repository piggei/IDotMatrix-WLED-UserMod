# WLED iDotMatrix Usermod

> **Stable 0.7.1:** adds 32x32 and 64x64 logical profiles to the 0.7.0 media baseline, including a low-memory 64x64 GIF path for classic ESP32 without PSRAM. The final no-PSRAM path keeps the full 4096-code LZW12 semantics, predecodes downscaled frames to LittleFS, releases the decoder before playback, and has passed repeated large-GIF/replacement tests with the WLED Web UI remaining responsive.

WLED Usermod for ESP32 that emulates an iDotMatrix BLE peripheral and lets the
official iDotMatrix app drive a WLED 2D matrix.

The project implements the peripheral/server side of the protocol: WLED
advertises the expected BLE services, accepts commands from the app, validates
bulk transfers, and renders supported content through one WLED effect named
`iDotMatrix Display`.

## Release status

Version **0.7.1** is the current stable release.

It preserves the hardware-validated 16x16 behavior from 0.7.0 and adds the
larger logical-profile work developed through the 0.7.1-dev series.

### Hardware validation scope

| Configuration | GIF backend | Status |
|---|---|---|
| 16x16 logical / 16x16 physical, classic ESP32 | `animatedgif10` | Stable 0.7.0 path retained; host-regressed in 0.7.1 |
| 32x32 logical -> 16x16 physical, `rescale=true`, classic ESP32 | `animatedgif11` | Hardware-validated: clock/text and repeated GIF playback |
| 64x64 logical -> 16x16 physical, `rescale=true`, classic ESP32 without PSRAM, `64x64-lite` build | `compact12/cache` | Hardware-validated: static images, clocks, large/100-frame GIFs, repeated GIF replacement, WLED/clock/image transitions, responsive Web UI |
| 64x64 with PSRAM | `animatedgif12/psram` | Implemented; hardware validation still pending |
| Native physical 64x64 and HUB75 DMA | depends on build | Not yet release-validated |

The final classic-ESP32 64x64 validation included more than ten consecutive GIF
replacements, large animations, clock/date -> GIF, WLED effect -> GIF, static
image -> GIF, return to normal WLED effects, and live `/json/info` access while
media was active. No reboot, WLED Error 8/90, or manual Solid reset was required.

## Supported functionality

| Function | Protocol / source | WLED mapping | 0.7.1 status |
|---|---|---|---|
| Discovery | FA/AE GATT + manufacturer data | BLE Usermod | Verified |
| Screen power | FA02 | WLED power | Verified |
| Brightness | FA02 | WLED master brightness | Verified |
| Full-screen RGB | FA02 | `Solid` + primary colour | Verified |
| DIY/Graffiti | FA02 | `iDotMatrix Display` | Verified on 16x16; larger logical coordinates supported |
| Clock | FA02 | `iDotMatrix Display`, WLED local time | Verified on 16x16, 32x32 and 64->16 rescale |
| Text | bulk type `0x03` | app bitmaps rendered by `iDotMatrix Display` | Verified; see rescale limitation below |
| RAW/cloud image | bulk type `0x02` | atomic/downscaled RGB framebuffer | Verified on 16x16 and 64->16 rescale |
| Compact PNG | inline type `0x00` | decoded RGB/RGBA framebuffer | Verified on 16x16; larger-profile coverage remains partial |
| GIF animation | bulk type `0x01` | direct decoder or LittleFS frame cache | Verified on 16x16, 32->16 and 64->16 no-PSRAM path |
| 32x32 profile | profile `0x03` | logical profile + optional rescale | Hardware-validated with physical 16x16 |
| 64x64 profile | profile `0x04` | logical profile + optional low-memory rescale | Hardware-validated with physical 16x16/no PSRAM |

Countdown, stopwatch, scoreboard, alarms, schedules, buzzer behavior, rotation,
and original-device standalone effects are not implemented.

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
the tested 4 MB layout. The supplied partition table therefore uses a single
large application partition and disables WLED OTA. Flash this environment over
USB/serial.

The Usermod forces Wi-Fi modem sleep on (`noWifiSleep = false`) because it is
required for Wi-Fi/Bluetooth coexistence with the pinned ESP-IDF generation.

## Software requirements

| Component | Version / setting |
|---|---|
| WLED | 16.0.1 |
| PlatformIO environment | `esp32dev_idotmatrix` |
| Platform | `espressif32@~6.13.0` |
| Arduino-ESP32 | 2.0.17 |
| ESP-IDF | 4.4.7 |
| BLE stack | `h2zero/NimBLE-Arduino@1.4.3` |
| GIF library | `bitbank2/AnimatedGIF@1.4.7` |
| LED driver | I2S |

Other WLED/framework/ESP32 variants may work, but they are not part of the
0.7.1 release validation.

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

### 2. Choose a PlatformIO override

The repository includes:

- `platformio_override.ini.example` — normal 16x16 BLE/iDotMatrix build;
- `platformio_override.ini.32x32` — adds `-D IDOT_GIF_LZW11`;
- `platformio_override.ini.64x64` — adds `-D IDOT_GIF_LZW12` while keeping normal WLED integrations;
- `platformio_override.ini.64x64-lite` — adds `IDOT_GIF_LZW12` and disables optional WLED integrations to create more classic-ESP32 RAM margin; this is the **hardware-validated no-PSRAM 64->16 configuration**;
- `platformio_override.ini.hub75` — optional HUB75 example supplied by the project; it is separate from the release-validated BLE/media profiles and does not by itself select the 64x64 GIF decoder.

Copy the appropriate file to WLED's `platformio_override.ini`, or merge the
`esp32dev_idotmatrix` environment into your existing override.

Do **not** define `IDOT_GIF_LZW11` and `IDOT_GIF_LZW12` together.
Do **not** add `esp-nimble-cpp` or the registry package `ESP32 BLE Arduino`.

### 3. Clean and build

From the WLED source directory:

```powershell
pio run -e esp32dev_idotmatrix -t clean
pio run -e esp32dev_idotmatrix
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
4. choose Usermod `screenType` = `16x16`, `32x32`, or `64x64`;
5. if the logical profile is larger than the physical matrix, enable `rescale`;
6. optionally change `deviceName`; `IDM-` is added automatically;
7. reboot after changing the BLE device name or logical profile.

For the validated classic-ESP32 64x64 logical test, use
`platformio_override.ini.64x64-lite`, set `screenType=64x64`, enable
`rescale=true`, and keep the physical WLED matrix at 16x16.

### 6. Pair from the iDotMatrix app

Open the official iDotMatrix app and scan for the configured `IDM-...` device.
A successful connection should let the app control the WLED matrix directly.

## Configuration options

- `enabled`: enables the BLE emulator;
- `screenType`: logical profile (`16x16`, `32x32`, `64x64`);
- `deviceName`: advertised BLE name/suffix, normalized to `IDM-...`;
- `rescale`: nearest-neighbour mapping from logical coordinates/media to the selected WLED 2D segment/storage canvas.

## Runtime status

`/json/info` exposes compact diagnostics under `u.iDotMatrix`. A 64x64
classic-ESP32/no-PSRAM build may report:

```text
BLE connected
profile=64x64
canvas=16x16
name=IDM-123456
build=0.7.1
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

Possible content owners are `WLED`, `graffiti`, `clock`, `text`, `image`, and
`gif`. Other media errors include `gif-invalid`, `gif-decoder-oom`,
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

App-rendered content selects the single `iDotMatrix Display` WLED effect. A
full-screen RGB command intentionally returns to WLED `Solid`. Selecting another
WLED effect manually replaces app content until the app sends another supported
content command.

For no-PSRAM 64x64 GIF preparation, WLED internally uses `Static` because it has
a small RAM footprint. The physical segment is deliberately blanked during this
short staging period and the previous WLED primary colour is restored before
playback/recovery.

### Rescaled text

The 64x64 logical -> 16x16 physical path is intended primarily to validate the
larger protocol/media profile on a small panel. Text glyphs are currently
rendered in logical coordinates and then sampled down. Thin strokes can therefore
become hard to read at aggressive ratios such as 64->16. This is a rendering
quality limitation, not a BLE/media transport failure.

### LittleFS frame-cache tradeoff

On classic ESP32 without PSRAM, 64x64 GIF playback trades temporary flash I/O
for RAM headroom. Each valid GIF is predecoded to `/idot_cache.bin`; the cache is
removed when playback ends. Very frequent GIF replacement therefore performs
more flash writes than the PSRAM/direct backend. The cache has a 512 KiB limit.

### Validation boundaries

The automatic PSRAM direct backend is implemented but has not yet been tested on
the pending PSRAM hardware. Native physical 64x64 output and HUB75 DMA are also
outside the 0.7.1 release-validation matrix.

## Repository layout

- `usermod_idotmatrix.cpp` — Usermod lifecycle, configuration, startup guards, runtime status;
- `IDotMatrixBLEServer.*` — NimBLE GATT server, reassembly, notifications;
- `IDotMatrixFA02Assembler.*` — bounded fragmented FA02 reconstruction;
- `IDotMatrixBulkTransfer.*` — bulk framing, CRC32, TEXT/RAW/GIF chunk state;
- `IDotMatrixProtocol.*` — protocol validation and command decoding;
- `IDotMatrixRenderer.*` — RGB storage canvas, clock/text rendering, RAW/rescale handling;
- `IDotMatrixMedia.*` — PNG decode, GIF RX files, direct playback, frame-cache orchestration;
- `IDotMatrixCompactGif.*` — compact-safe full-code-space LZW12 predecoder for no-PSRAM 64x64;
- `IDotMatrixWLEDAdapter.*` — protocol-to-WLED state, ownership, staging, display effect;
- `patch_animatedgif_profiles.py` — selects 10/11/12-bit AnimatedGIF build profile;
- `WLED_ESP32_4MB_IDOT_NO_OTA.csv` — 4 MB single-app partition table;
- `platformio_override.ini.*` — example build profiles;
- `tests/` — host regression tests and compact-GIF fixtures.

Further documentation:

- [`PROTOCOL.md`](PROTOCOL.md) — implemented wire-protocol subset;
- [`ARCHITECTURE.md`](ARCHITECTURE.md) — component boundaries, current memory model, and RAM-engineering history;
- [`TESTING.md`](TESTING.md) — host/build/hardware regression procedure;
- [`HISTORY.md`](HISTORY.md) — release/development history;
- [`TODO.md`](TODO.md) — post-0.7.1 roadmap;
- [`RELEASE_NOTES_0.7.1.md`](RELEASE_NOTES_0.7.1.md) — release summary and validation scope.

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

## License

This project is licensed under the **European Union Public Licence (EUPL) v1.2**.
See [`LICENSE`](LICENSE).

The licence was chosen to align this WLED usermod with the current licensing of
WLED, which is distributed under EUPL v1.2 or later. WLED remains copyright of
Christian Schwinne and the individual WLED contributors. Third-party dependencies
used by this project remain subject to their respective licences.
