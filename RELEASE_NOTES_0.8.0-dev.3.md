# 0.8.0-dev.3

Diagnostic prerelease based on dev.2.

- Adds ESP32 reset reason to `/json/info`.
- Adds current free/minimum heap and largest free 8-bit block.
- Adds a small RTC-RAM flight recorder sampled every 250 ms. After a non-power-on reboot, `/json/info` reports the last sampled pre-reset heap, largest block, uptime and active media class.
- No protocol, renderer, BLE, GIF or RAW media behavior is intentionally changed from dev.2.
