# iDotMatrix WLED Usermod 0.9.4-dev.14

**Release:** 0.9.4  
**Build:** 0.9.4-dev.14  
**Stable behavioral baseline:** 0.9.3

## Purpose

Build 0.9.4-dev.14 starts the MatrixPortal-specific PSRAM optimization line after the Waveshare path was qualified through dev.12 and cleaned up in dev.13. Real hardware on Adafruit MatrixPortal ESP32-S3 with one native 64x64 HUB75 panel established a 2 MiB PSRAM baseline under active Carousel/GIF load, so this build enables only the first conservative optimization: transient whole-file GIF source staging.

No persistent source cache or Carousel look-ahead is enabled on MatrixPortal in this build.

## MatrixPortal baseline used for this policy

Reported real-device state after one Carousel pass on `0.9.4-dev.13`:

```text
psram=total:2097152 free:1940364 minFree:1940364 peakUsed:156788 largest:1933312 minLargest:1900544
gifDecoder=animatedgif12/psram
gifStage=state:fs bytes:0 peak:0 attempts:0 ok:0 fallback:0 max:0 reserve:4194304
gifSourceCache=state:off entries:0 bytes:0
gifPrefetch=attempts:0 ok:0 cached:0 fail:0 bytes:0
```

The target therefore retained about 1.85 MiB free PSRAM and about 1.84 MiB in the current largest contiguous block while running a real 64x64 Carousel/GIF workload.

## Changes

For `overrides/matrixportal-s3-hub75.ini` only:

- enables transient whole-file GIF source staging with `IDOT_GIF_PSRAM_STAGE_MAX=262144` (256 KiB);
- changes the staging reserve to `IDOT_GIF_PSRAM_STAGE_RESERVE=1048576` (1 MiB);
- keeps persistent source cache disabled;
- keeps one-item Carousel prefetch disabled;
- keeps automatic 16x16/32x32/64x64 output scaling and the dev.13 Rescale cleanup unchanged.

The source-stage implementation itself is unchanged from the Waveshare-qualified code path. AnimatedGIF still receives the same read/seek callbacks; when staging is not eligible or cannot preserve the target reserve, playback automatically falls back to the original LittleFS path.

## Waveshare policy unchanged

The Waveshare ESP32-S3-RGB-Matrix keeps its already-qualified policy unchanged:

```text
stage maximum       2 MiB
pre-allocation reserve 4 MiB
persistent cache    1 MiB
per-entry admission 512 KiB
metadata entries    12
Carousel look-ahead enabled
```

## MatrixPortal qualification gate

Use a cold boot and a Carousel containing several GIFs, then verify:

1. `/json/info` reports `release=0.9.4` / `build=0.9.4-dev.14`;
2. `gifStage max:262144 reserve:1048576`;
3. eligible GIFs increment `attempts` and `ok`;
4. `fallback` remains zero for the current test corpus if every GIF is below 256 KiB and the reserve guard passes;
5. `gifSourceCache` remains `state:off` and `gifPrefetch attempts` remains zero;
6. PSRAM invariants remain valid: `minFree <= free`, `minLargest <= largest`, `peakUsed = total - minFree`;
7. current `largest` recovers after GIF changes rather than declining cumulatively.

Do not enable persistent MatrixPortal caching or prefetch until this staging-only gate has passed on physical hardware.
