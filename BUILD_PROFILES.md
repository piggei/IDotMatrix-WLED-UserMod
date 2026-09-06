# Build profiles and hardware targets

Version 0.8.0 separates two independent build choices that were previously
combined in the single `esp32dev_idotmatrix` example:

1. **iDotMatrix media profile** — the largest logical GIF/protocol resolution
   compiled into the Usermod (`16x16`, `32x32`, or `64x64`);
2. **WLED hardware target** — MCU family, flash size, PSRAM configuration, and,
   for HUB75, the controller/pinout selected by WLED.

The supplied profiles target **WLED v16.0.1**. They deliberately inherit WLED's
board definitions, flash/PSRAM modes, USB settings, HUB75 flags, and board
pinouts instead of duplicating them in this repository.

## Media profiles

| Override file | Decoder | Maximum logical profile | Normal purpose |
|---|---|---:|---|
| `platformio_override.ini.example` | LZW10 | 16x16 | smallest stable baseline |
| `platformio_override.ini.32x32` | compact LZW11 | 32x32 | 16x16/32x32 protocol and media |
| `platformio_override.ini.64x64` | complete LZW12 | 64x64 | normal 64x64-capable build; PSRAM direct path or no-PSRAM cache selected at runtime |
| `platformio_override.ini.64x64-lite` | complete LZW12 | 64x64 | classic ESP32 low-internal-RAM build with optional WLED integrations removed |
| `platformio_override.ini.hub75` | complete LZW12 | 64x64 | WLED HUB75 output plus iDotMatrix BLE/media; experimental in this project |

The compiled maximum controls what the Usermod settings page exposes. The
hardware target does not change the protocol profile by itself.

## Standard hardware targets

The normal `example`, `32x32`, and `64x64` override files contain the same seven
environments:

| iDotMatrix environment | WLED v16.0.1 base environment | Flash | PSRAM | Project hardware validation |
|---|---|---:|---|---|
| `esp32dev_idotmatrix` | `esp32dev` | 4 MB | no | **validated baseline** |
| `esp32dev_8M_idotmatrix` | `esp32dev_8M` | 8 MB | no in the generic WLED target | pending |
| `esp32dev_16M_idotmatrix` | `esp32dev_16M` | 16 MB | no in the generic WLED target | pending |
| `esp32_wrover_idotmatrix` | `esp32_wrover` | 4 MB | yes; classic ESP32 WROVER | pending |
| `esp32s3dev_8MB_opi_idotmatrix` | `esp32s3dev_8MB_opi` | 8 MB | OPI, >= 8 MB in WLED profile | pending |
| `esp32s3dev_8MB_qspi_idotmatrix` | `esp32s3dev_8MB_qspi` | 8 MB | QSPI | pending |
| `esp32s3dev_16MB_opi_idotmatrix` | `esp32s3dev_16MB_opi` | 16 MB | OPI, >= 8 MB in WLED profile | pending |

Use the OPI or QSPI S3 target that matches the actual module. A successful flash
configuration for one PSRAM wiring mode is not interchangeable with the other.

`platformio_override.ini.64x64-lite` intentionally supplies only the three
classic-ESP32 targets. More flash does not create more internal DRAM, so the
same low-RAM feature reductions are retained on the 8 MB and 16 MB variants.
The 4 MB version is the hardware-validated `64x64 logical -> 16x16 physical`
configuration; the larger-flash variants remain pending hardware validation.

## HUB75 targets

WLED 16.0.1 contains native HUB75 build environments. The iDotMatrix HUB75
override wraps those environments rather than re-defining their DMA flags or
pin maps:

| iDotMatrix environment | WLED HUB75 base | Controller / purpose | Validation in this project |
|---|---|---|---|
| `esp32dev_hub75_idotmatrix` | `esp32dev_hub75` | classic ESP32, WLED default HUB75 pinout | pending |
| `esp32dev_hub75_forum_pinout_idotmatrix` | `esp32dev_hub75_forum_pinout` | classic ESP32, SmartMatrix/forum pinout | pending |
| `esp32s3dev_4MB_qspi_hub75_idotmatrix` | `esp32s3dev_4MB_qspi_hub75` | Huidu HD-WF2 profile; WLED explicitly removes PSRAM for this target | pending; memory-constrained |
| `adafruit_matrixportal_esp32s3_idotmatrix` | `adafruit_matrixportal_esp32s3` | Adafruit MatrixPortal ESP32-S3 | pending |
| `esp32s3dev_16MB_opi_hub75_idotmatrix` | `esp32s3dev_16MB_opi_hub75` | MOONHUB / LilyGo T7-S3 | pending; preferred PSRAM-class test target |
| `waveshare_esp32s3_32MB_hub75_idotmatrix` | `waveshare_esp32s3_32MB_hub75` | Waveshare ESP32-S3-RGB-Matrix | pending |

