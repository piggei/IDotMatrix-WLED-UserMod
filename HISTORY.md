# History

## 0.8.0 - 2026-09-07 - Stable feature release

- Kept ESP32-C3 out of the stable target matrix after physical testing exposed
  visible RMT LED flicker/spikes with the BLE-capable framework. C3 support remains
  under investigation for a later development/maintenance release.
- Replaced the fixed default BLE suffix with a stable six-digit value derived
  from the ESP32 eFuse MAC (`IDM-xxxxxx`). User-saved names remain unchanged.
- Decoupled UI/profile capacity (`IDOT_SCREEN_MAX_DIM`) from GIF decoder
  capacity, allowing the validated compact12/cache decoder to serve a 16x16-only
  UI with Rescale hidden.
- Corrected all supplied PlatformIO `custom_usermods` entries to use only the
  external iDotMatrix symlink, preventing accidental AudioReactive inheritance.
- Promoted the behavior validated through `0.8.0-dev.20` to stable 0.8.0 and
  incorporated the final memory-profile, BLE-name, PlatformIO, test,
  partition, release-metadata, and documentation corrections.
- Added seven source-isolated standalone light effects, including deterministic
  one-pixel movement for effects 3, 4, and 5.
- Added countdown, stopwatch, scoreboard, BUILD80 timer artwork, and the
  asynchronous countdown-complete notification.
- Added persistent alarms and up to 32 program/schedule activities with media,
  weekday/time-window rules, WLED-time preference, and BLE-time fallback.
- Added optional non-blocking active-buzzer support, a protected settings-page
  test, repeating alarm sound, and a finite three-groups-of-three program notice.
- Restored all five LEVEL and five FFT Audio/Rhythm visualizers with stream
  reassembly across BLE write boundaries.
- Finalized the Usermod settings layout and bounded ScreenType/Rescale choices to
  the decoder profile compiled by each PlatformIO override.
- Split media profile from hardware target in the supplied WLED 16.0.1
  PlatformIO overrides: normal LZW10/LZW11/LZW12 profiles now include classic
  ESP32 4/8/16 MB, ESP32-WROVER/PSRAM, and ESP32-S3 8/16 MB PSRAM environments.
- Gave every media profile a unique PlatformIO environment suffix
  (`_16x16`, `_32x32`, `_64x64`, `_64x64_lite`) so decoder-profile changes cannot
  reuse the same `.pio/build` or patched AnimatedGIF `.pio/libdeps` namespace.
- Replaced the old HUB75 merge sketch with wrappers around WLED 16.0.1's native
  HUB75 controller/pinout environments, preserving upstream DMA/PSRAM/pin rules.
- Added matching 8/16/32 MB single-app/no-OTA partition tables and static host
  regression checks for every supplied build environment and partition layout.
- Retained the stable 0.7.1 media/memory architecture, including complete
  4096-code LZW12 semantics, the classic-ESP32 compact12/LittleFS cache backend,
  transactional GIF replacement, and low-memory rescale storage.
- Completed the available classic-ESP32 hardware regressions and the complete
  host test suite. PSRAM/direct 64x64, native physical 64x64, and HUB75 DMA remain
  explicitly outside this release's validated hardware matrix.

## 0.8.0-dev.20 - 2026-09-06 - Build-aware resolution choices

- Added a shared compile-time capability policy driven by `IDOT_GIF_MAX_DIM`.
- The standard LZW10 build now exposes only 16x16 and omits/forces off Rescale.
- LZW11 exposes 16x16/32x32; LZW12 exposes 16x16/32x32/64x64, with Rescale
  available only in these multi-profile builds.
- Older larger-profile configurations are reduced to the nearest supported
  value instead of advertising an undecodable resolution.
- Added host coverage for all three compile-time profiles and expanded comments
  in every supplied PlatformIO override.

## 0.8.0-dev.19 - 2026-09-06 - Final settings labels

- Replaced the remaining `Rescale` caption through a DOM-independent nearest-label
  lookup rather than assuming table rows.
- The full test-only rescale description now appears to the left of its checkbox.
- Added consistent trailing colons to Enabled, ScreenType, DeviceName, rescale,
  Buzzer Pin and BuzzerActiveHigh labels.
- Kept the corrected fixed `IDM-` prefix and all dev.18 compatibility behavior.

## 0.8.0-dev.18 - 2026-09-06 - Correct settings field targeting

- Fixed the two dev.17 layout transformations that did not appear in WLED's
  generated form because the controls are not exposed through the assumed IDs.
- The fixed `IDM-` text is now inserted before the actual device-name input.
- The left-hand `Rescale` label is now replaced by the complete sentence
  `Scale the logical profile to the selected WLED 2D segment.`
- Field lookup uses the generated `name` attribute with a row-label fallback;
  BLE, media, rendering and buzzer behavior are unchanged.

## 0.8.0-dev.17 - 2026-09-06 - Settings-page cleanup

- Reworked the Usermod settings layout to remove long side descriptions.
- Screen profile and device-name changes now show dedicated orange warnings
  that both reboot and app reconnection are required.
- The fixed `IDM-` prefix is rendered outside a suffix-only input; legacy full
  names remain accepted and normalized without duplicating the prefix.
- The rescale description is now the checkbox label and documentation defines
  rescale as a test/diagnostic option, not a normal way to display higher-resolution
  content on a smaller physical matrix.
- The buzzer reminder now appears below the test button in orange.

## 0.8.0-dev.16 - 2026-09-06 - Audio/Rhythm restoration

- Restored all five LEVEL and five FFT BUILD80 visualizers under the single
  `iDotMatrix Display` effect.
- Added a 21-byte carry parser for FFT data, including the observed 33-byte BLE
  writes containing one complete frame plus 12 bytes of the next frame.
- Audio uses the existing RGB canvas and adds no framebuffer; 16x16 artwork is
  scaled through the established renderer path for larger and low-memory profiles.
- Added protocol, renderer, adapter, ownership and diagnostics coverage;
  `/json/info` now reports `content=audio LEVEL mode=N` or `content=audio FFT mode=N`.
- Preserved the 0.7.1 GIF/media pipeline, all 4096 LZW12 codes, and dev.15 buzzer timing.
- Hardware validation confirmed all LEVEL/FFT visualizers and the automatic
  return to `iDotMatrix Display`.
- Documentation now treats OTA as intentionally disabled by default: official
  WLED images omit the Usermod, while the validated 4 MB layout has one large
  application slot. Rotation, energy-saving and reset remain WLED-owned policy.

## 0.8.0-dev.15 - 2026-09-06 - Finite program activation sound

- Changed program/schedule audio from a continuous activity-long trill to a finite activation notice: three groups of three short trills.
- Kept alarm buzzer semantics unchanged: when requested by the alarm, the trill repeats for the configured alarm duration.
- Added a dedicated finite `startScheduleAlert()` pattern and explicit alarm/program buzzer ownership so alarms can pre-empt a program notice without causing it to restart afterward.
- Kept the settings-page **Test buzzer** as one group of three beeps and retained GPIO/polarity behavior.
- Removed user-facing `-1` wording for the buzzer pin; the settings UI is intentionally just a pin selector plus polarity/test controls.
- Hardware feedback on dev.14 confirmed the alarm/program path and buzzer were functioning; dev.15 changes only the schedule sound policy plus related documentation/tests.

