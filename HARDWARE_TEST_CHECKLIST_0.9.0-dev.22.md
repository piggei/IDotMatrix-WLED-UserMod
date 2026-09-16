# Hardware test checklist — 0.9.0-dev.22

Target: MatrixPortal ESP32-S3 + 64x64 HUB75, official Android iDotMatrix app.

## Preset / Default

- [ ] Two image/GIF entries in slots 14 and 15 cycle `14 -> 15 -> 14` at about 3 s each.
- [ ] Mixed Preset: static TEXT, image/GIF, scrolling TEXT.
- [ ] Static TEXT remains visible for about 3 s after its final page is reached.
- [ ] Horizontal scrolling TEXT completes its visual pass before advancing.
- [ ] At least one media object larger than 4096 bytes receives continuation ACK `0x01` and final ACK `0x03` without restarting staging.
- [ ] Five or six entries exercise the full slot range 14..19 and preserve requested order.
- [ ] Uploading a replacement Preset while one is already playing does not partially switch the visible playlist.
- [ ] New playlist starts from its first slot only after the new `06/02` command.
- [ ] BLE disconnect does not corrupt an already active Preset.
- [ ] Reboot clears the volatile Preset bank and does not restore it.
- [ ] Protocol reset clears Preset files/state.

## Regressions

- [ ] Carousel slots 0..11 still upload/play/restore normally.
- [ ] Alarm media still upload and trigger.
- [ ] Program/Schedule multipart media still upload with `01 -> 03` flow control.
- [ ] Live TEXT and live GIF/image remain immediate-display paths outside Preset slots.
- [ ] Clock, Countdown, Stopwatch and Scoreboard still take display ownership and stop Preset playback as expected.
- [ ] Switching to a native WLED effect stops autonomous Preset playback.
