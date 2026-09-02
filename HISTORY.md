# Release history

This file records stable snapshots and significant experimental builds. Failures
are retained because they define compatibility constraints that should not be
rediscovered later.

## 0.6.1 - 2026-09-02 - Stable graffiti framebuffer

- Promoted the exact `0.6.1-dev.2` implementation to stable without runtime
  changes after validation with the official app on a physical 16x16 WLED
  matrix.
- Confirmed that DIY/Graffiti selects `iDotMatrix Framebuffer`, clears the
  display, and makes received pixel updates visible.
- Confirmed that full-screen RGB releases the framebuffer and returns WLED to
  `Solid`, while a later graffiti command reclaims the custom effect.
- Confirmed that the previous power, brightness, and full-screen RGB functions
  continue to work.
- Recorded the validation diagnostics: `rx=41`, `dropped=0`,
  `pixelUpdates=34`, `effectFrames=3186`, and `target=16x16` on a selected 16x16
  segment with grouping 1 and spacing 0.
- Retained the unsuccessful `0.6.1-dev.1` overlay approach and the successful
  `0.6.1-dev.2` candidate below so the architectural decision remains explicit.

## 0.6.1-dev.2 - 2026-09-02 - First-class WLED framebuffer effect

- Replaced the unsuccessful `handleOverlayDraw()` output path with a registered
  WLED 2D effect named `iDotMatrix Framebuffer`.
- Hardware diagnostics from `0.6.1-dev.1` proved that BLE transport and graffiti
  parsing were correct (`dropped=0`, increasing `pixelUpdates`), while WLED
  remained in `Solid` and did not display the external overlay.
- The effect now renders the logical RGB canvas from inside WLED's normal effect
  service, where the current segment's virtual XY dimensions and physical matrix
  mapping are valid.
- Entering DIY automatically selects the framebuffer effect. A valid graffiti
  pixel packet also selects it, making the implementation tolerant of app packet
  ordering.
- Full-screen RGB releases framebuffer ownership and returns the selected segment
  to WLED's `Solid` effect.
- Added effect registration, active-state, frame-count and target-dimension
  diagnostics to `/json/info`.
- Confirmed from the test device's JSON state that WLED exposes one selected
  16x16 2D segment with grouping 1 and spacing 0.
- Updated adapter tests for effect registration, activation, rendering, release,
  and pixel-driven reclamation.
- Hardware validation succeeded; this exact implementation was promoted to
  stable 0.6.1.

## 0.6.1-dev.1 - 2026-09-02 - Multisize framebuffer and graffiti candidate

- Added the WLED-independent `IDotMatrixRenderer` with dynamically allocated
  RGB canvases for 16x16, 32x32, and 64x64 profiles.
- Added confirmed DIY enter/exit parsing for `05 00 04 01 STATE` and its standard
  ACK.
- Added confirmed graffiti parsing for `LEN 00 05 01 ? R G B X Y...`, preserving
  the unknown byte and no-ACK behavior documented by the reference.
- Initially routed logical pixels through WLED's `handleOverlayDraw()` and the
  selected 2D segment. Hardware testing showed that packets reached the canvas,
  but WLED remained in `Solid` and the drawing was not displayed; this path was
  replaced in `0.6.1-dev.2`.
- Matched reference behavior: a new DIY session clears the canvas, leaving the
  editor preserves it, and full-screen RGB replaces it.
- Added canvas, content-owner, and accepted-pixel diagnostics.
- Added host tests for all canvas sizes, bounds, protocol extraction, lifecycle,
  and exact RGB-to-XY transfer.
- Kept the stable BLE queue and callback behavior unchanged. Fragmented logical
  commands and writes above 64 bytes remain unsupported.
- This is a hardware-test candidate; 0.6.0 remains the stable release.

## 0.6.0 - 2026-09-02 - Stable basic-command foundation

- Promoted the exact `0.6.0-dev.7` code validated on physical hardware to the
  first stable command-capable release; no runtime behavior was changed during
  consolidation.
- Confirmed discovery and connection by the official iDotMatrix app alongside
  working WLED Wi-Fi control.
- Confirmed initial app power-state initialization through screen-ON-on-connect
  plus two delayed device-information notifications.
- Confirmed screen on/off, brightness, and full-screen RGB control from the app
  to WLED.
- Kept the protocol parser, BLE transport, and WLED adapter as separate layers
  and retained bounded, non-blocking callback-to-loop processing.
- Added consolidated protocol, architecture, testing, installation, limitation,
  and roadmap documentation.
