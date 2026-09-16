# iDotMatrix WLED Usermod 0.9.0-dev.6

Release: `0.9.0`  
Build: `0.9.0-dev.6`

## Focus

This development build adds a procedural transfer indicator for Carousel uploads while preserving the validated 64x64 MatrixPortal S3/HUB75 path from dev.5.

## Changes

- Animated down-arrow feeding a small matrix icon during Carousel asset upload.
- Current-asset progress bar based on received bytes and declared asset size.
- 250 ms visibility threshold to avoid flashes for short transfers.
- Indicator participates in normal iDotMatrix display ownership and automatic 16/32/64 output scaling.
- No bitmap assets and no additional filesystem writes are required.
- Generic adapter API retained for possible long standalone GIF upload indication in a future build.

## Scope

The indicator is enabled for Carousel asset upload only in dev.6. Standalone gallery GIF transfers are intentionally unchanged until the Carousel behavior is validated on hardware.
