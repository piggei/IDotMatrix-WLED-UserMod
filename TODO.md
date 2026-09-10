# Roadmap after Release 0.8.1

## 0.8.1 audit-remediation status

Public release value: **0.8.1**  
Released internal build: **0.8.1-audit-fix1**

Completed in the released corrective build:

- [x] AUDIT-001 GIF promotion failure/recovery fix;
- [x] AUDIT-002 transactional schedule/program replacement;
- [x] boot-time stale/corrupt schedule metadata repair;
- [x] GIF promotion failure-injection tests;
- [x] behavioural `IDotMatrixAutomation` host regression suite;
- [x] ASan/UBSan host test pass;
- [x] release/build identifier separation;
- [x] documentation consistency pass for LZW12, FA02 capacity, Usermod inheritance, time fallback, PNG wording and 0.8.1 references;
- [x] final ESP32-C3 hardware smoke/stress test of build `0.8.1-audit-fix1`;
- [x] package public Release 0.8.1 from the exact hardware-qualified build.

The audit's lower-priority architectural hardening items are deliberately deferred
to 0.9 unless a hardware test proves they are required: renderer failure-atomic
reinitialization, wider FA02 synchronization refactoring, additional RX-drop
diagnostics, and broader Preferences write-result plumbing. No wire-protocol or
user-facing semantic change is planned for those items.

## 0.9 priorities

- bring up and hardware-validate ESP32-S3 targets, including PSRAM behavior;
- evaluate native 64x64 and HUB75 output paths without conflating them with the BLE protocol layer;
- implement the next iDotMatrix behavior changes only after the user-facing requirements are defined;
- revisit build-profile consolidation once C3/S3 can share a proven WLED IDF5 base;
- continue long-run memory, BLE reconnect, GIF replacement and Web UI stress testing;
- evaluate whether the pinned C3 WLED development commit can be replaced by a future stable WLED release without regressing shared-RMT behavior.

## Deferred cleanup

- review optional feature/library pruning on the IDF5 profiles after the WLED module validator behavior stabilizes;
- keep passive/PWM buzzer support out of scope unless it becomes a real hardware requirement;
- retain experimental larger-profile/HUB75 wrappers as unsupported until physical hardware validation exists.
