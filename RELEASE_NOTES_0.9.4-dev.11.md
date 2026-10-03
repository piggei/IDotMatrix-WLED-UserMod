# iDotMatrix WLED Usermod 0.9.4-dev.11

**Release:** 0.9.4  
**Build:** 0.9.4-dev.11  
**Stable behavioral baseline:** 0.9.3  
**Qualified optimization baseline:** 0.9.4-dev.10  
**Primary target:** Waveshare ESP32-S3-RGB-Matrix / WLED 17.0.0-devV5

## Purpose

Build 0.9.4-dev.11 is the third measured PSRAM optimization step. It keeps the dev.10 persistent whole-file GIF source cache and adds conservative one-item Carousel look-ahead prefetch for the Waveshare direct-AnimatedGIF path.

The goal is narrow: after the current Carousel item is visibly active, dev.11 warms only the **immediately following playable GIF source** into the existing bounded PSRAM cache. This moves the LittleFS-to-PSRAM copy away from the next item transition without changing Carousel order, dwell semantics, AnimatedGIF callbacks, frame timing or the cache budgets qualified in dev.10.

## Qualified dev.10 baseline

Real-device testing of dev.10 on the Waveshare 64x64 target demonstrated the intended steady state:

```text
gifStage=state:psram bytes:6386 peak:173821 attempts:7 ok:7 fallback:0
gifSourceCache=state:active entries:7 bytes:249267 hits:119 misses:7 stores:7 evict:0 invalid:0
```

Across the later 593-second observation window:

- cache hits increased by **117**;
- cache misses remained **7**;
- source-stage attempts remained **7**;
- evictions remained **0**;
- fallback remained **0**;
- PSRAM `free`, `minFree`, `largest` and `minLargest` remained unchanged byte-for-byte;
- the seven-GIF Carousel therefore ran at a measured **100% warm-cache source hit rate** after its compulsory first-load misses.

That hardware result closes the dev.10 source-reuse gate and is the baseline for dev.11.

## One-item look-ahead prefetch

Dev.11 adds a background optimization at the Carousel layer:

1. the current item must first become visible;
2. a 250 ms guard delay keeps prefetch work away from the item transition itself;
3. only the immediately following playable Carousel item is examined;
4. if that item is a GIF, its source is offered to the existing persistent PSRAM source cache;
5. if the source is already resident, no copy is performed;
6. if prefetch fails for any reason, playback behavior is unchanged and the normal dev.10 stage/cache/fallback path handles the GIF when its turn arrives.

The implementation does **not** scan or warm the whole Carousel bank. A following TEXT item stops look-ahead for the current dwell; when that TEXT becomes visible, the next item is considered in turn. This keeps memory residency and I/O aligned with actual playback order.

## Source-stage lifetime safety

`IDotMatrixGifSourceStage::prefetch()` does not call `release()` and never changes the active source pointer used by AnimatedGIF. It allocates and populates a separate cache entry while the current decoder source remains valid. The existing LRU policy still excludes the active entry from eviction.

Prefetch uses the same safeguards as normal source staging:

- cache enabled only on the Waveshare target by default;
- 1 MiB total persistent-cache budget;
- 512 KiB admission ceiling per persistent source;
- 12 metadata entries;
- 4 MiB free-PSRAM reserve guard before a new allocation;
- 4096-byte copy chunks with `yield()` on ESP32;
- filesystem source remains authoritative;
- committed Carousel replacement/reset invalidation remains unchanged.

Normal playback staging keeps its existing 2 MiB per-GIF maximum.

## Telemetry

The dev.10 lines remain unchanged:

```text
gifStage=state:<psram|fs> bytes:<current> peak:<peak> attempts:<n> ok:<n> fallback:<n> max:<bytes> reserve:<bytes>
gifSourceCache=state:<off|idle|active> entries:<n> bytes:<n> hits:<n> misses:<n> stores:<n> evict:<n> invalid:<n> maxEntries:<n> maxBytes:<bytes> entryMax:<bytes>
```

Dev.11 adds:

```text
gifPrefetch=attempts:<n> ok:<n> cached:<n> fail:<n> bytes:<n>
```

Semantics:

- `attempts`: look-ahead operations that required a new filesystem copy;
- `ok`: successful new prefetch copies admitted to the PSRAM source cache;
- `cached`: look-ahead requests already satisfied by a resident entry;
- `fail`: failed background prefetch attempts; these are **not** playback failures;
- `bytes`: cumulative source bytes copied by successful prefetch operations.

Playback `gifStage attempts/ok/fallback` remains reserved for real playback staging work. Prefetch does not inflate those counters.

## Deliberately unchanged

- BLE protocol, FA02 framing and multipart transport.
- Carousel order, dwell duration, persistence, failed-slot semantics and state machine.
- AnimatedGIF implementation, callbacks and frame timing.
- Renderer/scaling behavior.
- Dev.10 cache budgets and LRU replacement policy.
- No-PSRAM filesystem frame-cache backend (`gifCache=...`).
- Alarm/Program, Preset, Clock and TEXT behavior.
- External Buzzer service bridge.
- Non-Waveshare targets: source reuse/prefetch remain disabled by default.

## Qualification gate

After host/package tests, flash dev.11 using the same Waveshare firmware composition as the dev.10 comparison and run the same repeated seven-GIF Carousel.

Expected first-loop behavior:

1. the first GIF may require the normal playback stage;
2. once it is visible, `gifPrefetch attempts/ok` should begin increasing for the next GIFs;
3. later first-loop GIFs should increasingly arrive as `gifSourceCache` hits rather than new `gifStage attempts`;
4. after the working set is warm, `gifPrefetch cached` should increase while new prefetch copies stop;
5. `gifPrefetch fail` and `gifStage fallback` should remain zero under normal conditions;
6. cache entries/bytes must stay within the unchanged dev.10 limits;
7. PSRAM invariants must remain true and no sustained largest-block degradation may appear.

The key comparison is the **cold first traversal**. Dev.10 required seven playback stages for the tested seven-GIF Carousel. Dev.11 should reduce that number if one-item look-ahead completes before each following transition, while producing visually identical playback.
