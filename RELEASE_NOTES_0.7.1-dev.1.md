# 0.7.1-dev.1

First development snapshot of the 0.7.1 release line.

## What is already validated

- 16x16 / LZW10 stable baseline from 0.7.0.
- 32x32 / compact LZW11 on classic ESP32 without PSRAM.
- `gifDecoderBytes=9372` for the validated 32x32 build.
- Repeated GIF replacement and return to normal WLED effects without Error 8 or reboot.

## 64x64 changes

- `platformio_override.ini.64x64` enables full 12-bit LZW and a 4096-entry dictionary.
- AnimatedGIF's stream cache is kept at 1024 bytes to recover about 3 KiB while retaining the complete LZW dictionary.
- PSRAM is preferred automatically when present.
- Classic ESP32 may use internal DRAM only if the decoder allocation leaves a 12 KiB WLED safety reserve.
- If the safety condition is not met, GIF playback fails with `mediaError=gif-ram-reserve`; WLED/BLE should remain alive.
- `platformio_override.ini.64x64-lite` is supplied as an optional measurement build with unused integrations disabled.

The 64x64 path is experimental until the hardware stress sequence in TESTING.md passes.
