# Release 0.8.1 Audit Remediation

**Release:** `0.8.1`  
**Build:** `0.8.1-audit-fix1`  
**Status:** final hardware-qualified remediation build, released as 0.8.1

This document records the corrective pass performed after the independent audit
of the 0.8.1 release candidate. The validated architecture and hardware profiles
were retained; the work was limited to confirmed failure paths, documentation
consistency, and regression coverage. This exact build subsequently passed the
final ESP32-C3 hardware qualification.

## Corrected runtime issues

### GIF promotion failure recovery

`IDotMatrixMedia::promoteGif()` now publishes `GifCacheIo` when both the direct
LittleFS rename and the streamed-copy fallback fail. The failed receive slot is
retired only after both promotion strategies have been attempted. This makes the
failure visible to the existing WLED-adapter recovery path and leaves the next
valid GIF transfer usable.

Regression coverage injects filesystem failures for:

- direct rename success;
- direct rename failure followed by successful streamed copy;
- direct rename failure plus copy failure;
- a subsequent valid GIF after the terminal failure.

### Transactional program replacement

`IDotMatrixAutomation::commitScheduleUpload()` no longer deletes a previously
valid program before the replacement is known to be usable. For each activity it
now stages the previous media under a temporary backup name, promotes the new
media, commits the new NVS metadata, and only then removes the backup.

If media promotion fails, the previous media and metadata are retained when
possible. If the new NVS write fails, the previous media is restored and its
metadata is re-asserted. If either half cannot be restored reliably, the entry is
cleared so persistence converges to the safe state of no metadata and no media.

`loadPersistence()` also repairs stale state from older builds by removing corrupt
metadata and clearing configured schedules whose media file is missing or has an
unexpected size.

## Release/build identification

The public release and internal build identifiers are now separate:

```text
release=0.8.1
build=0.8.1-audit-fix1
```

The public release value remains 0.8.1. The build value identifies the exact
corrective revision used during final hardware qualification.

## Documentation corrections

The documentation now consistently describes:

- standard 16x16 builds as `IDOT_GIF_LZW12` with `IDOT_SCREEN_MAX_DIM=16` and
  `compact12/cache` on supported no-PSRAM targets;
- FA02 as 4112 bytes of permanent inline reassembly storage with an 8192-byte
  maximum logical packet size and temporary dynamic storage above 4112 bytes;
- supplied `custom_usermods` lists as explicit replacement lists containing only
  iDotMatrix, rather than inherited base-environment lists;
- WLED local time as the primary clock authority, with the last valid application
  time retained as an offline fallback while WLED time is invalid;
- compact PNG validation as validation of the supported decoder subset and zlib
  payload, not full general-purpose PNG/chunk-CRC validation;
- exact NimBLE versions as target-profile responsibilities rather than a broad
  `library.json` dependency range.

## Regression coverage added

A behavioral host suite now exercises `IDotMatrixAutomation`, including:

- alarm and schedule persistence;
- successful program creation and replacement;
- temporary-file write failure;
- media promotion/rename failure;
- rollback failure;
- NVS commit failure and metadata re-assertion;
- failure of both NVS commit and rollback metadata write;
- reboot/loadPersistence recovery;
- missing media and corrupt metadata;
- WLED-time authority, application-time fallback, weekday matching, and
  midnight-spanning schedules.

The selected protocol, renderer, bulk-transfer, FA02, automation, and media host
tests are also run under AddressSanitizer and UndefinedBehaviorSanitizer.

## Intentionally deferred items

The audit also proposed lower-priority hardening such as a failure-atomic renderer
reinitialization, broader FA02 synchronization review, additional RX-drop
counters, and more systematic Preferences write-result diagnostics. These are
not required to correct the confirmed 0.8.1 blockers and are deferred to the
0.9 development line to avoid destabilizing the hardware-validated release path.

No BLE wire format, protocol command semantics, renderer behavior, normal GIF
behavior, persisted structure layout, supported pin defaults, WLED base revision,
or ESP32-C3 shared-RMT configuration was intentionally changed by this pass.

## Hardware qualification result

Host regression and sanitizer tests were followed by physical validation of this
exact build on the supported ESP32-C3 4 MB / 16x16 configuration. Qualification
confirmed:

1. boot and Wi-Fi/Web UI reachability;
2. `/json/info` reporting `release=0.8.1` and `build=0.8.1-audit-fix1`;
3. BLE advertising and application connection;
4. iDotMatrix content/effect operation and repeated transitions back to WLED;
5. approximately 50 WLED changes + 50 iDotMatrix changes + 50 further WLED changes;
6. successful program/schedule operation after the transactional persistence fix;
7. no reported LED instability, watchdog, panic, or reboot during qualification.

The final snapshot reported 75,296 bytes free heap, `min=43012`, a 65,536-byte
largest contiguous block, `gifProbe=79444`, a 69,632-byte GIF-probe largest block,
and a 10,240-byte GIF reserve. The current C3/IDF5 reset-reason diagnostic reports
`reset=unknown`; this is recorded as a non-blocking diagnostic cleanup item for a
later release because no reset occurred during the qualification run.
