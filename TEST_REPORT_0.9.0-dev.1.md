# Test Report - 0.9.0-dev.1

## Scope

First clean 0.9 development build for ESP32-S3 / MatrixPortal / native WLED HUB75 / 64x64 / PSRAM.

## Source baseline

Derived from the 0.8.2 stable source package. The temporary 0.8.2-diag.1 power-source instrumentation is not present.

## Automated verification

- Host behavior/regression suite: **PASS** (`All iDotMatrix host tests passed.`).
- Host sanitizer suite: **PASS** (`ASan/UBSan host tests passed.`).
- Package/profile checks: **PASS**, including the new MatrixPortal S3/HUB75 profile contract.

## Firmware/hardware verification

A complete PlatformIO build is not claimed by this source-package preparation environment. The firmware must be compiled against the same WLED 0.17 development checkout on which the stock `adafruit_matrixportal_esp32s3` environment has already been validated.

Physical 64x64 iDotMatrix behavior is intentionally marked **pending** until the first target test.
