# Hardware test checklist - 0.9.0-dev.21

1. Flash the MatrixPortal S3 / HUB75 64x64 target and confirm `/json/info` reports `build=0.9.0-dev.21`.
2. Create one Program with one small single-packet image/GIF; confirm upload completes and the activity runs.
3. Create one Program with one large >4096-byte media asset; confirm `programRx` shows marker `0x00` then `0x02`, monotonically increasing `recv`, ACK status `1` while incomplete, then status `3` at commit.
4. Create a Program containing at least two activities, one small and one large; confirm both are retained and execute in their configured windows.
5. Verify no large activity stalls after the first ~4096 bytes.
6. Retest a large Alarm; Alarm ACK behavior must remain unchanged and the Alarm must trigger normally.
7. Regression-check Carousel, normal GIF, TEXT, Clock, Countdown and Stopwatch.
