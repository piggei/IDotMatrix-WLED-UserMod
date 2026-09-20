# Build profiles

This document describes the PlatformIO profiles shipped with iDotMatrix WLED
Usermod **release 0.9.1 / build 0.9.1**. Stable 0.9.0 remains the previous release baseline.

PlatformIO override templates are stored under `overrides/`. Custom partition
tables are stored under `partitions/`. Copy one override to WLED's
`platformio_override.ini` before building.

## Media profiles

The compiled media profile limits the largest logical iDotMatrix screen type and
selects the GIF decoder strategy. It is independent from the physical matrix
geometry configured in WLED.

| Override | Decoder / capacity | Logical profiles | Purpose |
|---|---|---|---|
| `overrides/16x16.ini` | LZW12 + `IDOT_SCREEN_MAX_DIM=16` | 16x16 | classic ESP32 16x16 baseline |
| `overrides/32x32.ini` | compact LZW11 | 16x16, 32x32 | 32x32-capable test profile |
| `overrides/64x64.ini` | complete LZW12 | 16x16, 32x32, 64x64 | 64x64-capable targets |
| `overrides/64x64-lite.ini` | complete LZW12 / low internal RAM | 16x16, 32x32, 64x64 | classic ESP32 no-PSRAM 64x64 logical path |
| `overrides/matrixportal-s3-hub75.ini` | LZW12 + PSRAM | 16x16, 32x32, 64x64 | primary MatrixPortal S3 / HUB75 target |

The settings UI never advertises a logical profile larger than the compiled
capacity. On the 0.9 native-matrix path logical and physical resolutions may
differ; 16x16, 32x32 and 64x64 are scaled by the normal renderer/output path.

## Standard hardware targets

The classic profiles preserve the established WLED 16.x target matrix. The
repository also contains dedicated C3 and HUB75 wrappers because their framework,
BLE and LED-backend requirements differ materially.

For the exact environment names exposed by each override, inspect the selected
`.ini` file or run:

```sh
pio project config | grep '^env:'
```

## ESP32-C3 status

The supported ESP32-C3 baseline is pinned to WLED commit:

```text
d55037f7510541eddc390c8f3d01afc5787aa44a
```

That tree identifies itself as WLED `17.0.0-devV5` and provides the qualified
Arduino 3.3.8 / ESP-IDF 5.5.4 shared-RMT stack. The C3 profiles intentionally
inherit `env:esp32c3dev`; they do not specify their own `platform` because doing
so would bypass the pinned WLED framework configuration.

### C3 iDotMatrix-only profile

```text
overrides/esp32c3-16x16.ini
env:esp32c3dev_idotmatrix_16x16
```

This is the conservative 16x16 C3 profile. It uses the single-application
no-OTA partition table and does not include WLED AudioReactive.

### C3 AudioReactive profile

```text
overrides/esp32c3-16x16-audio.ini
env:esp32c3dev_idotmatrix_audio_16x16
```

This profile retains the same C3 framework/media baseline and adds WLED
AudioReactive. AudioReactive remains runtime-configurable; microphone GPIOs are
not hard-coded by this project.

### C3 AudioReactive + OTA profile

```text
overrides/esp32c3-16x16-audio-ota.ini
env:esp32c3dev_idotmatrix_audio_16x16_ota
```

This is the recommended C3 profile when OTA is required. It was hardware-
validated on an ESP32-C3 SuperMini with 4 MB flash and a WS2812B ECO 16x16
matrix. The exact validation build used:

```ini
custom_usermods =
  audioreactive
  animartrix
  wled-usermod-idotmatrix = symlink://../wled-usermod-idotmatrix
```

The resulting `firmware.bin` measured **1,551,008 bytes**. The `0x1A0000`
application slot is 1,703,936 bytes, leaving 152,928 bytes (about 149 KiB) of
headroom per OTA slot in the validated build.

Hardware qualification completed three consecutive WLED OTA cycles with the
filesystem already populated. Stress state included 12 Carousel assets, six
Preset assets, a three-activity Schedule, BLE, shared-RMT output, GIF cache and
AudioReactive. After returning to native WLED content the observed diagnostics
were approximately 62,384 bytes free heap, 31,756 bytes historical minimum heap
and a 49,152-byte largest free block.

The first installation of this partition layout must be done over USB/serial.
Subsequent updates may use the WLED OTA page with a `firmware.bin` built from the
same customized source tree.

## HUB75 targets

`overrides/hub75-legacy.ini` contains legacy WLED 16.x HUB75 wrappers. HUB75 GPIO
mapping is board-specific; never choose a profile only because flash/PSRAM size
looks similar.

The 0.9 qualification target is instead:

```text
overrides/matrixportal-s3-hub75.ini
env:adafruit_matrixportal_esp32s3_idotmatrix_64x64
```

It extends WLED's official `env:adafruit_matrixportal_esp32s3` and therefore
inherits the MatrixPortal pinout, native HUB75 backend, ESP-IDF 5.x stack, PSRAM
setup, upstream partition table and OTA policy.

The exact WLED qualification commit is:

```text
06ae26db67107cb3f6a3d107a92340035991a063
```

## Usermod inheritance policy

Most legacy supplied overrides deliberately replace the base environment's
`custom_usermods` list with only the Usermods required by that profile. Do not
blindly inherit `${env:<base>.custom_usermods}` because upstream environments may
change over time and silently alter memory use or behaviour.

There are explicit, documented exceptions:

- `overrides/esp32c3-16x16-audio.ini` adds AudioReactive;
- `overrides/esp32c3-16x16-audio-ota.ini` adds AudioReactive and Animartrix because
  that exact combination was used for OTA/memory qualification;
