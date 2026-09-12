# iDotMatrix WLED Usermod 0.8.2-rc.2

**Release:** 0.8.2  
**Build:** 0.8.2-rc.2

## Purpose

0.8.2 is the protocol-convergence release for the existing supported hardware line. It brings the behavior developed and hardware-tested after 0.8.1 into a release-candidate branch without moving the project to the new-hardware 0.9 line. Release 0.9 is reserved for ESP32-S3/PSRAM, native larger-matrix validation and HUB75 work.

## Main changes since 0.8.1

- Added persistent **Device Assets / Carousel** support with one 12-slot bank, mixed GIF/TEXT content, per-slot dwell time and autonomous playback from LittleFS.
- Added the WLED `iDotMatrix Display` standalone policy: when the effect is selected, a stored Carousel is used when available; otherwise the Clock is used as the fallback content.
- Added long-TEXT viewport behavior, including complete glyph traversal, geometry-derived page capacity, continuous LEFT/RIGHT scrolling, continuous UP/DOWN page tape and phase continuity for page-based visual effects.
- Added optional WLED **AudioReactive** input while retaining Phone/BLE as the compatibility/default source. `Auto` may prefer AudioReactive data and fall back to Phone/BLE data.
- Kept Device Info release bytes separate from the internal build identifier. For this release the application-facing major/minor bytes are `00 08`; `0.8.2-rc.2` remains an internal build string only.
- Preserved the command-specific Schedule ACK behavior established during protocol validation.

## Device reset

RC2 completes the iDotMatrix protocol reset path. The recognized `03 80` reset is a **live iDotMatrix state reset**, not an ESP32/WLED reboot.

The reset now:

- erases all persistent Carousel assets and manifest metadata;
- erases configured alarms and their persisted media/state;
- erases schedules/programs, staging/backup media and persisted schedule state;
- stops active alarm/program ownership and buzzer activity;
- clears transient iDotMatrix content and leaves the logical display available for the next command;
- preserves WLED global configuration, Wi-Fi/BLE configuration and the current system/application time authority.

This reset is also the supported protocol operation for removing a stored Carousel bank from the device.

## WLED effect and boot behavior

The `iDotMatrix Display` effect uses the normal WLED effect-selection path. Persistent Carousel playback does not require the official phone application to remain connected: assets are discovered from LittleFS during Usermod startup. A valid WLED boot preset that selects `iDotMatrix Display` therefore starts the stored Carousel autonomously; without a stored Carousel, the effect falls back to Clock.

WLED presets store the selected effect state. If a preset was created while experimenting with an older development build, recreate that preset before release qualification so it contains the current valid effect selection.

## Compatibility

The supported 16x16 platform baseline remains unchanged from 0.8.1:

- classic ESP32 with the documented WLED 16.0.1/I2S constraints;
- ESP32-C3 with pinned WLED commit `d55037f7510541eddc390c8f3d01afc5787aa44a`, IDF5/shared-RMT, NimBLE-Arduino 2.x and the compact12/cache GIF path;
- no PSRAM requirement for the supported 16x16 profiles.

The existing BLE protocol, FA02 handling, GIF playback, alarms/programs, build-aware resolution handling and WLED configuration keys are retained.

## Deferred work

The following remain intentionally outside 0.8.2 and require the new hardware line or additional protocol evidence:

- ESP32-S3/PSRAM qualification;
- physical 32x32/64x64 validation and the third 64x64 TEXT format;
- native HUB75 output work;
- complete password SET/VERIFY/enforcement behavior;
- unrelated architectural hardening that would add release risk without fixing a confirmed defect.

## Qualification status

Host regression tests and ASan/UBSan tests are required to pass on the final packaged source tree. Physical ESP32-C3 16x16 testing has already validated the principal 0.8.2 behavior developed in this branch, including long TEXT, mixed GIF/TEXT Carousel playback, autonomous stored-asset playback and the release-candidate reset behavior.