## 0.8.0-dev.14 - 2026-09-05 - Alarms and programs/schedules

- Added the BUILD80 alarm subsystem: 10 persistent slots, enable/day/one-shot logic, duration, RAW/GIF media and per-alarm buzzer request.
- Added up to 32 persistent program activities with global enable/sound flags, weekday/time windows, midnight crossing, staged list replacement and GIF/PNG/TEXT media.
- Added `IDotMatrixAutomation` with NVS metadata, LittleFS media payloads, WLED local-time preference and BLE-time fallback.
- Connected alarm/program requests to the optional non-blocking active buzzer and added `/json/info` automation diagnostics.
- Removed the redundant buzzer GPIO help sentence from the Usermod settings page.

## 0.8.0-dev.13 - 2026-09-05 - Buzzer configuration test

- Added a **Test buzzer** control to Config → Usermods → iDotMatrix.
- Added an authenticated same-origin POST endpoint used only by that settings control.
- Added a finite one-shot three-beep trill for wiring/polarity validation; the repeating trill remains available for future alarm/schedule playback.
- Kept the buzzer pattern engine running even if BLE startup is blocked by an incompatible RMT LED bus.
- No protocol, GIF, media-cache, timer, light-effect, or override behavior changed.

## 0.8.0-dev.12 - 2026-09-05 - Buzzer foundation and timer artwork

- Added optional active-buzzer hardware configuration to the iDotMatrix usermod (`buzzer-pin`, `buzzerActiveHigh`).
- Added a non-blocking three-pulse trill engine matching the standalone ESP32 server; it is intentionally idle until alarm/schedule support is connected to it.
- Added GPIO validity/conflict checks and `/json/info` buzzer diagnostics. WLED 0.16.x has no collision-safe PinOwner for an out-of-tree usermod, so the module does not impersonate another usermod owner; the config pin is still exposed to WLED's Usermod settings pin scanner and already-allocated pins are refused at runtime.
- Restored the BUILD80 countdown/stopwatch UI: 9x9 orange timer icon with a moving red hand above the MM:SS digits.
- Countdown/stopwatch rendering now keeps millisecond precision for the timer-hand phase while retaining the validated 200 ms refresh cadence.

## 0.8.0-dev.11 - 2026-09-05 - Countdown, stopwatch and scoreboard

- Added the confirmed countdown command `08 80 MODE MINUTES SECONDS` with reset/start/pause/resume semantics from the standalone ESP32 emulator.
- Added the confirmed stopwatch command `09 80 MODE` with reset/start/pause/resume semantics.
- Added the confirmed scoreboard command `0A 80 A_lo A_hi B_lo B_hi`; the wire values remain 16-bit while the verified 16x16 artwork displays the last two decimal digits, matching the standalone reference.
- Added the shared 16x16 `MM:SS` renderer used by countdown and stopwatch; countdown turns red for the final five seconds, stopwatch remains white, and the artwork scales through the existing logical/physical canvas path.
- Added the reference asynchronous countdown-complete FA03 status `05 00 08 80 03`.
- Timer state is retained independently of WLED display ownership, so a countdown/stopwatch continues to advance if the user temporarily selects a native WLED effect; the next timer command from the app can reclaim `iDotMatrix Display` without reconstructing state from WLED.
- Added `/json/info` diagnostics for countdown, stopwatch, scoreboard, and their active content modes.
- Added host regression coverage for command parsing/ACKs, asynchronous completion status, timer pause/resume across WLED takeover, `MM:SS` rendering, scoreboard colours, and logical scaling.
- No changes were made to the 0.7.1 GIF/compact12/cache memory architecture or to the dev.10 light-effect engine.

## 0.8.0-dev.10 - 2026-09-05 - Smooth one-pixel band scrolling and Usermod inheritance

- Fixed standalone light effects 3, 4 and 5 after the first hardware pass showed visible multi-pixel jumps.
- Scrolling effects now advance by exactly one physical canvas pixel for each accepted render; app speed controls only the frame interval, so a delayed WLED loop iteration can slow motion temporarily but cannot skip columns/diagonals.
- Added host regression coverage that verifies no motion before the frame interval and exactly one-pixel motion even after a deliberately late render call.
- Updated the normal 16x16, 32x32, 64x64 and HUB75 PlatformIO overrides to inherit `${env:esp32dev.custom_usermods}` before adding the external iDotMatrix Usermod, preserving WLED's existing/default Usermods.
- Kept `platformio_override.ini.64x64-lite` intentionally isolated from inherited Usermods because the classic-ESP32/no-PSRAM compact12 cache path depends on maximum internal-RAM margin. The inheritance line is included but commented so advanced users can re-enable it explicitly and hardware-test the resulting RAM pressure.
- No changes were made to the 0.7.1 GIF decoder, compact12 cache, RAM guards, BLE transport, or app/WLED source-isolation model.

## 0.8.0-dev.9 - 2026-09-05 - Seven standalone light effects and source-isolated app rendering

- Resumed the 0.8.0 prerelease sequence after the earlier dev.2..dev.8 larger-profile experiments, whose stable memory/media architecture was ultimately released as 0.7.1.
- Added the confirmed FA02 light-effect command `03 02 EFFECT SPEED COUNT [R G B]...`, including the app's 0..127 palette-channel expansion and 16-colour bound.
- Ported all seven standalone light-effect renderers (effect ids 0..6) from the standalone ESP32 emulator into `IDotMatrixRenderer`.
- All app-originated light effects render into the existing bounded RGB canvas and are published only through the single WLED `iDotMatrix Display` effect; no native WLED effect mapping or effect-specific heap allocation is used.
- Changed app-originated full-screen RGB to the same framebuffer model: iDotMatrix Solid no longer rewrites WLED's native `Solid` effect or primary colour. This avoids presenting two unsynchronized apps as if they shared one effect/colour state.
- A WLED-side effect selection remains an explicit source takeover; the next supported iDotMatrix content command reclaims `iDotMatrix Display`. No hybrid iDot/WLED effect state is synthesized.
- Added `content=solid`, `content=light`, and `lightEffect=<id> speed=<n> colors=<n>` runtime diagnostics.
- Added host regressions for protocol parsing/channel scaling, all seven renderers, Solid framebuffer publication, WLED takeover, and iDotMatrix reclaim.
- Kept the complete 0.7.1 GIF/compact12/frame-cache architecture and memory thresholds unchanged. The new effect state is bounded to a 16-colour palette plus small metadata/timestamps and introduces no second framebuffer.

## 0.7.1 - 2026-09-05 - Larger logical profiles and low-memory 64x64 media

