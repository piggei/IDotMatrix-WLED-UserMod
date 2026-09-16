# Test report — 0.9.0-dev.23

## Scope

Preset / Default upload-indicator integration on top of the dev.22 Preset implementation.

## Result

`./run_host_tests.sh`: **PASS**.

Covered regression surfaces include protocol, BLE framing, Bulk multipart, renderer, WLED adapter, Alarm/Schedule automation, PlatformIO profiles and release-package checks.

Dev.23-specific package guards verify that Preset upload begin/data paths call the shared transfer-indicator API and that the 5-second abandoned-upload timeout remains present.

## Hardware validation requested

- long Preset upload shows the red-arrow / blue-tray indeterminate animation;
- animation remains continuous across consecutive Preset assets;
- `06/02` replaces the indicator directly with the first activated Preset entry;
- interrupted upload clears the indicator after approximately 5 seconds;
- existing Carousel indicator behavior remains unchanged.
