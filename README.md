# WLED iDotMatrix Usermod

An ESP32 WLED Usermod that emulates an iDotMatrix BLE peripheral and is
recognized by the official iDotMatrix application.

Unlike projects that control real iDotMatrix hardware, this project implements
the server/emulator side: WLED advertises the expected BLE services, receives
commands from the official app, and maps supported commands to WLED state.

## Stable release

Version **0.6.2** is the current stable release. It has been compiled, uploaded,
and tested with WLED 0.16.0.1 on a classic ESP32. The
official app successfully:

- discovers and connects to the emulated device;
- initializes its displayed power switch correctly;
- switches the WLED output on and off;
- changes WLED master brightness;
- selects full-screen RGB colours;
- enters DIY/Graffiti and selects the custom iDotMatrix display effect;
- draws received pixels on the physical 16x16 WLED matrix;
- renders the app-selected clock through the same `iDotMatrix Display` effect;
- returns to WLED `Solid` when a full-screen RGB command is sent.

Version **0.6.1** remains the frozen graffiti baseline, **0.6.0** the basic-
command baseline, and **0.5.0** the BLE-transport-only baseline.

### Development snapshot 0.6.3-dev.3

This repository currently contains the first TEXT rendering candidate. Stable
0.6.2 remains the hardware-verified display fallback, while the corrected FA02
bulk transport in 0.6.3-dev.2 has also been verified with the official app.

The candidate adds:

- FA02 reassembly for an observed 4096-byte payload chunk plus the 16-byte bulk
  header, matching the BUILD 80 source;
- bounded assembly for type `0x03` TEXT payloads up to 4096 bytes;
- CRC32 validation and confirmed continue/complete ACKs;
- confirmed marker `0x02` 8x16 and marker `0x05` 16x32 bitmap parsing;
- fixed and dynamic colours, background, movement, and visual text effects;
- TEXT ownership inside the existing `iDotMatrix Display` effect;
- fragment, parsing, glyph, and transfer diagnostics.

The validated 0.6.2 foundation already includes:

- confirmed clock-command decoding and ACK;
- one WLED effect named `iDotMatrix Display`, shared by graffiti and clock and
  ready to host future text, media, and timer rendering;
- all eight 16x16 clock styles reconstructed in the standalone reference;
- app-selected colour, 12/24-hour mode, and the optional 30-second `HH:MM` /
  5-second `DD/MM` cycle;
- WLED `localTime`/NTP as the sole runtime clock source;
- a dropdown for the advertised 16x16, 32x32, or 64x64 logical profile;
- strict dimension checking by default and optional nearest-neighbour rescale
  from the logical canvas to the active WLED 2D segment;
- automatic `IDM-` device-name normalization and a restart diagnostic derived
  from the name/profile actually active in the BLE stack.

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
| Graffiti pixels | `LEN 00 05 01 ? R G B X Y...` | `iDotMatrix Display` 2D effect | Verified on 16x16 |
| Clock | `08 00 06 01 FLAGS R G B` | `iDotMatrix Display` 2D effect using WLED time | Verified on 16x16 |
| Text | FA02 bulk type `0x03`, markers `0x02`/`0x05` | App bitmaps rendered by `iDotMatrix Display` | Rendering candidate |

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

Entering DIY selects the custom `iDotMatrix Display` effect on the first
selected WLED segment. A valid pixel packet also reselects it if necessary.
Selecting another WLED effect manually temporarily replaces the app canvas; the
next graffiti packet returns ownership to iDotMatrix. A full-screen RGB command
intentionally switches back to `Solid`.

The current stable release targets the first selected segment. The normal one-segment
matrix configuration is the supported layout for this milestone.

### Clock time comes from WLED

The app time-synchronization packet is acknowledged for compatibility, but its
fields are not installed into a second Usermod clock. The clock effect reads
WLED's local time, so WLED NTP, timezone, and daylight-saving configuration
remain authoritative. Until WLED has valid time, the displayed value may be the
epoch/default time.

### Logical profile, physical matrix, and rescale

`screenType` controls what WLED advertises to the app and therefore the logical
canvas size. The physical target is the first selected WLED 2D segment and its
dimensions are detected inside the effect callback.

By default `rescale` is disabled. A logical/physical size mismatch is then
blocked and rendered black instead of silently clipping data. Enabling
`rescale` applies nearest-neighbour sampling, allowing for example a logical
64x64 app profile to preview on a physical 16x16 matrix. This is necessarily
lossy and does not change the BLE profile seen by the app.

### Text rendering candidate and unsupported media

