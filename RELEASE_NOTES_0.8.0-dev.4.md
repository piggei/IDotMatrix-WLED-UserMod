# 0.8.0-dev.4 - profile-sized GIF decoder preview

This development build fixes the RAM regression observed in dev.2/dev.3. The
default build returns to the same 10-bit/16x16 AnimatedGIF memory class that was
hardware-stable in 0.7.0.

GIF decoder build modes:

- default: 10-bit, max 16x16;
- `IDOT_GIF_LZW11`: 11-bit, max 32x32;
- `IDOT_GIF_LZW12`: 12-bit, max 64x64, PSRAM required.

The logical renderer remains runtime-selectable between 16x16, 32x32 and 64x64.
The 12-bit decoder never falls back to internal DRAM; if PSRAM is unavailable,
`/json/info` reports `mediaError=gif-psram-required`.

The dev.3 reset/heap flight recorder remains enabled for development testing.