Do not select a HUB75 environment only because its flash/PSRAM size looks
similar. HUB75 GPIO mapping is hardware-specific. For a controller not listed
above, start from the matching WLED 16.0.1 HUB75 environment or board-specific
pinout and merge the same iDotMatrix additions.

The `IDOT_GIF_LZW12` flag is used for all supplied HUB75 wrappers so the Usermod
can accept all current iDotMatrix logical profiles up to 64x64. The physical
HUB75 geometry remains WLED's responsibility.

## Framework pinning

Every supplied iDotMatrix environment overrides WLED's default ESP32 platform
with:

```ini
platform = espressif32@~6.13.0
platform_packages =
```

This selects Espressif Arduino **2.0.17 / ESP-IDF 4.4.7**, the framework used by
the validated BLE implementation. The WLED v16.0.1 Tasmota framework does not
provide the complete BLE GATT server required by this Usermod.

NimBLE and AnimatedGIF are pinned explicitly in every environment:

```ini
h2zero/NimBLE-Arduino@1.4.3
bitbank2/AnimatedGIF@1.4.7
```

Do not add `esp-nimble-cpp` or `ESP32 BLE Arduino` to these profiles.

## No-OTA partition policy

All supplied iDotMatrix environments define `WLED_DISABLE_OTA` and use a
single-application partition table. This is intentional for two reasons:

- BLE plus the Usermod can exceed smaller WLED OTA application slots;
- an official WLED OTA image does not contain this out-of-tree Usermod and would
  replace the customized firmware.

The project provides:

| Flash | Partition table | Application slot | Filesystem |
|---:|---|---:|---:|
| 4 MB | `WLED_ESP32_4MB_IDOT_NO_OTA.csv` | `0x2F0000` | `0x0F0000` |
| 8 MB | `WLED_ESP32_8MB_IDOT_NO_OTA.csv` | `0x400000` | `0x3E0000` |
| 16 MB | `WLED_ESP32_16MB_IDOT_NO_OTA.csv` | `0x600000` | `0x9E0000` |
| 32 MB | `WLED_ESP32_32MB_IDOT_NO_OTA.csv` | `0x600000` | `0x19E0000` |

For 8/16/32 MB, the filesystem starts at the same offset used by the
corresponding WLED v16.0.1 partition layout; the two OTA app regions are merged
into one factory application region. Flash these profiles over USB/serial.

An advanced user can create an OTA-capable custom build, but it must be compiled
with this Usermod, must fit both OTA slots, and must use exactly the same
partition layout already installed on the controller. OTA-capable variants are
not release-validated or supplied by this project.

## Choosing and building a target

Place this repository beside the WLED source tree, then copy exactly one media
profile to `platformio_override.ini` inside WLED.

Example: 64x64-capable build for an ESP32-S3 with 16 MB flash and OPI PSRAM:

```sh
cp ../wled-usermod-idotmatrix/platformio_override.ini.64x64 platformio_override.ini
pio run -e esp32s3dev_16MB_opi_idotmatrix -t clean
pio run -e esp32s3dev_16MB_opi_idotmatrix
```

Example: standard 16x16 build on a classic 8 MB ESP32 target:

```sh
cp ../wled-usermod-idotmatrix/platformio_override.ini.example platformio_override.ini
pio run -e esp32dev_8M_idotmatrix -t clean
pio run -e esp32dev_8M_idotmatrix
```

Example: WLED's MOONHUB/LilyGo T7-S3 HUB75 target plus iDotMatrix:

```sh
cp ../wled-usermod-idotmatrix/platformio_override.ini.hub75 platformio_override.ini
pio run -e esp32s3dev_16MB_opi_hub75_idotmatrix -t clean
pio run -e esp32s3dev_16MB_opi_hub75_idotmatrix
```

A clean build is required whenever the decoder profile changes because
`patch_animatedgif_profiles.py` rewrites the selected AnimatedGIF dependency for
that PlatformIO environment.

## Validation terminology

The repository distinguishes three different claims:

- **profile-defined**: a PlatformIO environment is supplied and statically
  checked by the host regression suite;
- **build-validated**: that environment has completed a WLED v16.0.1 PlatformIO
  build with the pinned dependencies;
- **hardware-validated**: the resulting firmware has been exercised on the
  corresponding physical controller/display configuration.

The stable hardware baseline remains the classic 4 MB ESP32 configurations
listed in `README.md` and `TESTING.md`. ESP32-S3, PSRAM/direct 64x64, native
physical 64x64, and HUB75 remain the next hardware-validation phase.
