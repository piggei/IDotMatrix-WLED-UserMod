# Hardware Test Checklist - 0.9.0-dev.5

Release: `0.9.0`  
Build: `0.9.0-dev.5`

- [x] MatrixPortal S3 boots with HUB75 64x64 output.
- [x] `/json/info` reports `release=0.9.0` and `build=0.9.0-dev.5`.
- [x] 64x64 TEXT LEFT/RIGHT at normal speeds remains smooth.
- [x] 64x64 TEXT UP/DOWN at normal speeds remains smooth.
- [x] Speed 90..100 provides a visibly faster top-end motion than dev.4 without unacceptable jumping.
- [x] 16x16 Snowflake upscaled to physical 64x64 is continuously populated; no band -> black -> band cadence remains.
- [x] 32x32 Snowflake remains visually distributed.
- [x] 64x64 Snowflake remains visually distributed.
- [x] Font sizes including the dev.4 32x64 glyph path still work.
- [x] GIF, Carousel, static image and WLED <-> iDotMatrix ownership are unchanged.


Validation completed on Adafruit MatrixPortal ESP32-S3 + one physical 64x64 HUB75 panel. Physical 32x32 downscale combinations are outside this checklist because the required panel is not currently available.
