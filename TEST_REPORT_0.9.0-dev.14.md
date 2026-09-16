# Test Report - 0.9.0-dev.14

Release: `0.9.0`  
Build: `0.9.0-dev.14`

## Scope

Regression coverage for the first native-resolution light-effect tuning pass.

## Automated checks

The source package is tested with the repository host test suite, PlatformIO profile metadata checks, release-package checks and host sanitizer suite where supported by the execution environment.

## New regression coverage

- Effects 3 and 4 retain 4-pixel bands at 16x16 and expose 8-pixel / 16-pixel bands at 32x32 / 64x64.
- Effect 5 scales both colour and black band widths by the same 1x/2x/4x factor.
- Effect 6 keeps individual pixels at 16x16 and groups adjacent pixels into identical 2x2 / 4x4 colour cells at 32x32 / 64x64.
- Existing 16x16 scrolling-pattern timing remains unchanged.

## Hardware status

Pending MatrixPortal S3 / HUB75 visual validation.
