# iDotMatrix WLED Usermod 0.9.0

Release 0.9.0 is the stable culmination of the ESP32-S3 / native HUB75 development line. It preserves compatibility with the official iDotMatrix application while extending the Usermod to native 16x16, 32x32 and 64x64 logical rendering and the Adafruit MatrixPortal S3 + 64x64 HUB75 hardware target.

## Highlights

- Adafruit MatrixPortal S3 / ESP32-S3 support with 8 MB flash and 2 MB PSRAM.
- Native WLED HUB75 output on the qualified 64x64 target; no second display driver is introduced by the Usermod.
- Logical 16x16, 32x32 and 64x64 rendering with automatic nearest-neighbour upscale and box-average downscale where applicable.
- GIF/image playback with PSRAM-assisted paths on ESP32-S3.
- TEXT support for 16px, 32px and 64px fonts, including static multi-line layout and complete 16654-byte / 64-glyph 32x64 TEXT objects.
- Persistent 12-slot Carousel with mixed GIF/TEXT media, caching and transactional replacement.
- Separate volatile six-slot Preset / Default player with transactional in-session activation and rollback.
- Alarm and Program/Schedule multipart media assembly with complete-object CRC32 validation.
- Updated Clock, Countdown, Stopwatch and Scoreboard graphics for larger displays.
- AudioReactive integration using WLED processed audio data without opening a second I2S/FFT pipeline.
- Correct display ownership transitions between native WLED effects and iDotMatrix content, including live TEXT takeover from active Preset/Carousel playback.
- LittleFS ownership isolation: iDotMatrix cleanup/reset preserves unrelated WLED/PixelForge files.

## Primary qualified target

- **Board:** Adafruit MatrixPortal S3
- **Display:** 64x64 HUB75 RGB panel
- **WLED baseline:** 0.17.0-devV5
- **Qualified WLED commit:** `06ae26db67107cb3f6a3d107a92340035991a063`
- **PlatformIO environment:** `adafruit_matrixportal_esp32s3_idotmatrix_64x64`

Legacy classic ESP32 and ESP32-C3 16x16 profiles remain documented as previously qualified hardware paths.

## Protocol and storage

The 0.9.0 protocol implementation retains the verified iDotMatrix BLE framing and UUID model. CRC fields provide integrity checking rather than authentication. Alarm, Program/Schedule, Carousel and Preset media use bounded multipart/transactional handling appropriate to their feature semantics.

Preset / Default remains intentionally volatile and is cleared across reboot. Carousel remains persistent. iDotMatrix only deletes files belonging to its own media namespaces.

## Validation

The release package includes host regression tests, parser/media tests, filesystem fault-injection coverage, PlatformIO profile checks and sanitizer coverage for the critical protocol, media, Preset and Carousel paths. Hardware validation covers the primary MatrixPortal S3 / 64x64 HUB75 target and the documented legacy 16x16 paths.

## Deferred work

- Physical 32x32 panel qualification is deferred; the 32x32 logical path is already exercised through the 64x64 target.
- Additional iOS-specific work remains outside the 0.9.0 release scope.
- Future hardware combinations should be reported through the project hardware-compatibility documentation.
