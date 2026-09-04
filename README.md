# WLED iDotMatrix Usermod

WLED Usermod for classic ESP32 that emulates an iDotMatrix BLE peripheral and
lets the official iDotMatrix app drive a WLED 2D matrix.

The project implements the peripheral/server side of the protocol: WLED
advertises the expected BLE services, accepts commands from the app, validates
bulk transfers, and renders supported content through one WLED effect named
`iDotMatrix Display`.

## Stable release 0.7.0

Version **0.7.0** is the first stable media release. It was validated on a
classic ESP32 with WLED 16.0.1 and a physical 16x16 RGB matrix.

The validated 16x16 path supports:

- discovery and connection from the official iDotMatrix app;
- screen power and master brightness;
- full-screen RGB colours;
- DIY/Graffiti pixels and saved Graffiti images;
- all app clock commands already implemented by the project;
- app-rasterized scrolling/effect text, including the full speed-slider range;
- cloud/static RAW RGB images;
- compact PNG images used by the app;
- animated GIF transfers and playback;
- repeated GIF replacement followed by a return to clock/WLED content without
  reconnecting or rebooting.

The media fixes promoted in 0.7.0 include a 517-byte BLE MTU request, correct
FA02 fragment reassembly without fragment-level ACKs, recovery from abandoned
fragmented transfers, and a low-RAM 16x16 build of AnimatedGIF. The reduced GIF
decoder occupies about 8 KiB instead of the roughly 22 KiB required by the stock
configuration, which is necessary for stable coexistence with WLED, Wi-Fi, and
NimBLE on a classic ESP32.

## Supported functionality

| Function | Protocol / source | WLED mapping | 0.7.0 status |
|---|---|---|---|
| Discovery | FA/AE GATT + manufacturer data | BLE Usermod | Verified |
| Screen power | FA02 | WLED power | Verified |
| Brightness | FA02 | WLED master brightness | Verified |
| Full-screen RGB | FA02 | `Solid` + primary colour | Verified |
| DIY/Graffiti | FA02 | `iDotMatrix Display` | Verified on 16x16 |
| Clock | FA02 | `iDotMatrix Display`, WLED local time | Verified on 16x16 |
| Text | bulk type `0x03` | app bitmaps rendered by `iDotMatrix Display` | Verified on 16x16 |
| RAW/cloud image | bulk type `0x02` | atomic RGB framebuffer | Verified on 16x16 |
| Compact PNG | inline type `0x00` | decoded RGB/RGBA framebuffer | Verified on 16x16 |
| GIF animation | bulk type `0x01` | LittleFS + low-RAM AnimatedGIF | Verified on 16x16 |
| 32x32 / 64x64 profiles | profile `0x03` / `0x04` | dynamic logical framebuffer | Experimental |

Countdown, stopwatch, scoreboard, alarms, schedules, buzzer behavior, rotation,
and original-device standalone effects are not implemented.

## Hardware requirements

### Validated target

For the stable 0.7.0 experience use:

- **classic ESP32** compatible with PlatformIO `esp32dev`;
- at least **4 MB flash**;
- a **16x16 RGB matrix** configured as a WLED 2D matrix;
- a digital LED output supported by WLED's **I2S** backend;
- USB/serial access for flashing;
- Wi-Fi for normal WLED operation;
- Bluetooth/BLE enabled by the ESP32 hardware.

PSRAM is **not required** for the validated 16x16 configuration.

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

The stable build is pinned to:

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
0.7.0 stable target.

## Installation

### 1. Place the Usermod beside WLED

Clone or extract this repository beside the WLED source directory. For example:

```text
D:\WLED source\WLED-16.0.1
D:\WLED source\wled-usermod-idotmatrix
```

The directory name `wled-usermod-idotmatrix` is assumed by the supplied example
override. If you use another directory name, update its paths accordingly.

### 2. Add the PlatformIO environment

Copy the contents of `platformio_override.ini.example` into WLED's
`platformio_override.ini`, or include the equivalent environment yourself.

The supplied configuration:

- extends WLED's `esp32dev` environment;
- selects the official Espressif PlatformIO platform;
- enables NimBLE-Arduino 1.4.3;
- installs AnimatedGIF 1.4.7;
- disables WLED OTA;
- selects the supplied single-app partition layout;
- loads this repository as an external Usermod.

