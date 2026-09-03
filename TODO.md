# Roadmap and TODO

Version 0.6.2 is the stable foundation: official-app discovery and connection,
initial screen-state initialization, screen on/off, brightness, full-screen
RGB, graffiti, and clock are working on the physical ESP32/WLED target. New features
must be added in small, compilable, testable increments without regressing this
baseline.

## Baseline validation

- [ ] Run for at least 24 hours with Wi-Fi and BLE advertising active.
- [ ] Repeat official-app connect/disconnect cycles and verify advertising restarts.
- [ ] Test Wi-Fi reconnect after access-point loss while BLE remains enabled.
- [ ] Test WLED recovery AP behavior with BLE enabled.
- [ ] Record free heap, minimum free heap, and largest internal block before and
  after BLE initialization and repeated connections.
- [ ] Confirm profiles `0x01`, `0x03`, and `0x04` are discovered correctly.
- [ ] Confirm the RMT guard prevents BLE startup and reports a useful status.

## MVP: basic iDotMatrix control

- [x] Introduce `IDotMatrixProtocol` for framing, validation, command decoding,
  and responses, independent from BLE and WLED.
- [x] Reassemble logical FA02 packets split across multiple BLE writes; do not
  assume every command fits the current 64-byte queue slot.
- [x] Introduce `IDotMatrixWLEDAdapter` for state changes and rendering ownership.
- [x] Map screen on/off to WLED power without losing previous brightness.
- [x] Test whether the final device-info byte synchronizes the app's initial
  screen switch: it does not, so the experiment was reverted.
- [x] Confirm that matching the reference's screen-ON-on-connect behavior keeps
  the app and WLED interaction consistent.
- [x] Confirm that the two delayed device-info pushes initialize the official
  app's displayed power switch reliably across repeated connections.
- [x] Map iDotMatrix brightness to WLED master brightness.
- [x] Validate normal brightness control from the official app on the physical
  WLED matrix and in the WLED web UI.
- [ ] Complete edge-case validation for brightness values `0`, `1`, `100`, and
  over-range clamping.
- [x] Map RGB/full-screen commands to WLED solid colour.
- [x] Validate full-screen RGB control from the official app on the physical
  matrix.
- [ ] Complete the explicit red, green, blue, white, and black regression set
  and verify every result in the WLED UI.

## Next milestone: graffiti and framebuffer

- [x] Add a resolution-independent framebuffer for 16x16, 32x32, and 64x64.
- [x] Decode graffiti/pixel commands into the framebuffer.
- [x] Map logical `(x,y)` through WLED's 2D segment/matrix mapping.
- [x] Validate `0.6.1-dev.1` transport/parser diagnostics on hardware; packets
  reached the framebuffer but its overlay output was not displayed.
- [x] Validate `0.6.1-dev.2` and the first-class `iDotMatrix Framebuffer` effect
  against official-app graffiti on physical hardware; promoted unchanged to
  stable 0.6.1.
- [ ] Confirm corner orientation and RGB/black drawing on a 16x16 WLED matrix.
- [ ] Capture actual graffiti packet sizes and confirm the 64-byte queue does not
  truncate normal app strokes.
- [x] Define ownership between iDotMatrix realtime content and WLED effects:
  app-rendered content selects the single `iDotMatrix Display` effect,
  full-screen RGB selects `Solid`, and a later valid content packet reclaims
  the display effect.
- [ ] Add tests using captured packets where practical.

## Target architecture

- [x] Keep `IDotMatrixBLEServer` limited to transport, connections,
  notifications, and bulk byte delivery.
- [x] Add `IDotMatrixProtocol` with typed commands/events and no WLED dependency.
- [x] Add `IDotMatrixRenderer` for framebuffer drawing and graffiti mode.
- [x] Add `IDotMatrixWLEDAdapter` for the implemented power, brightness, colour,
  and WLED notification paths; extend it only as later milestones require.
