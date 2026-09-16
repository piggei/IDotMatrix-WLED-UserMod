# iDotMatrix WLED Usermod 0.9.0-dev.15

Release: `0.9.0`  
Build: `0.9.0-dev.15`

## Purpose

This build keeps the successful native-resolution tuning for effects 3-5 and reverts the experimental point enlargement applied to effect 6 in dev.14.

## Changes

- Effects 3 and 4 keep the proportional stripe widths validated in dev.14:
  - 16x16: 4 pixels
  - 32x32: 8 pixels
  - 64x64: 16 pixels
- Effect 5 keeps proportional coloured/black diagonal bands:
  - 16x16: 5-pixel colour / 4-pixel black
  - 32x32: 10-pixel colour / 8-pixel black
  - 64x64: 20-pixel colour / 16-pixel black
- Effect 6 is restored to the original independently seeded per-pixel rendering at every logical resolution. The 2x2 cells at 32x32 and 4x4 cells at 64x64 introduced in dev.14 are removed after hardware review found them visually too coarse.
- Effect 6 timing, colour interpolation and palette selection are unchanged from the original implementation.
- No Carousel, BLE protocol, GIF, automation, ownership or output-scaling behaviour is intentionally changed in this build.

## Hardware validation requested

Confirm that effects 3, 4 and 5 retain the improved band proportions and that effect 6 once again matches the original fine per-pixel texture on the 64x64 panel.