- Documented the current brightness limitation explicitly: the app controls
  WLED, but changes made directly in WLED cannot update the app slider because
  no verified device-to-app brightness-state message is known.
- Documented the current transport limit: fragmented logical FA02 commands are
  not reassembled yet and the command queue uses fixed 64-byte slots.
- Graffiti/framebuffer, clock, text, GIF/media, timers, alarms, and schedules
  remain outside this release.

## 0.6.0-dev.7 - 2026-09-02 - Full-screen RGB

- Added confirmed `07 00 02 02 R G B` decoding and standard ACK.
- Mapped full-screen colour to WLED's static effect and primary RGB colour using
  its normal global-colour update path.
- Preserved WLED's active/selected segment semantics; the standard one-segment
  matrix is filled completely.
- Added protocol and adapter tests for exact RGB channel transfer and static-mode
  selection.
- Documented brightness directionality explicitly: app slider changes update
  WLED, but WLED changes cannot update the app without a confirmed reverse-state
  message.
- Confirmed the two-push connection initialization from `.6` works correctly in
  the official app.

## 0.6.0-dev.6 - 2026-09-02 - Robust app-state initialization

- Found that the single 1.2-second device-info push was timing-sensitive: it
  initialized the app switch in `.4`, but not reliably in the later build.
- Added a second identical, non-blocking device-info push at about 2.5 seconds
  after connection.
- Added cumulative `infoPushAttempts` diagnostics to WLED JSON info.
- Added host-side adapter tests for ON/OFF, remembered brightness, zero, and
  percentage conversion behavior.
- Kept WLED as the only brightness persistence authority; no Usermod brightness
  preference was introduced.
- Did not invent an unconfirmed device-to-app brightness-state packet.

## 0.6.0-dev.5 - 2026-09-02 - WLED master brightness

- Added the confirmed `05 00 04 80 PERCENT` brightness command.
- Clamped protocol input to `0..100` exactly like the standalone reference.
- Added rounded linear conversion from iDotMatrix percent to WLED `0..255`
  master brightness.
- Kept logical screen power separate so brightness changes while OFF do not
  inadvertently power the output on.
- Preserved the last non-zero WLED brightness across zero-brightness and power
  cycles.
- Added standard ACK generation and host tests for normal and over-range values.

## 0.6.0-dev.4 - 2026-09-02 - Delayed device-info push

- Added the second connection action present in the standalone reference:
  schedule an unsolicited device-information notification 1.2 seconds after
  connection.
- Kept scheduling, response construction, and notification delivery outside the
  NimBLE callback.
- Cancelled a pending push when the app disconnects.
- Exposed device-information response construction through the protocol boundary
  so both app requests and connection-time pushes use identical bytes.
- Added host coverage for direct device-information response construction.
- Confirmed in the initial experiment that the delayed push can initialize the
  official app's displayed power switch; later testing showed the timing was not
  yet reliable.

## 0.6.0-dev.3 - 2026-09-02 - Screen ON at connection

- Matched the standalone reference behavior by treating every new app
  connection as a screen-ON event.
- Deferred the actual WLED power change from the NimBLE callback to the normal
  Usermod loop.
- Reverted the `.2` device-information state-byte hypothesis after the official
  app test showed that it did not initialize the switch.
- Restored the confirmed device-information response's final byte to fixed
  `00`.
- Added a host-side test for the connection-to-screen-ON event.

## 0.6.0-dev.2 - 2026-09-02 - Initial power-state experiment

- Experimented with making the device-information response read the current power state through the
  WLED adapter instead of always ending in `00`.
- Experimentally encoded the state in the response's final byte (`00` off,
  `01` on) to test whether the official app uses it to initialize its switch.
- The official-app test showed no effect; the experiment was reverted in `.3`.
- Temporarily extended the host test to cover a live ON state in the
  device-information response.

## 0.6.0-dev.1 - 2026-09-02 - Protocol boundary and screen power

- Added `IDotMatrixProtocol`, independent from BLE and WLED APIs.
- Moved device-information and time-sync response construction out of the BLE
  transport.
- Added strict little-endian packet-length validation for FA02 commands.
- Added host-side protocol tests for device information, power events, standard
  acknowledgements, invalid lengths, and unknown commands.
- Added `IDotMatrixWLEDAdapter` as the only protocol-to-WLED state boundary.
- Implemented the confirmed `05 00 07 01 STATE` screen command and standard
  `05 00 07 01 01` acknowledgement.
- Mapped screen state idempotently to WLED power using WLED's own
  `toggleOnOff()` and `stateUpdated()` paths, preserving the previous brightness.
- No brightness, colour, framebuffer, or graffiti commands are claimed yet.

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
