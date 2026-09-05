# 0.7.1-dev.9

- Keeps the staged GIF activation introduced in dev.8.
- Reduces the post-staging classic-ESP32 DRAM reserve from 12 KiB to 8 KiB.
- The reserve check now reflects that the WLED display effect has already been staged before decoder allocation.
- Adds `gifProbe=<free> largest=<largest> reserve=8192` to `/json/info` after a GIF allocation attempt.
- Keeps the safe full 12-bit / 4096-entry GIF dictionary.
