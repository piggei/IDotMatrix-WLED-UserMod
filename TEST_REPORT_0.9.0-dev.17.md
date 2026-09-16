# Test Report - 0.9.0-dev.17

## Scope

Targeted Alarm/Program time-authority correction derived from comparison with the working iDotMatrix ESP32 Emulator reference implementation.

## Host regression

Command executed:

```sh
./run_host_tests.sh
```

Result: **PASS**.

The full host suite passed, including protocol, renderer, media, Carousel, automation, PlatformIO-profile and release-package checks.

## New regression

`testAppTimeSyncOverridesValidWledClockForAlarm()` reproduces the S3 failure mode:

- WLED exposes a valid local clock;
- the iDotMatrix app synchronizes a different local time;
- a one-shot Alarm is configured for the app time;
- the automation loop must trigger the Alarm using the app-synchronized time;
- the one-shot enable bit must then be consumed.

This prevents WLED/NTP timezone configuration from silently shifting iDotMatrix Alarm/Program execution after an app synchronization.

## Hardware validation required

A physical MatrixPortal S3/HUB75 smoke test remains required. Configure an Alarm 1-2 minutes ahead with the official app and verify trigger, duration, buzzer/media and post-Alarm restore.
