# Hardware Qualification Checklist — iDotMatrix WLED Usermod 0.8.2

This checklist records the final stable-release hardware gates and can also be reused after future toolchain/platform rebuilds.

## Release identity

- [x] `/json/info` reports `release=0.8.2` and `build=0.8.2`.
- [ ] WLED UI shows the custom effect as `iDotMatrix` (stable-name smoke test).
- [ ] With a WLED playlist running, an iDotMatrix app content command terminates the playlist, cancels any already queued playlist preset and leaves `iDotMatrix` in control.

## Core app / BLE

- [x] Official iDotMatrix app connects and controls the ESP32-C3 target.
- [x] Images, TEXT, GIF and normal visual modes operate correctly.
- [x] FA02 transport changes show no observed nonzero drop/malformed/timeout diagnostics in normal hardware use.

## Carousel / GIF cache

- [x] Mixed Carousel content runs repeatedly.
- [x] First-use GIF cache generation may delay the first encounter, but later encounters run at normal speed.
- [x] `gifCacheStats` build count stabilizes while reuse count continues increasing.
- [x] Clock/TEXT/rainbow/scroll/snow/laser renderers do not contaminate a following GIF cache.
- [x] Carousel A -> Carousel B does not expose Clock; neutral/blank ownership is retained during replacement.
- [x] Stored animation/Carousel content can start correctly at boot when selected by the WLED boot preset.

## Reset / persistence

- [x] Reset while Carousel/GIF media is active closes media before storage deletion.
- [x] Reset reports `status:ok carousel:ok automation:ok`.
- [x] Carousel storage is reclaimed and `stored=0` after Reset.
- [x] WLED itself is not rebooted by protocol Reset.

## Automation

- [x] Alarms function on hardware.
- [x] Programs/schedules function on hardware.
- [x] Persisted animation/program behavior survives normal boot as expected.

## Audio

- [x] Phone/BLE Audio/Rhythm path remains functional.
- [x] WLED AudioReactive/local microphone input works on the C3 audio profile.
- [x] Visualizer reacts to external microphone/audio input once the official app has selected the Audio/Rhythm effect.

## Clock

- [x] Styles 0, 3, 5, 6 and 7 use the final +2 pixel HH:MM colon position.
- [x] Style 4 uses the final -1 pixel colon position.
- [x] Style 2 shifts both hour digits and the first minute digit left by one pixel while keeping the separator and second minute digit fixed.
- [x] Style 1 geometry remains unchanged.
- [x] HH:MM colon blinks continuously; DD/MM separator remains steady.
- [x] No edge clipping observed on the qualified physical 16x16 target.

## Final stable integration smoke test

The final package changes the public WLED effect label and adds playlist termination when iDotMatrix explicitly takes ownership. After compiling the final archive, verify once that:

1. WLED lists exactly one custom effect named `iDotMatrix`;
2. selecting it resumes stored Carousel content or Clock fallback as before;
3. a boot preset saved with the final package selects the same dynamically assigned effect correctly;
4. start a WLED playlist, wait until a native WLED effect is visibly active, then send any visual command from the iDotMatrix app: `iDotMatrix` must become selected immediately, the playlist must terminate, and no already queued or later playlist preset may steal the display;
5. restart the playlist manually from WLED and verify it can take control normally until another iDotMatrix content command arrives.

No long soak is required solely for these small stable-integration deltas; repeat the broader soak when changing WLED base, framework, BLE library, storage backend or hardware platform.
