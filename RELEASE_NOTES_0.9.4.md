# iDotMatrix WLED Usermod 0.9.4

**Release:** 0.9.4  
**Build:** 0.9.4  
**Status:** stable release; promoted from hardware-qualified 0.9.4-rc.2 with no functional runtime changes

## Summary

0.9.4 completes the native-S3 HUB75 optimization and qualification cycle while preserving the established iDotMatrix BLE behavior. The release adds qualified PSRAM-backed AnimatedGIF source staging, a persistent source cache for durable Carousel GIFs, one-item look-ahead prefetch, native-S3 scaling cleanup, and the rc.2 Automation persistence hardening identified by independent audit.

Stable behavioral baseline: `0.9.3` for protocol and feature behavior that 0.9.4 intentionally leaves unchanged.

## Automation fixes closed in rc.2

- Schedule publication is blocked while a following Program multipart transfer is still active, preventing the 900 ms quiet period from committing a partial generation.
- Alarm media + NVS replacement is transactional: metadata persistence is checked, the previous media remains recoverable until NVS commit succeeds, rollback is verified, and unrecoverable rollback converges to an empty slot.
- Alarm boot recovery clears unrecoverable metadata/media mismatches.
- Alarm and Schedule clock fields are range-validated before persistence.
- Schedule replacement staging is acquired before replacement flags are published.

## Qualified Waveshare policy

For Waveshare ESP32-S3-RGB-Matrix / ESP32-S3-N32R16:

- transient GIF source stage maximum: **2 MiB**;
- pre-allocation PSRAM reserve: **4 MiB**;
- persistent Carousel GIF source cache: **1 MiB**;
- maximum persistent entry: **512 KiB**;
- metadata entries: **12** with LRU replacement;
- one-item Carousel look-ahead: enabled, approximately **250 ms** after the current item becomes visible;
- filesystem fallback remains authoritative on size/reserve/allocation/copy failure.

Waveshare hardware qualification includes native 64x64, logical 16x16 -> 64x64 and 32x32 -> 64x64 automatic scaling, GIF/Carousel, cache invalidation/recovery, BLE, reboot/persistence, Alarm/Program, Matrix Auto Rotation and combined-load operation.

## Qualified MatrixPortal policy

For Adafruit MatrixPortal ESP32-S3:

- transient GIF source stage maximum: **256 KiB**;
- pre-allocation PSRAM reserve: **1 MiB**;
- persistent Carousel GIF source cache: **384 KiB**;
- maximum persistent entry: **256 KiB**;
- metadata entries: **8**;
- one-item Carousel look-ahead: enabled, approximately **250 ms** after the current item becomes visible.

The seven-GIF qualification corpus occupied 249267 bytes and demonstrated steady-state source-cache hits, successful prefetch, LRU/invalidation behavior and valid PSRAM telemetry invariants without fallback. Runtime qualification remains observable through `gifSourceCache=...` and `gifPrefetch=...` telemetry.

## Final hardware gate

The final rc.2 Waveshare smoke passed with the release candidate under combined real-device load. After reboot, one Alarm and a two-activity Schedule reloaded correctly from persistence, and the persisted Alarm subsequently fired successfully. Carousel/GIF staging/cache/prefetch, Matrix Auto Rotation, AudioReactive and the external Buzzer service remained operational. Final `0.9.4` changes only release/build identity and documentation relative to rc.2.

## Compatibility

- WLED target for the native-S3 line: **17.0.0-devV5**.
- Classic ESP32 qualification baseline remains **WLED 16.0.1** where documented.
- BLE wire formats are unchanged from the preceding stable line.
- Optional sound remains delegated to the standalone WLED Buzzer Usermod through the weak C bridge; iDotMatrix contains no local buzzer hardware/timing backend.
- The compatibility BLE profile remains intentionally unauthenticated, matching the observed original-device/app exchange.

## Deferred work

The following remain post-0.9.4 work rather than release blockers: explicit Schedule end-of-list/session semantics, failure-ACK compatibility study, BLE init retry/overflow hardening, versioned Automation persistence, boot CRC verification, fully transactional Carousel bank replacement, Preset preflight/rollback hardening, authenticated BLE mode, and progressive Carousel/Preset item pipelining.
