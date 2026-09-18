# iDotMatrix WLED Usermod 0.9.0-rc.4

RC4 is a focused TEXT transport correction discovered while comparing the release candidate with the B154 emulator/original-device behavior.

## Full 64-pixel TEXT capacity

The 32x64 font path already existed in the parser and renderer, but the common Bulk transport still limited TEXT objects to 4096 bytes. That allowed only 15 complete 32x64 glyph records before the transport rejected the object.

RC4 raises the TEXT object limit to **16654 bytes**, matching the complete original-device format:

- 14-byte TEXT global header;
- up to 64 glyphs;
- 4-byte glyph metadata + 256-byte bitmap per 32x64 glyph;
- `14 + 64 x 260 = 16654` bytes.

The correction applies consistently to:

- live/common Bulk TEXT;
- persistent Carousel TEXT playback;
- volatile Preset / Default TEXT playback.

## Memory policy

RC4 does not replace the previous 4096-byte buffers with three permanent 16654-byte arrays. Large TEXT memory is temporary:

- Bulk allocates the declared TEXT size only for the active transfer/result;
- Carousel and Preset allocate scratch memory only while reading stored TEXT for playback;
- ESP32 builds prefer PSRAM when present and fall back to internal heap;
- host builds use normal heap allocation.

## Regression coverage

New tests cover:

- a complete 16654-byte TEXT transfer split into 4096-byte chunks with CRC32 validation;
- rejection of 16655-byte TEXT objects;
- full-size 64-glyph 32x64 TEXT playback from Carousel;
- full-size 64-glyph 32x64 TEXT playback from Preset.

No renderer geometry, BLE command format, clock, timer, audio, GIF or image behavior is changed in this RC.
