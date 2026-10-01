# iDotMatrix WLED Usermod 0.9.4-dev.4

**Release:** `0.9.4`  
**Build:** `0.9.4-dev.4`  
**Status:** development / hardware qualification  
**Stable baseline:** `0.9.3`

## Purpose

0.9.4-dev.4 is a diagnostic follow-up to the physically working Waveshare 0.9.4-dev.2 build. It keeps stable 0.9.3 as the behavioral baseline and makes no intentional change to iDotMatrix protocol, rendering, media, automation, settings layout or external-buzzer semantics. The only runtime addition is temporary Waveshare audio diagnostics.

The development baseline is now **WLED 17.0.0-devV5**, matching the WLED line already used for the other current iDotMatrix controller profiles.

## dev.4 compile fix

The dev.3 diagnostic concept was correct, but enabling the pinned AudioReactive fork's global `SR_DEBUG` path exposed legacy debug macros (`DEBUGOUTLN`, `DEBUGOUTF`, `pcTaskGetTaskName`) that are not compatible with the WLED 17.0.0-devV5 ESP32-S3 USB-CDC build. Dev.4 removes only that upstream debug flag. The iDotMatrix-owned one-shot I2C probe remains enabled, so the board can still report the detected I2C addresses and explicit ES8311/ES7210 presence without patching AudioReactive.

## Waveshare build profile

```text
override    = overrides/waveshare-s3-hub75.ini
environment = waveshare
base env    = env:waveshare_esp32s3_32MB_hub75
```

The short `waveshare` environment name is intentional and is suitable for merging into the project's global `platformio_override.ini` alongside the other short board aliases.

The profile inherits the upstream Waveshare board definition, HUB75 pinout/backend, 32 MB flash layout/OTA policy, 16 MB octal PSRAM setup, ES8311 audio pins and SD-card flags. It adds the iDotMatrix 64x64 profile and NimBLE 2.5.1.

## WLED devV5 SHTC3 dependency workaround

The first dev.1 build attempt stopped before compilation while PlatformIO was resolving the Waveshare custom Usermods. WLED 17.0.0-devV5 declares SHTC3_v2 using this form:

```text
https://github.com/lost-hope/SHTC3_v2/commit/1f6e3fc7d6135b704aa41fadf09a36cbf6712834
```

That URL is a GitHub commit web page, not a cloneable repository URL. PlatformIO therefore invokes `git clone` on it and fails with `repository ... not found`.

Dev.3 avoids modifying the WLED checkout. Instead, the local `waveshare` environment reproduces the upstream Waveshare custom-usermod list and changes only SHTC3_v2 to the pinned cloneable form:

```text
SHTC3_v2 = git+https://github.com/lost-hope/SHTC3_v2.git#1f6e3fc7d6135b704aa41fadf09a36cbf6712834
```

The following upstream functionality remains present:

- `Internal_Temperature`;
- pinned Waveshare-compatible AudioReactive Usermod;
- SHTC3_v2 at the same commit;
- iDotMatrix as an out-of-tree symlinked Usermod.

## Hardware baseline

Before iDotMatrix, the physical board/panel combination was verified with the official WLED 16.0.1 Waveshare binary:

- Waveshare ESP32-S3-RGB-Matrix / ESP32-S3-N32R16;
- 32 MB flash;
- 16 MB PSRAM;
- one 64x64 HUB75 panel;
- HUB75 Half Scan;
- one panel, 1x1;
- full 4096-pixel output working correctly.

That result qualifies the board, panel and upstream Waveshare hardware path. The iDotMatrix 0.9.4-dev.4 WLED 17.0.0-devV5 build still requires compilation and physical qualification.

## First gate

1. `pio run -e waveshare` completes.
2. Firmware boots and drives the complete 64x64 panel.
3. `/json/info` reports 32 MB flash and approximately 16 MB PSRAM.
4. iDotMatrix BLE advertises and connects to the original app.
5. Clock, TEXT, static image, GIF and Carousel work.
6. 16x16 and 32x32 logical profiles scale to the 64x64 physical matrix.
7. Native 64x64 operation works.
8. Reboot/persistence paths remain correct.

PSRAM/cache tuning and larger 128x64/128x128 physical layouts remain deferred until this gate passes.

## Waveshare audio diagnostics

The Waveshare profile enables `IDOT_WAVESHARE_AUDIO_DIAG` in iDotMatrix only. AudioReactive `SR_DEBUG` is intentionally not enabled because the pinned fork uses legacy debug macros that do not compile against the WLED 17/devV5 USB-CDC debug API. About eight seconds after boot, iDotMatrix performs a one-shot scan of WLED's already initialized shared I2C bus. It does not call `Wire.begin()` and does not take ownership of the bus.

Serial output reports all detected I2C addresses plus explicit presence checks for `ES8311 @ 0x18` and `ES7210 @ 0x40`, together with the expected I2S pins (`SD 39`, `WS 38`, `SCK 43`, `MCLK 12`). The same result is summarized as `audioDiag=...` in `/json/info`.

The pinned AudioReactive branch already contains an ES8311 source path that probes `0x40`; when ES7210 is detected, it overrides the ES8311 ADC path and initializes ES7210 for microphone input. The diagnosis therefore relies on the iDotMatrix one-shot I2C probe and the existing AudioReactive runtime state rather than enabling the incompatible upstream debug macro.

This diagnostic instrumentation is intended for qualification only and should be removed or disabled after the onboard microphone path is understood.
