# Testing

This file defines the **0.7.1 stable regression procedure** and records the
hardware configurations that were used to promote the release.

## Host regression tests

From the repository root with a C++11 compiler and zlib development files:

```sh
./run_host_tests.sh
```

The suite covers protocol framing/ACKs, power/brightness/RGB, DIY/graffiti,
clock/text rendering, 2D mapping, bulk CRC32, RAW publication, FA02
fragmentation, compact PNG decode, GIF RX/promotion/playback, WLED ownership,
repeat-GIF replacement, and the compact 64x64 decoder/cache path.

The compact-GIF tests include real 64x64 fixtures and stress the 4096-code
LZW12 implementation without using a truncated dictionary.

## WLED build validation

From the WLED source directory:

```powershell
pio run -e esp32dev_idotmatrix -t clean
pio run -e esp32dev_idotmatrix
```

Expected facts:

- repository/library version: `0.7.1`;
- `NimBLE-Arduino @ 1.4.3`;
- `AnimatedGIF @ 1.4.7`;
- default build patch banner: 10-bit / max 16x16;
- 32x32 build patch banner: 11-bit / max 32x32;
- 64x64 build patch banner: 12-bit / max 64x64 / dict=4096 / filebuf=1024;
- no `.dram0.bss` overflow;
- no dependency on `esp-nimble-cpp` or `ESP32 BLE Arduino`.

## Build profiles

| Logical GIF profile | Recommended override | Expected runtime decoder |
|---|---|---|
| 16x16 | `platformio_override.ini.example` | `animatedgif10` |
| 32x32 | `platformio_override.ini.32x32` | `animatedgif11` |
| 64x64, normal WLED feature set | `platformio_override.ini.64x64` | PSRAM direct or no-PSRAM cache depending hardware |
| 64x64, classic ESP32 no PSRAM | `platformio_override.ini.64x64-lite` | `compact12/cache` (hardware-validated configuration) |

`platformio_override.ini.hub75` is an optional separate example and is not part
of the 0.7.1 release-validation matrix.

## Stable 16x16 hardware regression

Use a classic ESP32, a WLED 16x16 2D matrix, and I2S LED output.

1. Flash by USB/serial and reboot.
2. Confirm WLED remains reachable over Wi-Fi for at least 15 seconds.
3. Confirm `/json/info` reports `build=0.7.1` and BLE advertising.
4. Connect with the official iDotMatrix app.
5. Verify power OFF/ON and brightness changes.
6. Verify red, green, blue, white, and black full-screen colours.
7. Enter DIY/Graffiti, draw pixels, and display a saved Graffiti image.
8. Test a clock style and verify WLED/NTP time is shown.
9. Send scrolling text with the speed slider at both extremes and verify an obvious slow/fast difference.
10. Browse/send multiple cloud/static images.
11. Send at least ten GIF animations in sequence.
12. While a GIF is playing, return to clock and then to a static colour/WLED effect.
13. Disconnect and verify BLE advertising resumes.
14. Reconnect and repeat one image and one GIF transfer.

## 32x32 logical profile regression

Classic ESP32, physical 16x16 matrix, no HUB75:

1. use `platformio_override.ini.32x32`;
2. set `screenType=32x32` and `rescale=true`;
3. confirm `/json/info` reports `profile=32x32`, `canvas=16x16`, and `gifDecoder=animatedgif11`;
4. test clock and text;
5. send at least ten GIFs consecutively, including a complex animation;
6. return directly to normal WLED effects and confirm decoder RAM is released;
7. verify no WLED Error 8, reboot, stale GIF, or BLE reconnect requirement.

Recorded hardware result during development:

- `gifDecoderBytes=9372`;
- minimum heap 7904 B during the stress sequence;
- return to WLED recovered about 33.5 KiB free heap and a 28.6 KiB largest block.

## 64x64 logical / classic ESP32 no-PSRAM regression

This is the key 0.7.1 larger-profile release test.

Use:

- `platformio_override.ini.64x64-lite`;
- physical WLED matrix 16x16;
- `screenType=64x64`;
- `rescale=true`;
- no HUB75.

