# iDotMatrix WLED Usermod 0.9.4-dev.16

**Release:** 0.9.4  
**Build:** 0.9.4-dev.16  
**Status:** MatrixPortal one-item GIF look-ahead qualification build

**Stable behavioral baseline:** 0.9.3.

## Scope

Build 0.9.4-dev.16 advances only the Adafruit MatrixPortal ESP32-S3 PSRAM optimization path. Dev.15 persistent source cache qualification passed its physical hardware gate: all seven Carousel GIF sources were cached (`entries=7`, `bytes=249267`), normal staging stopped at seven cold loads while cache hits increased to 25, and no fallback or eviction occurred. The measured PSRAM snapshot remained valid with `free=1705280`, `minFree=1705280`, `largest=1671168` and `peakUsed=391872` bytes.

Dev.16 therefore enables the already-qualified one-item Carousel look-ahead scheduler on MatrixPortal without changing any stage/cache budget. Waveshare behavior remains unchanged.

## MatrixPortal policy

- transient stage maximum: **256 KiB**;
- pre-allocation free-PSRAM reserve: **1 MiB**;
- persistent source-cache budget: **384 KiB**;
- maximum persistent entry: **256 KiB**;
- metadata capacity: **8 entries**;
- replacement policy: existing LRU implementation;
- Carousel look-ahead: **enabled**, one immediately following playable GIF only;
- look-ahead delay: existing **250 ms** after the current item becomes visible;
- transient/app `/idot_play.gif`: never persistent.

No decoded-frame cache is introduced and no cache budget is increased.

## Dev.15 hardware evidence

The physical MatrixPortal cache-only test reported:

```text
gifStage=state:psram bytes:173821 peak:173821 attempts:7 ok:7 fallback:0 max:262144 reserve:1048576
gifSourceCache=state:active entries:7 bytes:249267 hits:25 misses:7 stores:7 evict:0 invalid:0 maxEntries:8 maxBytes:393216 entryMax:262144
gifPrefetch=attempts:0 ok:0 cached:0 fail:0 bytes:0
psram=total:2097152 free:1705280 minFree:1705280 peakUsed:391872 largest:1671168 minLargest:1671168
```

The earlier planning assumption that the seven-GIF corpus would exceed 384 KiB was disproved by hardware measurement: the complete source set occupies only 249,267 bytes and therefore fits inside the configured cache. No LRU eviction was expected or observed for this corpus.

## Hardware qualification gate

After flashing the MatrixPortal profile and starting a cold Carousel, verify:

1. `/json/info` reports `release=0.9.4` and `build=0.9.4-dev.16`;
2. `gifStage ... max:262144 reserve:1048576`;
3. `gifSourceCache ... maxEntries:8 maxBytes:393216 entryMax:262144`;
4. `gifPrefetch attempts` becomes non-zero during the first Carousel pass;
5. successful look-ahead increments `ok` and/or `cached`, while `fail` remains zero;
6. the first pass needs fewer normal `gifStage attempts` than the number of GIFs when prefetch warms later sources before playback;
7. after warm-up, cache hits increase while normal stage attempts stop for resident sources;
8. `gifStage fallback:0`;
9. PSRAM snapshot invariants remain valid: `minFree <= free`, `minLargest <= largest`, `peakUsed = total - minFree`;
10. Carousel timing and visible playback remain smooth with no regression.

Because the complete seven-GIF corpus fits inside the 384 KiB cache, `evict:0` is expected for this specific test set and is not a failure.

## Physical qualification result

The MatrixPortal dev.16 gate passed on hardware. Cold Carousel evidence showed one normal stage, six successful prefetch copies, seven resident GIFs / 249267 bytes, `gifPrefetch fail:0` and `gifStage fallback:0`. A later mixed Carousel/Preset soak exercised LRU eviction and invalidation (`evict:5`, `invalid:2`) while preserving zero fallback/fail and valid PSRAM invariants. Dev.16 is therefore the runtime baseline promoted to 0.9.4-rc.1.
