# 0.8.0-dev.6

- Keeps the validated 10-bit/16x16 GIF decoder in static DRAM.
- Changes the 11-bit/32x32 GIF decoder to on-demand heap allocation.
- Releases the 11-bit decoder when GIF playback stops, so normal WLED effects do not permanently lose RAM in 32x32 builds.
- Keeps the 12-bit/64x64 decoder PSRAM-only.
- Intended to fix `Error 8: Effect RAM depleted!` observed in dev.5 even while still running the 16x16 runtime profile.