- `overrides/matrixportal-s3-hub75.ini` preserves the pinned MatrixPortal base
  Usermods from the exact qualified WLED revision, then adds iDotMatrix.

These exceptions are qualification facts, not permission to assume arbitrary
future WLED revisions are equivalent.

## Framework pinning

The project intentionally supports different framework generations:

```text
classic ESP32: WLED 16.0.1 + Arduino 2.0.17 / IDF 4.4.7 + NimBLE 1.4.3
ESP32-C3:      WLED d55037f + Arduino 3.3.8 / IDF 5.5.4 + NimBLE 2.5.1
MatrixPortal:  WLED 06ae26d (17.0.0-devV5) + upstream S3/IDF5 configuration
```

The C3 profiles must inherit `env:esp32c3dev`. Compile-time guards require
ESP32-C3, IDF5, `WLED_USE_SHARED_RMT` and the NimBLE 2.x API. Do not add
`esp-nimble-cpp` or `ESP32 BLE Arduino`.

## OTA / partition policy

Custom partition CSV files live under `partitions/`.

### Legacy no-OTA tables

| Flash | Partition table | Application slot | Filesystem |
|---:|---|---:|---:|
| 4 MB | `partitions/WLED_ESP32_4MB_IDOT_NO_OTA.csv` | `0x2F0000` | `0x0F0000` |
| 8 MB | `partitions/WLED_ESP32_8MB_IDOT_NO_OTA.csv` | `0x400000` | `0x3E0000` |
| 16 MB | `partitions/WLED_ESP32_16MB_IDOT_NO_OTA.csv` | `0x600000` | `0x9E0000` |
| 32 MB | `partitions/WLED_ESP32_32MB_IDOT_NO_OTA.csv` | `0x600000` | `0x19E0000` |

These single-application layouts are retained for conservative/legacy builds.
Their matching overrides define `WLED_DISABLE_OTA`.

### Validated 4 MB dual-slot OTA table

`partitions/WLED_ESP32_4MB_IDOT_OTA.csv` is:

```text
nvs       0x009000  0x005000
otadata   0x00E000  0x002000
app0      0x010000  0x1A0000
app1      0x1B0000  0x1A0000
LittleFS  0x350000  0x0A0000
coredump  0x3F0000  0x010000
```

This yields two 1,703,936-byte application slots, 640 KiB nominal LittleFS and a
64 KiB coredump partition. WLED reports roughly 655 KiB filesystem capacity at
runtime because of filesystem/reporting units and metadata.

The MatrixPortal profile does not use these custom C3 tables; it inherits the
upstream MatrixPortal partition and OTA policy.

## Choosing and building a target

Place this repository beside the WLED source tree. Relative `board_build.partitions`
paths in the override files are intentionally written from the WLED project root,
for example:

```ini
board_build.partitions = ../wled-usermod-idotmatrix/partitions/WLED_ESP32_4MB_IDOT_OTA.csv
```

### Classic 16x16

```sh
cp ../wled-usermod-idotmatrix/overrides/16x16.ini platformio_override.ini
pio run -e esp32dev_8M_idotmatrix_16x16 -t clean
pio run -e esp32dev_8M_idotmatrix_16x16
```

### ESP32-C3 16x16

```sh
cp ../wled-usermod-idotmatrix/overrides/esp32c3-16x16.ini platformio_override.ini
pio run -e esp32c3dev_idotmatrix_16x16 -t clean
pio run -e esp32c3dev_idotmatrix_16x16
```

### ESP32-C3 16x16 + AudioReactive

```sh
cp ../wled-usermod-idotmatrix/overrides/esp32c3-16x16-audio.ini platformio_override.ini
pio run -e esp32c3dev_idotmatrix_audio_16x16 -t clean
pio run -e esp32c3dev_idotmatrix_audio_16x16
```

### ESP32-C3 16x16 + AudioReactive + OTA

```sh
cp ../wled-usermod-idotmatrix/overrides/esp32c3-16x16-audio-ota.ini platformio_override.ini
pio run -e esp32c3dev_idotmatrix_audio_16x16_ota -t clean
pio run -e esp32c3dev_idotmatrix_audio_16x16_ota
```

### MatrixPortal S3 / HUB75 64x64

```sh
cp ../wled-usermod-idotmatrix/overrides/matrixportal-s3-hub75.ini platformio_override.ini
pio run -e adafruit_matrixportal_esp32s3_idotmatrix_64x64 -t clean
pio run -e adafruit_matrixportal_esp32s3_idotmatrix_64x64
```

Expected `/json/info` markers for the 0.9.1 release include:

```text
release=0.9.1
build=0.9.1
```

plus target-specific markers such as `RMT+BLE=ESP32-C3 shared-RMT`,
`framework=WLED IDF5/shared-RMT`, `wledBase=d55037f` and `nimble=2.x API` on C3.

## Validation terminology

The repository distinguishes:

- **profile-defined**: the PlatformIO environment is supplied and statically
  checked by the host regression suite;
- **build-validated**: the environment completed a PlatformIO build with its
  documented WLED base and pinned dependencies;
- **hardware-validated**: resulting firmware was exercised on the corresponding
  physical controller/display configuration.

Current important hardware-validated targets are:

- classic ESP32 / 16x16 legacy line;
- ESP32-C3 SuperMini 4 MB / WS2812B 16x16 on the pinned IDF5/shared-RMT base;
- the same C3 with AudioReactive + the dual-slot OTA layout, including three
  consecutive OTA cycles and populated media/automation stress;
- Adafruit MatrixPortal S3 / 64x64 HUB75 on WLED commit
  `06ae26db67107cb3f6a3d107a92340035991a063`, including logical 16x16, 32x32 and
  64x64 operation.
