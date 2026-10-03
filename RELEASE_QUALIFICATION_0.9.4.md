# iDotMatrix WLED Usermod 0.9.4 release qualification

**Release:** 0.9.4  
**Build:** 0.9.4  
**Promotion baseline:** 0.9.4-rc.2  
**Date:** 2026-10-03

## Promotion decision

0.9.4 is promoted from rc.2 with no functional runtime changes. rc.2 was created after an independent audit of rc.1 reproduced two Automation/persistence blockers. Both were fixed with targeted regression coverage before the final hardware gate.

## Closed rc.1 blockers

- Schedule quiet-period publication is inhibited while a following Program multipart is active.
- Alarm media and NVS metadata replacement are transactional, including checked NVS writes, rollback and explicit empty-state convergence when recovery cannot be trusted.

## Automated validation

- `run_host_tests.sh`: PASS.
- PlatformIO profile static checks: PASS.
- AnimatedGIF profile patch checks: PASS.
- External Buzzer service integration contract checks: PASS.
- Release-package consistency checks: PASS.
- rc.2 changed Automation/Protocol sanitizer targets were previously re-run independently with ASan/UBSan: PASS. Final changes do not alter those runtime paths.

## Final Waveshare hardware gate

The final rc.2 candidate was exercised on Waveshare ESP32-S3-RGB-Matrix with WLED 17.0.0-devV5. Under combined load, Carousel/GIF staging, persistent source cache, one-item prefetch, cache invalidation, Matrix Auto Rotation, AudioReactive and the external Buzzer service remained operational.

After reboot:

- one configured Alarm reloaded correctly;
- an enabled Schedule reloaded with two activities;
- Carousel persistent storage reloaded;
- the persisted Alarm subsequently fired successfully.

This closes the normal write -> reboot -> reload -> execution hardware path for the Automation fixes. Failure-injection behavior remains covered by the host regression tests.

## Native-S3 media qualification retained

Final 0.9.4 does not change the qualified PSRAM policies from dev.16/rc.2. Waveshare retains the 2 MiB transient stage / 4 MiB reserve / 1 MiB cache policy. MatrixPortal retains the 256 KiB transient stage / 1 MiB reserve / 384 KiB cache policy. Both keep one-item Carousel look-ahead enabled with the target-specific limits documented in the release notes.

## External build note

A complete upstream WLED/PlatformIO compile matrix requires the corresponding WLED checkouts and toolchains and is therefore an external reproducibility check. The final build identity change does not modify runtime source paths relative to the hardware-tested rc.2 candidate.
