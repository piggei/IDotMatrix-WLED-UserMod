# Roadmap and TODO

The 0.5.0 BLE foundation is frozen. New features should be added in small,
compilable, testable increments without regressing Wi-Fi/BLE stability.

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

- [ ] Introduce `IDotMatrixProtocol` for framing, validation, command decoding,
  and responses, independent from BLE and WLED.
- [ ] Reassemble logical FA02 packets split across multiple BLE writes; do not
  assume every command fits the current 64-byte queue slot.
- [ ] Introduce `IDotMatrixWLEDAdapter` for state changes and rendering ownership.
- [ ] Map screen on/off to WLED power without losing previous brightness.
- [ ] Map iDotMatrix brightness to WLED master brightness.
- [ ] Map RGB/full-screen commands to WLED solid colour.
- [ ] Add a resolution-independent framebuffer for 16x16, 32x32, and 64x64.
- [ ] Decode graffiti/pixel commands into the framebuffer.
- [ ] Map logical `(x,y)` through WLED's 2D segment/matrix mapping.
- [ ] Define ownership between iDotMatrix realtime content and WLED effects.
- [ ] Add tests using captured packets where practical.

## Target architecture

- [ ] Keep `IDotMatrixBLEServer` limited to transport, connections,
  notifications, and bulk byte delivery.
- [ ] Add `IDotMatrixProtocol` with typed commands/events and no WLED dependency.
- [ ] Add `IDotMatrixRenderer` for framebuffer drawing and local display modes.
- [ ] Add `IDotMatrixWLEDAdapter` for power, brightness, segments, time,
  filesystem, and WLED notifications.
- [ ] Use bounded queues between callbacks and protocol processing.
- [ ] Define memory budgets for classic ESP32, PSRAM targets, and each resolution.

## Clock and text

- [ ] Reuse WLED time/NTP instead of creating a second time service.
- [ ] Implement clock effects and optional 30-second `HH:MM` / 5-second `DD/MM`
  alternation.
- [ ] Implement date display and clock colour/effect parameters.
- [ ] Decode app-rasterized text glyphs without device-side font assumptions.
- [ ] Support marker `0x02` (8x16 bitmap glyphs).
- [ ] Support marker `0x05` (16x32 bitmap glyphs).
- [ ] Preserve SimSun/SimHei behavior: the app supplies ready bitmaps.

## GIF and media

- [ ] Add a separate bulk path; do not pass 4096-byte chunks through the command
  queue.
- [ ] Preserve ACK semantics: `0x01` continue and `0x03` complete.
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

- [ ] Update `HISTORY.md` for every released snapshot.
- [ ] Change protocol documentation only with captures or controlled experiments.
- [ ] Distinguish confirmed protocol facts, WLED behavior, and architecture choices.
- [ ] Never remove confirmed findings without recording the reason and evidence.
- [ ] Keep the related-project list in `README.md` current.
