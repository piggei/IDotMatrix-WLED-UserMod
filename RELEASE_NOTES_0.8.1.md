# WLED iDotMatrix Usermod 0.8.1

**Public release:** `0.8.1`  
**Released internal build:** `0.8.1-audit-fix1`

This source archive is the final public 0.8.1 source release. The internal build
identifier is intentionally retained because this exact corrective revision
completed the final ESP32-C3 hardware qualification.

## Audit remediation

- fixed GIF promotion recovery: if both RX-to-play rename and streamed-copy
  fallback fail, media state now publishes `gif-cache-io`, allowing the existing
  adapter recovery path to run and leaving the next GIF transfer usable;
- made schedule/program replacement transactional per activity: the previous
  program file is retained under a temporary backup until the new media is
  installed and its NVS metadata write is confirmed; failures roll back to the
  previous valid program, or clear stale metadata when rollback is impossible;
- `loadPersistence()` now removes corrupt schedule/alarm metadata and clears
  persisted schedule entries whose media file is missing or has the wrong size;
- added behavioural host regression tests for automation persistence, schedule
  replacement failures, reboot recovery, time fallback and midnight windows;
- added GIF filesystem failure injection tests and an ASan/UBSan host pass;
- separated public release and internal build diagnostics: `release=0.8.1` and
  `build=0.8.1-audit-fix1`;
- corrected documentation for LZW12 16x16 builds, FA02 4112-byte inline vs
  8192-byte maximum capacity, `custom_usermods`, app-time fallback, and PNG
  validation scope.

## Summary

0.8.1 is a maintenance release that adds **official ESP32-C3 4 MB / 16x16
support** while preserving the stable classic-ESP32 feature set from 0.8.0.

The key C3 change is architectural: legacy IDF4 RMT produced visible pixel
spikes, especially with BLE active. The supported C3 profile instead uses the
pinned WLED IDF5/shared-RMT backend, where physical testing completed without
LED spikes.

## Supported release targets

- classic ESP32 / `esp32dev`, WLED 16.0.1, 16x16 baseline;
- ESP32-C3 4 MB / `esp32c3dev`, 16x16, pinned WLED commit
  `d55037f7510541eddc390c8f3d01afc5787aa44a`.

The C3 build uses Arduino Core 3.3.8, ESP-IDF 5.5.4, `WLED_USE_SHARED_RMT`,
NimBLE-Arduino 2.5.1 and AnimatedGIF 1.4.7. Classic ESP32 retains the existing
NimBLE 1.4.3 path.

## C3 hardware validation

The supported C3 path was exercised on a real 4 MB ESP32-C3 with a physical
16x16 matrix on GPIO4. Validation included:

- WLED boot and normal 2D effects;
- BLE advertising and connection from the iDotMatrix app;
- static images and animated GIF playback;
- switching back to normal WLED effects;
- roughly 100 media/animation changes and about 20 WLED effects;
- active WebSocket/Web UI use;
- no LED spikes and no reboot.

The final `0.8.1-audit-fix1` qualification added a release-candidate smoke/stress
run of approximately 50 WLED effect changes, 50 iDotMatrix content/effect
changes, and another 50 WLED effect changes, followed by successful program /
schedule operation. The final snapshot reported 75,296 bytes free heap,
`min=43012`, a 65,536-byte largest contiguous block, `gifProbe=79444`, a
69,632-byte GIF-probe largest block, and a 10,240-byte GIF reserve. BLE remained
connected and one WLED WebSocket remained active. No reset was observed during
the run; the current IDF5 reset-reason diagnostic reports `reset=unknown` on this
board and is deferred as a non-blocking diagnostic cleanup item.

Earlier aggressive Web UI stress on the same IDF5/shared-RMT path occasionally
showed a transient red WLED connection banner while the requested effect still
started and the device remained responsive. This was not observed as an LED,
BLE, watchdog, or reboot failure.

## Build requirements for C3

Use the exact WLED commit above and `platformio_override.ini.c3`. Linux/WSL is
recommended; the tested modern WLED/IDF5 PlatformIO command lines can exceed
Windows process limits. WLED's UI build requires Node.js 20 or newer.

```bash
git clone https://github.com/wled/WLED.git WLED-idot-c3
cd WLED-idot-c3
git checkout d55037f7510541eddc390c8f3d01afc5787aa44a
cp ../wled-usermod-idotmatrix/platformio_override.ini.c3 platformio_override.ini
pio run -e esp32c3dev_idotmatrix_16x16 -t clean
pio run -e esp32c3dev_idotmatrix_16x16
```

The WLED base identifies itself as `17.0.0-devV5`; its development-build alert
in the Web UI is expected. 0.8.1 deliberately pins the tested commit instead of
following a moving nightly.

## Internal fixes

- NimBLE 1.x/2.x API bridge retained so classic ESP32 and C3 use their proven
  dependency generations;
- C3 release guard now requires ESP32-C3 + IDF5 + `WLED_USE_SHARED_RMT` + NimBLE 2.x;
- FA02 large-packet dynamic storage is no longer freed while the BLE queue
  critical-section spinlock is held;
- stable package no longer ships the dev.1/dev.2/dev.3 C3 build profiles.

## Not changed

No intentional feature redesign was made to the iDotMatrix protocol, renderer,
GIF/media behavior, alarms/programs, Audio/Rhythm, timers, scoreboard or buzzer.
ESP32-S3 and HUB75 validation moves to the 0.9 development line.
