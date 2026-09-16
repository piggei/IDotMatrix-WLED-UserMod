# 0.9.0 development priorities

- [x] Hardware-qualify MatrixPortal ESP32-S3 + native WLED HUB75 + physical 64x64.
- [x] Validate PSRAM direct AnimatedGIF playback with Wi-Fi and BLE active.
- [x] Validate the main iDotMatrix visual paths at true 64x64, including TEXT, 32x64 glyphs, GIF, Carousel and procedural TEXT effects.
- [x] Validate automatic logical 32x32 -> physical 64x64 and logical 16x16 -> physical 64x64 output scaling.
- [ ] Validate physical 32x32 combinations on real hardware when a 32x32 panel is available.
- [x] Hardware-validate Alarm and Program/Schedule multi-packet transfer on the 64x64 target.
- [x] Hardware-validate Preset / Default transfer and playback, including long uploads with the shared transfer indicator.
- [ ] Continue general Carousel/Alarm/Program/Preset/reset soak testing on the new target as the 0.9 line matures.
- [ ] Investigate iOS BLE compatibility separately from the hardware migration.

# Post-0.8.2 roadmap

Release **0.8.2** is complete and stable for the currently qualified ESP32 / ESP32-C3 line.
The former `0.8.2-rc.2` ... `0.8.2-rc.7` sequence is retained in `HISTORY.md` as development history; the runtime identification of the published release is now:

```text
Release: 0.8.2
Build:   0.8.2
```

## Completed in 0.8.2

- [x] Persistent 12-slot Device Assets / Carousel with mixed GIF + TEXT content.
- [x] Carousel boot restore and standalone WLED-effect selection policy.
- [x] Long-TEXT viewport/paging on physical 16x16 hardware.
- [x] Optional WLED AudioReactive source, including external-microphone hardware validation.
- [x] Single-owner FA02 reassembly and BLE RX/timeout diagnostics.
- [x] Persistent per-slot GIF cache reuse and bad-slot quarantine.
- [x] Transactional cached-GIF replacement and boot recovery.
- [x] Framebuffer ownership isolation during GIF staging.
- [x] Carousel-to-Carousel transition ownership without Clock flash-through.
- [x] Verified `03 80` reset that removes Carousel, alarms and programs/schedules without rebooting WLED.
- [x] Alarms and programs/schedules validated on hardware.
- [x] Clock style geometry and blinking HH:MM separator validation.
- [x] Public WLED custom-effect name shortened from `iDotMatrix Display` to `iDotMatrix`.
- [x] Explicit iDotMatrix ownership terminates an active WLED playlist and clears any already queued playlist preset so deferred preset application cannot steal the display back.

## 0.9 hardware line

Release 0.9 is intentionally reserved for the new-hardware phase. Items in this section are not 0.8.2 regressions and should not be back-ported without a specific reason.

- [x] bring up ESP32-S3 on Adafruit MatrixPortal and establish the PSRAM allocation policy from real measurements;
- [x] validate native physical 64x64 operation;
- [x] use WLED 0.17 native HUB75 as the physical output backend;
- [x] validate the PSRAM/direct GIF path on real hardware;
- [x] implement and hardware-validate the 32x64 / 256-byte glyph path used by the app's 64-pixel TEXT size;
- [ ] validate the corresponding downscale combinations on a physical 32x32 panel when hardware is available.

## Deferred hardening / maintenance

- extend `bootReset` text mapping for newer ESP-IDF reset-reason values while retaining the raw numeric code already exposed in `/json/info`;
- continue failure-atomic renderer reinitialization for rare OOM paths;
- widen explicit Preferences/NVS write-result checking where the current successful path is already hardware-qualified;
- consider a versioned/CRC-protected internal persistent-record format only together with a migration design;
- evaluate an optional secure BLE mode without breaking official-app compatibility mode;
- harden the long-offline app-time fallback against `millis()` wrap;
- evaluate migration of classic ESP32 to a common IDF5 WLED base only as a separate experiment, never as an assumption for the supported 0.8.2 line.

## Development rule

Protocol/function changes should continue to be introduced only after they are understood from captures or validated in the standalone ESP32 emulator. The official iDotMatrix app/original-device wire behaviour remains the compatibility reference.

- [ ] Consider reusing the consolidated Carousel/Preset transfer-indicator infrastructure for long standalone/gallery GIF uploads if a real need is observed.

## Closed investigations

- ESP32-C3 long-run OFF event: diagnostic firmware proved the transition was a real WLED global power-off but not initiated by iDotMatrix BLE, Alarm, Schedule, Carousel or adapter calls. Home Assistant was subsequently found to include the matrix in a house-light group, so global “lights off” commands were the external cause. The dedicated `0.8.2-diag.2` branch is not part of the 0.9 source line.
