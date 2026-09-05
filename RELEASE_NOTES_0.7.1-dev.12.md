# 0.7.1-dev.12

This development build replaces the classic-ESP32/no-PSRAM 64x64 AnimatedGIF predecode backend with a compact-safe LZW12 decoder. The complete 4096-code GIF space is preserved; the prefix dictionary is packed to 12 bits instead of truncating legal codes. Frames are still downscaled and cached in LittleFS before playback, so the decoder workspace is released before `iDotMatrix Display` starts cached playback.

On PSRAM-equipped ESP32 boards, backend selection remains automatic and the full AnimatedGIF direct-playback path is used.

Expected `/json/info` on the current 64x64->16x16 classic ESP32 test setup includes `build=0.7.1-dev.12`, `gifDecoder=compact12/cache`, and `gifDecoderBytes=16128`.
