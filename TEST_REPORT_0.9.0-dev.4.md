# Test report - 0.9.0-dev.4

## Scope

64-pixel TEXT glyph support only. No behavioural changes were intended for BLE transport, GIF/media, Carousel, HUB75 output or automatic logical/physical scaling.

## Host tests

- `run_host_tests.sh`: PASS
- Protocol regression: PASS for 32x64 / 256-byte glyph records
- Renderer regression: PASS for native 64x64 rendering of a 32x64 glyph cell
- Targeted ASan/UBSan protocol + renderer tests: PASS
- Full sanitizer suite: not completed within the execution timeout; no failure observed before timeout

## Hardware status

Pending MatrixPortal S3 + 64x64 HUB75 validation of the app's size-64 TEXT option.
