# Test Report - 0.9.0-dev.3

## Scope

Regression validation for native-64x64 TEXT vertical scrolling on top of the hardware-validated dev.2 full-scale baseline.

## Static/host coverage

- Existing renderer/protocol/media/Carousel host suites.
- Existing automatic 16/32/64 scale regressions from dev.2.
- New 64x64 / 16x32-glyph UP/DOWN edge-entry regression.

## New regression contract

At t=0 the following vertical page must be completely outside the 64x64 viewport. After one movement step at maximum speed, exactly its first raster row reaches the entering edge. This prevents the visually abrupt multi-row insertion seen with the former glyph-height page spacing.

## Hardware status

Pending MatrixPortal S3 / 64x64 HUB75 validation.

## Executed in the preparation environment

- `python3 tests/test_platformio_profiles.py` - PASS
- `python3 tests/test_release_package.py` - PASS
- `bash run_host_tests.sh` - PASS
- Changed renderer under ASan/UBSan (`IDotMatrixRenderer.cpp` + `tests/test_renderer.cpp`) - PASS
- Full `run_host_sanitizers.sh` - not completed within the execution time limit; no failure was observed before timeout.
- Full PlatformIO firmware compilation - not executed in this environment; hardware-side compilation remains part of the MatrixPortal S3 validation.
