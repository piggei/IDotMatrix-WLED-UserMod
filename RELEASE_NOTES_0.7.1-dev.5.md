# 0.7.1-dev.5

- Adds `build=0.7.1-dev.5` to `/json/info` so hardware reports identify the exact usermod build.
- Keeps the validated 64x64 logical -> 16x16 low-memory rescale path from dev.3/dev.4.
- Further reduces the experimental classic-ESP32 64x64 GIF profile:
  - 12-bit LZW code width;
  - practical dictionary reduced from 2560 to 2304 entries;
  - GIF file buffer reduced from 2048 to 1024 bytes.
- Target: recover roughly another 2 KiB during 64x64 GIF playback and keep WLED above its effect-RAM failure threshold.
- GIFs that grow beyond the compact dictionary may still fail; this remains a validation build, not the final 0.7.1 release.
