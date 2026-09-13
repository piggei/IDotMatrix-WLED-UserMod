# Test Report — iDotMatrix WLED Usermod 0.8.2

**Release:** 0.8.2  
**Build:** 0.8.2  
**Date:** 2026-09-13  
**Package status:** stable source release

## Scope

This report consolidates the final 0.8.2 qualification. The stable source is based on the hardware-validated RC7 line plus two WLED-integration changes made for the final package: the custom effect is named `iDotMatrix` instead of `iDotMatrix Display`, and explicit iDotMatrix display acquisition terminates an active WLED playlist and cancels any playlist preset already queued for deferred application before selecting the framebuffer effect. No BLE wire/protocol or renderer behavior was intentionally changed by the stable promotion.

## Host regression coverage

The packaged host suite covers, among other areas:

- protocol command parsing and captured ACK behavior;
- FA02 framing/reassembly and complete-ATT-write routing;
- bulk-transfer sequencing/CRC and timeout behavior;
- renderer geometry, text traversal, clocks, timers, scoreboard and light/audio effects;
- exact 0.8.2 clock pixel positions and blink phases;
- WLED adapter ownership/fallback/Carousel-update hold, including active-playlist termination plus queued-preset cancellation on iDotMatrix reclaim;
- GIF media/cache promotion, rollback and boot recovery;
- Carousel reset/configure media-release ordering;
- alarms/programs/schedules and persistence failure paths;
- AudioReactive source mapping;
- build/profile/package contracts.

The final package contract additionally verifies runtime `release=0.8.2`, `build=0.8.2`, the stable document surface and the public WLED effect data string `iDotMatrix@;;;2`.

## Hardware qualification evidence

The final ESP32-C3 / 16x16 hardware validation reported the following as working:

- normal official-app BLE operation;
- Clock, TEXT and animated renderer transitions;
- all clock styles 0..7 after the final geometry update;
- blinking HH:MM separator and stable date separator;
- persistent mixed Carousel operation over repeated rotations;
- GIF cache reuse (build counter stabilizes while reuse counter increases);
- correct first-use cold-cache behavior followed by normal-speed cached rotations;
- Carousel-to-Carousel replacement without exposing Clock or the previous procedural renderer;
- Reset while Carousel/GIF media is active, producing `status:ok carousel:ok automation:ok` and reclaiming Carousel/cache filesystem storage;
- alarms;
- programs/schedules;
- persisted animations and normal startup/boot restore;
- WLED AudioReactive input using an external microphone.

During cache qualification the observed diagnostics showed stable filesystem occupancy during reuse, stable free heap between repeated samples and no nonzero BLE RX/drop/timeout diagnostics. The exact runtime figures are hardware-session evidence rather than release constants and are therefore not encoded as requirements.

## Audio application behavior note

Local microphone acquisition works when the official iDotMatrix app selects and streams the Audio/Rhythm mode. If music is stopped and the official app does not transmit the effect-selection command, the Usermod cannot change to that effect on its own; this is treated as client behavior rather than a microphone acquisition failure.

## Reset qualification

The final reset regression produced:

```text
carousel=off stored=0
protocolReset=... status:ok carousel:ok automation:ok
```

and reclaimed the Carousel/cache filesystem usage. The media-close-before-delete ordering introduced during the RC cycle is therefore considered hardware-qualified.

## Stable-promotion delta

Two WLED-integration changes were made directly for stable 0.8.2:

```text
iDotMatrix Display  ->  iDotMatrix
active WLED playlist + queued playlist preset + explicit iDotMatrix reclaim -> applyPreset(0), then iDotMatrix ownership
```

The dynamically assigned effect ID, boot-preset semantics and BLE protocol remain unchanged. The playlist rule deliberately matches WLED's existing direct-effect behavior; it terminates rather than pauses/resumes the playlist. Because WLED applies playlist entries asynchronously, the stable implementation uses `applyPreset(0)` on takeover: in the pinned WLED base this unloads the playlist and resets the private pending-preset sentinel to zero before `handlePresets()` runs. The packaged host regression reproduces a queued playlist preset and verifies that it cannot overwrite iDotMatrix after takeover.

The first hardware smoke attempt of the playlist integration exposed an important distinction that the original host regression did not model: `unloadPlaylist()` alone does not clear a preset already queued by `applyPresetFromPlaylist()`. That intermediate implementation could therefore set the iDotMatrix effect and then be overwritten by `handlePresets()` later in the same WLED main-loop cycle. The release source was corrected before publication, and the regression test now explicitly starts from a native WLED effect with both an active playlist and a queued playlist preset.

This final playlist-ownership delta is host/sanitizer-qualified but was added after the RC7 hardware session; the one remaining final-package smoke test is therefore explicitly listed in `HARDWARE_TEST_CHECKLIST_0.8.2.md` rather than being represented here as already hardware-validated.

## Remaining boundaries

Not claimed as hardware-qualified by 0.8.2:

- ESP32-S3 / PSRAM targets;
- native physical 64x64 operation;
- HUB75 output targets;
- the still-unknown third 64x64 TEXT format.

A complete fresh WLED/PlatformIO target build must always be performed by the release builder for the chosen target. Host/sanitizer tests do not replace hardware/firmware compilation.

## Final package verification in the construction environment

The stable source tree was rechecked after the version/effect-name/playlist-ownership/documentation changes.

**PASS:**

- release-package contract;
- PlatformIO static profile contract;
- AnimatedGIF profile-patch regression;
- build-profile host tests (16x16, 16x16/LZW12, 32x32, 64x64);
- protocol host tests;
- AudioReactive source host tests;
- renderer host tests;
- buzzer host tests;
- automation syntax + behavioural host tests;
- WLED-adapter behavioural host tests, including active-playlist termination and queued-preset cancellation on explicit iDotMatrix reclaim;
- bulk-transfer tests;
- FA02 assembler tests;
- BLE framing tests;
- NimBLE 2.x and 1.x production-source syntax checks;
- compact-GIF test;
- Carousel production-source syntax check;
- media tests for default, 11-bit and 12-bit backends.

The monolithic `run_host_tests.sh` exceeds the execution-time limit of the construction environment, so the same targets were executed in equivalent groups and all passed.

ASan/UBSan passes were obtained for protocol, AudioReactive source, renderer, bulk transfer, FA02 assembler, BLE framing and automation with GCC. The final WLED-adapter playlist-ownership regression was also run directly under ASan/UBSan with GCC. The media/default and media/12-bit sanitizer groups were also run successfully with Clang 17 after the GCC media-sanitizer compilation exceeded the environment time limit. No sanitizer finding was reported by the executed groups.

The construction environment does not contain the complete target WLED/PlatformIO firmware toolchain; therefore a full firmware compile is intentionally not represented as having been performed here. The release builder should still compile the final archive for the selected hardware target before flashing.
