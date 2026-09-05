# 0.7.1-dev.3

This development release targets the classic-ESP32 64x64 RAM bottleneck. With `rescale=true`, the logical iDotMatrix profile is now independent from renderer storage. A 64x64 app profile on the 16x16 test matrix keeps a 16x16 RGB canvas (768 bytes) instead of a 64x64 canvas (12,288 bytes), saving 11,520 bytes of persistent framebuffer RAM.

RAW media are sampled while chunks arrive and GIF pixels are sampled inside the AnimatedGIF draw callback. The decoder still sees a real 64x64/LZW12 stream. `/json/info` now exposes `profile=64x64` and `canvas=16x16` separately. Start testing with `platformio_override.ini.64x64`; the lite override is no longer the primary path.

The previous boot snapshot (`reset=panic`, pre-reset minimum heap around 6 KiB and largest block around 12 KiB) motivated this change. The goal is to recover framebuffer RAM rather than remove more WLED functionality.
