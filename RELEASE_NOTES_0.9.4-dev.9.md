# iDotMatrix WLED Usermod 0.9.4-dev.9

**Release:** 0.9.4  
**Build:** 0.9.4-dev.9  
**Stable behavioral baseline:** 0.9.3  
**Primary target:** Waveshare ESP32-S3-RGB-Matrix / WLED 17.0.0-devV5

## Purpose

Build 0.9.4-dev.9 is a refactor-only preparation step for the next PSRAM optimization. It starts from the hardware-qualified dev.8 source-staging baseline and deliberately does not add persistent caching, prefetch or new media behavior.

## Dev.8 qualification inherited by this build

The reported Waveshare Carousel/GIF soak reached about 22 minutes of uptime with:

```text
gifStage=state:psram bytes:1981 peak:173821 attempts:266 ok:266 fallback:0
psram=total:15269888 free:15116612 minFree:14942920 peakUsed:326968 largest:14942208 minLargest:14680064
```

The run preserved all dev.8 telemetry invariants, produced 266/266 successful stages with zero fallback, and showed recovery of the current largest contiguous PSRAM block to 14,942,208 bytes after a temporary 256 KiB low-water excursion. No persistent staging-related fragmentation or leak was observed in the reported test window.

## Changes

- Added `IDotMatrixGifSourceStage.h/.cpp`.
- Moved ownership of the active whole-file staged GIF buffer into `IDotMatrixGifSourceStage`.
- Moved the existing 2 MiB maximum, 4 MiB pre-allocation reserve, 4096-byte copy loop, scheduler yield, success/fallback counters and release logic into that component.
- `IDotMatrixMedia` continues to expose the same `gifStage=...` telemetry and the same public diagnostic accessors.
- AnimatedGIF open/read/seek/close callback semantics are unchanged.
- The staged source still outlives `AnimatedGIF::close()` and is released immediately afterward.
- Other targets still default `IDOT_GIF_PSRAM_STAGE_MAX` to zero.

## Deliberately unchanged

- BLE protocol and FA02 framing.
- Carousel state/dwell behavior.
- GIF decoder and frame timing.
- Filesystem fallback behavior.
- Existing no-PSRAM frame-cache path.
- Rendering/scaling, automation, persistence and external-buzzer semantics.
- `/json/info` field names and meanings.

## Qualification

Host regression and package checks must pass before packaging. Because dev.9 moves code on the qualified staging path, a short real-device Carousel/GIF smoke is still required before persistent source reuse is introduced. The smoke should verify build identity, repeated `gifStage` success, telemetry invariants, stop/reopen ownership, unchanged playback timing and concurrent AudioReactive/MAR operation.

The 16x16 -> 64x64 and 32x32 -> 64x64 Waveshare scaling checks remain pending and are independent of this refactor.
