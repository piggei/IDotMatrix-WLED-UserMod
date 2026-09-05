# 0.7.1-dev.6

Safety correction for 64x64 GIF decoding.

- Restores the complete 4096-entry LZW12 dictionary.
- Retains the 1024-byte file buffer and low-memory rescale canvas.
- Marks dev.4/dev.5 compact LZW12 dictionaries as unsafe: complex GIFs can legally emit codes above the truncated physical table and may corrupt RAM.
- Use `platformio_override.ini.64x64-lite` for classic ESP32 testing; the normal full-WLED profile may still have insufficient runtime RAM for the safe full dictionary.
- `/json/info` reports `build=0.7.1-dev.6`.
