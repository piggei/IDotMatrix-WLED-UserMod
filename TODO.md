# Roadmap after 0.7.1

Version 0.7.1 freezes the larger-profile compatibility milestone: the 0.7.0
16x16 behavior is retained, 32x32 logical operation is hardware-validated, and
the classic-ESP32/no-PSRAM 64x64 logical -> 16x16 rescale path is stable with the
compact12/LittleFS frame-cache backend.

## Stability and release engineering

- [ ] Run a 24-hour Wi-Fi + BLE + repeated-content soak test.
- [ ] Add CI for the complete host test suite.
- [ ] Add a pinned PlatformIO compile job for `esp32dev_idotmatrix`.
- [ ] Test Wi-Fi loss/reconnect and WLED recovery AP while BLE remains enabled.
- [ ] Re-evaluate OTA layouts on boards with 8 MB or more flash.
- [ ] Re-run the default 16x16 hardware matrix on the exact 0.7.1 tag as a packaging regression.

## Larger profiles / platform validation

- [x] Add build-time 10-bit/16x16, 11-bit/32x32, and 12-bit/64x64 decoder profiles.
- [x] Remove the duplicate GIF animation framebuffer.
- [x] Prefer PSRAM automatically for renderer/RAW/decoder allocations when present.
- [x] Hardware-validate the compact 11-bit/32x32 decoder on classic ESP32 without PSRAM (`gifDecoderBytes=9372`).
- [x] Implement low-memory `rescale` storage so a large logical profile can use a smaller physical RGB canvas.
- [x] Implement a safe no-PSRAM 64x64 path with all 4096 LZW12 codes, compact predecode, LittleFS frame cache, and decoder release before playback.
- [x] Hardware-validate 64x64 logical -> physical 16x16 (`rescale=true`) on classic ESP32 with repeated large GIFs, Web UI responsiveness, and WLED/clock/image transitions.
- [x] Validate repeated GIF replacement without requiring a manual WLED Solid reset.
- [x] Validate black staging/primary-colour restoration before the 0.7.1 release.
- [ ] Test the automatic 64x64 PSRAM/direct backend on the incoming PSRAM hardware.
- [ ] Test a native physical 64x64 RGB canvas.
- [ ] Test HUB75 DMA together with BLE/media and document the additional internal-RAM budget.
- [ ] Re-test the 64x64 profile with the normal (non-lite) WLED feature set on classic ESP32 and record the margin.

## Rendering quality

- [ ] Improve text rendering when a 32x32/64x64 logical profile is aggressively downscaled to a smaller physical canvas; thin app glyph strokes can currently disappear at 64->16.
- [ ] Hardware-validate marker `0x05` 16x32 glyphs across larger profiles.
- [ ] Verify every text motion, colour mode, and visual effect against the app.
- [ ] Verify all eight clock styles, 12/24-hour mode, colour, and date cycling on native larger panels.
- [ ] Decide whether app time sync should optionally seed WLED when NTP is absent.

## Media

- [ ] Capture and document more compact PNG envelopes from multiple app paths.
- [ ] Add captured-packet regression fixtures for RAW, PNG, GIF, and TEXT.
- [ ] Test malformed/corrupt GIF files and filesystem-full behavior on hardware.
- [ ] Measure LittleFS wear/performance under intentionally high-frequency GIF replacement.
- [ ] Add explicit diagnostics for actual PSRAM allocation source once PSRAM hardware testing starts.

## Protocol/features / 0.8.0

Version 0.8.0 remains reserved for feature work rather than another decoder-memory
experiment.

- [ ] Countdown timer.
- [ ] Stopwatch.
- [ ] Scoreboard.
- [ ] Alarms/schedules where WLED semantics match.
- [ ] Optional buzzer support and original-device buzzer pattern research.
- [ ] Rotation / energy-saving / original-device standalone effects.

## Platform expansion

- [ ] ESP32-S3 build and hardware validation.
- [ ] PSRAM-enabled targets.
- [ ] Newer WLED/framework versions.
- [ ] Reintroduce other WLED Usermods/integrations one at a time and document compatibility.
