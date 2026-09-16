# iDotMatrix WLED Usermod 0.9.0-dev.22

## Preset / Default support

This development build adds the reverse-engineered **Preset / Default** section from the official iDotMatrix Android app.

Implemented behavior:

- dedicated protocol slots `14..19` (six entries maximum);
- activation command `06/02` with ordered cyclic playback;
- Bulk transport reuse for GIF/image (`type 1`) and TEXT (`type 3`);
- repeated-header multi-packet objects with normal Bulk CRC and `0x01`/`0x03` continuation/final ACK flow;
- staging that does not change the display while uploads are in progress;
- pending-to-active promotion only when `06/02` arrives;
- approximately 3000 ms image/GIF dwell;
- TEXT dwell derived from the existing renderer motion/page timing;
- volatile LittleFS storage with no NVS persistence or boot restore;
- replacement of an active Preset only at the next activation command;
- compact `/json/info` Preset diagnostics.

Preset remains deliberately separate from the persistent 12-slot Carousel bank. Alarm/Program behavior from dev.21 is unchanged.

## Validation status

Host protocol, renderer, Bulk, BLE framing, PlatformIO profile and release-package regressions pass. Hardware validation is still required for mixed-media playback and active-Preset replacement on the MatrixPortal S3.
