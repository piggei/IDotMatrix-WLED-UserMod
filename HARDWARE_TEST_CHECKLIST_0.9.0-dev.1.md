# Hardware Test Checklist - 0.9.0-dev.1

Target: Adafruit MatrixPortal ESP32-S3 + one 64x64 HUB75 panel, WLED 0.17 development line.

## Build and boot

- [ ] Compile `adafruit_matrixportal_esp32s3_idotmatrix_64x64`.
- [ ] Flash without changing the already validated WLED HUB75 panel configuration.
- [ ] Confirm normal WLED boot and Web UI access.
- [ ] Confirm the HUB75 panel continues to display native WLED effects.
- [ ] Confirm `/json/info` reports `release=0.9.0` and `build=0.9.0-dev.1`.
- [ ] Confirm `framework=WLED IDF5/HUB75` and `target=MatrixPortal-S3`.
- [ ] Confirm PSRAM is detected and reported.
- [ ] Confirm `profile=64x64` and `canvas=64x64` on a fresh Usermod configuration.

## BLE/application

- [ ] Confirm BLE advertising appears as `IDM-xxxxxx`.
- [ ] Connect with the Android iDotMatrix app.
- [ ] Confirm Device Info completes and the app identifies a 64x64 target.
- [ ] Confirm reconnect after disconnect.

## Rendering

- [ ] Clock modes render correctly across the full 64x64 logical profile.
- [ ] TEXT renders and scrolls correctly.
- [ ] Graffiti/raw image upload renders correctly.
- [ ] Light/procedural effects render correctly.
- [ ] WLED -> iDotMatrix -> WLED ownership transitions remain clean.

## Media/PSRAM

- [ ] Upload and play a 64x64 animated GIF.
- [ ] Confirm `/json/info` reports `gifDecoder=animatedgif12/psram`.
- [ ] Exercise several GIF replacements.
- [ ] Configure and run a mixed Carousel.
- [ ] Confirm no unexpected LittleFS frame-cache build is used when PSRAM is available.
- [ ] Record free heap, minimum heap and free PSRAM before/after repeated media cycles.

## Automation

- [ ] Alarm playback.
- [ ] Program/schedule playback.
- [ ] Device protocol reset.
- [ ] Reboot persistence.

## Soak

- [ ] Run native WLED effects for at least 30 minutes after adding the Usermod.
- [ ] Run iDotMatrix Carousel for at least 2 hours.
- [ ] Verify BLE reconnect after the soak.
- [ ] Verify WLED Web UI remains responsive throughout.
