# Test Report - 0.9.0-dev.6

Release: `0.9.0`  
Build: `0.9.0-dev.6`

## Automated tests

- PlatformIO profile contract tests: PASS.
- Release package contract tests: PASS.
- Host protocol/renderer/adapter tests: PASS.
- New regression: transfer indicator remains hidden before its delay threshold and renders visible non-black framebuffer content after the threshold.

## Sanitizer checks

- WLED adapter ASan/UBSan regression: PASS.
- Full sanitizer suite: not rerun as a release gate for this focused build.

## Hardware validation

Pending. Validate on MatrixPortal ESP32-S3 + physical 64x64 HUB75 panel.
