# Test Report — Release 0.8.1 / Build 0.8.1-audit-fix1

## Scope

This report covers host-side verification plus the final physical qualification
of the audit-remediation source tree that is released as public version 0.8.1.
The internal build identifier remains `0.8.1-audit-fix1` because this exact
revision was tested on hardware.

## Host regression suite

Command:

```sh
./run_host_tests.sh
```

Result: **PASS**.

Coverage includes build-profile normalization, protocol parsing, renderer,
buzzer, automation persistence/recovery, WLED adapter behavior, bulk transfer,
FA02 reassembly, compact GIF decoding, GIF/media promotion and recovery,
AnimatedGIF profile patching, PlatformIO profile validation, and release-package
checks.

Audit-specific failure injection includes:

- GIF rename success;
- GIF rename failure with streamed-copy success;
- GIF rename plus streamed-copy failure;
- successful GIF playback after a failed promotion;
- schedule temporary-file write failure;
- schedule replacement promotion failure with rollback;
- rollback failure converging to an empty state;
- NVS commit failure with previous media/metadata restoration;
- failure of both new NVS commit and previous-metadata re-assertion;
- reboot/loadPersistence after replacement failure;
- missing media and corrupt persisted metadata.

## Sanitizer suite

Command:

```sh
./run_host_sanitizers.sh
```

Configuration:

```text
AddressSanitizer
UndefinedBehaviorSanitizer
-fno-omit-frame-pointer
halt_on_error=1
```

Result: **PASS**.

No relevant AddressSanitizer or UndefinedBehaviorSanitizer failure was observed
in the selected protocol, renderer, bulk-transfer, FA02, automation, and media
host regressions.

## Firmware/hardware status

The ESP32-C3 IDF5/shared-RMT environment remains pinned to WLED commit
`d55037f7510541eddc390c8f3d01afc5787aa44a` and NimBLE-Arduino 2.5.1. No
unrelated WLED upgrade was made during audit remediation.

The immediately preceding 0.8.1 baseline/dev.3 hardware validation passed BLE,
images, GIFs, WLED effects, WebSocket activity, roughly one hundred media changes,
and showed no LED spikes or reboot.

The exact `0.8.1-audit-fix1` remediation build was then compiled, flashed, and
qualified on the ESP32-C3 4 MB / 16x16 hardware. After boot with BLE advertising,
`/json/info` reported 76,220 bytes free heap and a 65,536-byte largest block. The
qualification run performed approximately 50 WLED effect changes, 50 iDotMatrix
content/effect changes, and another 50 WLED effect changes; program/schedule
operation was also verified successfully.

Final snapshot after 786 seconds:

```text
BLE connected
release=0.8.1
build=0.8.1-audit-fix1
freeheap=75296
min=43012
largest=65536
gifProbe=79444
gifProbeLargest=69632
reserve=10240
ws=1
```

No reboot, watchdog, panic, or LED instability was reported during this
qualification. The IDF5 reset-reason diagnostic displayed `reset=unknown`; no
actual reset was observed, so this is tracked as a non-blocking diagnostic issue
for later cleanup.

Result: **HARDWARE PASS**.
