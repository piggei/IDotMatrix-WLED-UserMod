# Testing

## Host tests

From the repository root, with a C++11 compiler:

```sh
g++ -std=c++11 -Wall -Wextra -Werror -pedantic \
  IDotMatrixProtocol.cpp tests/test_protocol.cpp \
  -o /tmp/idotmatrix_protocol_test
/tmp/idotmatrix_protocol_test

g++ -std=c++11 -Wall -Wextra -Werror -pedantic \
  IDotMatrixRenderer.cpp tests/test_renderer.cpp \
  -o /tmp/idotmatrix_renderer_test
/tmp/idotmatrix_renderer_test

g++ -std=c++11 -Wall -Wextra -Werror -pedantic \
  -Itests/wled_stub \
  IDotMatrixRenderer.cpp IDotMatrixWLEDAdapter.cpp tests/test_wled_adapter.cpp \
  -o /tmp/idotmatrix_adapter_test
/tmp/idotmatrix_adapter_test

g++ -std=c++11 -Wall -Wextra -Werror -pedantic \
  IDotMatrixBulkTransfer.cpp tests/test_bulk_transfer.cpp \
  -o /tmp/idotmatrix_bulk_test
/tmp/idotmatrix_bulk_test

g++ -std=c++11 -Wall -Wextra -Werror -pedantic \
  IDotMatrixFA02Assembler.cpp tests/test_fa02_assembler.cpp \
  -o /tmp/idotmatrix_fa02_test
/tmp/idotmatrix_fa02_test
```

Coverage includes device info, connection ON, power ACKs, malformed/unknown
packets, brightness/clamping/conversion/OFF behavior, RGB/static-mode mapping,
DIY commands, graffiti packet extraction, framebuffer sizes/bounds, persistent
canvas semantics, and WLED custom-effect registration/activation/XY output.
It also covers clock flags and ACK, 16-to-32 logical clock scaling, shared WLED
display-effect ownership, registration of exactly one custom effect, strict
mismatch blocking, and opt-in nearest-neighbour rescaling.
The bulk test covers multi-chunk reconstruction, continue/complete status,
valid and invalid CRC32, payload-size rejection, and malformed framing.
The FA02 assembler test covers complete and fragmented logical packets,
oversized/malformed starts, and protection of a completed pending packet.

## WLED build validation

```powershell
pio run -e esp32dev_idotmatrix -t clean
pio run -e esp32dev_idotmatrix
```

The dependency graph must include `NimBLE-Arduino @ 1.4.3` and
`wled-usermod-idotmatrix @ 0.6.3-dev.3`. It must not include `esp-nimble-cpp` or the
registry package `ESP32 BLE Arduino`.

## Hardware smoke test

1. Configure every digital LED output for I2S.
2. Upload over USB/serial and reboot.
3. Confirm WLED remains reachable over Wi-Fi.
4. Check `/json/info` for `BLE advertising` and `dropped=0`.
5. Connect with the official app and confirm its power switch initializes ON.
6. Confirm `infoPushAttempts` increases by two.
7. Test OFF/ON and previous-brightness restoration.
8. Test app brightness at 100%, 50%, minimum, and 0% if exposed.
9. Test red, green, blue, white, and black full-screen colours.
10. Confirm WLED shows static effect and the corresponding colour.
11. Disconnect and confirm advertising restarts.

### Additional 0.6.1 graffiti checks

1. Confirm the WLED effect list contains `iDotMatrix Display`.
2. Open DIY/Graffiti in the official app; WLED should select that effect and the
   physical matrix should clear.
3. Draw isolated pixels in every corner and confirm orientation.
4. Draw red, green, blue, white, and black strokes.
5. Draw quickly and verify `/json/info` keeps `dropped=0`.
6. Leave DIY and confirm the last drawing remains visible.
7. Send a full-screen RGB command and confirm WLED returns to `Solid`.
8. Re-enter DIY and confirm the previous canvas is cleared.
9. Confirm `canvas=16x16`, `displayFx=... active`, increasing
   `pixelUpdates`/`effectFrames`, and `target=16x16`.

The 0.6.1 promotion was validated on hardware through the complete effect
transition `Solid` -> `iDotMatrix Framebuffer` -> `Solid` before the effect was
renamed and unified as `iDotMatrix Display`. The captured status
reported `rx=41`, `dropped=0`, `pixelUpdates=34`, `effectFrames=3186`, and
`target=16x16` after returning to full-screen colour.

### Additional 0.6.2-dev.2 clock and mapping checks

1. Confirm Usermod settings show a 16x16/32x32/64x64 `screenType` dropdown and
   a `rescale` checkbox.
2. Leave 16x16 selected with rescale disabled and confirm `/json/info` reports
   `mapping=native` on the 16x16 test matrix.
3. Select Clock in the official app and confirm WLED selects
   `iDotMatrix Display`.
4. Move between Graffiti and Clock and confirm WLED stays on the same
   `iDotMatrix Display` effect ID while `content` changes.
5. Verify all eight styles, selected colours, and 12/24-hour conversion.
6. Enable date display and verify 30 seconds of `HH:MM` followed by 5 seconds
   of `DD/MM`.
7. Confirm the displayed time follows WLED timezone/NTP rather than maintaining
   a second Usermod clock.
8. Set profile 64x64, reboot, and leave rescale disabled: the mismatched 16x16
   target must remain black and report `mapping=mismatch blocked`.
9. Enable rescale without reboot: the 64x64 logical content must appear as a
   lossy 16x16 preview and report `mapping=rescale`.
10. Return to profile 16x16 before stable-release regression testing.

### Additional 0.6.3-dev.3 TEXT rendering checks

1. Open the app text editor, enter a short string, and send it.
2. Confirm WLED remains on `iDotMatrix Display`, changes to `content=text`, and
   displays the app-rasterized glyphs.
3. Confirm `bulkChunks`, `bulkComplete`, and `textParsed` increase together.
4. Confirm `bulkCrcErrors=0`, `bulkRejected=0`, `reassemblyErrors=0`,
   `unknown=0`, and `dropped=0`.
5. Confirm `textParseErrors=0`, and record `textBytes` plus `textGlyphs`.
6. Repeat with enough text to require more than one chunk; every incomplete
   chunk must receive status `0x01` and the final chunk status `0x03`.
7. Test fixed colour, background, left/right/up/down movement, blink, pulse,
   sparkle, laser, and every dynamic colour mode exposed by the app.
8. On a 32x32 logical profile, test a marker-`0x05` 16x32 glyph payload.

## Expected limitations

- WLED brightness changes do not move the app slider.
- Moving the app slider updates WLED.
- Full-screen RGB follows active/selected WLED segments.
- RMT prevents BLE startup and reports a diagnostic.
- The supplied 4 MB layout has no OTA slot; use USB/serial.

## Stable-release regression checks

- all five host tests pass with warnings as errors;
- a clean pinned WLED build succeeds;
- app discovery, connection, power, brightness, RGB, and graffiti pass;
- repeated connections do not increase `dropped`;
- Wi-Fi remains reachable during BLE use;
- README, protocol, history, and TODO report the same version/features.
