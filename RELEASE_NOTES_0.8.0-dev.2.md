# 0.8.0-dev.2 - 64x64 development preview

This preview starts the work required to move beyond the hardware-validated
16x16 scope of 0.7.0 while preserving 0.7.0 as the stable baseline.

## What changed

- Logical media validation now accepts 16x16, 32x32 and 64x64 profiles.
- The dedicated GIF animation framebuffer was removed. Animated scanlines are
  rendered directly into the logical canvas, saving 12,288 bytes at 64x64.
- Renderer, RAW and media scratch allocations prefer PSRAM when it is present.
- RAW reception can fall back to the logical canvas when a second full-size
  buffer is unavailable; the canvas stays hidden until CRC validation succeeds.
- AnimatedGIF is allocated lazily rather than occupying boot-time DRAM.
- The default decoder uses an 11-bit low-RAM LZW dictionary. This fully covers
  16x16 and 32x32 frames and provides a practical development mode for 64x64 on
  classic ESP32.
- Defining `IDOT_GIF_LZW12` enables the full 12-bit dictionary required for
  arbitrary 64x64 GIFs. PSRAM is strongly recommended for that configuration.
- Added a compact `mediaError=...` runtime status only when a media operation
  fails, so out-of-memory behavior can be diagnosed without restoring the large
  debug counter set from the 0.7 development cycle.
- Added `platformio_override.ini.hub75`, supplied by the project, for builds
  that enable the WLED HUB75 bus.
- Replaced `.gitignore` with the project-supplied file.

## Validation status

0.7.0 remains the stable, physically validated 16x16 release. This preview has
host-test coverage for the larger renderer/media paths, but 32x32 and 64x64
still require hardware validation before they can be promoted to a stable
release.

A physical 16x16 matrix can be used for early protocol testing: select a 32x32
or 64x64 logical `screenType`, enable `rescale`, save, and reboot. The official
app then talks to the larger logical profile while WLED downsamples the result
to the 16x16 output segment.

## Recommended first test

Start with the default 11-bit build on a classic ESP32. Validate 32x32 first,
then 64x64: text/clock, graffiti/static images, RAW/cloud images, GIF playback,
multiple consecutive GIF replacements, and return to clock/solid content. If a
64x64 media operation cannot allocate enough RAM, WLED should remain alive and
report `mediaError=...` rather than rebooting.
