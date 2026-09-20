# iDotMatrix WLED UserMod 0.9.1

Release `0.9.1`, build `0.9.1` is the stable maintenance release following 0.9.0. It preserves the qualified 0.9.0 feature set while closing the remaining original-app Graffiti multipart gap, validating a practical ESP32-C3 OTA layout, and cleaning the repository build-support structure.

## Highlights

- Hardware-validated original-app **Graffiti full-raster multipart** transport on the native 64x64 MatrixPortal/HUB75 target.
- Hardware-validated **ESP32-C3 SuperMini 4 MB + WS2812B 16x16 + AudioReactive + dual-slot OTA** profile.
- PlatformIO overrides moved to `overrides/` and custom partition tables moved to `partitions/`.
- Native 64x64 Clock styles 0 and 3 keep the qualified HH:MM row unchanged while moving only the date `/` separator and complete month field two physical LEDs to the right.
- Documentation, protocol reference, qualification records and package checks aligned to the final release.

## Graffiti full-raster multipart

The official 64x64 app can send a complete Graffiti canvas as multiple complete FA02 logical packets. This is a dedicated type-0 RAW RGB transport and is distinct from both compact inline PNG and the normal CRC Bulk RAW path.

Each logical packet uses a 9-byte header:

```text
LENlo LENhi 00 00 MARKER TOTAL_SIZE_LE32 RGB_CHUNK...
```

Confirmed marker/ACK flow:

```text
marker 0x00 = first packet
marker 0x02 = continuation

incomplete -> 05 00 00 00 02
complete   -> 05 00 00 00 01
```

For 64x64 RGB, the hardware-validated transfer is:

```text
4096 + 4096 + 4096 = 12288 = 64 * 64 * 3 bytes
```

The complete raster size is repeated in each chunk. No CRC field exists in this Graffiti envelope. BLE/ATT fragmentation is handled first by the FA02 assembler; the Graffiti transaction then joins multiple complete FA02 packets into one raster object.

Validation uses the original Bluetooth capture, standalone emulator B171, host regression and multiple complex photographic images sent from the official app to the physical 64x64 target.

## ESP32-C3 OTA

The validated C3 OTA profile is:

```text
overrides/esp32c3-16x16-audio-ota.ini
env:esp32c3dev_idotmatrix_audio_16x16_ota
```

Its 4 MB partition layout uses two `0x1A0000` application slots, 640 KiB nominal LittleFS and a 64 KiB coredump partition. The validated `firmware.bin` measured 1,551,008 bytes, leaving 152,928 bytes (about 149 KiB) per OTA slot.

Hardware qualification included:

- first installation of the new partition table over USB/serial;
- three consecutive successful WLED OTA updates;
- 12 stored Carousel assets;
- six Preset assets and Preset playback;
- a three-activity Schedule;
- BLE, shared-RMT output, GIF frame cache and AudioReactive in the same build;
- successful return from iDotMatrix-owned content to native WLED effects;
- persistent Carousel/Schedule filesystem content retained across OTA cycles; Preset remains intentionally volatile and is cleared by the OTA reboot.

The legacy no-OTA C3 profiles remain available.

## Compatibility retained from 0.9.0

0.9.1 retains the previously qualified 16x16/32x32/64x64 logical renderer, native MatrixPortal/HUB75 output, 16654-byte 64-pixel TEXT path, GIF/image playback, persistent Carousel, volatile transactional Preset/Default, Alarm and Program/Schedule multipart media, Audio/Rhythm visualizers, Clock/Countdown/Stopwatch/Scoreboard behavior, WLED ownership transitions, and scoped LittleFS cleanup that preserves unrelated WLED/PixelForge files.

## Qualified baselines

Primary MatrixPortal target:

```text
Adafruit MatrixPortal ESP32-S3 + 64x64 HUB75
WLED 17.0.0-devV5
06ae26db67107cb3f6a3d107a92340035991a063
```

ESP32-C3 target:

```text
ESP32-C3 SuperMini 4 MB + WS2812B ECO 16x16
WLED commit d55037f7510541eddc390c8f3d01afc5787aa44a
Arduino 3.3.8 / ESP-IDF 5.5.4 shared-RMT
NimBLE-Arduino 2.5.1
```

## Security model

The compatibility BLE GATT profile remains intentionally unauthenticated, matching the observed original-device/app behavior. Nearby BLE peers can issue supported commands. CRC fields protect media integrity, not authentication.

## Release promotion

The final 0.9.1 build is a documentation/version promotion of the qualified release-candidate runtime code. No new runtime feature is intentionally introduced during final promotion.
