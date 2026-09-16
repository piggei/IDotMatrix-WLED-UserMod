# iDotMatrix WLED Usermod 0.9.0-dev.14

Release: `0.9.0`  
Build: `0.9.0-dev.14`

## Purpose

This build starts native-resolution tuning of the iDotMatrix light effects after Carousel transfer handling was considered complete.

## Changes

- Effects 3 and 4 keep the original 16x16 stripe width and scale it proportionally with the logical framebuffer:
  - 16x16: 4 pixels
  - 32x32: 8 pixels
  - 64x64: 16 pixels
- Effect 5 scales both the coloured and black portions of its diagonal pattern by the same 1x/2x/4x factor:
  - 16x16: 5-pixel colour / 4-pixel black
  - 32x32: 10-pixel colour / 8-pixel black
  - 64x64: 20-pixel colour / 16-pixel black
- Effect 6 preserves the original per-pixel appearance at 16x16, while grouping the colour field into 2x2 cells at 32x32 and 4x4 cells at 64x64. Colour timing and interpolation are unchanged.
- No Carousel, BLE protocol, GIF, automation, ownership, or output-scaling behaviour is intentionally changed in this build.

## Hardware validation requested

Evaluate effects 3, 4, 5 and 6 at logical profiles 16x16, 32x32 and 64x64, with particular attention to visual density, apparent motion speed and whether the larger effect-6 cells are comfortable on the 64x64 panel.
