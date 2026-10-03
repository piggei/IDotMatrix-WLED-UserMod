# iDotMatrix WLED Usermod 0.9.4-dev.10

**Release:** 0.9.4  
**Build:** 0.9.4-dev.10  
**Stable behavioral baseline:** 0.9.3  
**Qualified optimization baseline:** 0.9.4-dev.9  
**Primary target:** Waveshare ESP32-S3-RGB-Matrix / WLED 17.0.0-devV5

## Purpose

Build 0.9.4-dev.10 is the second PSRAM optimization step. It keeps the qualified dev.7-dev.9 direct AnimatedGIF path and adds bounded persistent reuse of whole-file GIF source images for durable Carousel slots on the Waveshare ESP32-S3 RGB Matrix target.

The optimization is deliberately below the Carousel state machine and above the unchanged AnimatedGIF callbacks. It does not predecode frames, change frame timing, alter the BLE protocol, or change Carousel dwell/state semantics.

## Baseline inherited from dev.9

The dev.9 source-stage refactor passed its real-device regression smoke on the Waveshare target:

```text
build=0.9.4-dev.9
gifStage=state:fs bytes:0 peak:173821 attempts:22 ok:22 fallback:0
```

The snapshot was taken while TEXT/Preset content was active, therefore `state:fs bytes:0` correctly showed that no GIF source was active at that instant. The reported refactor smoke therefore closed at **22/22 successful source stages with zero fallback**. The dev.8 PSRAM telemetry invariants remained valid.

The total PSRAM reported by ESP-IDF can vary with the complete linked WLED/Usermod firmware composition. Qualification comparisons must therefore use the same firmware composition rather than treating `psram=total` as a board-only constant. The source-cache policy continues to protect a 4 MiB free-PSRAM reserve before each new source allocation.

## Persistent source reuse

On `IDOT_WAVESHARE_S3_RGB_MATRIX` builds, durable stored GIFs are eligible for a bounded PSRAM reuse cache:

- **maximum resident source-cache bytes:** 1 MiB;
- **maximum admitted source size:** 512 KiB per cache entry;
- **metadata capacity:** 12 entries, matching the Carousel slot count;
- **replacement policy:** least-recently-used eviction when the byte budget or entry capacity requires space;
- **stage maximum:** unchanged at 2 MiB per GIF;
- **pre-allocation PSRAM reserve:** unchanged at 4 MiB;
- **copy chunk:** unchanged at 4096 bytes with `yield()` on ESP32.

A cache miss uses the already-qualified dev.9 staging path. After a successful stage, an eligible source can become resident without a second copy: ownership of the staged allocation is transferred into the reuse cache. A later replay of the same stored path reuses that PSRAM image and does not perform another whole-file LittleFS copy.

GIFs larger than the 512 KiB persistent-entry limit can still use the normal 2 MiB one-play PSRAM stage. App-upload/transient `/idot_play.gif` content is deliberately not admitted to the persistent cache and retains the dev.8/dev.9 lifetime.

## Correctness and invalidation

Persistent reuse is an optimization only; the filesystem remains authoritative.

- Before a reuse hit is accepted, the stored file must still exist and its current size must match the resident entry.
- Carousel bank reset/reconfiguration clears all resident stored-GIF source entries.
- A committed slot replacement invalidates that slot's resident source explicitly, including same-size replacements.
- If a replaced entry is still being consumed by AnimatedGIF, its buffer is marked invalid and retired only after `AnimatedGIF::close()`/source release, preserving callback lifetime safety.
- If cache admission is unavailable, playback continues with a transient PSRAM stage.
- If source staging itself cannot be performed, the unchanged LittleFS callback fallback remains available.

## Telemetry

The existing `gifStage=...` line remains available. In dev.10 its counters intentionally describe **real staging work**, not every GIF playback:

```text
gifStage=state:<psram|fs> bytes:<current> peak:<peak> attempts:<n> ok:<n> fallback:<n> max:<bytes> reserve:<bytes>
```

A cache hit does not increment `attempts` or `ok` because no source copy is performed.

Dev.10 adds:

```text
gifSourceCache=state:<off|idle|active> entries:<n> bytes:<n> hits:<n> misses:<n> stores:<n> evict:<n> invalid:<n> maxEntries:<n> maxBytes:<bytes> entryMax:<bytes>
```

`state:active` means the currently open AnimatedGIF source is backed by a resident cache entry. `entries`/`bytes` report resident PSRAM allocations, while the counters make cache effectiveness and churn measurable.

## Deliberately unchanged

- BLE protocol, FA02 framing and multipart transport.
- Carousel ordering, dwell timing, persistence and state semantics.
- AnimatedGIF decoder implementation and frame timing.
- Renderer/scaling behavior.
- Existing no-PSRAM filesystem frame-cache backend (`gifCache=...`).
- Alarm/Program, Preset, Clock and TEXT semantics.
- External Buzzer service bridge.
- Non-Waveshare targets: persistent source reuse remains disabled by default.

## Qualification gate

After host/package tests, flash dev.10 on the same Waveshare firmware composition used for the dev.9 comparison and run a repeated Carousel containing GIF slots.

Expected warm-cache behavior:

1. first traversal: `gifSourceCache misses` and `stores` increase while `gifStage attempts/ok` increase for actual source copies;
2. later traversal(s): `hits` increase and repeated cached slots do **not** increase `gifStage attempts`;
3. `fallback` remains zero for eligible files under normal conditions;
4. `entries`/`bytes` settle within the 12-entry / 1 MiB limits;
5. PSRAM snapshot invariants remain true: `minFree <= free`, `minLargest <= largest`, `peakUsed = total - minFree`;
6. current `largest` must recover after normal allocation churn; sustained degradation while free memory recovers is a stop condition;
7. replacing/reconfiguring Carousel content must invalidate old resident sources and show the new content, never stale cached bytes.

A long soak is not required for the first gate. A short warmup plus multiple full Carousel loops is sufficient to decide whether the reuse mechanism is operating correctly before considering prefetch or larger cache budgets.