- Promoted the final dev.14 behavior to stable 0.7.1; runtime behavior is unchanged apart from the release/build version string.
- Preserved the stable 0.7.0 16x16 low-RAM media path as the default build.
- Hardware-validated the compact 11-bit/32x32 profile on classic ESP32, including repeated GIF playback and decoder release back to WLED.
- Added low-memory logical-to-physical storage so a 64x64 logical profile can use a 16x16 physical RGB canvas (768 B instead of 12,288 B) with `rescale=true`.
- Added safe 64x64 GIF support with all 4096 legal LZW12 codes. PSRAM-capable builds select full AnimatedGIF/direct playback; classic ESP32 without PSRAM selects the compact12/LittleFS frame-cache backend automatically.
- The no-PSRAM compact workspace is 16,128 B on the validated 64x64->16x16 setup, about 4.5 KiB smaller than the 20,660-byte full AnimatedGIF object measured with the same toolchain.
- Added 10 KiB predecode admission reserve plus a 9 KiB wait/yield runtime guard; transient low heap no longer causes one-sample aborts, while sustained pressure still fails cleanly with `gif-ram-reserve`.
- Fixed repeated GIF replacement lifecycle: old cached playback is retired only after the new transfer passes length/CRC validation, and failed post-validation replacement cannot reactivate an empty `iDotMatrix Display`.
- Added black/blank physical staging while the no-PSRAM cache builder internally uses WLED `Static`; the previous primary colour is restored before playback or recovery.
- Final hardware stress on classic ESP32 / physical 16x16 / `64x64-lite` passed large and 100-frame animations, 10+ alternating GIF replacements, clock/date/image/WLED transitions, and live Web UI/`/json/info` access without reboot or WLED Error 8/90.
- Retained runtime build/backend/heap/reset/cache diagnostics because they are useful for the still-pending PSRAM/native-64/HUB75 validation.
- Known release boundary: the PSRAM direct backend, native physical 64x64, and HUB75 DMA coexistence remain to be hardware-validated; aggressive 64->16 text rescale can make thin glyph strokes hard to read.

## 0.7.1-dev.14 - blank GIF staging and pre-release polish

- Kept the dev.12 compact-safe 4096-code LZW12 decoder, LittleFS frame cache, and dev.13 repeat-GIF lifecycle/wait-retry behavior unchanged.
- The classic-ESP32/no-PSRAM precache still switches internally to WLED `Static` for the lowest runtime RAM footprint, but the selected segment primary colour is temporarily forced to black so the physical panel looks off while frames are prepared.
- The previous segment primary colour is saved before staging and restored before `iDotMatrix Display` is activated, before failure recovery, and when WLED/API ownership interrupts a pending cache build. The temporary black state therefore cannot leak into later WLED effects.
- Added host regressions for black staging, colour restoration after successful cached playback, restoration after replacement failure, clock recovery, and user/API cancellation during staging.
- Hardware status entering dev.14: repeated A/B GIF replacement, large/100-frame GIFs, clock -> GIF, WLED effect -> GIF, static image transitions and Web UI responsiveness all passed on the classic ESP32 64x64-lite / physical 16x16 rescale setup.

## 0.7.1-dev.13 - repeat-GIF lifecycle and transient-RAM recovery

- Kept the dev.12 compact-safe 4096-code LZW12 decoder unchanged.
- On the no-PSRAM frame-cache path, a fully validated replacement GIF now retires the previous cached playback immediately; CRC-failed/incomplete transfers still leave the current GIF untouched.
- Replaced the one-sample 9 KiB cache-build abort with a wait/retry policy: transient low heap yields back to WLED/Wi-Fi/BLE and only a continuous 2-second low-heap condition fails with `gif-ram-reserve`.
- Fixed GIF-to-GIF failure recovery so an already-retired previous GIF is not restored as an empty `iDotMatrix Display` effect. Recovery lands on WLED `Static`, matching the manual action that restored subsequent transfers during hardware testing.
- Failed cache preparation from restorable non-GIF iDotMatrix content (clock/text/image/DIY) restores the previous display canvas visibility.
- Added `/json/info` diagnostics `gifCacheWaits=N low=M guard=9216` when the cache builder has deferred frames for transient RAM pressure.
- Added repeated cached-GIF replacement and failure-recovery host regressions.

## 0.7.1-dev.12 - compact-safe 4096-code LZW12 backend

- Added `IDotMatrixCompactGif`, a dedicated classic-ESP32/no-PSRAM decoder for the 64x64 frame-cache path.
- Keeps all 4096 legal 12-bit GIF LZW codes; unlike the unsafe dev.4/dev.5 experiments it does not truncate the dictionary.
- Packs the 4096 prefix entries into 6144 bytes (12 bits each), keeps 4096-byte suffix and reverse-stack tables, and allocates palette/disposal storage in one contiguous workspace.
- With a 16x16 physical canvas the compact workspace is 16128 bytes, roughly 4.5 KiB below the 20660-byte AnimatedGIF object seen on hardware.
- PSRAM-capable ESP32 targets continue to use AnimatedGIF/direct playback; backend selection is automatic at runtime.
- Added a real two-frame 64x64 GIF host test for the compact decoder and frame downscale.

## 0.7.1-dev.11 - guarded frame-cache predecode

- Reduced the classic-ESP32 LZW12 predecode admission reserve from 12 KiB to 10 KiB.
- Added a 9 KiB runtime free-heap guard before every cached GIF frame; if the guard trips, cache construction aborts cleanly, the decoder is released, and WLED remains responsive.
- Keeps the full 4096-entry LZW12 dictionary and the 64x64 -> physical-canvas low-memory rescale.

## 0.7.1-dev.10 - LittleFS GIF frame cache

- Kept the safe full 4096-entry LZW12 decoder for 64x64 GIF correctness.
- Added a classic-ESP32/no-PSRAM frame-cache path: GIFs are predecoded one frame per WLED loop, downscaled into the physical renderer canvas, and written to `/idot_cache.bin` on LittleFS.
- The ~20 KiB decoder is destroyed before `iDotMatrix Display` is activated; cached playback therefore needs only the renderer canvas plus normal file I/O.
- During predecode the adapter temporarily stages WLED `Static` rather than `iDotMatrix Display` to maximize contiguous heap for LZW12 and the network.
- Added `gifCache=building frames=N` / `gifCachedFrames=N` diagnostics and retained `build=...`.
- Added host coverage for the deferred frame-cache ownership transition while preserving direct 16x16/32x32 behavior.

## 0.7.1-dev.9

- Staged GIF activation now allocates the WLED display effect before the large GIF decoder.
- Decoder allocation/open is deferred until a later WLED loop, so heap reserve measurements include effect-side RAM.
- Failed GIF preparation restores the exact previous WLED effect instead of leaving an intermediate display state.
- Keeps the safe full 4096-entry LZW12 dictionary and 64x64 low-memory rescale path.

## 0.7.1-dev.8

- Fixed the dev.7 activation race by staging the WLED display effect first, then deferring decoder allocation/open until a later WLED loop so effect-side RAM was already accounted for.
- Failed preparation restored the previous WLED effect instead of exposing a partial GIF.
- Kept the safe full 4096-entry LZW12 dictionary.

## 0.7.1-dev.7

