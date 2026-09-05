# 0.7.1-dev.2

Build-script fix only.

The 32x32 compact AnimatedGIF profile leaves `.pio/libdeps` containing `codesize < 11 /* IDOT_LZW11C */`. The 0.7.1-dev.1 patch script did not list that exact form as a valid source when migrating to the 64x64/LZW12 profile, causing PlatformIO to stop with `unsupported 1.4.7 source (maximum code size)`.

0.7.1-dev.2 recognizes the compact 11-bit marker and can upgrade it directly to the 12-bit profile without manually deleting `.pio/libdeps`. A regression test covers the 11C -> 12C transition. Runtime code is unchanged.
