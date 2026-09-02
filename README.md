# WLED iDotMatrix Usermod

An ESP32 WLED Usermod that emulates an iDotMatrix BLE peripheral and is
recognized by the official iDotMatrix application.

Unlike projects that control real iDotMatrix hardware, this project implements
the server/emulator side: WLED advertises the expected BLE services, receives
commands from the official app, and maps supported commands to WLED state.

## Stable release

Version **0.6.1** is the current stable release. It has been compiled, uploaded,
and tested with WLED 0.16.0.1 on a classic ESP32. The
official app successfully:

- discovers and connects to the emulated device;
- initializes its displayed power switch correctly;
- switches the WLED output on and off;
- changes WLED master brightness;
- selects full-screen RGB colours;
- enters DIY/Graffiti and selects the custom `iDotMatrix Framebuffer` effect;
- draws received pixels on the physical 16x16 WLED matrix;
- returns to WLED `Solid` when a full-screen RGB command is sent.

Version **0.6.0** remains the frozen basic-command baseline, while **0.5.0**
remains the BLE-transport-only baseline.

### Graffiti framebuffer

Version 0.6.1 adds:

- a dynamically allocated RGB framebuffer for 16x16, 32x32, and 64x64;
- confirmed DIY enter/exit command handling;
- confirmed native-resolution graffiti pixel updates;
- a first-class WLED 2D effect named `iDotMatrix Framebuffer`;
- automatic effect selection when DIY starts or a valid pixel packet arrives;
- rendering through the current WLED effect segment, preserving WLED's physical
  matrix mapping;
- canvas, effect-state, frame-count and accepted-pixel diagnostics under
  `/json/info`.

`0.6.1-dev.1` correctly received and decoded graffiti, but hardware testing
showed that its `handleOverlayDraw()` path left WLED in `Solid` and did not make
the canvas visible. `0.6.1-dev.2` moved rendering into a registered WLED effect
and was validated on hardware before being promoted unchanged to 0.6.1. Both
development steps are retained in `HISTORY.md`.

## Supported functionality

| Function | Protocol | WLED mapping | Status |
|---|---|---|---|
| Discovery | FA/AE GATT and manufacturer data | BLE Usermod | Verified |
| Profile | `0x01`, `0x03`, `0x04` | 16x16, 32x32, 64x64 identity | Verified at discovery level |
| Device information | `04 00 01 80` | Profile response on FA03 | Verified |
| Time synchronization | `0B 00 01 80 ...` | ACK only | Partial |
| Screen power | `05 00 07 01 STATE` | WLED power | Verified |
| Brightness | `05 00 04 80 PERCENT` | WLED master brightness | Verified |
| Full-screen RGB | `07 00 02 02 R G B` | Static effect and primary colour | Verified |
| DIY mode | `05 00 04 01 STATE` | Graffiti session state | Verified on 16x16 |
| Graffiti pixels | `LEN 00 05 01 ? R G B X Y...` | `iDotMatrix Framebuffer` 2D effect | Verified on 16x16 |

See [`PROTOCOL.md`](PROTOCOL.md) for exact packets and ACKs.

## Important limitations

### Brightness is synchronized only from the app to WLED

Moving the iDotMatrix app slider sends a brightness command and updates WLED.
The official app does not request current brightness from the emulated device,
and no confirmed device-to-app brightness-state message is known.

Consequently:

- WLED is the only runtime and persistence authority for brightness;
- the Usermod does not store a second brightness preference;
- changing brightness in WLED does not move the slider in the iDotMatrix app;
- the displayed values may differ until the app slider is moved;
- connecting the app does not overwrite WLED with a guessed default value.

### Power-switch initialization uses a compatibility sequence

The standalone emulator turns its display on when the app connects and sends an
unsolicited device-info notification after 1.2 seconds. One notification proved
timing-sensitive inside WLED, so the Usermod sends the same confirmed packet at about
1.2 and 2.5 seconds. This reliably initializes the app's displayed switch in the
tested setup. Both sends are scheduled outside the BLE callback.

### RGB follows WLED segment selection

Full-screen RGB uses WLED's normal global-colour path. It sets the static effect
and primary colour on active, selected segments. With the standard single matrix
segment this fills the whole display. Custom multi-segment installations must
select every segment that should follow the app.

### Graffiti owns the selected segment through a WLED effect

Entering DIY selects the custom `iDotMatrix Framebuffer` effect on the first
selected WLED segment. A valid pixel packet also reselects it if necessary.
Selecting another WLED effect manually temporarily replaces the app canvas; the
next graffiti packet returns ownership to iDotMatrix. A full-screen RGB command
intentionally switches back to `Solid`.

The current stable release targets the first selected segment. The normal one-segment
matrix configuration is the supported layout for this milestone.

### Rendering beyond graffiti and media are not implemented yet

Version 0.6.1 implements graffiti and a logical framebuffer,
but not text, clock rendering, GIF/media, countdown, stopwatch, alarms, or
schedules.

Logical and WLED matrix dimensions should match. This snapshot deliberately
does not downscale a 32x32/64x64 logical profile onto a 16x16 physical matrix;
the optional diagnostic preview from the standalone reference remains disabled.

The FA02 queue currently stores writes up to 64 bytes and does not reassemble a
logical command split across multiple BLE writes. This is sufficient for the
currently observed control and normal graffiti packets, but not for text or
bulk/media.

## Protocol authority

