# Testing

This document is the consolidated validation plan for iDotMatrix WLED Usermod
`0.9.0-rc.1`. Historical per-build test reports and checklists are intentionally
not shipped; regression coverage lives in `tests/` and release qualification is
tracked here.

## Automated host regression

From the repository root:

```sh
./run_host_tests.sh
```

The suite covers protocol framing and ACKs, BLE FA02 assembly, Bulk/multipart media, Alarm and Program/Schedule transactions, Carousel and Preset routing, clock/text/timer/scoreboard/audio rendering, GIF/media handling, WLED ownership, build-profile normalization, partition geometry and release-package consistency.

Where supported by the host compiler:

```sh
./run_host_sanitizers.sh
```

This builds the main protocol, renderer, media and automation regressions with AddressSanitizer and UndefinedBehaviorSanitizer.

## Current 0.9 hardware status

Validated on Adafruit MatrixPortal S3 + 64x64 HUB75 with WLED 0.17.0-devV5/native HUB75 backend:

- logical profiles 16x16, 32x32 and 64x64 rendered through the universal scaler on the 64x64 panel;
- native 64x64 TEXT/font path, including large glyph payloads and scrolling;
- GIF playback and cached Carousel playback;
- persistent Carousel uploads and indeterminate transfer indicator;
- Alarm multi-packet media upload and execution;
- Program/Schedule multi-packet upload, flow-control ACKs and execution;
- Preset / Default volatile six-slot bank, mixed media playback and upload indicator;
- WLED <-> iDotMatrix ownership transitions;
- AudioReactive/phone audio visualizers;
- Countdown, Stopwatch and Scoreboard graphics;
- native 64x64 combined time/date layouts for clock styles 0 and 3, plus the final timer/scoreboard artwork.

The previous ESP32-C3 long-running OFF investigation was closed: the captured WLED global OFF transitions were traced to an external Home Assistant light-group command, not to the iDotMatrix firmware.

## Release-candidate hardware validation

Before 0.9.0 final release, perform a full RC pass on the 64x64 target:

1. verify all clock styles, including date mode and 12/24-hour handling;
2. exercise font sizes, static text, paging and horizontal/vertical scrolling;
3. upload and run several Alarm entries, including large media;
4. upload and run Program/Schedule entries, including mixed one-packet and multi-packet media;
5. upload and play Preset / Default sets with TEXT, images/GIF and large assets;
6. replace an active Preset and confirm the switch occurs only after `06/02` activation;
7. upload and play Carousel banks with mixed media and long transfers;
8. switch repeatedly between native WLED effects and iDotMatrix-owned content;
9. exercise all audio visualizers from the intended audio source;
10. verify Countdown, Stopwatch and Scoreboard start/pause/resume/end behavior;
11. leave the MatrixPortal S3 running for an extended burn/soak period while cycling representative content;
12. watch heap/PSRAM diagnostics for sustained fragmentation or allocation failures.

## 32x32 and iOS

A physical 32x32 panel is not a blocker for 0.9. The 32x32 logical path has been exercised through the 64x64 hardware/scaler and remains available for later physical validation.

iOS work remains isolated on its dedicated branch and is not a 0.9 release blocker unless a new requirement makes it necessary.

## Build profiles

`tests/test_platformio_profiles.py` statically checks the supported override/profile matrix and version contract during every host-test run. A successful compile proves build compatibility; physical validation remains target-specific.
