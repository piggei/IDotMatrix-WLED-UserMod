# Build profiles

This document describes the PlatformIO profiles shipped with iDotMatrix WLED
Usermod **release 0.9.4 / build 0.9.4-rc.1**. Stable 0.9.3 remains the current qualified release.

PlatformIO override templates are stored under `overrides/`. Custom partition
tables are stored under `partitions/`. Copy one override to WLED's
`platformio_override.ini` before building.

## Optional WLED Buzzer Usermod

The 0.9.4 line retains the 0.9.3 architecture and contains no buzzer hardware backend. Sound support is an
optional second out-of-tree Usermod. Add the Buzzer repository to the same
`custom_usermods` list when required:

```ini
custom_usermods =
  symlink://../wled-usermod-buzzer
  symlink://../wled-usermod-idotmatrix
```

The supplied iDotMatrix profiles intentionally do not force this optional
dependency. Without it, iDotMatrix still builds and the Buzzer Enable control is
shown disabled. With WLED Buzzer Usermod 0.1.0, the optional weak-link bridge is resolved at link time and sound requests are routed through the external service. The final 0.1.0 service preserves one-shot `triple_beep` behavior and applies the qualified 550 ms final gap only when the sound repeats.



## Waveshare ESP32-S3-RGB-Matrix — 0.9.4 qualification target

```text
override    = overrides/waveshare-s3-hub75.ini
environment = waveshare
base WLED   = 17.0.0-devV5
base env    = env:waveshare_esp32s3_32MB_hub75
board       = Waveshare ESP32-S3-RGB-Matrix / ESP32-S3-N32R16
flash       = 32 MB
PSRAM       = 16 MB
logical max = 64x64
default     = 64x64
```

This is the primary 0.9.4 Waveshare qualification/PSRAM-optimization profile; dev.14 leaves its qualified policy unchanged. It **does not** override the upstream PlatformIO platform, board definition, partition table or OTA policy. Those remain owned by WLED's Waveshare environment. Dev.8 completed the PSRAM telemetry/staging hardware gate with 266/266 successful stages and zero fallback during the reported ~22 minute Carousel/GIF soak. Dev.9 then passed a 22/22 zero-fallback hardware smoke after the source-stage refactor. Dev.10 qualified bounded persistent source reuse on a seven-GIF Carousel, reaching 119 cache hits with only seven playback stages and zero fallback. Dev.11 kept the same board/profile, added one-item look-ahead prefetch into that existing source cache and passed its real-hardware cold-cache/lifecycle gate without visible playback delay. Dev.12 kept the media policy unchanged, corrected `gifStage peak` accounting for cached/prefetched active sources and passed its hardware smoke. Dev.13 hides/forces off the historical low-memory Rescale option on native S3 HUB75 profiles; automatic output scaling remains unchanged.

The local environment is deliberately named `waveshare`, matching the short alias used in the project's global override. WLED 17.0.0-devV5 currently contains a malformed `SHTC3_v2` custom-usermod source (`.../commit/<sha>`), which PlatformIO cannot clone. For that reason the 0.9.4 Waveshare profile does **not** inherit the upstream `custom_usermods` string. It reproduces the same `Internal_Temperature` and pinned AudioReactive entries, rewrites only SHTC3_v2 as `git+https://github.com/lost-hope/SHTC3_v2.git#1f6e3fc...`, then adds iDotMatrix.

The iDotMatrix additions are limited to:

- complete 12-bit GIF support;
- `IDOT_SCREEN_MAX_DIM=64`;
- default logical screen type 64x64;
- the existing ESP32-S3/HUB75/IDF5 compile guard;
- a Waveshare-specific target guard/diagnostic marker (target identity only; the temporary dev.4 audio probe is removed);
- NimBLE-Arduino 2.5.1 and AnimatedGIF 1.4.7;
- the external iDotMatrix Usermod itself;
- Waveshare guarded GIF source staging policy (2 MiB maximum, 4 MiB reserve), dev.10 persistent Carousel-source reuse (1 MiB total, 512 KiB per admitted source, 12 metadata entries, LRU), and dev.11 one-item Carousel look-ahead into the same bounded cache, implemented in iDotMatrix code rather than by redefining the upstream board environment. Dev.12 changes only the associated `gifStage peak` diagnostic semantics.

