# iDotMatrix WLED Usermod 0.9.4-dev.2

**Release:** `0.9.4`  
**Build:** `0.9.4-dev.2`  
**Status:** development / hardware qualification  
**Stable baseline:** `0.9.3`

## Purpose

0.9.4-dev.2 follows the first Waveshare build attempt and keeps stable 0.9.3 as the behavioral baseline. No iDotMatrix protocol, rendering, media, automation, settings-layout or external-buzzer semantics are intentionally changed.

The development baseline is now **WLED 17.0.0-devV5**, matching the WLED line already used for the other current iDotMatrix controller profiles.

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

Dev.2 avoids modifying the WLED checkout. Instead, the local `waveshare` environment reproduces the upstream Waveshare custom-usermod list and changes only SHTC3_v2 to the pinned cloneable form:

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

That result qualifies the board, panel and upstream Waveshare hardware path. The iDotMatrix 0.9.4-dev.2 WLED 17.0.0-devV5 build still requires compilation and physical qualification.

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
