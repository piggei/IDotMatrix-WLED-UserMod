# Test Report - 0.9.0-dev.13

Release: `0.9.0`  
Build: `0.9.0-dev.13`

## Scope

- Carousel transfer indicator only.
- Verify indeterminate left/right activity sweep while upload is active.
- Verify that transfer completion does not fill the bar; the activity segment remains the same width until the indicator is retired.
- Preserve 16x16 and native 64x64 transfer artwork.

## Host validation

- Host regression suite: PASS.
- PlatformIO profile metadata tests: PASS.
- Release package checks: PASS.
- Adapter transfer-indicator regression updated for moving activity bar.

## Hardware validation

Pending. Test on MatrixPortal S3 / native 64x64 HUB75 and, if convenient, logical 16x16 -> physical 64x64.

- Full ASan/UBSan suite: TIMEOUT in the build environment; no sanitizer failure was observed before timeout.
