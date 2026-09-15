# Test Report - 0.9.0-dev.12

Release: `0.9.0`  
Build: `0.9.0-dev.12`

## Scope

- Carousel transfer indicator only.
- Verify indeterminate left/right activity sweep while upload is active.
- Verify full status bar after transfer completion.
- Preserve 16x16 and native 64x64 transfer artwork.

## Host validation

- Host regression suite: PASS.
- PlatformIO profile metadata tests: PASS.
- Release package checks: PASS.
- Adapter transfer-indicator regression updated for moving activity bar.

## Hardware validation

Pending. Test on MatrixPortal S3 / native 64x64 HUB75 and, if convenient, logical 16x16 -> physical 64x64.

- ASan/UBSan host tests passed.