The companion `IDotMatrix-ESP32-Emulator` repository and its `PROTOCOL.md` are
the primary source for the reverse-engineered protocol. This Usermod reuses
those experimentally verified findings and must not silently redefine or remove
them.

Confirmed discovery data:

- FA service: `000000fa-0000-1000-8000-00805f9b34fb`;
- FA02: app to device, write/write without response;
- FA03: device to app, read/notify;
- AE service: `0000ae00-0000-1000-8000-00805f9b34fb`;
- AE01: app to device;
- AE02: device to app;
- manufacturer data: `54 52 00 70 <screen-type>`.

Advertising uses the equivalent 16-bit FA UUID to fit the 31-byte legacy packet.
GATT still uses the full Bluetooth-base UUID.

## Supported build

| Component | Stable value |
|---|---|
| WLED | 0.16.0.1 |
| Target | Classic ESP32 / `esp32dev` |
| Platform | `espressif32@~6.13.0` |
| Arduino-ESP32 | 2.0.17 |
| ESP-IDF | 4.4.7 |
| BLE host | `h2zero/NimBLE-Arduino@1.4.3` |
| Digital LED driver | I2S |
| Flash layout | 4 MB, single application, no OTA |

Other targets and framework versions are not part of this stable release.

## Build constraints

The verified combination is the official Espressif platform, NimBLE-Arduino
1.4.3, Wi-Fi modem sleep, and WLED's I2S LED backend.

Development established these incompatibilities:

- WLED's compact Tasmota framework lacked the required compatible GATT-server
  environment;
- adding another NimBLE stack produced duplicate/incompatible symbols;
- classic Bluedroid was unstable with WLED's Wi-Fi lifecycle and memory use;
- disabled Wi-Fi modem sleep caused an ESP-IDF coexistence abort;
- RMT-HI conflicted with the Bluetooth controller and caused a reboot loop;
- the BLE-enabled image did not fit the tested board's normal OTA slot.

The Usermod detects a digital RMT bus and refuses to start BLE. Every digital LED
output must use **I2S**.

## Installation

Place this repository beside the WLED source tree:

```text
D:\WLED source\WLED-16.0.1
D:\WLED source\wled-usermod-idotmatrix
```

Copy `platformio_override.ini.example` into WLED's override or add its environment.
The essential configuration is:

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

The explicit NimBLE entry is required for a symlinked external Usermod. Do not
install `h2zero/esp-nimble-cpp` or the registry package `ESP32 BLE Arduino`.

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
wled-usermod-idotmatrix @ 0.6.1
```

## Configuration

- `enabled`: enable the BLE emulator;
- `screenType`: `1` = 16x16, `3` = 32x32, `4` = 64x64;
- `deviceName`: BLE name, default `IDM-858931`.

Changing name or profile requires a reboot. The Usermod forces
`noWifiSleep = false` for Bluetooth/Wi-Fi coexistence.

## Runtime diagnostics

The state appears under `/json/info` in `u.iDotMatrix`:

```json
"iDotMatrix": [
  "BLE connected",
  "profile=0x1",
  "rx=4",
  "dropped=0",
  "infoPushAttempts=2",
  "canvas=16x16",
  "content=graffiti",
  "pixelUpdates=21",
  "framebufferFx=187 active",
  "effectFrames=42",
  "target=16x16"
]
```

- `rx`: BLE writes received;
- `dropped`: writes discarded because the four-entry queue was full;
- `infoPushAttempts`: cumulative delayed device-info attempts, normally two per
  completed connection.
- `framebufferFx`: dynamically assigned WLED effect ID and whether it owns the
  selected segment;
- `effectFrames`: number of framebuffer-effect calls since boot;
- `target`: virtual WLED segment dimensions observed inside the effect callback.

## Architecture and documentation

- `usermod_idotmatrix.cpp`: lifecycle, settings, guards, and diagnostics;
- `IDotMatrixBLEServer.*`: BLE transport and bounded queue;
- `IDotMatrixProtocol.*`: WLED-independent validation and decoding;
- `IDotMatrixRenderer.*`: resolution-independent RGB framebuffer;
- `IDotMatrixWLEDAdapter.*`: protocol-to-WLED boundary and custom framebuffer
  effect;
- `tests/`: host-side protocol and adapter tests.

Further documents:

- [`PROTOCOL.md`](PROTOCOL.md): implemented wire-protocol subset;
- [`ARCHITECTURE.md`](ARCHITECTURE.md): boundaries and extension rules;
- [`TESTING.md`](TESTING.md): host and hardware validation;
- [`HISTORY.md`](HISTORY.md): stable and experimental history;
- [`TODO.md`](TODO.md): next milestones and unresolved work.

## Related projects

These complementary projects must remain credited:

- [dallanwagz/idotmatrix-ha](https://github.com/dallanwagz/idotmatrix-ha)
- [markusressel/idotmatrix-api-client](https://github.com/markusressel/idotmatrix-api-client)
- [derkalle4/python3-idotmatrix-client](https://github.com/derkalle4/python3-idotmatrix-client)
- [8none1/idotmatrix](https://github.com/8none1/idotmatrix)
- [nj-designs/go-idot](https://github.com/nj-designs/go-idot)
- [whybutter/idotmatrix](https://github.com/whybutter/idotmatrix)

Most are clients/controllers for real hardware. This project makes WLED behave
as the BLE peripheral expected by the official app.

## License

MIT. See `LICENSE`.
