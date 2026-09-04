# WLED iDotMatrix Usermod 0.7.0

First stable 16x16 media release.

## Highlights

- official iDotMatrix app discovery and BLE control;
- power, brightness, RGB, Graffiti, clock, and text;
- corrected text speed-slider range;
- static RAW/cloud images and compact PNG images;
- GIF animation upload, replacement, and playback;
- MTU 517 + correct FA02 fragment reassembly;
- recovery from interrupted fragmented transfers;
- low-RAM 16x16 AnimatedGIF build for classic ESP32;
- repeated animation testing followed by return to clock without reboot or BLE
  reconnect;
- development diagnostics removed from the stable release.

## Stable target

- WLED 16.0.1;
- classic ESP32 / `esp32dev`;
- 16x16 WLED 2D matrix;
- I2S LED backend;
- NimBLE-Arduino 1.4.3;
- AnimatedGIF 1.4.7 patched locally at build time;
- USB/serial flashing; supplied 4 MB no-OTA partition layout.

32x32/64x64 profiles remain experimental. Stable 0.7.0 GIF playback is
intentionally limited to 16x16.
