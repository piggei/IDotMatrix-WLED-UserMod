# iDotMatrix WLED Usermod 0.9.4-dev.15

**Release:** 0.9.4  
**Build:** 0.9.4-dev.15  
**Status:** MatrixPortal persistent-source-cache qualification build

**Stable behavioral baseline:** 0.9.3.

## Scope

Build 0.9.4-dev.15 advances only the Adafruit MatrixPortal ESP32-S3 PSRAM optimization path. Dev.14 transient whole-file staging passed its physical hardware gate with 19 staging attempts, 19 successes, zero fallback and a 173,821-byte largest active source. The current largest PSRAM block recovered to 1,933,312 bytes after staging activity.

Dev.15 therefore enables a bounded persistent source cache for durable Carousel GIF sources while deliberately keeping Carousel look-ahead prefetch disabled. Waveshare behavior is unchanged.

## MatrixPortal policy

- transient stage maximum: **256 KiB**;
- pre-allocation free-PSRAM reserve: **1 MiB**;
- persistent source-cache budget: **384 KiB**;
- maximum persistent entry: **256 KiB**;
- metadata capacity: **8 entries**;
- replacement policy: existing LRU implementation;
- Carousel prefetch: **disabled** for this build;
- transient/app `/idot_play.gif`: never persistent.

At preparation time the cache was conservatively expected to be smaller than the complete seven-GIF corpus. Physical qualification later measured the complete set at 249,267 bytes, so all seven sources fit inside the 384 KiB budget. The resulting hardware run reached `entries=7`, `hits=25`, `attempts=7`, `evict=0` and `fallback=0`; this corrected measurement is the basis for dev.16.

## Prefetch isolation

A new `IDOT_GIF_CAROUSEL_PREFETCH_ENABLED` compile-time policy gates only Carousel scheduling of speculative look-ahead. The underlying source-stage `prefetch()` implementation remains unchanged and host-tested. Waveshare defaults to enabled; MatrixPortal dev.15 explicitly sets the policy to zero. This keeps the cache-only qualification independent of speculative allocation behavior.

## Hardware qualification gate

After flashing the MatrixPortal profile and running multiple Carousel loops, verify:

1. `release=0.9.4` and `build=0.9.4-dev.15`;
2. `gifStage ... max:262144 reserve:1048576`;
3. `gifSourceCache ... maxEntries:8 maxBytes:393216 entryMax:262144`;
4. cold playback creates cache stores;
5. later loops increase cache hits without a corresponding new normal stage for every replay;
6. `gifPrefetch attempts:0`;
7. `gifStage fallback:0`;
8. PSRAM snapshot invariants remain valid;
9. current free/largest PSRAM recover after normal cache turnover and no playback regression is visible.

Dev.15 subsequently passed this cache-only hardware gate; dev.16 enables MatrixPortal look-ahead without changing cache or staging budgets.