- [x] Use bounded queues between callbacks and protocol processing.
- [x] Expose the logical 16x16/32x32/64x64 BLE profile as a dropdown.
- [x] Detect physical dimensions from the WLED 2D segment and block silent
  clipping when strict native-size mode is selected.
- [x] Add opt-in nearest-neighbour rescale for diagnostic previews and unusual
  WLED matrix dimensions.
- [x] Consolidate graffiti and clock into one `iDotMatrix Display` effect so
  future text, media, and timers do not each consume another WLED effect.
- [ ] Validate profile changes, mismatch blocking, and rescale on hardware.
- [ ] Define memory budgets for classic ESP32, PSRAM targets, and each resolution.

## Clock and text

- [x] Reuse WLED time/NTP instead of creating a second time service.
- [x] Implement clock effects and optional 30-second `HH:MM` / 5-second `DD/MM`
  alternation.
- [x] Implement date display and clock colour/effect parameters.
- [ ] Validate all eight styles, 12/24-hour mode, colours, and date cycling on
  physical hardware with the official app.
- [ ] Decide whether an app time packet should optionally seed WLED when NTP is
  unavailable; current behavior deliberately ACKs it without creating a second
  time authority.
- [x] Decode app-rasterized text glyphs without device-side font assumptions.
- [x] Support marker `0x02` (8x16 bitmap glyphs).
- [x] Support marker `0x05` (16x32 bitmap glyphs).
- [x] Preserve SimSun/SimHei behavior: the app supplies ready bitmaps.
- [ ] Validate glyph orientation, every app text motion/effect, background mode,
  dynamic colours, and marker `0x05` on physical hardware.

## GIF and media

- [x] Add a separate bulk path; do not pass 4096-byte chunks through the command
  queue.
- [x] Preserve ACK semantics: `0x01` continue and `0x03` terminated/complete.
- [x] Validate the corrected FA02 type-`0x03` TEXT bulk path and CRC diagnostics
  with the official app: three transfers completed with zero CRC/reassembly
  errors and the observed two-glyph payload contained 54 bytes.
- [ ] Stream uploads to LittleFS instead of buffering complete media in RAM.
- [ ] Use separate RX and PLAY files.
- [ ] Open and play media outside BLE callbacks.
- [ ] Recreate the AnimatedGIF decoder for every new GIF.
- [ ] Validate interrupted transfers and 64x64 GIFs around 80 KB.
- [ ] Evaluate WLED filesystem/image facilities before adding dependencies.
- [ ] Use PSRAM opportunistically, never as a 16x16 requirement.

## Timers, alarms, and schedules

- [ ] Add countdown rendering, red during the final five seconds.
- [ ] Add stopwatch rendering.
- [ ] Capture unknown app ACK/status behavior for countdown and stopwatch.
- [ ] Map alarms/schedules to WLED facilities only where semantics match.
- [ ] Add non-blocking active-buzzer support, initially compatible with GPIO18.
- [ ] Investigate original-device buzzer patterns before claiming parity.
- [ ] Define RTC/time persistence and boot behavior.

## Build and platform work

- [ ] Test ESP32-S3 and PSRAM builds separately.
- [ ] Re-evaluate the platform override against future WLED frameworks.
- [ ] Re-evaluate OTA layouts on boards with 8 MB or more flash.
- [ ] Reintroduce other WLED Usermods one at a time.
- [ ] Test Audio Reactive only after the basic MVP is stable.
- [ ] Add automated compile checks for the pinned environment.

## Documentation discipline

- [x] Publish consolidated `README.md`, `PROTOCOL.md`, `ARCHITECTURE.md`, and
  `TESTING.md`, updated through the 0.6.3-dev.3 candidate.
- [x] Update `HISTORY.md` through every released snapshot up to 0.6.3-dev.3.
- [ ] Change protocol documentation only with captures or controlled experiments.
- [ ] Distinguish confirmed protocol facts, WLED behavior, and architecture choices.
- [ ] Never remove confirmed findings without recording the reason and evidence.
- [ ] Keep the related-project list in `README.md` current.
