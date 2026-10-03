# iDotMatrix WLED Usermod 0.9.4-rc.1 pre-release audit

> **Superseded:** a subsequent independent audit reproduced two Automation/persistence runtime defects in rc.1. The statement below that no runtime code-path defect was found is historical and must not be used as the current release assessment. See `PRE_RELEASE_AUDIT_0.9.4-rc.2.md`.

**Release:** 0.9.4  
**Build:** 0.9.4-rc.1  
**Audit date:** 2026-10-03

## Result

The 0.9.4 development line is functionally frozen and the hardware-qualified 0.9.4-dev.16 runtime is promoted to rc.1 without an intentional runtime feature change. The audit found documentation/version drift but no new runtime defect requiring a code-path change.

## Corrections made during the audit

- updated current source/package identity from `0.9.4-dev.16` to `0.9.4-rc.1` while keeping public `release=0.9.4`;
- corrected `PROTOCOL.md`, which still identified the current implementation as dev.8;
- corrected `BUILD_PROFILES.md`, which still identified the current package as dev.15;
- corrected current MatrixPortal `/json/info` examples in `README.md` and `BUILD_PROFILES.md` that still showed `release=0.9.1 / build=0.9.1`;
- corrected the Waveshare override header that still identified dev.14;
- corrected Wiki hardware/status text that still described the dev.12 or dev.16 qualification gates as pending/in progress;
- consolidated the physical MatrixPortal dev.16 cold-prefetch and mixed Carousel/Preset soak evidence;
- moved progressive Carousel/Preset item pipelining to the post-0.9.4 TODO with the design rule **early display, late atomic commit**;
- retained the native-S3 Rescale cleanup: Waveshare and MatrixPortal omit the obsolete output-scaling control while selected classic low-memory profiles retain the storage-downscale mechanism.

## Hardware qualification carried into rc.1

### Waveshare ESP32-S3-RGB-Matrix

Qualified on WLED 17.0.0-devV5 with one native 64x64 HUB75 panel. The release line has hardware evidence for native 64x64 operation, logical 16x16 and 32x32 automatic upscaling, BLE/app operation, TEXT, static image, GIF, Carousel, persistence, Alarm/Program, Matrix Auto Rotation, PSRAM telemetry, transient source staging, persistent source reuse, one-item look-ahead and cache invalidation/recovery.

The qualified media policy remains 2 MiB stage maximum, 4 MiB pre-allocation PSRAM reserve, 1 MiB persistent source-cache budget, 512 KiB per-entry maximum and 12 metadata entries with LRU replacement.

### Adafruit MatrixPortal ESP32-S3

Qualified on WLED 17.0.0-devV5 pinned to `06ae26db67107cb3f6a3d107a92340035991a063` with one native 64x64 HUB75 panel and 2 MiB PSRAM.

The qualified policy is 256 KiB stage maximum, 1 MiB pre-allocation reserve, 384 KiB persistent source cache, 256 KiB per-entry maximum, 8 metadata entries and one-item / 250 ms look-ahead.

Cold dev.16 evidence:

```text
gifStage=state:psram bytes:173821 peak:173821 attempts:1 ok:1 fallback:0 max:262144 reserve:1048576
gifSourceCache=state:active entries:7 bytes:249267 hits:24 misses:1 stores:7 evict:0 invalid:0 maxEntries:8 maxBytes:393216 entryMax:262144
gifPrefetch=attempts:6 ok:6 cached:19 fail:0 bytes:247286
```

Mixed Carousel/Preset lifecycle soak:

```text
gifStage=state:psram bytes:45749 peak:173821 attempts:11 ok:11 fallback:0 max:262144 reserve:1048576
gifSourceCache=state:idle entries:8 bytes:82180 hits:69 misses:7 stores:15 evict:5 invalid:2 maxEntries:8 maxBytes:393216 entryMax:262144
gifPrefetch=attempts:8 ok:8 cached:63 fail:0 bytes:305588
psram=total:2097152 free:1827396 minFree:1658360 peakUsed:438792 largest:1671168 minLargest:1638400
```

The snapshot invariants remain valid and no GIF fallback or prefetch failure was observed.

## Automated validation

`./run_host_tests.sh` passes completely, including protocol/media regressions, build-profile normalization, AnimatedGIF profile patching and release-package checks.

The complete `run_host_sanitizers.sh` invocation exceeds the execution timeout of the audit environment. Targeted ASan/UBSan runs for the GIF source-stage/cache and Carousel/prefetch components pass.

A full real PlatformIO/WLED compile matrix is not executed inside this audit environment because the WLED source tree and PlatformIO executable are not present. The shipped override/profile definitions are nevertheless covered by the repository's static PlatformIO profile regression tests. The release-candidate hardware smoke/compile on the maintainer's WLED checkout remains the final external confidence check before stable promotion.

## Release-candidate policy

No new feature or memory-budget increase should be added to 0.9.4 after rc.1. Only regression fixes, build/profile corrections, packaging corrections and documentation fixes should enter the candidate cycle.
