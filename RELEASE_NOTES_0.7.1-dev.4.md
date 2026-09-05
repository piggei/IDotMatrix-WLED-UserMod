# 0.7.1-dev.4

Experimental compact LZW12 profile for classic ESP32 64x64 media.

- Keeps the validated low-memory 64x64 -> physical-canvas rescale from dev.3.
- Keeps true 12-bit LZW code-width handling.
- Reduces the physical LZW dictionary from 4096 to 2560 entries.
- Uses a 2048-byte reverse pixel stack/cache.
- Expected AnimatedGIF object size is roughly 15-16 KiB instead of 20.6 KiB.
- The goal is to keep WLED above its effect-RAM safety margin during GIF playback.
- This is intentionally experimental: unusually complex GIFs may exceed the compact dictionary and fail to decode. The firmware must fail/recover cleanly rather than driving WLED into Error 8 or a panic.

No BLE, RAW image, renderer ownership, or low-memory canvas behavior was changed.
