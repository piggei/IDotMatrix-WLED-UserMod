# Test report — 0.9.0-dev.24

## Purpose

Consolidation regression after Alarm/Program multipart fixes and Preset / Default implementation.

## Expected host coverage

- renderer and logical/physical scaling
- BLE framing / FA02 ownership
- protocol parsing and ACK semantics
- Alarm and Program/Schedule multipart assembly
- Bulk / Carousel / Preset routing
- WLED adapter ownership and transfer indicator
- PlatformIO profile consistency
- release-package consistency and repository cleanliness

## Consolidation-specific assertion

The upload indicator has no determinate completion state. It remains indeterminate while visible and completion is represented by retiring the indicator.

## Hardware evidence incorporated

Alarm and Program/Schedule now operate correctly on the 64x64 target after the repeated-header/ACK fixes. Preset / Default media transfers and playback also operate correctly, including long transfers with the upload indicator.

The separate ESP32-C3 burn-test OFF event was traced to an external Home Assistant light-group command and is not considered a firmware stability failure.

## Executed results

- `./run_host_tests.sh`: PASS
- `./run_host_sanitizers.sh`: PASS (ASan/UBSan)
- PlatformIO profile checks: PASS
- Release-package checks: PASS
- Repository cleanliness scan: PASS (no conflict markers or `.orig` / `.bak` / `.rej` artifacts)
