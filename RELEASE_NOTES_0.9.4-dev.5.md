# iDotMatrix WLED Usermod 0.9.4-dev.5

**Release:** `0.9.4`  
**Build:** `0.9.4-dev.5`  
**Status:** development / Waveshare qualification cleanup  
**Stable baseline:** `0.9.3`

## Purpose

0.9.4-dev.5 removes the temporary audio-diagnostic instrumentation used by dev.4 after the Waveshare onboard-audio path was identified and verified on real hardware. It intentionally keeps protocol, BLE framing, media behavior, rendering, automation, settings and external-buzzer semantics unchanged from the 0.9.3 behavioral baseline.

## Waveshare hardware evidence

The dev.4 one-shot I2C scan reported six devices on the shared `SDA 47 / SCL 48` bus, including:

```text
ES8311 @ 0x18 = present
ES7210 @ 0x40 = present
```

The working AudioReactive configuration uses:

```text
I2S SD   = 39
I2S WS   = 38
I2S SCK  = 43
I2S MCLK = 12
```

Local microphone processing works when **UDP Sound Sync receive mode is disabled**. When UDP receive is enabled, AudioReactive reports `UDP sound sync` and `Sound Processing: suspended`; this is a source-selection state rather than an iDotMatrix or codec-pin failure.

The following paths have been verified on the Waveshare ESP32-S3-RGB-Matrix with one 64x64 HUB75 panel and WLED 17.0.0-devV5:

- full native 64x64 output;
- PSRAM availability;
- iDotMatrix BLE advertising, app connection and reconnect;
- Clock;
- TEXT;
- static image;
- GIF;
- Carousel;
- reboot/persistence;
- Alarm and Program;
- Matrix Auto Rotation coexistence;
- onboard-microphone AudioReactive.

Still pending before the Waveshare 64x64 baseline is considered fully qualified:

- 16x16 -> 64x64 logical scaling;
- 32x32 -> 64x64 logical scaling;
- a sustained 30-60 minute GIF/Carousel + AudioReactive soak with final heap/PSRAM comparison.

## Diagnostic cleanup

Dev.5 removes:

- `IDOT_WAVESHARE_AUDIO_DIAG`;
- the one-shot I2C scan from iDotMatrix runtime;
- `audioDiag=...` from `/json/info`;
- the temporary serial `[IDM AUDIO DIAG]` output.

`SR_DEBUG` remains disabled because the pinned AudioReactive fork's legacy debug macros do not compile against the WLED 17.0.0-devV5 ESP32-S3 USB-CDC debug API.

## Waveshare build profile

Use `overrides/waveshare-s3-hub75.ini`. The short environment remains:

```text
environment = waveshare
base env    = env:waveshare_esp32s3_32MB_hub75
```

The local profile continues to preserve WLED's board definition, HUB75 backend, flash/OTA policy, PSRAM configuration and board-specific flags. It also keeps the local SHTC3_v2 dependency correction:

```text
SHTC3_v2 = git+https://github.com/lost-hope/SHTC3_v2.git#1f6e3fc7d6135b704aa41fadf09a36cbf6712834
```

## Next step

After the remaining 64x64 qualification items pass, the next development build should add **measurement-only PSRAM/cache telemetry** before changing cache policy. Larger 128x64/128x128 physical layouts remain deferred until matching hardware is available.
