# Test Report - 0.9.0-dev.9

Release: `0.9.0`  
Build: `0.9.0-dev.9`

## Automated tests

- PlatformIO profile contract tests: PASS.
- Release package contract tests: PASS.
- Full host protocol/renderer/adapter test suite: PASS.
- New regression: the canonical 16x16 transfer artwork contains the expected blue tray and red downward arrow.
- New regression: the pending status bar remains static across animation frames and only changes to fully filled after confirmed session completion.
- Existing delayed-visibility and consecutive-asset continuity regressions remain PASS.

## Sanitizer checks

- Full host ASan/UBSan suite: PASS.

## Hardware validation

Pending for dev.9. dev.8 hardware validation confirmed the 16x16 transfer artwork is readable, requested a one-pixel gap above the bar, and showed that an indeterminate waving bar is not desirable. dev.9 is intentionally diagnostic: upload one Carousel with a known image count, then inspect `carouselCfg` and `carouselUpload` in `/json/info`.
