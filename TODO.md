# 0.8.2 release-candidate status

- [x] Persistent 12-slot Carousel / mixed GIF+TEXT behaviour.
- [x] Long-text viewport/paging behaviour on physical 16x16 hardware.
- [x] Optional AudioReactive source integration.
- [x] WLED `iDotMatrix Display` standalone policy: Carousel when stored, Clock otherwise.
- [x] `03 80` live reset that erases Carousel, alarms and programs without rebooting WLED.

# 0.9 hardware line

Release 0.9 is intentionally reserved for the new-hardware phase: ESP32-S3/PSRAM, physical larger matrices, native 64x64 validation and HUB75 work. Items that require that hardware remain deferred until it is available.

# 0.8.2 completed convergence work

The internal `0.9.0-dev.*` builds were a development sequence only. Their validated protocol and renderer work was folded into Release 0.8.2 before the release-candidate stage. The current identification is:

```text
Release: 0.8.2
Build:   0.8.2-rc.2
```

Completed in 0.8.2 include optional AudioReactive input, long-TEXT viewport behavior, persistent 12-slot Device Assets/Carousel support, WLED Carousel/Clock standalone selection, and live `03 80` reset semantics. Historical build-by-build details are retained in `HISTORY.md`.

# Next development steps

- integrate protocol/function changes only after they are validated in the
  standalone ESP32 emulator;
- bring up ESP32-S3 when the new board arrives and establish PSRAM allocation
  policy from real measurements;
- validate native larger matrices, then evaluate the separate native WLED
  HUB75-output work;
- consider renderer failure-atomic reinitialization, broader FA02 synchronization
  review, RX queue-drop diagnostics and wider Preferences write-result checking;
- evaluate migration of classic ESP32 to a common IDF5 WLED base only as a
  separate experiment, never as an assumption.