- Made GIF activation transactional: a completed BLE transfer no longer claims the WLED display effect before filesystem promotion, decoder allocation, and `AnimatedGIF::open()` have succeeded.
- A failed GIF open/reserve check now leaves the current WLED effect untouched instead of reporting `content=gif` for a decoder that never started.
- Added `gifPending=1` to `/json/info` while a received GIF is waiting for asynchronous promotion/open.
- Keeps the safe full 4096-entry LZW12 dictionary and low-memory 64x64 rescale from dev.6.

## 0.7.1-dev.6 - safe LZW12

- Restored the full 4096-entry LZW12 dictionary after hardware stress testing exposed RAM corruption with truncated 64x64 dictionaries.
- Kept low-memory rescale and 1024-byte file I/O buffer.
- Classic ESP32 64x64 GIF validation now uses the lite WLED override while preserving GIF correctness.

## 0.7.1-dev.5

- Added a compact 64x64 LZW12 profile for classic ESP32 testing.
- Keeps 12-bit code-width handling while limiting the physical dictionary to 2560 entries and the reverse pixel stack/cache to 2048 bytes.
- Targets a ~5 KiB decoder reduction versus dev.3 to avoid WLED Error 8 during 64x64 GIF playback.
- 16x16, 32x32, BLE, RAW images and low-memory rescale are unchanged.
- Complex GIFs that require more dictionary entries remain an explicit compatibility limit of this development build.

## 0.7.1-dev.4

- Experimental memory-reduction attempt for 64x64/LZW12: kept 12-bit code-width handling but reduced the physical dictionary to 2560 entries and the reverse stack/cache to 2048 bytes.
- This was later proven unsafe by hardware stress: legal 12-bit codes could exceed the shortened arrays and corrupt RAM. The approach is retained only as historical evidence and must not be reused.

## 0.7.1-dev.3

- Added low-memory rescale storage: when `rescale=true`, the BLE/app profile remains 32x32 or 64x64 while the renderer stores only the physical WLED matrix dimensions. A 64x64 logical profile driving a 16x16 matrix now uses a 768-byte RGB canvas instead of 12,288 bytes.
- RAW 32/64 media are downsampled while BLE chunks are received; the full logical RGB image is no longer buffered when the physical target is smaller. Chunk boundaries may split RGB pixels without affecting the result.
- GIF draw callbacks now sample logical source coordinates directly into the smaller physical canvas. AnimatedGIF still decodes the real 64x64/LZW12 stream, but no full 64x64 RGB framebuffer is retained.
- GIF/PNG validation now checks the logical BLE profile rather than the storage canvas size.
- `/json/info` reports both `profile=...` and `canvas=...` so low-memory rescale is visible during testing.
- Added host regression coverage for 64x64 logical -> 16x16 storage, including RAW chunks split at 509 bytes and sampled animation pixels.

## 0.7.1-dev.2

- Fixed the AnimatedGIF profile patch migration from the compact 11-bit/32x32 profile (`IDOT_LZW11C`) to the 12-bit/64x64 profile.
- Added a regression test for switching profiles without deleting `.pio/libdeps`.
- No media, BLE, renderer, or memory-policy changes from 0.7.1-dev.1.

## 0.7.1-dev.1 - 2026-09-05 - 64x64/LZW12 validation line

- Renumbered the larger-media work toward **0.7.1**. Version 0.8.0 is reserved for later feature/effect work; the previous 0.8.0-dev.N labels below are retained only as experimental history.
- Promoted the compact 11-bit/32x32 path after hardware validation on classic ESP32: clock/text, ten consecutive GIFs, decoder release back to WLED, and no `Effect RAM depleted` failure.
- Kept the validated 16x16/LZW10 and 32x32/LZW11 implementations unchanged.
- Added `platformio_override.ini.64x64` with the full 12-bit/4096-entry GIF dictionary required for arbitrary 64x64 GIFs.
- Reduced only AnimatedGIF's stream cache for the 12-bit profile to 1024 bytes, recovering roughly 3 KiB without reducing the 12-bit dictionary.
- The 12-bit decoder prefers PSRAM. On classic ESP32 without PSRAM it may use internal DRAM only when at least 12 KiB remain reserved for WLED after the decoder allocation; otherwise media fails cleanly with `mediaError=gif-ram-reserve`.
- Added optional `platformio_override.ini.64x64-lite` to measure whether disabling unused WLED integrations provides enough classic-ESP32 margin for 64x64 GIF playback.
- Extended host tests to compile the media path under 10/11/12-bit profiles and to verify the 12-bit patch idempotently.

### Experimental numbering note

The `0.8.0-dev.2` through `0.8.0-dev.8` entries below were internal development builds created while solving larger-profile RAM constraints. Their technical findings remain valid, but the release line is now 0.7.1.

## 0.8.0-dev.8 - 2026-09-04 - Compact 32x32 LZW11 decoder

- Kept the validated 10-bit/16x16 decoder unchanged.
- Reduced the transient 11-bit/32x32 GIF decoder by about 4 KiB versus dev.7.
- A 32x32 frame can output at most 1024 pixels; therefore the LZW dictionary cannot reach the generic 2048-entry 11-bit ceiling. The compact profile retains 11-bit code-width handling while reserving 1282 dictionary entries and a 1024-byte reverse pixel stack.
- Kept the dev.7 ownership fix: dynamic 11-bit storage is released immediately when normal WLED effects regain control.
- Added `gifDecoderBytes=` to development `/json/info` so hardware tests can verify the actual compiled object size.
- Host integration tests and patch idempotence tests pass.

## 0.8.0-dev.7 - 2026-09-04 - Release GIF RAM when WLED takes control

- Fixed a dynamic-decoder retention bug in the 32x32/LZW11 build.
- If the Web UI/API changes the selected segment away from `iDotMatrix Display`, the Usermod now immediately releases iDotMatrix display ownership.
- Active GIF playback is stopped, the dynamic 11-bit decoder storage is freed, content flags are cleared, and the private canvas is hidden before normal WLED effects continue.
- This specifically addresses `Error 8: Effect RAM depleted!` seen after playing a GIF and then browsing ordinary WLED effects.
- Added a host regression test for WLED effect takeover without a BLE content command.

## 0.8.0-dev.6 - 2026-09-04 - Dedicated 32x32 validation build

- Kept the media/renderer implementation from dev.4 unchanged after the 16x16 LZW10 profile passed ten consecutive GIFs without reboot or WLED effect-RAM exhaustion.
- Added `platformio_override.ini.32x32`, a normal BLE/iDotMatrix build with `IDOT_GIF_LZW11` enabled and HUB75 deliberately disabled.
- The 32x32 test build therefore isolates BLE/media memory use from HUB75 DMA memory use.
- The existing HUB75 override remains available as an optional, separate build configuration.
- Updated installation and testing instructions for the 32x32 validation sequence.

## 0.8.0-dev.4 - 2026-09-04 - Profile-sized GIF decoders

