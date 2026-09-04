# Roadmap after 0.7.0

Version 0.7.0 freezes the first stable 16x16 baseline: BLE discovery/control,
graffiti, clock, text, static images, and GIF animations are working together on
a classic ESP32/WLED target.

## Stability and release engineering

- [ ] Run a 24-hour Wi-Fi + BLE + repeated-content soak test.
- [ ] Add CI for all six host tests.
- [ ] Add a pinned PlatformIO compile job for `esp32dev_idotmatrix`.
- [ ] Test Wi-Fi loss/reconnect and WLED recovery AP while BLE remains enabled.
- [ ] Re-evaluate OTA layouts on boards with 8 MB or more flash.

## Larger logical profiles

- [ ] Hardware-validate 32x32 and 64x64 graffiti, clock, text, and RAW images.
- [ ] Define a memory budget for each profile.
- [ ] Decide whether larger GIF profiles use PSRAM, a different decoder, or a
      profile-specific low-RAM AnimatedGIF build.
- [ ] Validate `rescale` from 32/64 logical canvases to smaller WLED matrices.

## Media

- [ ] Capture and document more compact PNG envelopes from multiple app paths.
- [ ] Add captured-packet regression fixtures for RAW, PNG, GIF, and TEXT.
- [ ] Test malformed/corrupt GIF files and filesystem-full behavior.
- [ ] Evaluate PSRAM opportunistically without making it a 16x16 requirement.

## Text and clock

- [ ] Hardware-validate marker `0x05` 16x32 glyphs.
- [ ] Verify every text motion, colour mode, and visual effect against the app.
- [ ] Verify all eight clock styles, 12/24-hour mode, colour, and date cycling.
- [ ] Decide whether app time sync should optionally seed WLED when NTP is absent.

## Protocol/features

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
- [ ] Reintroduce other WLED Usermods one at a time and document compatibility.
