# Test report - 0.9.0-dev.20

## Scope

Automation multipart correction for Alarm and Program/Schedule logical packets.

## Host regression coverage

- Existing single-packet Alarm parsing/CRC/callback path.
- Existing single-packet Program parsing/CRC/callback path.
- Alarm total media 6706 bytes split exactly 4096 + 2610, with repeated 24-byte headers and different `reserved2` values.
- No Alarm callback/commit after the first chunk.
- Complete Alarm callback only after 6706 bytes and valid full-media CRC.
- Incomplete Alarm transaction timeout without commit.
- Program media 9000 bytes split 4096 + 4096 + 808 across three repeated 23-byte activity headers.
- No Program callback before the final chunk.
- Complete Program callback only after all 9000 bytes and valid full-media CRC.
- Alarm filesystem replacement now uses temp + backup + promote/rollback; syntax/automation host tests cover the updated path.
- Existing build/profile/package regressions.

## Result

Relevant protocol tests, automation host tests, AnimatedGIF profile patch tests, PlatformIO profile tests and release-package checks pass in the build environment. The monolithic host script exceeds the execution window here after completing its compiled C++ stages, so the constituent critical checks were rerun directly.

## Hardware status

Pending MatrixPortal S3 / official-app validation. The critical expected observation is that setting a 64x64 Alarm updates `alarmSlot` to the new time and the final `alarmRx` reports `result:committed`.
