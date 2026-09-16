# Hardware test checklist - 0.9.0-dev.4

## Target

- Adafruit MatrixPortal ESP32-S3
- Native WLED HUB75 backend
- Physical 64x64 panel
- iDotMatrix profile 64x64

## TEXT validation

- [ ] Font/text size 16 still renders
- [ ] Font/text size 32 still renders
- [ ] Font/text size 64 now renders
- [ ] Size-64 fixed text is complete and correctly oriented
- [ ] Size-64 LEFT/RIGHT scroll works
- [ ] Size-64 UP/DOWN scroll is smooth
- [ ] Blink/Breathe/Snowflake/Laser modes remain functional
- [ ] Background colour mode remains functional
- [ ] Long text does not corrupt or truncate subsequent glyph records

## Regression smoke test

- [ ] GIF playback
- [ ] Carousel
- [ ] Clock
- [ ] 32->64 logical scaling
- [ ] 16->64 logical scaling
- [ ] WLED effect handoff
