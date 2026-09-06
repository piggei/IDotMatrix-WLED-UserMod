# Roadmap after 0.8.0

Version 0.8.0 freezes the current stable feature milestone on the validated
classic-ESP32 platform. It combines the 0.7.1 media/memory architecture with
light effects, timers, scoreboard, alarms, programs/schedules, active-buzzer
support, Audio/Rhythm visualizers, and build-aware resolution settings.

The next development cycle is primarily hardware/platform validation: native
64x64 output, PSRAM, ESP32-S3, and HUB75 DMA.

## Stability and release engineering

- [ ] Run a 24-hour Wi-Fi + BLE + repeated-content soak test.
- [ ] Add CI for the complete host test suite.
- [ ] Add pinned PlatformIO compile jobs for every supplied override.
- [ ] Test Wi-Fi loss/reconnect and the WLED recovery AP while BLE remains enabled.
- [x] Run the complete packaging regression on the final 0.8.0 source archive.

## Larger profiles and platform validation

- [x] Add build-time 10-bit/16x16, 11-bit/32x32, and 12-bit/64x64 decoder profiles.
- [x] Remove the duplicate GIF animation framebuffer.
- [x] Prefer PSRAM automatically for renderer/RAW/decoder allocations when present.
- [x] Hardware-validate the compact 11-bit/32x32 decoder on classic ESP32 without PSRAM.
- [x] Implement low-memory Rescale storage for a larger logical profile and smaller physical canvas.
- [x] Implement and hardware-validate the safe no-PSRAM 64x64 compact12/LittleFS frame-cache path.
- [x] Validate repeated GIF replacement, black staging, primary-colour restoration, and Web UI responsiveness.
- [ ] Test the automatic 64x64 PSRAM/direct backend on the incoming PSRAM hardware.
- [ ] Test a native physical 64x64 RGB canvas.
- [ ] Test HUB75 DMA together with BLE/media and document the additional internal-RAM budget.
- [ ] Re-test the 64x64 profile with the normal non-lite WLED feature set on classic ESP32 and record the margin.

## Rendering validation

- [ ] Hardware-validate marker `0x05` 16x32 glyphs across larger profiles.
- [ ] Verify every text motion, colour mode, and visual effect against the app.
- [ ] Verify all eight clock styles, 12/24-hour mode, colour, and date cycling on native larger panels.
- [ ] Decide whether app time sync should optionally seed WLED when NTP is absent.

## Media

- [ ] Capture and document more compact PNG envelopes from multiple app paths.
- [ ] Add captured-packet regression fixtures for RAW, PNG, GIF, and TEXT.
- [ ] Test malformed/corrupt GIF files and filesystem-full behavior on hardware.
- [ ] Measure LittleFS wear/performance under intentionally high-frequency GIF replacement.
- [ ] Add explicit diagnostics for the actual PSRAM allocation source once PSRAM hardware testing starts.

## Feature follow-up

- [x] Seven standalone light effects with deterministic one-pixel scrolling.
- [x] Countdown, stopwatch, scoreboard, and asynchronous countdown-complete status.
- [x] Persistent alarms and programs/schedules with media and active-buzzer integration.
- [x] Five LEVEL and five FFT Audio/Rhythm visualizers.
- [x] Build-aware ScreenType choices and Rescale visibility.
- [ ] Add an optional passive/PWM buzzer backend and configurable tone frequency.

## Platform expansion

- [x] Add WLED 16.0.1 target definitions for classic ESP32 4/8/16 MB, ESP32-WROVER/PSRAM, and ESP32-S3 8/16 MB PSRAM builds.
- [x] Add wrappers for WLED 16.0.1's native HUB75 board/pinout environments.
- [x] Add matching 4/8/16/32 MB single-app/no-OTA partition tables and host regression checks.
- [ ] Run the full WLED PlatformIO compile matrix for every supplied target in CI/release infrastructure.
- [ ] ESP32-S3 hardware validation.
- [ ] PSRAM-enabled target validation.
- [ ] HUB75 wrapper hardware validation on the incoming controller/panel.
- [ ] Newer WLED/framework versions.
- [ ] Measure the RAM impact of inherited/default WLED Usermods on classic ESP32,
      especially when manually re-enabled in `64x64-lite`, and document safe combinations.
- [ ] Revisit true PinManager ownership when WLED exposes a collision-safe owner mechanism for out-of-tree Usermods.
