# Hardware checklist — 0.9.0-dev.24

Dev.24 is a consolidation build. A short smoke test is sufficient unless regressions are observed.

- [ ] Boot MatrixPortal S3 / HUB75 64x64 and confirm normal WLED output.
- [ ] Connect official iDotMatrix app over BLE.
- [ ] Verify Clock/TEXT/GIF basic display paths.
- [ ] Verify stored Carousel starts and rotates normally.
- [ ] Verify one Alarm can be uploaded and triggered.
- [ ] Verify one Program/Schedule can be uploaded and executed.
- [ ] Verify Preset / Default upload, indicator, activation and cyclic playback.
- [ ] Verify WLED -> iDotMatrix -> WLED ownership transitions.
- [ ] Confirm `/json/info` reports release 0.9.0 / build 0.9.0-dev.24.

Already validated before this consolidation: 64x64 native output, 16/32 -> 64 scaling, Alarm/Program multipart transfer, Preset transfer/playback, and the shared indeterminate upload indicator.
