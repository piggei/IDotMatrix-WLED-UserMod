# iDotMatrix WLED Usermod 0.9.4-dev.6

**Release:** `0.9.4`  
**Build:** `0.9.4-dev.6`  
**Status:** development / measurement-only PSRAM-cache telemetry  
**Stable baseline:** `0.9.3`

## Purpose

0.9.4-dev.6 establishes a measurable memory baseline on the Waveshare ESP32-S3-RGB-Matrix before any PSRAM staging, GIF prefetch or Carousel preloading is introduced. It intentionally changes diagnostics only.


## Build profile

The Waveshare target continues to use `overrides/waveshare-s3-hub75.ini` with the local short environment `waveshare` on WLED 17.0.0-devV5. **Stable baseline:** 0.9.3.

## New telemetry

The iDotMatrix `/json/info` array now includes a richer PSRAM line on ESP32-S3 targets:

```text
psram=total:<bytes> free:<bytes> minFree:<bytes> peakUsed:<bytes> largest:<bytes> minLargest:<bytes>
```

`minFree` and `minLargest` are low-water marks sampled during runtime; `peakUsed` is derived from total PSRAM minus the lowest free-PSRAM sample.

GIF/cache state is also reported:

```text
gifCache=state:<direct|idle|building|playback> bytes:<n> frameBytes:<n> frames:<n> builds:<n> reuse:<n> waits:<n> lowHeapMin:<n>
```

On the Waveshare PSRAM direct-decoder path, `state:direct` and zero frame-cache bytes are expected unless a later build deliberately introduces a PSRAM media cache. The counters are still useful as a baseline and keep low-memory frame-cache targets observable.

## Behavior contract

Dev.6 does **not** change:

- allocator selection or PSRAM preference;
- GIF decoder/cache policy;
- Carousel dwell or preloading behavior;
- BLE framing or protocol semantics;
- rendering/scaling;
- Alarm/Program/Preset behavior;
- external WLED Buzzer Usermod integration.

## Waveshare status

The Waveshare ESP32-S3-RGB-Matrix 64x64 path is already hardware-verified for native 64x64 output, BLE connect/reconnect, Clock, TEXT, static image, GIF, Carousel, reboot/persistence, Alarm/Program, Matrix Auto Rotation and onboard-microphone AudioReactive. AudioReactive local input uses `SD 39`, `WS 38`, `SCK 43`, `MCLK 12`; UDP Sound Sync receive mode must be disabled for local processing.

Remaining qualification items are 16x16 -> 64x64 scaling, 32x32 -> 64x64 scaling and the sustained GIF/Carousel + AudioReactive soak. Dev.6 telemetry should be captured at the start and end of that soak.
