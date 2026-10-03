# iDotMatrix WLED Usermod 0.9.4-rc.2

**Release:** `0.9.4`  
**Build:** `0.9.4-rc.2`  
**Date:** 2026-10-03

## Purpose

0.9.4-rc.2 is a regression-fix-only candidate produced after an independent audit of rc.1 found two reproducible Automation/persistence defects. It does not add media features, change BLE wire formats, or alter the qualified Waveshare/MatrixPortal PSRAM budgets.

## Runtime fixes

### Schedule multipart / quiet-commit coordination

Schedule generation publication still uses the existing 900 ms quiet period, but the commit is now explicitly blocked while the protocol still owns an active Program multipart transfer. This prevents activity N from being committed alone while activity N+1 is still arriving. The quiet period restarts when the next complete activity is accepted.

A host regression reproduces the rc.1 failure mode with the second activity held in multipart state beyond 900 ms and verifies that both activities survive the final commit.

### Alarm file + NVS transaction

Alarm replacement now keeps the previous media backup until the matching `Preferences::putBytes()` metadata write is confirmed. `saveAlarmMeta()` returns success/failure instead of discarding the NVS result.

If the new metadata write fails, rc.2 restores the previous media and re-asserts the previous metadata. If recovery itself cannot be made trustworthy, the slot converges to an explicit empty state rather than leaving mismatched NVS/file state.

Boot recovery now also clears a configured Alarm whose media cannot be recovered with the expected size. This matches the existing safe-convergence behavior of Schedule. One-shot Alarm consumption also treats an NVS failure as a persistence fault and converges the slot to empty so an enabled one-shot cannot reappear after reboot.

### Automation field validation and staging publication order

Alarm `hour/minute` and Schedule `start/end hour/minute` are rejected when outside the normal clock ranges. Schedule replacement staging is allocated before publishing the replacement flags, so staging OOM cannot persist a new enabled state without opening the generation.

## QA hardening

`run_host_sanitizers.sh` now includes the previously manual WLED-adapter, CompactGif, 11-bit media and external-buzzer bridge host targets in addition to the existing sanitizer set.


Stable behavioral baseline: `0.9.3`. The qualified S3 media telemetry remains available through `gifSourceCache` and `gifPrefetch`; rc.2 does not change their counters or budgets.

## Hardware/media policy

Unchanged from the qualified dev.16 / rc.1 baseline:

- Waveshare ESP32-S3-RGB-Matrix: 2 MiB transient GIF stage maximum, 4 MiB reserve, 1 MiB persistent source cache, 512 KiB per entry, 12 metadata entries, one-item / 250 ms look-ahead.
- MatrixPortal ESP32-S3: 256 KiB transient GIF stage maximum, 1 MiB reserve, 384 KiB persistent source cache, 256 KiB per entry, 8 metadata entries, one-item / 250 ms look-ahead.
- Automatic 16/32/64 logical-to-physical scaling remains unchanged.
- BLE protocol, Carousel order/dwell, Preset semantics, AnimatedGIF callbacks and external-buzzer bridge remain unchanged.

## Documentation and packaging corrections

The rc.1 audit follow-up also closes the confirmed documentation/package defects: the classic ESP32 historical baseline is consistently documented as WLED 16.0.1; stale Waveshare qualification text is removed; MatrixPortal is named consistently as the 0.9.x S3/HUB75 reference platform while Waveshare is the 0.9.4 optimization/qualification target; release navigation includes dev.14-dev.16; and the Wiki archive now carries the six local images referenced by the hardware pages. The rc.1 pre-release audit retained in source is explicitly marked superseded.

## Deferred items

The audit also identified broader hardening opportunities that are intentionally not folded into this regression candidate: explicit Schedule end-of-list/session semantics, fully atomic Carousel bank replacement, versioned Automation persistence schemas, alternative authenticated BLE mode, low-memory RAW transactional fallback, and progressive Carousel/Preset playback. These remain post-0.9.4 work unless a new release blocker is demonstrated.