After reboot, confirm:

```text
build=0.7.1
profile=64x64
canvas=16x16
gifDecoder=compact12/cache
gifDecoderBytes=16128
```

Then test in this order:

1. normal WLED effect -> GIF;
2. clock/date -> GIF;
3. static/cloud image -> GIF;
4. one light GIF;
5. one large/complex GIF;
6. one long animation (100-frame class if available);
7. alternate two known-good GIFs at least ten times **without** manually selecting Solid;
8. GIF -> clock -> GIF -> static image -> GIF -> normal WLED effect;
9. repeatedly open the Web UI and `/json/info` during cache preparation and playback;
10. leave a representative cached GIF looping for at least 15-20 minutes and recheck heap/network responsiveness.

During cache preparation the physical panel should go black rather than visibly
showing WLED Solid. The original WLED primary colour must be restored after
success, failure, or a Web UI/API takeover.

A successful cached GIF should report `gifCachedFrames=N` and `content=gif`.
`gifCacheWaits=N low=M guard=9216` is allowed and means the cache builder yielded
to transient WLED/network allocations.

A cache build should fail with `mediaError=gif-ram-reserve` only after free heap
remains below the guard continuously for roughly two seconds. The Web UI/BLE
must remain alive after such a failure and the next valid GIF must be able to
start without a manual Solid reset.

A cache exceeding 512 KiB must fail cleanly with `mediaError=gif-cache-full`.

### Release hardware result

The final dev.13/dev.14 sequence passed:

- 10+ A/B GIF replacements;
- large and 100-frame animations;
- WLED effect -> GIF;
- clock/date -> GIF;
- static image -> GIF;
- return to WLED;
- live Web UI and `/json/info` access;
- black staging with primary-colour restoration.

Representative final GIF status:

```text
gifDecoder=compact12/cache
gifDecoderBytes=16128
gifProbe=31848 largest=26612 reserve=10240
gifCachedFrames=20
heap=33076 min=7468 largest=26612
content=gif
```

The historical `min` heap is expected to be lower than current playback heap;
cache predecode is the high-pressure phase.

## Recovery tests

- Interrupt/cancel a media transfer and then send a normal command.
- Disconnect during a transfer, reconnect, and verify the next valid command.
- Send a CRC-invalid replacement GIF and confirm the currently playing GIF is not destroyed before validation.
- Force a post-validation GIF preparation failure; recovery must not select an empty `iDotMatrix Display`.
- Change WLED effect from the Web UI while GIF staging is active; the saved primary colour must be restored.
- Change `deviceName` or `screenType`; verify `/json/info` reports that a restart is required until reboot.
- Configure a digital RMT bus and verify the Usermod refuses to start BLE rather than entering the known Bluetooth/RMT reboot loop.

## Memory-safety checks

For 64x64 no-PSRAM testing:

- never treat a truncated 12-bit dictionary as an acceptable optimization;
- `largest` must be large enough for the compact workspace before allocation;
- free heap after admission must preserve the 10 KiB reserve;
- the 9 KiB runtime guard is a wait/yield threshold, not a one-sample abort;
- Web UI responsiveness is a release criterion, not merely absence of reboot;
- after cached playback starts, the compact decoder workspace must have been released.

The detailed rationale and failed experiments are in `ARCHITECTURE.md`.

## Pending validation after 0.7.1

- automatic PSRAM/full-AnimatedGIF 64x64 direct path on real hardware;
- native physical 64x64 output;
- HUB75 DMA + BLE/media coexistence;
- 24-hour Wi-Fi/BLE/content soak;
- final 16x16 hardware regression on the exact 0.7.1 tag if release packaging differs from the last dev build.

## Historical development notes

The `RELEASE_NOTES_0.7.1-dev.*.md` files preserve the individual experiments,
including the unsafe truncated-LZW12 branches. They are retained for engineering
history, not as recommended build targets. In particular, dev.4/dev.5 must not
be used as a model for future memory optimization because their physical LZW12
dictionaries were smaller than the legal 4096-code space.
