# iDotMatrix WLED Usermod 0.9.0-dev.4

**Release:** `0.9.0`  
**Build:** `0.9.0-dev.4`

This development build closes the remaining known TEXT-size gap on the
MatrixPortal S3 / native HUB75 64x64 target.

## Changes

- Adds 32x64 1-bit app-rasterized glyph support for the 64-pixel TEXT size.
- Supports 256 bitmap bytes per glyph by widening the internal glyph-size field.
- Accepts the expected `0x08`/`0x09` marker family and a strict structural fallback for equivalent 260-byte glyph records.
- Preserves the app-provided bitmap exactly; WLED still does not provide or substitute fonts.
- Preserves dev.3 smooth vertical TEXT paging and dev.2 automatic logical/physical scaling.

## Validation target

Primary hardware target remains Adafruit MatrixPortal ESP32-S3 + one 64x64 HUB75 panel on WLED 0.17 beta / IDF5. The new 64-pixel TEXT path is the specific hardware-validation item for this build.
