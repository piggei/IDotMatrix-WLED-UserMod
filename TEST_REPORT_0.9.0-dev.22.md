# Test report — 0.9.0-dev.22

## Scope

Development validation for the new Preset / Default transport and playback path.

## Host results

`./run_host_tests.sh`: **PASS**.

Coverage relevant to this build includes:

- `06/02` activation parsing and ACK;
- valid Preset slot range `14..19` and invalid-slot rejection;
- non-audio BLE framing recognition for `06/02`;
- syntax validation of the dedicated `IDotMatrixPreset` filesystem player;
- existing Bulk repeated-header multipart assembly;
- Bulk CRC validation and intermediate `0x01` / final `0x03` ACK semantics;
- renderer TEXT timing regressions;
- existing Carousel, Alarm, Schedule, live GIF/TEXT, AudioReactive and build-profile regressions;
- release-package consistency checks.

## Hardware status

Not yet hardware-validated in this build. Required validation is listed in `HARDWARE_TEST_CHECKLIST_0.9.0-dev.22.md`.
