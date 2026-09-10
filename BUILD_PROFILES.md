# Build profiles and hardware targets

Version 0.8.1 keeps two independent build choices that were previously
combined in the old single `esp32dev_idotmatrix` example:

1. **iDotMatrix media profile** — the largest logical GIF/protocol resolution
   compiled into the Usermod (`16x16`, `32x32`, or `64x64`);
2. **WLED hardware target** — MCU family, flash size, PSRAM configuration, and,
   for HUB75, the controller/pinout selected by WLED.

Classic/media/HUB75 profiles retain the WLED v16.0.1 build family. The supported
ESP32-C3 profile is intentionally separate and targets pinned WLED commit
`d55037f7510541eddc390c8f3d01afc5787aa44a`, whose `esp32c3dev` environment uses
ESP-IDF 5/shared-RMT. Profiles inherit the matching WLED board definitions and
driver flags rather than duplicating them.

## Media profiles

| Override file | Decoder | Maximum logical profile | Normal purpose |
|---|---|---:|---|
| `platformio_override.ini.example` | complete LZW12, UI capped by `IDOT_SCREEN_MAX_DIM=16` | 16x16 | stable low-feature baseline |
| `platformio_override.ini.32x32` | compact LZW11 | 32x32 | 16x16/32x32 protocol and media |
| `platformio_override.ini.64x64` | complete LZW12 | 64x64 | normal 64x64-capable build; PSRAM direct path or no-PSRAM cache selected at runtime |
| `platformio_override.ini.64x64-lite` | complete LZW12 | 64x64 | classic ESP32 low-internal-RAM build with optional WLED integrations removed |
| `platformio_override.ini.hub75` | complete LZW12 | 64x64 | WLED HUB75 output plus iDotMatrix BLE/media; experimental in this project |

The compiled maximum controls what the Usermod settings page exposes. The
hardware target does not change the protocol profile by itself. Changing
`ScreenType` at runtime also does **not** downgrade the compiled GIF decoder: a
64x64/LZW12 firmware set to `16x16` still uses the LZW12 backend. The standard
16x16 build intentionally also uses LZW12 because the compact12/cache runtime
proved more stable on the validated classic ESP32 than the smaller LZW10 path;
`IDOT_SCREEN_MAX_DIM=16` keeps the UI/profile constrained to 16x16.

## Standard hardware targets

The 32x32 and 64x64 override files contain the same seven established hardware
bases. **Each media
profile has a unique PlatformIO environment name**, giving every decoder profile
its own `.pio/build` and `.pio/libdeps` namespace and preventing a previously
patched AnimatedGIF profile from being reused by another build.

Environment naming is:

```text
<hardware-stem>_<media-profile>
```

where `<media-profile>` is `16x16`, `32x32`, or `64x64`.

| Hardware stem | WLED v16.0.1 base environment | Flash | PSRAM | Project hardware validation |
|---|---|---:|---|---|
| `esp32dev_idotmatrix` | `esp32dev` | 4 MB | no | **validated baseline** |
| `esp32dev_8M_idotmatrix` | `esp32dev_8M` | 8 MB | no in the generic WLED target | pending |
| `esp32dev_16M_idotmatrix` | `esp32dev_16M` | 16 MB | no in the generic WLED target | pending |
| `esp32_wrover_idotmatrix` | `esp32_wrover` | 4 MB | yes; classic ESP32 WROVER | pending |
| `esp32s3dev_8MB_opi_idotmatrix` | `esp32s3dev_8MB_opi` | 8 MB | OPI, >= 8 MB in WLED profile | pending |
| `esp32s3dev_8MB_qspi_idotmatrix` | `esp32s3dev_8MB_qspi` | 8 MB | QSPI | pending |
| `esp32s3dev_16MB_opi_idotmatrix` | `esp32s3dev_16MB_opi` | 16 MB | OPI, >= 8 MB in WLED profile | pending |



### ESP32-C3 status

ESP32-C3 4 MB / 16x16 is a **supported 0.8.1 target** through:

```text
platformio_override.ini.c3
env:esp32c3dev_idotmatrix_16x16
```

The release profile must be used with WLED commit
`d55037f7510541eddc390c8f3d01afc5787aa44a`. It inherits Arduino Core 3.3.8 /
ESP-IDF 5.5.4, `WLED_USE_SHARED_RMT`, and the CORE3 NeoPixelBus backend, and pins
NimBLE-Arduino 2.5.1.

