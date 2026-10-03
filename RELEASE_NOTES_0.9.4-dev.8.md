# iDotMatrix WLED Usermod 0.9.4-dev.8

**Release:** 0.9.4  
**Build:** 0.9.4-dev.8  
**Status:** development / Waveshare PSRAM telemetry correction

## Scope

Build 0.9.4-dev.8 is a narrow correction to the dev.7 PSRAM diagnostics on the Waveshare ESP32-S3-RGB-Matrix. The qualification profile remains `overrides/waveshare-s3-hub75.ini` / `env:waveshare`. Stable baseline: 0.9.3. The GIF whole-file PSRAM staging path introduced by dev.7 is unchanged.

During real hardware testing, a `/json/info` request could occasionally report a current `free` PSRAM value lower than `minFree`. This was possible because the runtime low-water marks were sampled every 250 ms while `/json/info` independently sampled current PSRAM at request time.

Dev.8 folds the exact current `free` and `largest` samples into the low-water marks before serializing the diagnostic line and derives `peakUsed` afterwards. The invariant is therefore maintained in each emitted snapshot:

```text
minFree <= free
minLargest <= largest
peakUsed = total - minFree
```

## Unchanged from dev.7

- Waveshare-only whole-file GIF staging in PSRAM.
- 2 MiB per-GIF staging limit.
- 4 MiB pre-allocation free-PSRAM reserve guard for source staging (decoder storage may consume a small additional amount afterward).
- Automatic fallback to the LittleFS source path.
- `gifStage=...`, `gifCache=...` and PSRAM diagnostics.
- BLE protocol, renderer, Carousel, automation, settings and external buzzer behavior.

## Hardware observation leading to this build

The staging path itself was already successful in dev.7: repeated Carousel playback showed successful PSRAM staging with zero fallbacks and no reduction in the largest free PSRAM block. Dev.8 changes only the accuracy/self-consistency of the telemetry used for continued soak testing.
