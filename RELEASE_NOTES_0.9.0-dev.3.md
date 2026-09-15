# iDotMatrix WLED Usermod 0.9.0-dev.3

**Release:** 0.9.0  
**Build:** 0.9.0-dev.3

## Focus

This development build refines native 64x64 TEXT UP/DOWN scrolling without changing the BLE protocol, app-provided glyph bitmaps, automatic logical-to-physical scaling, GIF/media handling, Carousel behaviour or HUB75 output integration.

## Changes

- Fixes native 64x64 vertical TEXT transitions with 16x32 glyph pages.
- The following page now starts fully outside the logical viewport and enters one raster row at a time.
- Keeps the existing speed field semantics and per-pixel motion cadence.
- Leaves LEFT/RIGHT motion, stationary/page effects and color effects unchanged.
- Preserves the full automatic scaling matrix introduced in dev.2 (16/32/64 logical profiles to 16/32/64 physical matrices).

## Hardware validation target

Adafruit MatrixPortal ESP32-S3, WLED 0.17 beta / IDF5, native HUB75, physical 64x64 panel. Test all three app profiles (64x64, 32x32 and 16x16), with special attention to TEXT effect UP and DOWN at multiple speed values.
