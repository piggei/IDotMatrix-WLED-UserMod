# iDotMatrix WLED Usermod 0.9.0-dev.17

## Purpose

Targeted Alarm/Program compatibility fix following comparison with the working iDotMatrix ESP32 Emulator reference implementation.

## Fix

The automation clock source now follows the iDotMatrix protocol semantics more closely:

- after a valid app `01/80` time synchronization, Alarm and Program/Schedule use the app-provided local time;
- WLED `localTime`/NTP remains available as a fallback when the app has not supplied a time sync;
- this prevents a valid but differently configured WLED/NTP timezone from silently shifting Alarm execution.

This specifically addresses the S3/HUB75 case where WLED can have a valid system clock while the original iDotMatrix app supplies its own local clock for device automation.

## Regression coverage

A host test now reproduces the failure mode: WLED exposes a valid clock at one time, the app syncs a different local time, and a one-shot Alarm is verified to trigger according to the app time.

## Scope

No intended changes to rendering, resolution scaling, GIF, Carousel, transfer indicator, procedural effects, BLE framing, or HUB75 output.
