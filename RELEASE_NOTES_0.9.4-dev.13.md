# iDotMatrix WLED Usermod 0.9.4-dev.13

**Release:** 0.9.4  
**Build:** 0.9.4-dev.13  
**Stable behavioral baseline:** 0.9.3

## Purpose

Build 0.9.4-dev.13 is a narrow pre-release cleanup build. The Waveshare media path through dev.12 is already hardware-qualified, including PSRAM source staging, persistent GIF-source reuse, one-item look-ahead, cache invalidation/release and corrected `gifStage peak` telemetry. Dev.13 does not add another optimization.

The cleanup removes an obsolete user-facing ambiguity: native WLED 2D output scaling already handles 16x16/32x32/64x64 logical-to-physical mapping, so the historical `rescale` storage option is not needed on the dedicated S3 HUB75 targets.

## Changes

- Added compile-time `IDOT_LOW_MEMORY_RESCALE` capability control.
- `overrides/waveshare-s3-hub75.ini` and `overrides/matrixportal-s3-hub75.ini` set `IDOT_LOW_MEMORY_RESCALE=0`.
- On those profiles the `rescale` configuration key is not emitted, the checkbox is absent, and any stale stored value is forced off.
- Classic larger-profile builds keep the historical low-memory storage path by default. Where exposed, the visible label is now `Low-memory canvas downscale:` with an explicit RAM-saving explanation.
- Automatic final WLED 2D output scaling is unchanged.

## Hardware evidence carried into dev.13

Waveshare ESP32-S3-RGB-Matrix / one 64x64 HUB75 panel:

- dev.12 telemetry smoke: `gifStage=state:psram bytes:49799 peak:49799 attempts:1 ok:1 fallback:0`;
- dev.12 cache/prefetch smoke: `gifSourceCache entries:4 bytes:234104`, `gifPrefetch attempts:3 ok:3 cached:1 fail:0`;
- 16x16 logical -> 64x64 physical: PASS with Rescale disabled;
- 32x32 logical -> 64x64 physical: PASS with Rescale disabled;
- 64x64 native path: previously qualified.

## Unchanged

- BLE protocol/framing and device identity behavior;
- AnimatedGIF callbacks and frame timing;
- Waveshare 2 MiB transient stage / 4 MiB reserve policy;
- 1 MiB persistent source-cache / 512 KiB entry / 12-entry LRU policy;
- one-item Carousel look-ahead;
- LittleFS fallback behavior;
- Alarm, Program, Carousel, Preset and Device Reset semantics;
- external Buzzer service boundary.

## MatrixPortal note

The MatrixPortal S3 profile receives only the Rescale UI/capability cleanup in this build. Waveshare PSRAM staging/cache/prefetch limits are **not** enabled on MatrixPortal. Its 2 MiB PSRAM target requires a separate physical measurement baseline before any target-specific media policy is introduced.

## Qualification gate

A short hardware smoke is sufficient:

1. confirm `release=0.9.4` / `build=0.9.4-dev.13`;
2. confirm the Waveshare settings page has no Rescale checkbox;
3. confirm 16x16, 32x32 and 64x64 still scale/render correctly;
4. confirm GIF source cache/prefetch counters remain healthy.
