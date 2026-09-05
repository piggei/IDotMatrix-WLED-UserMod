# 0.7.1-dev.11

Classic-ESP32 64x64 GIF frame-cache admission tuning.

- Predecode admission reserve: 10 KiB.
- Runtime cache-build guard: 9 KiB free internal heap before each decoded frame.
- Full 4096-entry LZW12 dictionary retained.
- Decoder remains temporary and is destroyed before cached playback begins.
- No changes to the validated 16x16/32x32 direct paths.
