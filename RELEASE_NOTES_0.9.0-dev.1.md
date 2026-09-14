# iDotMatrix WLED Usermod 0.9.0-dev.1

**Release:** 0.9.0  
**Build:** 0.9.0-dev.1

This is the first development build of the new-hardware 0.9 line. It is derived
from the clean 0.8.2 stable source tree and intentionally excludes the temporary
0.8.2 diagnostic instrumentation used to investigate an unrelated ESP32-C3
power-off event.

## Primary target

- Adafruit MatrixPortal ESP32-S3
- WLED 0.17 development line
- WLED native HUB75 output backend
- one 64x64 HUB75 panel
- 2 MB PSRAM
- NimBLE-Arduino 2.x

## Changes from 0.8.2

- Public release identifier advanced to `0.9.0`.
- Internal build identifier set to `0.9.0-dev.1`.
- App-facing release byte advanced from `00 08` to `00 09`.
- Added `platformio_override.ini.matrixportal-s3-hub75` for the official WLED
  `adafruit_matrixportal_esp32s3` environment.
- The MatrixPortal profile uses LZW12 / 64x64 media capability and defaults a
  fresh iDotMatrix configuration to screen type `0x04` (64x64).
- Added compile-time guards requiring ESP32-S3, ESP-IDF 5.x, native WLED HUB75
  support and NimBLE-Arduino 2.x for the new target.
- Added `/json/info` markers for the WLED IDF5/HUB75 path, MatrixPortal-S3 target,
  NimBLE API generation and PSRAM size/free space.
- The new profile preserves the upstream WLED MatrixPortal partition, OTA,
  HUB75 pinout and default-user-mod policy instead of forcing the legacy WLED
  16.x project wrappers.

## Intentionally unchanged

The BLE protocol, Carousel, alarms, programs, reset semantics, WLED ownership,
clock/text/light rendering and media state machines are carried forward directly
from stable 0.8.2. The purpose of dev.1 is to establish the clean hardware
baseline before changing renderer or protocol behavior.

## Validation status

Host regression tests are expected to pass. A complete PlatformIO firmware build
and all physical MatrixPortal/HUB75 tests must be performed on the target WLED
checkout. This build is not yet a stable release.
