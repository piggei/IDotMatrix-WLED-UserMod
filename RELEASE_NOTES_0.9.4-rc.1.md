# iDotMatrix WLED Usermod 0.9.4-rc.1

**Release:** 0.9.4  
**Build:** 0.9.4-rc.1  
**Status:** first release candidate

**Stable behavioral baseline:** 0.9.3.

## Scope

0.9.4-rc.1 promotes the hardware-qualified 0.9.4-dev.16 runtime to the first release candidate. No new runtime feature is intentionally introduced in this promotion. The change set is limited to version identity, release documentation, qualification consolidation, stale-document cleanup and release-package checks.

## Qualified native S3 targets

### Waveshare ESP32-S3-RGB-Matrix

- WLED target: 17.0.0-devV5;
- native 64x64 HUB75 plus logical 16x16 and 32x32 automatic upscaling: PASS;
- transient whole-file GIF staging: 2 MiB maximum / 4 MiB reserve;
- persistent source cache: 1 MiB total / 512 KiB per entry / 12 metadata entries / LRU;
- one-item Carousel look-ahead after 250 ms: PASS;
- observed staging, cache reuse, invalidation/recovery and PSRAM telemetry gates: PASS;
- `gifStage fallback:0` and no persistent largest-block loss in the reported qualification runs.

### Adafruit MatrixPortal ESP32-S3

- WLED target: 17.0.0-devV5, pinned baseline `06ae26db67107cb3f6a3d107a92340035991a063`;
- native 64x64 HUB75: PASS;
- transient whole-file GIF staging: 256 KiB maximum / 1 MiB reserve;
- persistent source cache: 384 KiB total / 256 KiB per entry / 8 metadata entries / LRU;
- one-item Carousel look-ahead after 250 ms: PASS.

The cold dev.16 Carousel showed only one normal stage and six successful prefetch copies before the seven-GIF set became resident:

```text
gifStage=state:psram bytes:173821 peak:173821 attempts:1 ok:1 fallback:0 max:262144 reserve:1048576
gifSourceCache=state:active entries:7 bytes:249267 hits:24 misses:1 stores:7 evict:0 invalid:0 maxEntries:8 maxBytes:393216 entryMax:262144
gifPrefetch=attempts:6 ok:6 cached:19 fail:0 bytes:247286
```

A later mixed Carousel/Preset soak exercised lifecycle paths rather than only the steady resident-cache case:

```text
gifStage=state:psram bytes:45749 peak:173821 attempts:11 ok:11 fallback:0 max:262144 reserve:1048576
gifSourceCache=state:idle entries:8 bytes:82180 hits:69 misses:7 stores:15 evict:5 invalid:2 maxEntries:8 maxBytes:393216 entryMax:262144
gifPrefetch=attempts:8 ok:8 cached:63 fail:0 bytes:305588
psram=total:2097152 free:1827396 minFree:1658360 peakUsed:438792 largest:1671168 minLargest:1638400
```

The PSRAM invariants remained valid (`minFree <= free`, `minLargest <= largest`, `peakUsed = total - minFree`), prefetch failures remained zero, and no GIF fallback occurred.

## UI cleanup retained from dev.13

Native S3 HUB75 profiles do not expose the obsolete `Scale the logical profile to the selected WLED 2D segment` control. Output scaling is automatic. The historical storage optimization remains available only on selected classic low-memory profiles and is labelled `Low-memory canvas downscale`.

## Deferred item-pipeline work

Progressive Carousel/Preset playback is explicitly deferred beyond 0.9.4. The intended design is **early display, late atomic commit**: the first complete/validated item may become visible while later items continue transferring, but persistent state is promoted only after the entire transfer validates. Partial media objects are never rendered.

## Release-candidate rule

After rc.1, only regression fixes, documentation/package corrections and build-profile corrections belong in the 0.9.4 candidate cycle. New features and larger memory budgets are deferred.
