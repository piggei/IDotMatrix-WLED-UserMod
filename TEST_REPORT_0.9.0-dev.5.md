# Test Report - 0.9.0-dev.5

Release: `0.9.0`  
Build: `0.9.0-dev.5`

## Automated checks

- Host renderer regression suite: PASS.
- PlatformIO profile checks: PASS.
- Release-package checks: PASS.

## New regressions

1. Native 64x64 TEXT at speed 100 advances two logical pixels on an accepted motion render, proving that the upper speed range can exceed the one-pixel-per-WLED-frame ceiling.
2. Native 16x16 Snowflake with an empty glyph produces at least 12 visible snow pixels distributed across at least 8 distinct logical rows, preventing the former travelling-band pattern.
3. Existing dev.3/dev.4 vertical scrolling and 32x64 glyph regressions remain part of the suite.

## Hardware validation result

PASS on Adafruit MatrixPortal ESP32-S3 + one physical 64x64 HUB75 panel, WLED 0.17 beta.

Validated items:

- native 64x64 profile and `scale=1x1`;
- logical 32x32 -> physical 64x64 automatic 2x upscale;
- logical 16x16 -> physical 64x64 automatic 4x upscale;
- 32x64 / 256-byte glyph path used by the app's 64-pixel TEXT size;
- TEXT LEFT/RIGHT and UP/DOWN motion;
- higher top-end 64x64 TEXT speed;
- Snowflake at 16x16, 32x32 and 64x64 logical profiles, with the former band/blank/band artifact absent;
- animated GIF playback through `animatedgif12/psram`;
- Carousel, static images and WLED <-> iDotMatrix ownership transitions.

Representative runtime snapshot after the validation sequence: 43 FPS, 2 MB PSRAM with about 1.98 MB free, approximately 48 KB internal free heap, `displayFx active=1`, `lease=1`, `logical=1`, and `content=text`.

Physical 32x32 downscale combinations remain unvalidated on hardware because no physical 32x32 panel is currently available; corresponding scaler paths remain host-tested.

## Sanitizers

- Targeted renderer ASan/UBSan regression: PASS.
- Full sanitizer suite: exceeded the execution window in the packaging environment; no full-suite PASS is claimed.