The development snapshot receives, validates, parses, and renders bounded TEXT
bulk transfers. GIF/media, countdown, stopwatch, alarms, and schedules are not
implemented yet.

The first hardware check should confirm glyph orientation and each visual mode.
The wave-based dynamic colour modes use a lightweight integer wave in this
candidate; their timing and appearance may differ slightly from BUILD 80's
FastLED `sin8()` implementation while preserving the same protocol fields.

Complete short FA02 commands retain their four-entry 64-byte queue. Fragmented
or large FA02 traffic has a separate 4112-byte assembler and TEXT has a
4096-byte payload limit; GIF/RAW bulk handling will use its own storage policy
in later milestones. AE01 remains present for GATT compatibility but BUILD 80
does not dispatch bulk content from it.

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
wled-usermod-idotmatrix @ 0.6.3-dev.3
```

## Configuration

- `enabled`: enable the BLE emulator;
- `screenType`: dropdown selecting `1` = 16x16, `3` = 32x32, or `4` = 64x64;
- `rescale`: optionally scale the logical canvas to the WLED 2D segment;
- `deviceName`: BLE name or suffix, normalized to `IDM-...`, default
  `IDM-858931`.

Changing name or profile requires a reboot and is reported in `/json/info`;
`rescale` applies immediately. The Usermod forces
`noWifiSleep = false` for Bluetooth/Wi-Fi coexistence.

The configured advertising name can be verified with `name` and `nameActive`.
If both contain the new value but the official app still shows an older name,
the old label is cached by the app or operating system for the same BLE device
identity. This was observed with both diagnostics reporting `IDM-666` while the
app retained an earlier label. It is not a failed Usermod save; clear the app's
Bluetooth/cache state or test discovery with a generic BLE scanner.

## Runtime diagnostics

The state appears under `/json/info` in `u.iDotMatrix`:

```json
"iDotMatrix": [
  "BLE connected",
  "profile=0x1",
  "name=IDM-PAPERO",
  "nameActive=IDM-PAPERO",
  "rx=4",
  "dropped=0",
  "bulkChunks=1",
  "bulkComplete=1",
  "bulkCrcErrors=0",
  "bulkRejected=0",
  "fragments=4",
  "reassemblyErrors=0",
  "unknown=0",
  "textBytes=34",
  "textParsed=1",
  "textParseErrors=0",
  "infoPushAttempts=2",
  "canvas=16x16",
  "content=text",
  "pixelUpdates=21",
  "displayFx=187 active",
  "textGlyphs=1 8x16",
  "effectFrames=42",
  "target=16x16",
  "mapping=native"
]
```

- `rx`: BLE writes received;
- `dropped`: writes discarded because the FA02 queue or assembler was busy;
- `bulkChunks`: accepted type-`0x03` TEXT chunks;
- `bulkComplete`: completed TEXT transfers with valid CRC32;
- `bulkCrcErrors` and `bulkRejected`: transfer failure diagnostics;
- `fragments`, `reassembly`, and `reassemblyErrors`: FA02 logical-packet
  reconstruction status;
- `unknown` and `lastUnknown`: count and first bytes of unhandled FA02 packets;
- `textBytes`: size retained after the latest valid TEXT transfer;
- `textParsed` and `textParseErrors`: payloads accepted or rejected by the
  glyph-record parser;
- `infoPushAttempts`: cumulative delayed device-info attempts, normally two per
  completed connection.
- `displayFx`: the single dynamically assigned WLED effect ID shared by
  graffiti, clock, and future app-rendered content, plus its active state;
- `textGlyphs`: count and dimensions of the active app-rasterized glyphs;
- `effectFrames`: combined number of iDotMatrix effect calls since boot;
- `target`: virtual WLED segment dimensions observed inside the effect callback.
- `mapping`: `native`, `rescale`, or `mismatch blocked`.

## Architecture and documentation

- `usermod_idotmatrix.cpp`: lifecycle, settings, guards, and diagnostics;
- `IDotMatrixBLEServer.*`: BLE transport and bounded queue;
- `IDotMatrixBulkTransfer.*`: bounded TEXT bulk assembly and CRC32;
- `IDotMatrixFA02Assembler.*`: bounded logical-packet reconstruction;
- `IDotMatrixProtocol.*`: WLED-independent validation and decoding;
- `IDotMatrixRenderer.*`: resolution-independent RGB framebuffer and clock
  artwork;
- `IDotMatrixWLEDAdapter.*`: protocol-to-WLED boundary, time source, rescale,
  and the unified custom display effect;
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
