# iDotMatrix WLED Usermod 0.9.4-rc.2 pre-release audit response

**Release:** 0.9.4  
**Build:** 0.9.4-rc.2  
**Date:** 2026-10-03

## Background

An independent audit of 0.9.4-rc.1 found two reproducible runtime blockers in Automation/persistence. That evidence supersedes the original rc.1 pre-release statement that no runtime code-path defect had been found. rc.2 is intentionally limited to regression fixes and documentation/package corrections.

## Closed blockers

### Schedule commit while the next Program multipart is active

rc.1 could publish a Schedule generation after 900 ms from the last complete activity even when the next activity was still being assembled by `IDotMatrixProtocol`. The protocol now exposes the active Program multipart state as a read-only commit guard, and `IDotMatrixAutomation::updateSchedule()` does not publish while that transfer is active.

Regression coverage holds the second activity in multipart state beyond the quiet timeout, verifies that no partial generation is published, completes the second activity, then verifies that both activities survive the final quiet-period commit.

### Alarm replacement not atomic across LittleFS and NVS

rc.1 removed the old media backup before checking whether the new Alarm metadata had been persisted. rc.2 keeps rollback media until NVS confirmation, checks the return value of `putBytes()`, restores prior media/metadata after NVS failure, and converges to an empty slot if recovery itself fails.

Boot recovery now clears unrecoverable Alarm metadata/media mismatches instead of keeping a configured slot whose file does not match the persisted metadata. One-shot consumption is also regression-tested with injected NVS failure: the slot converges to empty instead of being allowed to resurrect after reboot.

## Additional low-risk hardening included

- Alarm and Schedule clock fields are range-validated before persistence.
- Schedule replacement staging is obtained before the new global flags are published.
- sanitizer coverage includes WLED adapter, CompactGif, 11-bit media and both external-buzzer bridge host harnesses.
- current Wiki/source documentation is updated to identify rc.2 and the rc.1 findings.
- confirmed rc.1 documentation/package defects are corrected: classic ESP32 baseline, stale Waveshare status, release navigation, target-role terminology and missing Wiki image assets.

## Intentionally deferred architecture work

The following findings remain documented but are not release blockers for rc.2 without new evidence: Schedule end-of-list/session redesign, ACK compatibility semantics, BLE init retry policy, renderer fail-open capability reporting, versioned raw-struct persistence, boot CRC verification, fully atomic Carousel bank replacement, Preset preflight/rollback hardening, low-memory RAW transactional fallback, alternative BLE authentication, and progressive Carousel/Preset item pipelining.

## Validation status

`run_host_tests.sh` passes after the rc.2 changes. The automation regression harness includes Alarm replacement/rollback NVS failure injection, one-shot-consumption NVS failure coverage, boot recovery, and multipart/quiet-period coverage. The complete sanitizer script exceeds the execution limit of this constrained environment, but the changed Automation/Protocol target pair was re-run independently with ASan/UBSan and passed. The script also now includes the previously omitted WLED-adapter, CompactGif, 11-bit media and external-buzzer bridge targets for external/full execution. A real PlatformIO/WLED compile matrix remains an external final gate because this package does not include the upstream WLED checkout/toolchain.

No PSRAM/media budget or qualified S3 playback behavior is changed by rc.2.
