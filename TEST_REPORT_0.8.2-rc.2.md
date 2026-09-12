# Test Report — iDotMatrix WLED Usermod 0.8.2-rc.2

**Release:** 0.8.2  
**Build:** 0.8.2-rc.2  
**Date:** 2026-09-12

## Scope

This report covers the 0.8.2-rc.2 source package. The release candidate consolidates the protocol-convergence work developed after 0.8.1 while keeping the same supported 16x16 hardware baseline. New-hardware work is deliberately deferred to 0.9.

## Automated host regression

The release package is checked with:

```text
./run_host_tests.sh
./run_host_sanitizers.sh
```

The host suite covers the existing protocol, renderer, media, automation, Carousel, AudioReactive-source and package-contract regressions. The sanitizer pass exercises the supported host tests with AddressSanitizer and UndefinedBehaviorSanitizer.

Final RC2 package result:

```text
AnimatedGIF profile patch tests passed.
PlatformIO profile tests passed.
Release package checks passed.
All iDotMatrix host tests passed.
ASan/UBSan host tests passed.
```

## Reset regression added for RC2

The protocol test sends the confirmed reset frame:

```text
04 00 03 80
```

and verifies the normal command ACK:

```text
05 00 03 80 01
```

The reset regression verifies that all three reset targets are invoked: display/runtime content, persistent Carousel state and persistent automation state.

The automation regression additionally verifies that reset:

- removes alarm metadata;
- removes schedule/program metadata and associated media;
- clears schedule global flags;
- leaves no automation configuration after a simulated reboot;
- retains the last valid application time synchronization rather than treating protocol reset as a power cycle.

The Carousel implementation removes slot assets, replacement backups, receive staging and manifest files, then persists an explicit empty manifest so stale Carousel metadata cannot reappear after reboot.

## Hardware evidence carried into RC2

Physical ESP32-C3 16x16 testing performed during the development line established the following behavior used by RC2:

- long TEXT traverses all pages correctly on the 16x16 matrix;
- mixed GIF/TEXT Device Assets can be persisted and played as a Carousel;
- stored Carousel playback continues as standalone device content and does not require a live phone/BLE connection;
- selecting WLED `iDotMatrix Display` starts a stored Carousel and the no-Carousel policy falls back to Clock;
- WLED boot behavior works when the boot preset contains a valid `iDotMatrix Display` selection;
- AudioReactive can run as an optional local source without replacing the Phone/BLE compatibility path;
- Schedule multi-event handling and buzzer event changes remain functional;
- protocol reset clears Carousel, alarms and programs without rebooting WLED.

During qualification, an apparent `iDotMatrix Display` boot-selection failure was traced to an incorrectly/stale saved WLED preset rather than to Carousel playback or BLE ownership. Recreating the preset restored normal boot behavior. Direct JSON effect selection and Web UI `setFX(...)` testing both confirmed the WLED effect path.

## Platform/build status

The RC2 source package retains the supported build profiles and pinned dependencies documented in `BUILD_PROFILES.md`.

A full WLED PlatformIO firmware compilation is not performed by the artifact-generation environment used for this package. The distributed profile/static checks and host regression suite pass; final firmware compilation and flash qualification remain part of the normal hardware release workflow.

## Result

**0.8.2-rc.2 is suitable for release-candidate hardware validation.**

Remaining planned work requiring ESP32-S3/PSRAM, larger physical matrices or HUB75 hardware is intentionally deferred to Release 0.9 and is not an RC2 blocker.
