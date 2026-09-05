# 0.8.0-dev.8

This development build targets the transient RAM-pressure observed with the dev.7 11-bit/32x32 GIF decoder.

The 16x16/10-bit baseline is unchanged. For `IDOT_GIF_LZW11`, the decoder still reads 11-bit LZW codes but no longer reserves a generic 2048-entry dictionary. A 32x32 frame contains at most 1024 output pixels, which bounds the number of dictionary entries that can be created between clear codes; dev.8 therefore reserves 1282 entries and a 1024-byte reverse pixel stack. This saves about 4 KiB of decoder RAM compared with dev.7.

The 11-bit decoder remains on-demand and is released as soon as WLED retakes display ownership. `gifDecoderBytes=` is temporarily exposed in `/json/info` for hardware validation.

Validation sequence: compile with `platformio_override.ini.32x32`, leave runtime profile at 16x16 first, exercise GIFs and then normal WLED effects, and inspect heap/min/largest. If stable, switch to 32x32 + rescale and repeat.
