# Release history

This file records stable snapshots and significant experimental builds. Failures
are retained because they define compatibility constraints that should not be
rediscovered later.

## 0.5.0 - 2026-09-02 - First stable BLE foundation

- Froze the first experimentally verified baseline.
- Confirmed WLED 0.16.0.1 remains operational over Wi-Fi.
- Confirmed discovery and successful connection from the official iDotMatrix app.
- Pinned Espressif platform 6.13.x, Arduino-ESP32 2.0.17, ESP-IDF 4.4.7, and
  NimBLE-Arduino 1.4.3.
- Added the required explicit NimBLE dependency to the documented override.
- Retained I2S enforcement, the RMT safety guard, and Wi-Fi modem sleep.
- Documented architecture, constraints, installation, diagnostics, related
  projects, and roadmap.
- No LED command rendering is claimed in this release.

## 0.4.9 - 2026-09-02 - Working NimBLE backend

- Replaced classic Bluedroid with `h2zero/NimBLE-Arduino` 1.4.3.
- Preserved the confirmed FA/AE GATT database and manufacturer data.
- Used compact 16-bit service UUIDs in the advertising packets.
- Kept delayed BLE startup and Wi-Fi modem sleep enabled.
- Confirmed official-app discovery and connection experimentally.
- Found that the symlinked Usermod requires NimBLE explicitly in environment
  `lib_deps`.

## 0.4.8 - 2026-09-02 - Split Bluedroid initialization experiment

- Initialized Bluedroid/GATT early to reserve internal RAM.
- Deferred advertising until after the first Wi-Fi initialization period.
- Still crashed when WLED uninitialized Wi-Fi (`clear_bss_queue` and
  `scan_inter_channel_timeout_process`).
- Rejected as a stable architecture.

## 0.4.7 - 2026-09-02 - Delayed Bluedroid with modem sleep

- Combined delayed BLE initialization with `noWifiSleep = false`.
- Eliminated the intentional Wi-Fi power-save abort.
- Bluedroid then failed while allocating internal queues and buffers
  (`fixed_queue_new`, `vQueueDelete`, and `btm_ble_init`).
- Demonstrated that late Bluedroid initialization was not RAM-safe inside WLED.

## 0.4.6 - 2026-09-02 - Wi-Fi modem-sleep correction

- Forced `noWifiSleep = false` before enabling Bluetooth.
- Removed the coexistence error requiring Wi-Fi modem sleep.
- Early Bluedroid still crashed Wi-Fi scanning in `clear_bss_queue` and
  `scan_inter_channel_timeout_process`.

## 0.4.5 - 2026-09-02 - Early Bluedroid initialization

- Moved BLE initialization back into Usermod setup after the I2S guard.
- Avoided the earlier late `coex_core_enable` path.
- Exposed WLED's default disabled Wi-Fi modem sleep as another incompatibility;
  Wi-Fi deliberately aborted in `pm_set_sleep_type`.

## 0.4.4 - 2026-09-02 - RMT guard and delayed BLE startup

- Detected digital WLED buses using the RMT backend.
- Blocked BLE safely and reported the requirement to select I2S.
- Deferred BLE startup by five seconds.
- Eliminated the RMT-triggered boot loop when the guard was active.
- With I2S selected, delayed Bluedroid initialization aborted in coexistence setup.

## 0.4.0-0.4.3 - 2026-09-02 - Official platform and classic BLE

- Moved from WLED's compact Tasmota framework to official Espressif
  `espressif32@~6.13.0`.
- Reused the standalone emulator's classic `BLEDevice.h` API.
- Added the 4 MB single-app/no-OTA partition table after the image exceeded the
  standard OTA application slot.
- Fixed Arduino-ESP32 2.x `setValue` const-correctness differences.
- Reached a complete link and upload.
- Identified the RMT-HI/Bluetooth-controller interrupt conflict causing a reboot
  loop.

## 0.3.1-0.3.6 - 2026-09-01/02 - NimBLE wrapper experiments

- Added correct WLED `library.json` metadata and `libArchive: false`.
- Tested `esp-nimble-cpp` 2.3.2 and framework include-path injection.
- Encountered missing include paths, wrapper/framework API mismatches, missing
  server symbols, and incompatible HCI/notify functions.
- Rejected `esp-nimble-cpp` 2.x for the Arduino-ESP32 2.0.x build.

## 0.2.0 - 2026-09-01 - Initial BLE extraction

- Extracted the first `IDotMatrixBLEServer` from the standalone reference.
- Added FA/AE services, characteristics, advertising, device information,
  time-sync acknowledgement, and a fixed receive queue.
- Established separation between BLE transport and WLED lifecycle.
- Did not yet have a compatible BLE dependency/framework combination.

## 0.1.0 - 2026-09-01 - Usermod skeleton

- Created the external WLED Usermod package.
- Added registration, enable setting, configuration persistence, loop hook, and
  WLED status output.
- Confirmed the symlinked Usermod compiled, registered, and appeared in WLED.
