# WLED iDotMatrix Usermod

An ESP32 WLED Usermod that emulates an iDotMatrix BLE peripheral and is
recognized by the official iDotMatrix application.

Unlike projects that act as controllers for a real iDotMatrix display, this
project implements the server/emulator side: WLED advertises the expected BLE
services, receives commands from the official application, and will render them
through WLED's matrix and segment infrastructure.

## Stable baseline

Version **0.5.0** is the first frozen, experimentally verified BLE foundation.
On the tested classic ESP32 board:

- WLED 0.16.0.1 boots and remains reachable over Wi-Fi;
- the official iDotMatrix application discovers the emulated device;
- the application connects successfully;
- the FA and AE services and characteristics are exposed;
- profiles `0x01` (16x16), `0x03` (32x32), and `0x04` (64x64) are selectable;
- BLE callbacks feed a fixed, non-blocking receive queue;
- device-information and time-synchronization responses are implemented.

This snapshot is a stable transport baseline, not yet the complete LED-command
MVP. Screen power, brightness, full-screen RGB, and graffiti/pixel rendering are
the next implementation milestone.

## Protocol authority

The companion `IDotMatrix-ESP32-Emulator` repository and its `PROTOCOL.md` remain
the primary source for the reverse-engineered protocol. This Usermod reuses those
verified findings and must not silently redefine or discard them.

Confirmed discovery data used here:

- FA service: `000000fa-0000-1000-8000-00805f9b34fb`;
- FA02: application to device, write/write without response;
- FA03: device to application, read/notify;
- AE service: `0000ae00-0000-1000-8000-00805f9b34fb`;
- AE01: application to device;
- AE02: device to application;
- manufacturer data: `54 52 00 70 <screen-type>`.

The advertising packet uses the equivalent 16-bit FA UUID representation to stay
within the 31-byte legacy BLE advertising limit. The full Bluetooth-base UUID is
still used by the GATT service.

## Supported build

The stable 0.5.0 combination is deliberately pinned:

| Component | Stable value |
|---|---|
| WLED | 0.16.0.1 |
| Target | Classic ESP32 / `esp32dev` |
| Platform | `espressif32@~6.13.0` |
| Arduino-ESP32 | 2.0.17 |
| ESP-IDF | 4.4.7 |
| BLE host | `h2zero/NimBLE-Arduino@1.4.3` |
| Digital LED driver | I2S |
| Flash layout | 4 MB, single app, no OTA |

Other boards and framework versions are not yet part of this frozen baseline.

## Why this exact combination is required

WLED's default Tasmota framework package does not expose the complete compatible
BLE peripheral/GATT-server interface needed here. Adding another NimBLE stack on
top of it produced duplicate or incompatible symbols.

The official Espressif Arduino platform provides the required controller support,
but the classic Bluedroid host proved unstable with WLED's dynamic Wi-Fi lifecycle:

- late initialization failed in `fixed_queue_new()` and `btm_ble_init()`;
- early initialization corrupted/crashed Wi-Fi scanning in `clear_bss_queue()`;
- disabling Wi-Fi modem sleep caused an intentional coexistence abort;
- the RMT-HI LED backend conflicted with the Bluetooth controller interrupt.

The verified solution is the official Espressif platform, NimBLE-Arduino 1.4.3,
Wi-Fi modem sleep enabled, and WLED's I2S LED backend.

## Installation

Place this repository beside the WLED source tree. Example:

```text
D:\WLED source\WLED-16.0.1
D:\WLED source\wled-usermod-idotmatrix
```

Add the following environment to WLED's `platformio_override.ini`:

```ini
[env:esp32dev_idotmatrix]
extends = env:esp32dev

platform = espressif32@~6.13.0
platform_packages =

board_build.partitions = ../wled-usermod-idotmatrix/WLED_ESP32_4MB_IDOT_NO_OTA.csv

build_flags =
  ${env:esp32dev.build_flags}
  -D WLED_DISABLE_OTA

lib_deps =
  ${env:esp32dev.lib_deps}
  h2zero/NimBLE-Arduino@1.4.3

custom_usermods =
  symlink://../wled-usermod-idotmatrix
```

The explicit `lib_deps` entry is required. WLED's external/symlinked Usermod build
does not reliably propagate the dependency declared in this repository's
`library.json`.

Do not add either of these packages:

- `h2zero/esp-nimble-cpp`;
- the registry library `ESP32 BLE Arduino`.

Build and upload over USB/serial:

```powershell
pio run -e esp32dev_idotmatrix -t clean
pio run -e esp32dev_idotmatrix
pio run -e esp32dev_idotmatrix -t upload --upload-port COM3
pio device monitor -e esp32dev_idotmatrix
```

The dependency graph must contain:

```text
NimBLE-Arduino @ 1.4.3
wled-usermod-idotmatrix @ 0.5.0
```

## WLED configuration

Every digital LED output must use the **I2S** driver. The Usermod detects an RMT
digital bus and refuses to start BLE rather than entering a reboot loop.

The Usermod also forces `noWifiSleep = false` at runtime. ESP-IDF requires Wi-Fi
modem sleep when Wi-Fi and Bluetooth coexist on this target.

Available Usermod settings:

- `enabled`: enables the BLE emulator;
- `screenType`: `1` = 16x16, `3` = 32x32, `4` = 64x64;
- `deviceName`: BLE name, default `IDM-858931`.

Changes to the BLE name or profile require a reboot.

## Runtime status

WLED exposes the module state under `/json/info`, in the `u.iDotMatrix` array.
After startup the expected state is similar to:

```json
"iDotMatrix": [
  "BLE advertising",
  "profile=0x1",
  "rx=0",
  "dropped=0"
]
```

## Current code structure

- `usermod_idotmatrix.cpp`: WLED lifecycle, configuration, compatibility guards,
  delayed startup, and status reporting;
- `IDotMatrixBLEServer.h/.cpp`: BLE transport, GATT database, advertising,
  callbacks, receive queue, and the first protocol responses;
- `WLED_ESP32_4MB_IDOT_NO_OTA.csv`: single-application 4 MB partition layout;
- `platformio_override.ini.example`: reproducible stable build environment;
- `HISTORY.md`: development and release history;
- `TODO.md`: implementation and validation roadmap.

The transport, protocol parser, renderer, and WLED adapter will remain separate as
the project grows. The current BLE class must not become a copy of the standalone
monolithic sketch.

## Resource and scheduling rules

- no `delay()` in the Usermod loop or protocol processing;
- BLE callbacks must remain short and non-blocking;
- command processing happens from the WLED loop;
- GIF/media must stream through the filesystem rather than requiring a complete
  file in RAM;
- PSRAM may be used when available, but correctness must not depend on it for the
  16x16 baseline;
- WLED facilities should be reused for LED mapping, brightness, power, time,
  configuration, filesystem, and scheduling where they fit naturally.

## Related projects

These projects are complementary and must remain credited in future documentation:

- [dallanwagz/idotmatrix-ha](https://github.com/dallanwagz/idotmatrix-ha)
- [markusressel/idotmatrix-api-client](https://github.com/markusressel/idotmatrix-api-client)
- [derkalle4/python3-idotmatrix-client](https://github.com/derkalle4/python3-idotmatrix-client)
- [8none1/idotmatrix](https://github.com/8none1/idotmatrix)
- [nj-designs/go-idot](https://github.com/nj-designs/go-idot)
- [whybutter/idotmatrix](https://github.com/whybutter/idotmatrix)

Most of them control a real iDotMatrix device. This project instead makes an ESP32
running WLED behave as the BLE peripheral expected by the official application.

## License

MIT. See `LICENSE`.