- Restored the default GIF decoder to the proven 10-bit/16x16 low-RAM profile used by 0.7.0.
- Added build-time `IDOT_GIF_LZW11` for 32x32 GIFs.
- Added build-time `IDOT_GIF_LZW12` for full 64x64 GIFs.
- 10/11-bit decoder storage is fixed in DRAM to avoid the heap fragmentation observed in dev.2/dev.3.
- The 12-bit decoder is allocated from PSRAM only; no-PSRAM builds fail with `gif-psram-required` instead of starving WLED effect RAM.
- Added `gifDecoder=<bits>bit/<size>x<size>` to the temporary development status output.
- Kept the dev.3 reset/heap flight recorder while larger profiles are being validated.
- Host test suite remains green.

## 0.8.0-dev.3 - 64x64 development

- Packaging follow-up on this experimental line restored the BLE `sendFA03()` and `startAdvertising()` definitions accidentally dropped during the 0.7.0 cleanup.
- Kept 0.7.0 as the stable 16x16 baseline and opened `develop-64x64` for the
  next compatibility step.
- Raised GIF validation from 16x16 to 64x64. The default build moves to an
  11-bit low-RAM dictionary (complete for 32x32); `IDOT_GIF_LZW12` selects the
  full 12-bit dictionary required by arbitrary 64x64 GIF files.
- Removed the dedicated GIF animation framebuffer. AnimatedGIF `playFrame()` is
  synchronous in the WLED loop, so decoded scanlines can safely update the
  logical framebuffer directly; this saves 12,288 bytes at the 64x64 profile.
- Large renderer and RAW buffers now prefer PSRAM on ESP32 when available and
  fall back to internal RAM otherwise.
- GIF decoder storage is allocated lazily, prefers PSRAM, is reused across
  consecutive GIF replacements, and is released when GIF playback ends. An
  allocation failure is therefore a media failure rather than a boot-loop risk.
- Added the project HUB75 PlatformIO override as
  `platformio_override.ini.hub75`.
- Replaced the repository `.gitignore` with the project-supplied version.
- Added a larger-profile regression plan using `rescale` so 32x32/64x64 logical
  behavior can be exercised on a physical 16x16 matrix before larger hardware
  is connected.

## 0.7.0 - 2026-09-04 - Stable media release

- Promoted the 16x16 media branch after physical validation with the official
  iDotMatrix app on classic ESP32/WLED 16.0.1.
- Fixed TEXT speed so the app slider now spans a clearly observable range from
  approximately 500 ms to 15 ms per pixel instead of being effectively capped
  by visual-effect refresh timing.
- Fixed cloud/static-image transport by requesting BLE MTU 517 and matching the
  reference behavior: ATT fragments are reassembled first and are not ACKed as
  independent protocol packets.
- Added a five-second stale-FA02 recovery path so an interrupted media transfer
  cannot poison later clock, colour, or content commands until reconnect.
- Verified RAW/cloud images with complete CRC-valid type-`0x02` transfers.
- Stabilized GIF reception and playback: type-`0x01` streams to LittleFS, is
  promoted outside BLE callbacks, and opens in a later loop iteration.
- Identified the stock AnimatedGIF object's RAM footprint as incompatible with
  WLED + Wi-Fi + NimBLE on the tested classic ESP32. Added a build-local 16x16
  AnimatedGIF patch, reducing the decoder object to about 8 KiB and using fixed
  placement storage to avoid heap fragmentation.
- Earlier stabilization experiments are intentionally recorded here: allocating
  the stock decoder late failed on fragmented heap; placing the full stock
  decoder in static DRAM overflowed `.dram0.bss`; reserving its full footprint
  dynamically at startup/after BLE initialization caused runtime reset loops.
  Reducing the decoder itself was the successful fix.
- Hardware-tested multiple GIFs in sequence, GIF replacement, and transition
  back to clock without reboot, BLE reconnect, or stuck display state.
- Removed development-only packet/CRC/heap/fragment diagnostics from
  `/json/info`; stable status now exposes only connection, profile, name,
  restart requirement, and active content owner.
- Removed temporary PJ debug notes and consolidated installation, hardware
  requirements, architecture, protocol, testing, and roadmap documentation.
- Stable scope is explicitly 16x16. The 32x32/64x64 logical profiles remain
  experimental, and the low-RAM GIF decoder is intentionally capped at 16x16.

This file records stable snapshots and significant experimental builds. Failures
are retained because they define compatibility constraints that should not be
rediscovered later.

## 0.7.0-dev.3 - 2026-09-04 - Media hardening and TEXT speed range

- Kept the complete type-`0x00` PNG, type-`0x01` GIF, type-`0x02` RAW, and
  type-`0x03` TEXT routing introduced by the preceding candidate.
- Widened TEXT movement timing from 140..20 ms to 500..15 ms per pixel so the
  official app's speed-slider extremes are clearly distinguishable.
- Added `textSpeed` and `pngPackets` diagnostics for direct hardware evidence.
- Separated the completed GIF pending slot from the active receive slot. Rapid
  consecutive uploads now use newest-valid-wins semantics without losing or
  confusing the file waiting for deferred promotion.
- Added a sixth host test for GIF streaming, deferred promotion/open, failed
  replacement, rapid replacement, and malformed PNG rejection.

## 0.7.0-dev.2 - 2026-09-03 - Complete image/media candidate

- Added common bulk type `0x01` GIF alongside type `0x02` RAW RGB and type
  `0x03` TEXT.
- GIF chunks stream to alternating LittleFS RX files. A valid completion is
  promoted to a distinct PLAY file outside BLE callback context.
- Recreate `AnimatedGIF` for every upload. Decoder teardown, file promotion,
  decoder open, and playback occur in separate WLED loop iterations without
  `delay()`.
- Added an animation canvas for atomic frame publication.
- Added the newly observed compact type-`0x00` PNG envelope: a 9-byte header,
  little-endian PNG size at offsets 5..8, and PNG at offset 9. This remains an
  experimental inference from the trace `140 = 9 + 131`.
- Ported the reference non-interlaced 8-bit RGB/RGBA PNG decoder. Its miniz
  inflater state is heap allocated to protect the ESP32 loop-task stack.
- Added expected/calculated CRC diagnostics and PNG/GIF counters.
- Retained 0.6.3 as stable; this build requires physical validation.

## 0.7.0-dev.1 - 2026-09-03 - Atomic RAW/cloud images

- Extended the confirmed FA02 bulk transport to type `0x02` RAW RGB while
  retaining type `0x03` TEXT behavior and ACK semantics.
- Added chunk offset/length delivery from the WLED-independent bulk layer so a
  64x64 image does not require a second permanent 12288-byte payload buffer.
- Added a temporary renderer canvas sized exactly to the current logical
  profile: 768, 3072, or 12288 bytes.
- RAW chunks are copied in loop context. The temporary canvas replaces the
  visible framebuffer only after the full transfer passes CRC32 validation.
- A CRC error, malformed transaction, allocation failure, size mismatch, or
  disconnect discards the temporary canvas without publishing partial pixels.
- RAW image content reuses the existing `iDotMatrix Display` effect and the
  existing native/rescale mapping path; no new WLED effect was added.
