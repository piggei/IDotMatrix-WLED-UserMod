# iDotMatrix WLED Usermod 0.9.0-rc.2

RC2 is a consolidation release candidate. It does not add app-visible features or change the verified BLE protocol/rendering behavior.

## Main corrections

- Preset / Default activation is now transactional within the current session. Multi-slot replacement either commits completely or restores the previous active bank after a LittleFS rename/promotion failure.
- Preset remains deliberately volatile: reboot/startup cleans active, pending, cache and transaction-backup files. RC2 does not introduce Preset persistence or recovery across reboot.
- Added behavioral Preset host tests with LittleFS fault injection for multi-slot activation, intermediate backup/promotion failure, retry, write failure, CRC rejection and reboot cleanup.
- Added behavioral Carousel host tests for manifest-save failure and rollback. Critical `saveManifest()` results are now recorded instead of being silently ignored; `/json/info` reports `carouselManifest=save-failed` after a persistence failure.
- `enabled` is explicitly a reboot-required structural setting; RC2 does not attempt partial runtime BLE/Usermod teardown or startup.
- Qualified MatrixPortal baseline pinned to WLED 0.17.0-devV5 commit `06ae26db67107cb3f6a3d107a92340035991a063`.
- MatrixPortal deliberately retains the pinned upstream `${common.default_usermods}`, partition table and OTA policy; legacy classic/C3 profiles retain their no-OTA single-app policy.
- README, BUILD_PROFILES, TESTING, PROTOCOL, ARCHITECTURE and HISTORY were reconciled around the 0.9 MatrixPortal target and legacy profiles.
- Current effect name is documented as `iDotMatrix`; stale operational `iDotMatrix Display` references were removed.
- TEXT scratch RAM is documented (~12 KiB across BulkTransfer/Carousel/Preset); no risky pre-release buffer-sharing refactor was introduced.

## Compatibility

No UUID, framing, CRC, renderer, Alarm/Program multipart, Carousel wire behavior, Preset wire behavior, media decoder or clock/timer behavior was intentionally changed.

## Qualified reference target

```text
Adafruit MatrixPortal ESP32-S3
64x64 HUB75 panel
WLED 0.17.0-devV5
WLED commit 06ae26db67107cb3f6a3d107a92340035991a063
environment adafruit_matrixportal_esp32s3_idotmatrix_64x64
```

Final 0.9.0 promotion still requires the documented RC hardware regression/soak gates.
