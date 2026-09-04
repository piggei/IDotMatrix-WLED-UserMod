# Testing

This is the release-validation procedure for 0.7.0.

## Host regression tests

From the repository root with a C++11 compiler and zlib development files,
the complete suite can be run with:

```sh
./run_host_tests.sh
```

Equivalent individual commands are:

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

g++ -std=c++11 -Wall -Wextra -Werror -pedantic \
  -Itests/media_stub \
  IDotMatrixRenderer.cpp IDotMatrixMedia.cpp tests/test_media.cpp \
  -lz -o /tmp/idotmatrix_media_test
/tmp/idotmatrix_media_test
```

Coverage includes protocol framing, ACK behavior, power/brightness/RGB, DIY and
graffiti parsing, clock/text rendering, 2D mapping, bulk CRC32, RAW atomic
publication, FA02 fragmentation, compact PNG decode, and deferred GIF
reception/promotion/playback behavior.

## WLED build validation

From the WLED source directory:

```powershell
pio run -e esp32dev_idotmatrix -t clean
pio run -e esp32dev_idotmatrix
```

Expected dependency/build facts:

- `wled-usermod-idotmatrix @ 0.7.0`;
- `NimBLE-Arduino @ 1.4.3`;
- `AnimatedGIF @ 1.4.7`;
- build output contains the low-RAM 16x16 AnimatedGIF patch message;
- no `.dram0.bss` overflow;
- no dependency on `esp-nimble-cpp` or `ESP32 BLE Arduino`.

## Stable 16x16 hardware regression

Use a classic ESP32, a WLED 16x16 2D matrix, and I2S LED output.

1. Flash by USB/serial and reboot.
2. Confirm WLED remains reachable over Wi-Fi for at least 15 seconds.
3. Confirm `/json/info` reports `BLE advertising` under `u.iDotMatrix`.
4. Connect with the official iDotMatrix app.
5. Verify power OFF/ON and brightness changes.
6. Verify red, green, blue, white, and black full-screen colours.
7. Enter DIY/Graffiti, draw pixels, and display a saved Graffiti image.
8. Test a clock style and verify WLED/NTP time is shown.
9. Send scrolling text with the speed slider at both extremes and verify an
   obvious slow/fast difference.
10. Browse/send multiple cloud/static images and verify they appear without app
    transfer errors.
11. Send at least ten GIF animations in sequence. Verify every selected GIF can
    replace the previous animation without a reboot or BLE disconnect.
12. While a GIF is playing, return to clock and then to a static colour. Verify
    normal operation without reconnecting.
13. Disconnect the app and verify BLE advertising resumes.
14. Reconnect and repeat one image and one GIF transfer.

The debug counters used during development are intentionally absent from 0.7.0;
release testing is behavior-based plus the compact runtime status.

## Recovery tests

- Interrupt or cancel a media transfer and then send a normal command. The
  device must recover without requiring a BLE reconnect.
- Disconnect during a transfer. Reconnect and verify the next valid command
  works.
- Change `deviceName` or `screenType`; verify `/json/info` reports that a restart
  is required until reboot.
- Configure a digital RMT bus and verify the Usermod refuses to start BLE rather
  than entering the known Bluetooth/RMT reboot loop.

## Experimental profiles

32x32 and 64x64 are not release-gating targets for 0.7.0. Test them only as
development profiles. In particular, GIF playback is intentionally limited to
16x16 by the low-RAM AnimatedGIF build.
