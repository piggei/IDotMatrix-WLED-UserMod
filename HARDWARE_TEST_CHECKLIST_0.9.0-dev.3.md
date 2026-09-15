# Hardware Test Checklist - 0.9.0-dev.3

## Build/boot

- [ ] Build `adafruit_matrixportal_esp32s3_idotmatrix_64x64`.
- [ ] Confirm `/json/info` reports `release=0.9.0` and `build=0.9.0-dev.3`.
- [ ] Confirm HUB75 remains 64x64 / 4096 pixels and PSRAM is detected.

## TEXT vertical motion

- [ ] Profile 64x64: UP enters one row at a time from the bottom edge.
- [ ] Profile 64x64: DOWN enters one row at a time from the top edge.
- [ ] Test slow, medium and fast speed values.
- [ ] Confirm no block of rows appears at once at page boundaries.
- [ ] Confirm no blank full-frame transition between pages.

## Scaling regression

- [ ] Profile 32x32 on physical 64x64 remains correct (`scale=auto-up:2x2`).
- [ ] Profile 16x16 on physical 64x64 remains correct (`scale=auto-up:4x4`).
- [ ] LEFT/RIGHT TEXT remains unchanged.
- [ ] GIF, static images and Carousel remain unchanged.
- [ ] WLED -> iDotMatrix -> WLED ownership transitions remain clean.
