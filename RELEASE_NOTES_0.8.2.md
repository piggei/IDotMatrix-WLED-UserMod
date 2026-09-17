# iDotMatrix WLED Usermod 0.8.2

**Release:** 0.8.2  
**Build:** 0.8.2  
**Date:** 2026-09-13  
**Status:** stable

Release 0.8.2 is the stable culmination of the RC2-RC7 convergence and audit cycle on the existing ESP32 / ESP32-C3 hardware line. It adds persistent Device Assets/Carousel behavior, optional WLED AudioReactive input, protocol/reset convergence and substantial transport/storage hardening without changing the captured official-app BLE command model.

## Final stable delta from RC7

- The public WLED custom-effect name is now **`iDotMatrix`** instead of `iDotMatrix Display`.
- Explicit iDotMatrix display acquisition now terminates any active WLED playlist and cancels a playlist preset already queued for asynchronous application before selecting the framebuffer effect, preventing WLED's deferred preset handler from immediately stealing the display back.
- Runtime identification is now `release=0.8.2`, `build=0.8.2`.
- Documentation, package contracts, test procedures, history and roadmap have been consolidated for the stable release.
- No BLE wire command, captured ACK value, Carousel format, persistent-state format or renderer behavior is intentionally changed by this final promotion.

## Major additions since 0.8.1

### Persistent Device Assets / Carousel

- Persistent 12-slot Device Assets bank with mixed GIF and TEXT slots.
- App-provided dwell time per slot and automatic playback after upload quiet time.
- Stored Carousel can be selected directly through the single WLED `iDotMatrix` effect and can be restored at boot when that effect is in the WLED boot preset.
- Carousel replacement holds iDotMatrix ownership while the new bank is uploaded, preventing Clock or another procedural renderer from flashing through the transition.
- Invalid assets are quarantined for the current bank generation so later valid slots continue to play.

### GIF cache and storage robustness

- Persistent Carousel GIFs use per-slot frame caches and reuse them on later rotations instead of rebuilding equivalent LittleFS content on every visit.
- Cached transient GIF replacement is transactional: the previous known-good source/cache pair remains recoverable until the new candidate is fully playable and committed.
- Feature-owned temporary/backup files are reconciled at boot.
- Runtime filesystem reserve/capacity checks prevent protocol size ceilings from being confused with actual device capacity.
- Bulk and partial FA02 transfers expire after inactivity instead of remaining indefinitely staged.

### Framebuffer ownership fixes

- Procedural renderers such as Clock, animated TEXT, scroll, rainbow, snow/laser effects and Audio no longer continue writing into the shared framebuffer while a following GIF is being decoded/cached.
- GIF dwell time starts when playback is actually active, not while a first-use cache is being built.
- Carousel-to-Carousel replacement keeps logical iDotMatrix ownership through the upload gap and uses a neutral/blank transition rather than falling back to Clock.

### BLE/FA02 ownership and diagnostics

- NimBLE callbacks only enqueue bounded complete ATT writes.
- The WLED loop is the sole owner of FA02 assembly, dynamic reassembly storage, timeout handling and transfer cancellation.
- A complete ATT write can finish one logical FA02 packet and begin the next without silently discarding trailing bytes.
- Diagnostics expose RX drops, oversize/malformed frames and FA02/bulk timeouts when relevant.
- Normal commands such as Reset and Carousel setup are no longer consumed by sticky Audio/Rhythm routing.

### Reset, alarms and programs

- Protocol Reset (`03 80`) removes Usermod-owned Carousel, alarm and program/schedule state without rebooting WLED or erasing normal WLED configuration.
- Open Carousel GIF/cache media is closed before reset/configure storage mutation, allowing physical files to be removed reliably.
- `/json/info` reports aggregate reset status and independent `carousel:ok|fail` / `automation:ok|fail` results.
- Alarms and programs/schedules retain persistent metadata/media and active-buzzer integration.

### AudioReactive input

Three runtime sources are available:

- **Phone / BLE** — original official-app stream, still the default;
- **WLED AudioReactive** — uses WLED AudioReactive processed local audio data;
- **Auto** — prefers local AudioReactive data and falls back to Phone/BLE.

The Usermod does not create a second I2S/FFT pipeline. It consumes WLED AudioReactive's existing processed data. The dedicated `platformio_override.ini.c3-audio` profile includes both Usermods.

Hardware validation confirmed local microphone acquisition and Audio/Rhythm visualization. Effect-selection commands remain driven by the official iDotMatrix app; if the app does not send an Audio/Rhythm selection while music is stopped, the Usermod cannot infer that selection independently.

### Clock renderer

- Styles 0, 3, 5, 6 and 7 move the HH:MM colon two pixels right.
- Style 4 moves the HH:MM colon one pixel left.
- Style 2 moves both hour digits and the first minute digit one pixel left while keeping the separator and second minute digit fixed.
- HH:MM colon pixels blink on a one-second cycle (500 ms visible / 500 ms hidden).
- DD/MM separators remain steady and retain their validated date positions.

## WLED integration

All app-originated visual content is exposed through one custom WLED effect named **`iDotMatrix`**. WLED remains authoritative for normal WLED effects, 2D segments, presets, playlist creation/execution, brightness, JSON/HTTP APIs, Home Assistant and network realtime protocols.

Selecting another WLED effect is an explicit source takeover. Selecting or reclaiming `iDotMatrix` resumes a stored Carousel when available, otherwise the local Clock fallback is used. If a WLED playlist is active at that moment, it is terminated and any already queued playlist preset is cleared before the framebuffer effect is selected. This follows WLED's existing rule that a direct effect change terminates the active playlist while accounting for WLED's asynchronous preset queue; the Usermod does not attempt to pause and later resume playlist position.

## Compatibility/security note

The compatibility GATT profile remains intentionally unauthenticated because that matches the observed official-app/original-device exchange. A nearby BLE peer can therefore issue supported iDotMatrix commands, including persistent/reset operations. CRC fields provide integrity checking, not authentication. Mandatory pairing/encryption was not added because it would break compatibility unless the official client performs the same procedure.

## Hardware qualification summary

The stable release retains the qualified classic ESP32 and ESP32-C3 platform baselines documented in `BUILD_PROFILES.md`. On the ESP32-C3 16x16 target, final hardware validation covered:

- BLE connection and normal iDotMatrix app control;
- images, animated GIFs and persistent mixed Carousel playback;
- Carousel cache build/reuse and boot restore;
- Carousel-to-Carousel replacement without Clock flash-through;
- Clock styles 0..7 and blinking separators;
- protocol Reset with `status:ok carousel:ok automation:ok` and storage reclamation;
- alarms and programs/schedules;
- local WLED AudioReactive input from an external microphone;
- normal boot behavior and persisted animation startup.

ESP32-S3, native physical 64x64, HUB75 and the unclassified third 64x64 TEXT format remain outside the 0.8.2 release gate and are reserved for the 0.9 hardware line.

## Build/version compatibility

Application-facing Device Info bytes remain `00 08`. The public library version and runtime release/build are all `0.8.2`.

See `TESTING.md`, `HISTORY.md` and `TODO.md` for the consolidated regression procedure, qualification status, development history and deferred work.