Before iDotMatrix integration, the board was physically verified with the official WLED 16.0.1 binary, one 64x64 HUB75 panel, Half Scan, 1x1. `/json/info` reported 4096 LEDs, 32 MB flash and 16 MB PSRAM. The 0.9.4 runtime on WLED 17.0.0-devV5 has passed native 64x64 rendering, BLE/app connectivity, TEXT, static image, GIF, Carousel, reboot/persistence, Alarm/Program, Matrix Auto Rotation and onboard-microphone AudioReactive checks. Dev.8 closed the PSRAM telemetry/staging soak gate; dev.9 closed the source-stage refactor smoke. Dev.10 source-cache reuse and dev.11 one-item look-ahead are hardware-qualified. Waveshare 16x16 -> 64x64 and 32x32 -> 64x64 automatic scaling have also passed on physical hardware with low-memory Rescale disabled. Absolute `psram=total` should only be compared across identical full firmware/Usermod compositions.

The older Waveshare entry has been removed from `overrides/hub75-legacy.ini`; the dedicated profile above is the only supported recipe for this board in the 0.9.4 development line.

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
| `overrides/matrixportal-s3-hub75.ini` | LZW12 + PSRAM | 16x16, 32x32, 64x64 | qualified MatrixPortal S3 / HUB75 target |
| `overrides/waveshare-s3-hub75.ini` | LZW12 + PSRAM | 16x16, 32x32, 64x64 | 0.9.4 Waveshare qualification target |

The settings UI never advertises a logical profile larger than the compiled
capacity. On the 0.9 native-matrix path logical and physical resolutions may
differ; 16x16, 32x32 and 64x64 are scaled by the normal renderer/output path.

`IDOT_LOW_MEMORY_RESCALE` controls only the historical storage-downscale mode. It defaults on for larger classic profiles, but the dedicated Waveshare and MatrixPortal S3 HUB75 overrides set it to `0`, so their settings pages do not expose an obsolete Rescale checkbox and any stale stored value is forced off.

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

`overrides/waveshare-s3-hub75.ini` is the dedicated 0.9.4 Waveshare qualification profile.

`overrides/hub75-legacy.ini` contains the remaining legacy WLED 16.x HUB75 wrappers. HUB75 GPIO
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

Dev.14 qualified transient whole-file staging on MatrixPortal after a real 64x64 Carousel baseline measured `total=2097152`, `free=1940364`, `largest=1933312`, `minLargest=1900544`; the physical staging gate then reached 19/19 successes with zero fallback and a 173821-byte largest active source. Dev.15 retained `IDOT_GIF_PSRAM_STAGE_MAX=262144` and `IDOT_GIF_PSRAM_STAGE_RESERVE=1048576`, added `IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_BYTES=393216`, `IDOT_GIF_PSRAM_SOURCE_CACHE_ENTRY_MAX=262144`, `IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_ENTRIES=8`, and passed hardware qualification with all seven sources resident (`249267` bytes), 25 hits, seven normal stages, zero fallback and zero eviction. Dev.16 sets `IDOT_GIF_CAROUSEL_PREFETCH_ENABLED=1` and otherwise leaves the MatrixPortal policy unchanged. This remains intentionally separate from the Waveshare 2 MiB stage / 4 MiB reserve / 1 MiB persistent-cache policy.

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
  Usermods from the exact qualified WLED revision, then adds iDotMatrix;
- `overrides/waveshare-s3-hub75.ini` preserves the official WLED 16.0.1
  Waveshare Usermods, including AudioReactive, then adds iDotMatrix.

These exceptions are qualification facts, not permission to assume arbitrary
future WLED revisions are equivalent.

## Framework pinning

The project intentionally supports different framework generations:

```text
classic ESP32: WLED 16.0.1 + Arduino 2.0.17 / IDF 4.4.7 + NimBLE 1.4.3
ESP32-C3:      WLED d55037f + Arduino 3.3.8 / IDF 5.5.4 + NimBLE 2.5.1
MatrixPortal:  WLED 06ae26d (17.0.0-devV5) + upstream S3/IDF5 configuration
Waveshare S3: WLED 17.0.0-devV5 + upstream 32 MB S3/HUB75/IDF5 configuration
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

Expected `/json/info` identity markers for the current 0.9.4 release candidate include:

```text
release=0.9.4
build=0.9.4-rc.1
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
