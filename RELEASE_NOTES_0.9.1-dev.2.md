# iDotMatrix WLED Usermod 0.9.1-dev.2

Release `0.9.1`, build `0.9.1-dev.2` is a qualification and documentation consolidation build based on `0.9.1-dev.1`. No runtime protocol or renderer behavior is intentionally changed from dev.1.

## Graffiti multipart hardware validation

The original-app Graffiti full-raster multipart implementation introduced in dev.1 is now hardware-validated on the qualified Adafruit MatrixPortal S3 + 64x64 HUB75 target.

Validation used the official iDotMatrix app and several complex photographic images. All tested transfers rendered successfully. The validated wire behavior remains:

```text
first  4096-byte RGB chunk, marker 0x00 -> ACK 05 00 00 00 02
next   4096-byte RGB chunk, marker 0x02 -> ACK 05 00 00 00 02
final  4096-byte RGB chunk, marker 0x02 -> ACK 05 00 00 00 01
```

The result confirms the protocol reconstruction derived from the Bluetooth HCI capture and emulator B171. Compact inline PNG and generic Bulk RAW remain separate protocol paths.

## ESP32-C3 OTA qualification

The 4 MB ESP32-C3 / WS2812B 16x16 / AudioReactive OTA profile remains hardware-validated. Qualification evidence includes:

- three consecutive WLED OTA updates;
- 12 stored Carousel assets;
- six Preset assets with playback;
- three Schedule activities;
- BLE + WLED shared-RMT + AudioReactive + GIF frame cache;
- return to native WLED effects after iDotMatrix content;
- filesystem persistence across OTA cycles.

The tested `firmware.bin` size was 1,551,008 bytes in a `0x1A0000` application slot, leaving 152,928 bytes (~149 KiB) of headroom per slot. The profile uses `partitions/WLED_ESP32_4MB_IDOT_OTA.csv` with two application slots and 640 KiB nominal LittleFS.

## Repository layout

PlatformIO templates remain under `overrides/` and custom partition tables under `partitions/`. Current documentation and regression checks use only the reorganized paths.

## Validation status

- Host regression suite: PASS.
- PlatformIO profile / partition static checks: PASS.
- Release-package consistency checks: PASS.
- Graffiti multipart on physical 64x64 hardware: PASS.
- ESP32-C3 dual-slot OTA hardware qualification: PASS.

`0.9.1-dev.2` is intended as the final development qualification build before an RC, provided no regression is found in the remaining quick hardware smoke tests.
