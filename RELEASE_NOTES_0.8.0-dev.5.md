# 0.8.0-dev.5

32x32 validation build based on the dev.4 memory architecture.

## What changed

- No media-path code change from dev.4.
- Added `platformio_override.ini.32x32` with `IDOT_GIF_LZW11`.
- HUB75 is intentionally not enabled in this override, keeping the test focused on the memory footprint of WLED + Wi-Fi + NimBLE + iDotMatrix media.
- Documentation now distinguishes the normal 16x16 override, the 32x32 test override, and the optional HUB75 override.

## Validation target

Use a physical 16x16 matrix if desired, set the iDotMatrix Usermod to `screenType=32x32` with `rescale=true`, and verify text, clock, static images, then at least ten GIFs. `/json/info` should report `gifDecoder=11bit/32x32` and retain a healthy largest free block without `Effect RAM depleted`.