- Added `content=image`, `rawImages`, `rawRejected`, and `rawBytes` diagnostics.
- Extended protocol, bulk, renderer, and WLED-adapter host tests for RAW begin,
  multi-chunk writes, atomic publication, rejection, and RGB pixel order.
- GIF type `0x01` remains deliberately unsupported in this snapshot; its next
  implementation step will stream alternating RX files into LittleFS before
  any AnimatedGIF decoder is introduced.

## 0.6.3 - 2026-09-03 - Stable text milestone

- Promoted 0.6.3-dev.3 after successful physical testing with the official app.
- Confirmed that a valid TEXT transfer replaces clock content inside the same
  `iDotMatrix Display` effect and renders correctly on the 16x16 matrix.
- Retained marker `0x05` and the complete visual-mode matrix as explicit
  follow-up hardware tests rather than overstating their validation status.

## 0.6.3-dev.3 - 2026-09-03 - App-rasterized text rendering

- Hardware validation of 0.6.3-dev.2 completed three TEXT transfers with valid
  CRC (`bulkChunks=3`, `bulkComplete=3`, `textBytes=54`, no drops/errors).
- Confirmed that 54 bytes represent a 14-byte TEXT header plus two 20-byte
  marker-`0x02` glyph records.
- Added WLED-independent TEXT payload parsing for confirmed marker `0x02`
  (8x16, 16 bitmap bytes) and marker `0x05` (16x32, 64 bitmap bytes).
- Kept the unconfirmed `0x03`/`0x06` aliases rejected rather than presenting
  them as verified protocol.
- Added dynamically allocated glyph bitmap storage in `IDotMatrixRenderer`,
  allocated only from normal loop context and reused when capacity permits.
- Ported fixed/background colours, horizontal and vertical movement, blink,
  pulse, sparkle, laser, and dynamic colour modes from the BUILD 80 renderer.
- Used a lightweight integer wave for the two wave-based dynamic colour modes;
  their appearance is a candidate for hardware comparison with FastLED
  `sin8()` before promotion.
- Preserved LSB-leftmost bitmap orientation and app-side SimSun/SimHei
  rasterization: no device font dependency was introduced.
- TEXT now takes internal ownership of the existing `iDotMatrix Display`
  effect; no additional WLED effect is registered.
- Added `content=text`, `textParsed`, `textParseErrors`, and glyph-size
  diagnostics plus protocol, renderer, and adapter regression coverage.

## 0.6.3-dev.2 - 2026-09-03 - Correct FA02 bulk reassembly

- Hardware testing of 0.6.3-dev.1 produced an app error, `rx=5`, and zero bulk
  chunks. Re-reading BUILD 80 identified the cause: bulk packets are assembled
  and dispatched from FA02; AE01 is only logged by the reference callback.
- Replaced the incorrect AE01 bulk hand-off with reference-matching FA02 logical
  packet reassembly based on the little-endian length in bytes 0..1.
- Preserved the four-entry queue for already-complete short FA02 commands while
  routing fragmented or large logical packets through one bounded 4112-byte
  assembler.
- Kept dispatch, CRC32 work, and FA03 acknowledgements in WLED loop context.
  The callback performs only validation and bounded copies.
- Added unknown-command bytes, fragment counts, current reassembly progress,
  and reassembly-error diagnostics.
- Matched BUILD 80's tolerant ACK for unknown short commands, without falsely
  acknowledging unsupported GIF/RAW bulk content.
- Added a fifth host test for complete, fragmented, oversized, malformed, and
  busy FA02 assembler behavior.
- Corrected all documentation that had assigned the bulk data path to AE01.

## 0.6.3-dev.1 - 2026-09-03 - TEXT bulk transport probe

- Added a dedicated AE01 receive slot sized for an observed 4096-byte payload
  chunk plus its 16-byte bulk header. The existing four-entry 64-byte FA02
  command queue remains unchanged.
- Added a WLED-independent bulk assembler for confirmed type `0x03` TEXT
  transfers, limited to 4096 payload bytes.
- Added repeated-header validation, bounded assembly, streaming CRC32, and the
  confirmed FA03 acknowledgements: `0x01` while incomplete and `0x03` when the
  transaction terminates.
- Increased the AE01 characteristic capacity explicitly; bulk writes are copied
  in the BLE callback but parsed and acknowledged from the normal Usermod loop.
- Added bulk chunk, completion, CRC-error, rejection, oversized-write, and
  completed-text-size diagnostics.
- Added a fourth host test covering a two-chunk transfer, correct payload
  reconstruction, valid/bad CRC, size rejection, and malformed packet length.
- This candidate deliberately does not render text yet. Its purpose is to
  verify actual WLED/NimBLE transfer behavior before glyph parsing is connected
  to `iDotMatrix Display`.
- Hardware result: the app stopped before bulk processing (`bulkChunks=0`). The
  incorrect AE01 routing assumption was fixed in 0.6.3-dev.2.

## 0.6.2 - 2026-09-03 - Stable clock and unified display

- Promoted the 0.6.2 development line after physical-hardware validation of the
  clock and the shared `iDotMatrix Display` effect.
- Confirmed native 16x16 clock output and transitions between clock and
  graffiti without allocating separate WLED effects.
- Includes the configurable 16x16/32x32/64x64 logical profile, strict size
  matching, and optional nearest-neighbour rescale introduced in 0.6.2-dev.1.
- Retains WLED as the clock/timezone authority and retains `Solid` for the
  full-screen RGB command.

## 0.6.2-dev.2 - 2026-09-03 - Unified display effect

- Replaced the separate `iDotMatrix Framebuffer` and `iDotMatrix Clock` effects
  with one `iDotMatrix Display` effect. Graffiti and clock now switch the
  renderer's internal content without consuming another WLED effect entry.
- Kept full-screen RGB mapped intentionally to WLED `Solid`; the next graffiti
  or clock command reclaims `iDotMatrix Display`.
- Confirmed the initial clock implementation on physical hardware with the
  official app and native 16x16 mapping.
- Fixed the sticky `BLE restart required` diagnostic. It is now derived from
  the configured name/profile versus the values active in the BLE stack.
- Observed `name=IDM-666` and `nameActive=IDM-666` while the app continued to
  show an older name. This demonstrates app/OS-side discovery caching rather
  than a Usermod configuration or advertising-name failure.
- Extended the adapter test to prove that clock and graffiti share exactly one
  dynamically registered effect. This remains a candidate pending hardware
  regression of the unified effect.

## 0.6.2-dev.1 - 2026-09-03 - Clock and configurable logical mapping

- Added confirmed `08 00 06 01 FLAGS R G B` clock-command decoding and ACK.
- Added the dedicated `iDotMatrix Clock` WLED effect and explicit display-mode
  ownership transitions between clock, graffiti, and `Solid`.
- Ported the eight reconstructed 16x16 clock styles from the standalone
  reference into the WLED-independent renderer.
- Added app-selected colour, 12/24-hour conversion, and the optional 30-second
  time / 5-second date cycle.
- Reused WLED `localTime`, NTP, timezone, and DST instead of creating another
  clock. The app time packet remains acknowledged but is not applied.
