# 0.7.1-dev.10

Experimental 64x64 classic-ESP32 GIF frame-cache build.

- Full safe 4096-entry LZW12 decoder; no truncated dictionary.
- 64x64 logical -> physical low-memory rescale remains unchanged.
- On classic ESP32 without PSRAM, GIFs are predecoded one frame per WLED loop into a temporary LittleFS cache (maximum 512 KiB).
- The LZW12 decoder is released before `iDotMatrix Display` takes ownership, so playback no longer reserves ~20 KiB of decoder DRAM.
- Predecode temporarily uses WLED `Static` to maximize free/contiguous heap.
- Adds `gifCache=building frames=N` and `gifCachedFrames=N` diagnostics.
- 16x16/32x32 direct GIF playback remains unchanged.
