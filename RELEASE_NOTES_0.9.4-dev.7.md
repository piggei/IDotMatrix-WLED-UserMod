# iDotMatrix WLED Usermod 0.9.4-dev.7

**Release:** 0.9.4  
**Build:** 0.9.4-dev.7  
**Stable baseline:** 0.9.3

## Scope

Build 0.9.4-dev.7 is the first PSRAM-use optimization after the dev.6 measurement baseline on the Waveshare ESP32-S3-RGB-Matrix. The build profile remains `overrides/waveshare-s3-hub75.ini` / `env:waveshare`. It keeps the qualified 64x64 runtime, WLED 17.0.0-devV5 target, protocol behavior, BLE framing, renderer, Carousel ownership rules, automation, settings and external-buzzer boundary unchanged.

## GIF source staging in PSRAM

On the Waveshare target, direct AnimatedGIF playback now attempts to copy the complete GIF source file from LittleFS into PSRAM before opening the decoder.

The initial policy is deliberately conservative:

- maximum staged GIF size: **2 MiB**;
- PSRAM reserve that must remain free before staging: **4 MiB**;
- source is copied in 4096-byte chunks and yields between chunks;
- the staged buffer stays alive for the lifetime of the AnimatedGIF decoder;
- decoder reads and seeks are served from the staged PSRAM image;
- the buffer is released immediately after the decoder closes.

If the GIF is larger than the staging limit, PSRAM is unavailable, contiguous/free PSRAM is below the guard, allocation fails, or the source copy fails, playback automatically falls back to the pre-dev.7 LittleFS callback path. A staging failure is therefore not a media-playback failure by itself.

This policy is enabled by default only for `IDOT_WAVESHARE_S3_RGB_MATRIX`. Other profiles retain their previous behavior unless a future build explicitly enables `IDOT_GIF_PSRAM_STAGE_MAX` for them.

## Diagnostics

The dev.6 PSRAM/cache telemetry remains. Dev.7 adds:

```text
gifStage=state:<psram|fs> bytes:<current> peak:<peak> attempts:<n> ok:<n> fallback:<n> max:<bytes> reserve:<bytes>
```

During active staged GIF playback, `state:psram` and a non-zero `bytes` value confirm that LittleFS has been removed from the decoder's realtime read/seek path. `fallback` records attempts that returned to the original filesystem path.

## Waveshare status

The Waveshare ESP32-S3-RGB-Matrix remains the only active physical target for this optimization. Hardware already verified in the 0.9.4 line includes native 64x64 output, BLE/app connectivity, TEXT, static image, GIF, Carousel, reboot/persistence, Alarm/Program, Matrix Auto Rotation and onboard-microphone AudioReactive with UDP Sound Sync receive mode disabled.

The remaining qualification work is the 16x16 -> 64x64 and 32x32 -> 64x64 scaling checks plus continued soak testing under GIF/Carousel + AudioReactive load.
