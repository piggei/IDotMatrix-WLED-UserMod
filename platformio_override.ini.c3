; WLED iDotMatrix Usermod - supported ESP32-C3 16x16 profile
; Usermod release: 0.8.1
; Required WLED base commit:
;   d55037f7510541eddc390c8f3d01afc5787aa44a (2026-09-07)
;
; This profile intentionally inherits WLED's ESP32-C3 IDF5/shared-RMT stack.
; Do not add platform/platform_packages here and do not use this profile with
; WLED 16.0.1. The source guards require ESP32-C3, ESP-IDF 5.x,
; WLED_USE_SHARED_RMT, and NimBLE-Arduino 2.x.
;
; The C3 hardware release path was validated on a 4 MB ESP32-C3 with a 16x16
; matrix on GPIO4: BLE advertising/connection, images, animated GIFs, WLED
; effects, WebSocket activity, and repeated media changes all ran without LED
; spikes on the pinned WLED shared-RMT backend.

[env:esp32c3dev_idotmatrix_16x16]
extends = env:esp32c3dev
board_build.partitions = ../wled-usermod-idotmatrix/WLED_ESP32_4MB_IDOT_NO_OTA.csv

build_flags =
  ${env:esp32c3dev.build_flags}
  -D WLED_DISABLE_OTA
  -D IDOT_GIF_LZW12
  -D IDOT_SCREEN_MAX_DIM=16
  -D IDOT_C3_WLED_IDF5
  -D WLED_DISABLE_ALEXA
  -D WLED_DISABLE_HUESYNC
  -D WLED_DISABLE_MQTT
  -D WLED_DISABLE_INFRARED
  -D WLED_DISABLE_ADALIGHT
  -D WLED_DISABLE_LOXONE

lib_deps =
  ${env:esp32c3dev.lib_deps}
  h2zero/NimBLE-Arduino@2.5.1
  bitbank2/AnimatedGIF@1.4.7

custom_usermods =
  symlink://../wled-usermod-idotmatrix