- Added a 16x16/32x32/64x64 profile dropdown, strict dimension matching by
  default, and optional nearest-neighbour logical-to-segment rescale.
- Added `IDM-` name normalization, a 15-byte advertising-safe maximum, and
  diagnostics when a name/profile change requires reboot.
- Extended protocol, renderer, and adapter host tests. The clock was subsequently
  validated on hardware and the effect model was simplified in 0.6.2-dev.2;
  stable 0.6.1 remains the fallback release.

## 0.6.1 - 2026-09-02 - Stable graffiti framebuffer

- Promoted the exact `0.6.1-dev.2` implementation to stable without runtime
  changes after validation with the official app on a physical 16x16 WLED
  matrix.
- Confirmed that DIY/Graffiti selects `iDotMatrix Framebuffer`, clears the
  display, and makes received pixel updates visible.
- Confirmed that full-screen RGB releases the framebuffer and returns WLED to
  `Solid`, while a later graffiti command reclaims the custom effect.
- Confirmed that the previous power, brightness, and full-screen RGB functions
  continue to work.
- Recorded the validation diagnostics: `rx=41`, `dropped=0`,
  `pixelUpdates=34`, `effectFrames=3186`, and `target=16x16` on a selected 16x16
  segment with grouping 1 and spacing 0.
- Retained the unsuccessful `0.6.1-dev.1` overlay approach and the successful
  `0.6.1-dev.2` candidate below so the architectural decision remains explicit.

## 0.6.1-dev.2 - 2026-09-02 - First-class WLED framebuffer effect

- Replaced the unsuccessful `handleOverlayDraw()` output path with a registered
  WLED 2D effect named `iDotMatrix Framebuffer`.
- Hardware diagnostics from `0.6.1-dev.1` proved that BLE transport and graffiti
  parsing were correct (`dropped=0`, increasing `pixelUpdates`), while WLED
  remained in `Solid` and did not display the external overlay.
- The effect now renders the logical RGB canvas from inside WLED's normal effect
  service, where the current segment's virtual XY dimensions and physical matrix
  mapping are valid.
- Entering DIY automatically selects the framebuffer effect. A valid graffiti
  pixel packet also selects it, making the implementation tolerant of app packet
  ordering.
- Full-screen RGB releases framebuffer ownership and returns the selected segment
  to WLED's `Solid` effect.
- Added effect registration, active-state, frame-count and target-dimension
  diagnostics to `/json/info`.
- Confirmed from the test device's JSON state that WLED exposes one selected
  16x16 2D segment with grouping 1 and spacing 0.
- Updated adapter tests for effect registration, activation, rendering, release,
  and pixel-driven reclamation.
- Hardware validation succeeded; this exact implementation was promoted to
  stable 0.6.1.

## 0.6.1-dev.1 - 2026-09-02 - Multisize framebuffer and graffiti candidate

- Added the WLED-independent `IDotMatrixRenderer` with dynamically allocated
  RGB canvases for 16x16, 32x32, and 64x64 profiles.
- Added confirmed DIY enter/exit parsing for `05 00 04 01 STATE` and its standard
  ACK.
- Added confirmed graffiti parsing for `LEN 00 05 01 ? R G B X Y...`, preserving
  the unknown byte and no-ACK behavior documented by the reference.
- Initially routed logical pixels through WLED's `handleOverlayDraw()` and the
  selected 2D segment. Hardware testing showed that packets reached the canvas,
  but WLED remained in `Solid` and the drawing was not displayed; this path was
  replaced in `0.6.1-dev.2`.
- Matched reference behavior: a new DIY session clears the canvas, leaving the
  editor preserves it, and full-screen RGB replaces it.
- Added canvas, content-owner, and accepted-pixel diagnostics.
- Added host tests for all canvas sizes, bounds, protocol extraction, lifecycle,
  and exact RGB-to-XY transfer.
- Kept the stable BLE queue and callback behavior unchanged. Fragmented logical
  commands and writes above 64 bytes remain unsupported.
- This is a hardware-test candidate; 0.6.0 remains the stable release.

## 0.6.0 - 2026-09-02 - Stable basic-command foundation

- Promoted the exact `0.6.0-dev.7` code validated on physical hardware to the
  first stable command-capable release; no runtime behavior was changed during
  consolidation.
- Confirmed discovery and connection by the official iDotMatrix app alongside
  working WLED Wi-Fi control.
- Confirmed initial app power-state initialization through screen-ON-on-connect
  plus two delayed device-information notifications.
- Confirmed screen on/off, brightness, and full-screen RGB control from the app
  to WLED.
- Kept the protocol parser, BLE transport, and WLED adapter as separate layers
  and retained bounded, non-blocking callback-to-loop processing.
- Added consolidated protocol, architecture, testing, installation, limitation,
  and roadmap documentation.
- Documented the current brightness limitation explicitly: the app controls
  WLED, but changes made directly in WLED cannot update the app slider because
  no verified device-to-app brightness-state message is known.
- Documented the current transport limit: fragmented logical FA02 commands are
  not reassembled yet and the command queue uses fixed 64-byte slots.
- Graffiti/framebuffer, clock, text, GIF/media, timers, alarms, and schedules
  remain outside this release.

## 0.6.0-dev.7 - 2026-09-02 - Full-screen RGB

- Added confirmed `07 00 02 02 R G B` decoding and standard ACK.
- Mapped full-screen colour to WLED's static effect and primary RGB colour using
  its normal global-colour update path.
- Preserved WLED's active/selected segment semantics; the standard one-segment
  matrix is filled completely.
- Added protocol and adapter tests for exact RGB channel transfer and static-mode
  selection.
- Documented brightness directionality explicitly: app slider changes update
  WLED, but WLED changes cannot update the app without a confirmed reverse-state
  message.
- Confirmed the two-push connection initialization from `.6` works correctly in
  the official app.

## 0.6.0-dev.6 - 2026-09-02 - Robust app-state initialization

- Found that the single 1.2-second device-info push was timing-sensitive: it
  initialized the app switch in `.4`, but not reliably in the later build.
- Added a second identical, non-blocking device-info push at about 2.5 seconds
  after connection.
- Added cumulative `infoPushAttempts` diagnostics to WLED JSON info.
- Added host-side adapter tests for ON/OFF, remembered brightness, zero, and
  percentage conversion behavior.
- Kept WLED as the only brightness persistence authority; no Usermod brightness
  preference was introduced.
- Did not invent an unconfirmed device-to-app brightness-state packet.

## 0.6.0-dev.5 - 2026-09-02 - WLED master brightness

- Added the confirmed `05 00 04 80 PERCENT` brightness command.
- Clamped protocol input to `0..100` exactly like the standalone reference.
- Added rounded linear conversion from iDotMatrix percent to WLED `0..255`
  master brightness.
- Kept logical screen power separate so brightness changes while OFF do not
  inadvertently power the output on.
- Preserved the last non-zero WLED brightness across zero-brightness and power
  cycles.
- Added standard ACK generation and host tests for normal and over-range values.

## 0.6.0-dev.4 - 2026-09-02 - Delayed device-info push

