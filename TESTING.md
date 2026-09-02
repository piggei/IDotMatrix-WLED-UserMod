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
```

Coverage includes device info, connection ON, power ACKs, malformed/unknown
packets, brightness/clamping/conversion/OFF behavior, RGB/static-mode mapping,
DIY commands, graffiti packet extraction, framebuffer sizes/bounds, persistent
canvas semantics, and WLED custom-effect registration/activation/XY output.

## WLED build validation

```powershell
pio run -e esp32dev_idotmatrix -t clean
pio run -e esp32dev_idotmatrix
```

The dependency graph must include `NimBLE-Arduino @ 1.4.3` and
`wled-usermod-idotmatrix @ 0.6.1`. It must not include `esp-nimble-cpp` or the
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

1. Confirm the WLED effect list contains `iDotMatrix Framebuffer`.
2. Open DIY/Graffiti in the official app; WLED should select that effect and the
   physical matrix should clear.
3. Draw isolated pixels in every corner and confirm orientation.
4. Draw red, green, blue, white, and black strokes.
5. Draw quickly and verify `/json/info` keeps `dropped=0`.
6. Leave DIY and confirm the last drawing remains visible.
7. Send a full-screen RGB command and confirm WLED returns to `Solid`.
8. Re-enter DIY and confirm the previous canvas is cleared.
9. Confirm `canvas=16x16`, `framebufferFx=... active`, increasing
   `pixelUpdates`/`effectFrames`, and `target=16x16`.

The 0.6.1 promotion was validated on hardware through the complete effect
transition `Solid` -> `iDotMatrix Framebuffer` -> `Solid`. The captured status
reported `rx=41`, `dropped=0`, `pixelUpdates=34`, `effectFrames=3186`, and
`target=16x16` after returning to full-screen colour.

## Expected limitations

- WLED brightness changes do not move the app slider.
- Moving the app slider updates WLED.
- Full-screen RGB follows active/selected WLED segments.
- RMT prevents BLE startup and reports a diagnostic.
- The supplied 4 MB layout has no OTA slot; use USB/serial.

## Stable-release regression checks

- all three host tests pass with warnings as errors;
- a clean pinned WLED build succeeds;
- app discovery, connection, power, brightness, RGB, and graffiti pass;
- repeated connections do not increase `dropped`;
- Wi-Fi remains reachable during BLE use;
- README, protocol, history, and TODO report the same version/features.
