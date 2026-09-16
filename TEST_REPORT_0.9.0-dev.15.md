# Test Report - 0.9.0-dev.15

Release: `0.9.0`  
Build: `0.9.0-dev.15`

## Scope

Regression coverage for the second native-resolution light-effect tuning pass.

## Automated checks

The source package is tested with the repository host test suite, PlatformIO profile metadata checks, release-package checks and host sanitizer suite where supported by the execution environment.

## Regression coverage

- Effects 3 and 4 retain 4-pixel bands at 16x16 and 8-pixel / 16-pixel bands at 32x32 / 64x64.
- Effect 5 retains proportional colour and black band widths at all three logical resolutions.
- Effect 6 uses independent per-pixel seeds at 16x16, 32x32 and 64x64; no resolution-scaled point grouping remains.
- Existing Carousel transfer-indicator and playback behaviour remain unchanged.

## Hardware status

Effects 3-5 were accepted on hardware after dev.14. Effect 6 reversion requires a quick visual confirmation on the 64x64 HUB75 panel.
