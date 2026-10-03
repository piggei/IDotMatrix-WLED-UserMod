# iDotMatrix WLED Usermod 0.9.4-dev.12

**Release:** 0.9.4  
**Build:** 0.9.4-dev.12  
**Stable behavioral baseline:** 0.9.3  
**Primary optimization target:** Waveshare ESP32-S3-RGB-Matrix / WLED 17.0.0-devV5

## Scope

Build 0.9.4-dev.12 is intentionally narrow. It does **not** add another media optimization. It consolidates the real-device-qualified dev.11 one-item Carousel GIF look-ahead path and fixes one diagnostic semantic exposed by source-cache/prefetch playback.

Dev.11 proved that the existing bounded PSRAM cache can be warmed ahead of the next Carousel GIF without visible delay on the qualified Waveshare 64x64 target. Dev.12 therefore changes only the meaning of the existing `gifStage peak` counter so the emitted snapshot remains intuitive when the active AnimatedGIF source came from a persistent/prefetched cache hit rather than from a normal stage copy.

## Dev.11 real-hardware qualification carried forward

The reported dev.11 Waveshare run passed both cold-cache and lifecycle checks:

- cold sample: `gifStage attempts=1 ok=1 fallback=0` while three additional sources were prefetched successfully;
- `gifPrefetch attempts=3 ok=3 fail=0` at the first 21-second sample, with four resident sources / 234104 bytes already available;
- continued use remained visually responsive with no reported animation/dwell delay;
- later mixed Carousel/Preset activity reached `gifStage attempts=9 ok=9 fallback=0`;
- prefetch reached **12/12 successful copies**, `cached=19`, `fail=0`, `bytes=496706`;
- source-cache lifecycle reached `stores=21`, `invalid=21`, `entries=0`, `bytes=0`, demonstrating complete release after content invalidation;
- current largest PSRAM block recovered to **14155776 bytes** after the cache was emptied while the historical low-water remained **13893632 bytes**;
- the final reported PSRAM snapshot remained self-consistent: `total=14352384`, `free=14240004`, `minFree=13910912`, `peakUsed=441472`;
- Matrix Auto Rotation and AudioReactive remained operational during the reported workload.

This evidence closes the dev.11 one-item look-ahead gate for the qualified Waveshare configuration.

## Telemetry correction

In dev.11, `gifStage bytes` already described the **currently active PSRAM source image**, regardless of whether that image came from a normal filesystem-to-PSRAM stage or from a cache/prefetch hit. `gifStage peak`, however, was updated only after a normal stage copy.

That allowed a valid snapshot such as:

```text
gifStage=state:psram bytes:49799 peak:1981 attempts:1 ok:1 fallback:0 ...
```

The media path was correct, but the diagnostic relationship was misleading because the active cached source was larger than every source copied through the normal stage path so far.

Dev.12 updates `peakBytes_` whenever a cached source becomes the active AnimatedGIF source. `peak` now means:

> the largest whole-file PSRAM source image that has been active for AnimatedGIF since boot, independent of whether it arrived through normal staging or persistent/prefetched reuse.

The runtime invariant is therefore restored:

```text
gifStage peak >= gifStage bytes
```

This does **not** change the meaning of `attempts` or `ok`: cache hits and prefetch still do not count as normal playback stage copies.

## Existing telemetry remains

```text
gifStage=state:<psram|fs> bytes:<current> peak:<active-peak> attempts:<n> ok:<n> fallback:<n> max:<bytes> reserve:<bytes>
gifSourceCache=state:<off|idle|active> entries:<n> bytes:<n> hits:<n> misses:<n> stores:<n> evict:<n> invalid:<n> maxEntries:<n> maxBytes:<bytes> entryMax:<bytes>
gifPrefetch=attempts:<n> ok:<n> cached:<n> fail:<n> bytes:<n>
```

The PSRAM snapshot invariants introduced by dev.8 remain unchanged:

```text
minFree <= free
minLargest <= largest
peakUsed = total - minFree
```

## Waveshare-only optimization policy

The whole-file stage, persistent source cache and one-item look-ahead remain enabled by default only when `IDOT_WAVESHARE_S3_RGB_MATRIX` is compiled. Current Waveshare defaults remain unchanged:

- normal stage maximum: 2 MiB;
- free-PSRAM reserve before a new source allocation: 4 MiB;
- persistent cache budget: 1 MiB;
- maximum admitted persistent source: 512 KiB;
- metadata capacity: 12 entries;
- replacement: LRU;
- copy chunk: 4096 bytes with scheduler yield.

Non-Waveshare targets keep the pre-dev.7 behavior unless a future target-specific qualification explicitly enables another policy.

## MatrixPortal evaluation note

Adafruit MatrixPortal ESP32-S3 remains a qualified iDotMatrix 64x64 direct-AnimatedGIF/PSRAM target, but it has only 2 MiB PSRAM rather than the Waveshare board's 16 MiB. The Waveshare numerical policy must therefore **not** be copied unchanged: a 4 MiB reserve alone would permanently disable staging.

Dev.12 does not enable source staging/cache/prefetch on MatrixPortal. The existing S3 `/json/info` PSRAM telemetry is sufficient to collect a physical baseline first. A future MatrixPortal experiment should be target-specific and measurement-driven, with smaller stage/cache/reserve limits chosen only after observing `free`, `minFree`, `largest` and `minLargest` under BLE + HUB75 + AnimatedGIF + Carousel load.

## Regression coverage

Host regression now explicitly verifies the diagnostic case that exposed the dev.11 issue:

1. prefetch a durable GIF into the persistent PSRAM cache without making it active;
2. confirm `peakBytes == 0` before playback;
3. activate that GIF through a cache hit;
4. confirm `peakBytes == current bytes` without incrementing normal stage `attempts/ok`.

No BLE protocol, AnimatedGIF callback/frame timing, Carousel order/dwell, source-cache limits, prefetch policy, renderer/scaling, automation or external-buzzer behavior changes in dev.12.

## Hardware smoke required

A long soak is not required. On the qualified Waveshare profile:

1. verify `release=0.9.4` and `build=0.9.4-dev.12`;
2. run a cold Carousel until at least one prefetched source larger than the first normally staged source becomes active;
3. verify `gifStage peak >= gifStage bytes` in every observed snapshot;
4. verify `gifPrefetch fail=0` and `gifStage fallback=0` under the normal test set;
5. verify the dev.8 PSRAM invariants remain true;
6. confirm no visible playback regression.

If these checks pass, dev.12 becomes the consolidated Waveshare PSRAM/media baseline for subsequent work.
