# 0.7.1 - larger logical profiles and low-memory 64x64 media

Version 0.7.1 promotes the larger-profile work developed after the stable 0.7.0
16x16 media baseline.

## Highlights

- Added 32x32 and 64x64 logical iDotMatrix profiles while preserving the 16x16 default path.
- Added low-memory logical-to-physical storage: with `rescale=true`, a 64x64 logical source can drive a 16x16 physical canvas using 768 bytes of persistent RGB storage instead of 12,288 bytes.
- Hardware-validated the compact 11-bit/32x32 decoder on classic ESP32.
- Added full 12-bit/64x64 GIF semantics with all 4096 legal LZW codes.
- Added automatic runtime backend selection for the 64x64 build:
  - PSRAM present -> full AnimatedGIF direct path, preferring PSRAM;
  - no PSRAM -> compact-safe LZW12 predecode + LittleFS frame-cache playback.
- Added `IDotMatrixCompactGif`, which uses a 16,128-byte workspace on the validated 64x64->16x16 setup versus about 20,660 bytes for the full AnimatedGIF object.
- Added a 512 KiB cached-frame limit and releases the decoder workspace before playback.
- Added transactional repeated-GIF replacement and clean recovery after preparation failure.
- Added transient low-heap wait/retry behavior so Wi-Fi/BLE/WebSocket allocations do not cause random one-sample `gif-ram-reserve` failures.
- Blank the physical panel during no-PSRAM GIF staging while WLED internally uses its low-RAM Static effect; restore the user's primary colour before playback/recovery.
- Retained compact `/json/info` diagnostics for build/backend, heap/largest block, cache progress/waits, reset reason, and media errors.

## Hardware validation

The release was validated on classic ESP32 / WLED 16.0.1 with a physical 16x16
matrix and no HUB75.

### 32x32 logical profile

With `rescale=true` and the 11-bit profile:

- clock and text passed;
- ten consecutive GIFs passed, including a complex animation;
- `gifDecoderBytes=9372` on the tested build;
- returning to WLED released the decoder and recovered heap;
- no WLED Error 8 or reboot.

### 64x64 logical profile without PSRAM

With `platformio_override.ini.64x64-lite`, physical 16x16, and `rescale=true`:

- static/cloud images passed;
- clock/date transitions passed;
- light, large, complex, and 100-frame-class GIFs passed;
- more than ten alternating GIF replacements passed without manually selecting Solid;
- WLED effect -> GIF, clock/date -> GIF, static image -> GIF, and return to WLED passed;
- Web UI and `/json/info` remained responsive during/after media activity;
- final black staging behavior and WLED primary-colour restoration passed;
- no reboot, WLED Error 8/90, or persistent media state was observed in the final stress sequence.

A representative final GIF reported:

```text
gifDecoder=compact12/cache
gifDecoderBytes=16128
gifProbe=31848 largest=26612 reserve=10240
gifCachedFrames=20
heap=33076 min=7468 largest=26612
content=gif
```

## Important memory-safety decision

Experimental dev.4/dev.5 builds attempted to reduce the physical 12-bit LZW
dictionary below 4096 entries. Hardware stress exposed memory corruption/panic
because valid GIF code values could index beyond those shortened arrays.

0.7.1 does **not** use that technique. The final compact decoder retains all
4096 legal codes and saves RAM by packing the prefix table and sizing
frame-dependent storage to the physical canvas.

See `ARCHITECTURE.md` for the complete memory-engineering history and the reasons
behind the final allocation/guard/cache design.

## Known limits / pending validation

- The 64x64 PSRAM/direct backend is implemented but still awaits hardware validation.
- Native physical 64x64 output is not yet release-validated.
- HUB75 DMA + BLE/media coexistence is not yet release-validated.
- At aggressive rescale ratios such as 64->16, thin text glyph strokes can become hard to read because text is rendered in logical coordinates and then sampled down.
- The no-PSRAM 64x64 GIF path writes a temporary LittleFS frame cache and is capped at 512 KiB.
- The supplied 4 MB partition layout disables WLED OTA; use USB/serial flashing.

## Upgrade notes from 0.7.0

The default 16x16 build keeps the low-RAM behavior established by 0.7.0. Larger
GIF profiles require the matching build flag/override:

- 32x32: `platformio_override.ini.32x32` / `IDOT_GIF_LZW11`;
- 64x64: `platformio_override.ini.64x64` or the hardware-validated classic-ESP32 `platformio_override.ini.64x64-lite` / `IDOT_GIF_LZW12`.
