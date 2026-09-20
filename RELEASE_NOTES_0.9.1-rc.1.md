# iDotMatrix WLED UserMod 0.9.1-rc.1

Release `0.9.1`, build `0.9.1-rc.1` is the first release candidate for the 0.9.1 maintenance line.

## Highlights

- Hardware-validated original-app Graffiti full-raster multipart transport on the native 64x64 MatrixPortal/HUB75 target.
- Hardware-validated ESP32-C3 4 MB AudioReactive + dual-slot OTA profile with 640 KiB LittleFS.
- Repository housekeeping: PlatformIO overrides are under `overrides/`; custom partition tables are under `partitions/`.
- Native 64x64 Clock styles 0 and 3 keep the HH:MM row unchanged while moving only the date `/` separator and complete month field two physical LEDs to the right.

## Graffiti multipart protocol

The original application sends a 64x64 full-raster Graffiti image as three complete FA02 logical packets. Each packet uses a 9-byte type-0 header and carries up to 4096 RAW RGB bytes.

- marker `0x00`: first packet;
- marker `0x02`: continuation;
- ACK `05 00 00 00 02`: accepted but incomplete;
- ACK `05 00 00 00 01`: complete.

For a 64x64 RGB raster, the validated transfer is `4096 + 4096 + 4096 = 12288` bytes. This path remains distinct from compact inline PNG and the normal CRC Bulk RAW transport.

## ESP32-C3 OTA qualification

The validated 4 MB C3 OTA partition layout uses two `0x1A0000` application slots, 640 KiB LittleFS and a 64 KiB coredump partition. Hardware qualification included three consecutive OTA updates with populated Carousel, Preset and Schedule data, plus BLE/shared-RMT/GIF-cache/AudioReactive activity.

## Release-candidate scope

RC1 is a promotion of the validated development code. No new runtime feature is intentionally introduced during this promotion.
