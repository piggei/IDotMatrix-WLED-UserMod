# Hardware test checklist - 0.9.0-dev.20

Target: MatrixPortal ESP32-S3 + 64x64 HUB75, official iDotMatrix app.

## Alarm multipart

- Set a new Alarm with a 64x64 media asset large enough to require more than one logical packet.
- Immediately inspect `/json/info`: during transfer `alarmRx` should show `result:receiving`, `chunk:<n>` and increasing `recv:<n>`.
- After the final packet, `alarmRx` should show `result:committed`, `crc:1`, `commit:1` and `recv` equal to total `media`.
- `alarmSlot` must show the newly selected Alarm time rather than the previous slot contents.
- Let the Alarm time arrive and confirm media playback/Alarm duration behaviour.

## Program multipart

- Upload a Program/Schedule containing a large 64x64 media asset.
- Inspect `programRx` for increasing `recv` and final `result:committed`.
- Confirm the activity appears at its configured time and its media is intact.

## Safety / compatibility

- Confirm a small single-packet Alarm still saves and triggers.
- Confirm a small single-packet Program still saves and runs.
- Interrupt one large upload for more than 5 seconds, then verify the previously committed Alarm/Program remains usable.
- Verify Clock, Carousel, GIF and normal WLED effect ownership still operate normally after the tests.