Do **not** add `esp-nimble-cpp` or the registry package `ESP32 BLE Arduino`.

### 3. Clean and build

From the WLED source directory:

```powershell
pio run -e esp32dev_idotmatrix -t clean
pio run -e esp32dev_idotmatrix
```

During dependency preparation you should see:

```text
[iDotMatrix] AnimatedGIF 1.4.7 patched for 16x16 low-RAM decoder
```

The Usermod's PlatformIO extra script modifies only the per-environment copy of
AnimatedGIF under `.pio/libdeps`. It does not modify your global PlatformIO
package cache or other build environments.

### 4. Flash over USB/serial

Replace `COM3` with the actual serial port:

```powershell
pio run -e esp32dev_idotmatrix -t upload --upload-port COM3
```

Optional serial monitor:

```powershell
pio device monitor -e esp32dev_idotmatrix
```

### 5. Configure WLED

In WLED:

1. configure the physical panel as a **16x16 2D matrix**;
2. set every digital LED output used by this build to **I2S**, not RMT;
3. configure Wi-Fi, timezone, and NTP if you want the clock to be correct;
4. open Usermods settings and leave `screenType` at **16 x 16** for the stable
   0.7.0 configuration;
5. optionally change `deviceName`; `IDM-` is added automatically;
6. reboot after changing the BLE device name or logical profile.

### 6. Pair from the iDotMatrix app

Open the official iDotMatrix app and scan for the configured `IDM-...` device.
A successful connection should let the app control the WLED matrix directly.

## Configuration options

- `enabled`: enables the BLE emulator;
- `screenType`: logical profile (`16x16`, `32x32`, `64x64`);
- `deviceName`: advertised BLE name/suffix, normalized to `IDM-...`;
- `rescale`: nearest-neighbour scaling from the logical profile to the selected
  WLED 2D segment.

For 0.7.0, **16x16 is the only fully validated profile**. The 32x32 and 64x64
logical framebuffers remain available for continued development, but the
low-RAM GIF decoder is deliberately limited to images no larger than 16x16.

## Runtime status

`/json/info` intentionally exposes only compact operational status. The debug
counters used while developing 0.7.0 were removed from the stable release.
Typical output under `u.iDotMatrix` is:

```json
"iDotMatrix": [
  "BLE connected",
  "profile=16x16",
  "name=IDM-666",
  "content=gif"
]
```

Possible content owners are `WLED`, `graffiti`, `clock`, `text`, `image`, and
`gif`.

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

### GIF implementation

GIF data is streamed to LittleFS, validated after the completed CRC-protected
bulk transfer, and then opened outside BLE callbacks. AnimatedGIF 1.4.7 is
patched at build time for the 16x16 stable target: maximum width 16, smaller file
buffers, and a reduced 10-bit LZW dictionary. This is the key RAM optimization
that makes repeated animations stable on the tested classic ESP32.

## Repository layout

- `usermod_idotmatrix.cpp` — Usermod lifecycle, configuration, startup guards;
- `IDotMatrixBLEServer.*` — NimBLE GATT server, reassembly, notifications;
- `IDotMatrixFA02Assembler.*` — bounded fragmented FA02 reconstruction;
- `IDotMatrixBulkTransfer.*` — bulk framing, CRC32, TEXT/RAW/GIF chunk state;
- `IDotMatrixProtocol.*` — protocol validation and command decoding;
- `IDotMatrixRenderer.*` — logical RGB framebuffer, clock and text rendering;
- `IDotMatrixMedia.*` — PNG decode and LittleFS/GIF playback;
- `IDotMatrixWLEDAdapter.*` — protocol-to-WLED state and display effect;
- `patch_animatedgif_16x16.py` — per-build low-RAM AnimatedGIF patch;
- `WLED_ESP32_4MB_IDOT_NO_OTA.csv` — 4 MB single-app partition table;
- `tests/` — host regression tests.

Further documentation:

- [`PROTOCOL.md`](PROTOCOL.md) — implemented wire-protocol subset;
- [`ARCHITECTURE.md`](ARCHITECTURE.md) — component boundaries and memory model;
- [`TESTING.md`](TESTING.md) — host/build/hardware regression procedure;
- [`HISTORY.md`](HISTORY.md) — release history;
- [`TODO.md`](TODO.md) — post-0.7.0 roadmap.

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

MIT. See `LICENSE`.