Because the supported classic and C3 profiles require different NimBLE major
APIs, `library.json` does not declare a broad NimBLE dependency. The official
PlatformIO overrides are authoritative and pin the exact validated versions:
1.4.3 for classic ESP32 and 2.5.1 for ESP32-C3.

Hardware evidence behind the support decision:

| Build path | Result on physical 16x16 C3 |
|---|---|
| IDF 4.4.7 legacy RMT + BLE | strong visible spikes |
| IDF 4.4.8 legacy RMT + BLE | same class of spikes despite more heap |
| IDF 5.5.4 shared-RMT + BLE | no spikes through advertising, connection, images, GIFs, WLED effects and ~100 media changes |

The final stress snapshot retained about 74 KB free heap and a 64 KB largest
contiguous block with `reset=poweron`. The old development profiles are recorded
in `HISTORY.md` but are not shipped as build choices in the stable package.

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

## Usermod inheritance policy

Every supplied override sets `custom_usermods` to only:

```ini
custom_usermods =
  symlink://../wled-usermod-idotmatrix
```

Do not inherit `${env:<base>.custom_usermods}` here. WLED base environments can
include AudioReactive; inheriting it changed the memory budget and was the cause
of misleading low-heap behavior during 0.8.0 validation.

## Framework pinning

The release intentionally supports two framework generations:

```text
classic ESP32: WLED 16.0.1 + Arduino 2.0.17 / IDF 4.4.7 + NimBLE 1.4.3
ESP32-C3:      WLED d55037f + Arduino 3.3.8 / IDF 5.5.4 + NimBLE 2.5.1
```

`platformio_override.ini.c3` contains no `platform` or `platform_packages` key;
it must inherit them from the pinned WLED `env:esp32c3dev`. Compile-time guards
require ESP32-C3, IDF5, `WLED_USE_SHARED_RMT`, and the NimBLE 2.x API.

The C3 profile deliberately does **not** define `WLED_DISABLE_ESPNOW` on this
pinned WLED base. During release compilation WLED's module validator rejected the
inherited `wled-espnow` module when that flag produced an empty linked module.
Other optional features disabled by the release profile remain unchanged.

Do not add `esp-nimble-cpp` or `ESP32 BLE Arduino`.

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
pio run -e esp32s3dev_16MB_opi_idotmatrix_64x64 -t clean
pio run -e esp32s3dev_16MB_opi_idotmatrix_64x64
```

Example: standard 16x16 build on a classic 8 MB ESP32 target:

```sh
cp ../wled-usermod-idotmatrix/platformio_override.ini.example platformio_override.ini
pio run -e esp32dev_8M_idotmatrix_16x16 -t clean
pio run -e esp32dev_8M_idotmatrix_16x16
```

Example: supported **ESP32-C3 4 MB / 16x16** build after checking out the pinned WLED commit:

```sh
cp ../wled-usermod-idotmatrix/platformio_override.ini.c3 platformio_override.ini
pio run -e esp32c3dev_idotmatrix_16x16 -t clean
pio run -e esp32c3dev_idotmatrix_16x16
```

Expected `/json/info` markers include `release=0.8.1`, `build=0.8.1-audit-fix1`,
`RMT+BLE=ESP32-C3 shared-RMT`, `framework=WLED IDF5/shared-RMT`,
`wledBase=d55037f`, and `nimble=2.x API`.

Example: WLED's MOONHUB/LilyGo T7-S3 HUB75 target plus iDotMatrix:

```sh
cp ../wled-usermod-idotmatrix/platformio_override.ini.hub75 platformio_override.ini
pio run -e esp32s3dev_16MB_opi_hub75_idotmatrix -t clean
pio run -e esp32s3dev_16MB_opi_hub75_idotmatrix
```

The shipped media profiles use different environment names, so switching between
16x16/32x32/64x64 no longer shares the same PlatformIO build or dependency
cache. A clean build is still recommended after changing framework/dependency
versions or editing an override in place. If upgrading from a pre-release 0.8.0
layout that used unsuffixed names, remove the old `.pio` directory once before
building the new environments.

## Validation terminology

The repository distinguishes three different claims:

- **profile-defined**: a PlatformIO environment is supplied and statically
  checked by the host regression suite;
- **build-validated**: that environment has completed a PlatformIO build with its documented WLED base and pinned dependencies;
- **hardware-validated**: the resulting firmware has been exercised on the
  corresponding physical controller/display configuration.

The supported hardware baselines are the classic 4 MB ESP32 and the documented
4 MB ESP32-C3 16x16 profile. ESP32-S3, PSRAM/direct 64x64, native physical 64x64,
and HUB75 remain the next hardware-validation phase.
