# iDotMatrix WLED Usermod 0.9.0-dev.5

Release: `0.9.0`  
Build: `0.9.0-dev.5`

This development build is a focused visual-motion refinement on top of the hardware-validated MatrixPortal S3 / HUB75 / PSRAM path.

## Changes

- Extends the usable top end of TEXT scroll speed on a native 64x64 logical canvas.
  The historical 15 ms/pixel minimum is already below the WLED render period at roughly 43 FPS, so reducing the interval alone cannot make the fastest setting visibly faster. In the final 10% of the app speed range, native 64x64 positional TEXT motion may therefore advance two logical pixels per accepted render. Lower speeds and 16x16/32x32 logical profiles keep the existing one-pixel cadence.
- Reworks TEXT `Snowflake` particle placement. The previous fixed eight-particle `index*3` vertical phase produced a visible band/blank/band pattern on a 16x16 logical canvas, especially when automatically upscaled 4x to a 64x64 physical panel.
- Snowflake now uses deterministic per-particle horizontal positions, vertical phase offsets and two fall rates, with particle density scaled for 16x16, 32x32 and 64x64 canvases.
- No changes to the BLE protocol, output scaler, HUB75 backend, GIF/Carousel pipeline, 64-pixel glyph format or persistent storage.

## Hardware validation result

Hardware validation completed successfully on Adafruit MatrixPortal ESP32-S3 + one physical 64x64 HUB75 panel running WLED 0.17 beta. The validated runtime reported `framework=WLED IDF5/HUB75`, `profile=64x64`, `canvas=64x64`, `output=64x64 scale=1x1`, NimBLE 2.x, 2 MB PSRAM and `gifDecoder=animatedgif12/psram`.

The same physical 64x64 panel also validated logical 32x32 -> 64x64 and 16x16 -> 64x64 automatic upscale. Hardware checks passed for the 32x64 glyph/TEXT path, LEFT/RIGHT and UP/DOWN scrolling, extended top-end TEXT speed, Snowflake at 16/32/64 logical profiles, animated GIF playback, Carousel, static images and WLED <-> iDotMatrix ownership transitions. Physical 32x32 downscale combinations remain host-tested only because no 32x32 panel is currently available.
