# iDotMatrix WLED Usermod 0.9.0-rc.1

`0.9.0-rc.1` is the first release candidate for the ESP32-S3 / native HUB75
generation. The planned 0.9 feature set is complete; this build is intended for
final hardware qualification rather than feature expansion.

## Highlights

- Adafruit MatrixPortal ESP32-S3 + 64x64 HUB75 reference target on WLED 0.17.
- Independent logical 16x16 / 32x32 / 64x64 rendering with automatic output scaling.
- Native 64x64 TEXT/font, clock, timer, scoreboard and audio-visualizer artwork.
- GIF playback with PSRAM-aware decoder selection and the established no-PSRAM cache path.
- Persistent 12-slot Carousel with transactional replacement and indeterminate upload status.
- Multi-packet Alarm and Program/Schedule media with complete-object CRC validation.
- Correct Schedule flow control: `0x01` while incomplete, `0x03` after valid completion.
- Volatile six-slot Preset / Default bank (`14..19`) with mixed TEXT/GIF playback and upload status.
- Optional WLED AudioReactive input while retaining the original phone/BLE audio path.
- Finalized Countdown, Stopwatch, Scoreboard and audio-effect artwork.

## Qualification status

The primary 64x64 MatrixPortal S3 path has been exercised for all major app
features. Final qualification should cover fonts/text, Alarm, Program/Schedule,
Preset / Default, Carousel, clocks/timers, repeated ownership transitions and an
extended burn/soak run. See `TESTING.md`.

Physical 32x32 validation and iOS compatibility are deferred and are not 0.9
release blockers. The previous C3 power-off investigation was closed after the
OFF command was traced to Home Assistant grouping rather than firmware.

## Source-package cleanup

Development-only RX telemetry and protocol-study counters were removed.
Historical per-dev release notes are consolidated in `HISTORY.md`; the package
keeps this RC note plus the stable 0.8.2 release notes. No protocol or renderer
behavior is intentionally changed by the RC cleanup.