- Added the second connection action present in the standalone reference:
  schedule an unsolicited device-information notification 1.2 seconds after
  connection.
- Kept scheduling, response construction, and notification delivery outside the
  NimBLE callback.
- Cancelled a pending push when the app disconnects.
- Exposed device-information response construction through the protocol boundary
  so both app requests and connection-time pushes use identical bytes.
- Added host coverage for direct device-information response construction.
- Confirmed in the initial experiment that the delayed push can initialize the
  official app's displayed power switch; later testing showed the timing was not
  yet reliable.

## 0.6.0-dev.3 - 2026-09-02 - Screen ON at connection

- Matched the standalone reference behavior by treating every new app
  connection as a screen-ON event.
- Deferred the actual WLED power change from the NimBLE callback to the normal
  Usermod loop.
- Reverted the `.2` device-information state-byte hypothesis after the official
  app test showed that it did not initialize the switch.
- Restored the confirmed device-information response's final byte to fixed
  `00`.
- Added a host-side test for the connection-to-screen-ON event.

## 0.6.0-dev.2 - 2026-09-02 - Initial power-state experiment

- Experimented with making the device-information response read the current power state through the
  WLED adapter instead of always ending in `00`.
- Experimentally encoded the state in the response's final byte (`00` off,
  `01` on) to test whether the official app uses it to initialize its switch.
- The official-app test showed no effect; the experiment was reverted in `.3`.
- Temporarily extended the host test to cover a live ON state in the
  device-information response.

## 0.6.0-dev.1 - 2026-09-02 - Protocol boundary and screen power

- Added `IDotMatrixProtocol`, independent from BLE and WLED APIs.
- Moved device-information and time-sync response construction out of the BLE
  transport.
- Added strict little-endian packet-length validation for FA02 commands.
- Added host-side protocol tests for device information, power events, standard
  acknowledgements, invalid lengths, and unknown commands.
- Added `IDotMatrixWLEDAdapter` as the only protocol-to-WLED state boundary.
- Implemented the confirmed `05 00 07 01 STATE` screen command and standard
  `05 00 07 01 01` acknowledgement.
- Mapped screen state idempotently to WLED power using WLED's own
  `toggleOnOff()` and `stateUpdated()` paths, preserving the previous brightness.
- No brightness, colour, framebuffer, or graffiti commands are claimed yet.

## 0.5.0 - 2026-09-02 - First stable BLE foundation

- Froze the first experimentally verified baseline.
- Confirmed WLED 16.0.1 remains operational over Wi-Fi.
- Confirmed discovery and successful connection from the official iDotMatrix app.
- Pinned Espressif platform 6.13.x, Arduino-ESP32 2.0.17, ESP-IDF 4.4.7, and
  NimBLE-Arduino 1.4.3.
- Added the required explicit NimBLE dependency to the documented override.
- Retained I2S enforcement, the RMT safety guard, and Wi-Fi modem sleep.
- Documented architecture, constraints, installation, diagnostics, related
  projects, and roadmap.
- No LED command rendering is claimed in this release.

## 0.4.9 - 2026-09-02 - Working NimBLE backend

- Replaced classic Bluedroid with `h2zero/NimBLE-Arduino` 1.4.3.
- Preserved the confirmed FA/AE GATT database and manufacturer data.
- Used compact 16-bit service UUIDs in the advertising packets.
- Kept delayed BLE startup and Wi-Fi modem sleep enabled.
- Confirmed official-app discovery and connection experimentally.
- Found that the symlinked Usermod requires NimBLE explicitly in environment
  `lib_deps`.

## 0.4.8 - 2026-09-02 - Split Bluedroid initialization experiment

- Initialized Bluedroid/GATT early to reserve internal RAM.
- Deferred advertising until after the first Wi-Fi initialization period.
- Still crashed when WLED uninitialized Wi-Fi (`clear_bss_queue` and
  `scan_inter_channel_timeout_process`).
- Rejected as a stable architecture.

## 0.4.7 - 2026-09-02 - Delayed Bluedroid with modem sleep

- Combined delayed BLE initialization with `noWifiSleep = false`.
- Eliminated the intentional Wi-Fi power-save abort.
- Bluedroid then failed while allocating internal queues and buffers
  (`fixed_queue_new`, `vQueueDelete`, and `btm_ble_init`).
- Demonstrated that late Bluedroid initialization was not RAM-safe inside WLED.

## 0.4.6 - 2026-09-02 - Wi-Fi modem-sleep correction

- Forced `noWifiSleep = false` before enabling Bluetooth.
- Removed the coexistence error requiring Wi-Fi modem sleep.
- Early Bluedroid still crashed Wi-Fi scanning in `clear_bss_queue` and
  `scan_inter_channel_timeout_process`.

## 0.4.5 - 2026-09-02 - Early Bluedroid initialization

- Moved BLE initialization back into Usermod setup after the I2S guard.
- Avoided the earlier late `coex_core_enable` path.
- Exposed WLED's default disabled Wi-Fi modem sleep as another incompatibility;
  Wi-Fi deliberately aborted in `pm_set_sleep_type`.

## 0.4.4 - 2026-09-02 - RMT guard and delayed BLE startup

- Detected digital WLED buses using the RMT backend.
- Blocked BLE safely and reported the requirement to select I2S.
- Deferred BLE startup by five seconds.
- Eliminated the RMT-triggered boot loop when the guard was active.
- With I2S selected, delayed Bluedroid initialization aborted in coexistence setup.

## 0.4.0-0.4.3 - 2026-09-02 - Official platform and classic BLE

- Moved from WLED's compact Tasmota framework to official Espressif
  `espressif32@~6.13.0`.
- Reused the standalone emulator's classic `BLEDevice.h` API.
- Added the 4 MB single-app/no-OTA partition table after the image exceeded the
  standard OTA application slot.
- Fixed Arduino-ESP32 2.x `setValue` const-correctness differences.
- Reached a complete link and upload.
- Identified the RMT-HI/Bluetooth-controller interrupt conflict causing a reboot
  loop.

## 0.3.1-0.3.6 - 2026-09-01/02 - NimBLE wrapper experiments

- Added correct WLED `library.json` metadata and `libArchive: false`.
- Tested `esp-nimble-cpp` 2.3.2 and framework include-path injection.
- Encountered missing include paths, wrapper/framework API mismatches, missing
  server symbols, and incompatible HCI/notify functions.
- Rejected `esp-nimble-cpp` 2.x for the Arduino-ESP32 2.0.x build.

## 0.2.0 - 2026-09-01 - Initial BLE extraction

- Extracted the first `IDotMatrixBLEServer` from the standalone reference.
- Added FA/AE services, characteristics, advertising, device information,
  time-sync acknowledgement, and a fixed receive queue.
- Established separation between BLE transport and WLED lifecycle.
- Did not yet have a compatible BLE dependency/framework combination.

## 0.1.0 - 2026-09-01 - Usermod skeleton

- Created the external WLED Usermod package.
- Added registration, enable setting, configuration persistence, loop hook, and
  WLED status output.
- Confirmed the symlinked Usermod compiled, registered, and appeared in WLED.
