# Release history

This file records stable snapshots and significant experimental builds. Failures
are retained because they define compatibility constraints that should not be
rediscovered later.

## 0.6.3-dev.3 - 2026-09-03 - App-rasterized text rendering

- Hardware validation of 0.6.3-dev.2 completed three TEXT transfers with valid
  CRC (`bulkChunks=3`, `bulkComplete=3`, `textBytes=54`, no drops/errors).
- Confirmed that 54 bytes represent a 14-byte TEXT header plus two 20-byte
  marker-`0x02` glyph records.
- Added WLED-independent TEXT payload parsing for confirmed marker `0x02`
  (8x16, 16 bitmap bytes) and marker `0x05` (16x32, 64 bitmap bytes).
- Kept the unconfirmed `0x03`/`0x06` aliases rejected rather than presenting
  them as verified protocol.
- Added dynamically allocated glyph bitmap storage in `IDotMatrixRenderer`,
  allocated only from normal loop context and reused when capacity permits.
- Ported fixed/background colours, horizontal and vertical movement, blink,
  pulse, sparkle, laser, and dynamic colour modes from the BUILD 80 renderer.
- Used a lightweight integer wave for the two wave-based dynamic colour modes;
  their appearance is a candidate for hardware comparison with FastLED
  `sin8()` before promotion.
- Preserved LSB-leftmost bitmap orientation and app-side SimSun/SimHei
  rasterization: no device font dependency was introduced.
- TEXT now takes internal ownership of the existing `iDotMatrix Display`
  effect; no additional WLED effect is registered.
- Added `content=text`, `textParsed`, `textParseErrors`, and glyph-size
  diagnostics plus protocol, renderer, and adapter regression coverage.

## 0.6.3-dev.2 - 2026-09-03 - Correct FA02 bulk reassembly

- Hardware testing of 0.6.3-dev.1 produced an app error, `rx=5`, and zero bulk
  chunks. Re-reading BUILD 80 identified the cause: bulk packets are assembled
  and dispatched from FA02; AE01 is only logged by the reference callback.
- Replaced the incorrect AE01 bulk hand-off with reference-matching FA02 logical
  packet reassembly based on the little-endian length in bytes 0..1.
- Preserved the four-entry queue for already-complete short FA02 commands while
  routing fragmented or large logical packets through one bounded 4112-byte
  assembler.
- Kept dispatch, CRC32 work, and FA03 acknowledgements in WLED loop context.
  The callback performs only validation and bounded copies.
- Added unknown-command bytes, fragment counts, current reassembly progress,
  and reassembly-error diagnostics.
- Matched BUILD 80's tolerant ACK for unknown short commands, without falsely
  acknowledging unsupported GIF/RAW bulk content.
- Added a fifth host test for complete, fragmented, oversized, malformed, and
  busy FA02 assembler behavior.
- Corrected all documentation that had assigned the bulk data path to AE01.

## 0.6.3-dev.1 - 2026-09-03 - TEXT bulk transport probe

- Added a dedicated AE01 receive slot sized for an observed 4096-byte payload
  chunk plus its 16-byte bulk header. The existing four-entry 64-byte FA02
  command queue remains unchanged.
- Added a WLED-independent bulk assembler for confirmed type `0x03` TEXT
  transfers, limited to 4096 payload bytes.
- Added repeated-header validation, bounded assembly, streaming CRC32, and the
  confirmed FA03 acknowledgements: `0x01` while incomplete and `0x03` when the
  transaction terminates.
- Increased the AE01 characteristic capacity explicitly; bulk writes are copied
  in the BLE callback but parsed and acknowledged from the normal Usermod loop.
- Added bulk chunk, completion, CRC-error, rejection, oversized-write, and
  completed-text-size diagnostics.
- Added a fourth host test covering a two-chunk transfer, correct payload
  reconstruction, valid/bad CRC, size rejection, and malformed packet length.
- This candidate deliberately does not render text yet. Its purpose is to
  verify actual WLED/NimBLE transfer behavior before glyph parsing is connected
  to `iDotMatrix Display`.
- Hardware result: the app stopped before bulk processing (`bulkChunks=0`). The
  incorrect AE01 routing assumption was fixed in 0.6.3-dev.2.

## 0.6.2 - 2026-09-03 - Stable clock and unified display

- Promoted the 0.6.2 development line after physical-hardware validation of the
  clock and the shared `iDotMatrix Display` effect.
- Confirmed native 16x16 clock output and transitions between clock and
  graffiti without allocating separate WLED effects.
- Includes the configurable 16x16/32x32/64x64 logical profile, strict size
  matching, and optional nearest-neighbour rescale introduced in 0.6.2-dev.1.
- Retains WLED as the clock/timezone authority and retains `Solid` for the
  full-screen RGB command.

## 0.6.2-dev.2 - 2026-09-03 - Unified display effect

- Replaced the separate `iDotMatrix Framebuffer` and `iDotMatrix Clock` effects
  with one `iDotMatrix Display` effect. Graffiti and clock now switch the
  renderer's internal content without consuming another WLED effect entry.
- Kept full-screen RGB mapped intentionally to WLED `Solid`; the next graffiti
  or clock command reclaims `iDotMatrix Display`.
- Confirmed the initial clock implementation on physical hardware with the
  official app and native 16x16 mapping.
- Fixed the sticky `BLE restart required` diagnostic. It is now derived from
  the configured name/profile versus the values active in the BLE stack.
- Observed `name=IDM-666` and `nameActive=IDM-666` while the app continued to
  show an older name. This demonstrates app/OS-side discovery caching rather
  than a Usermod configuration or advertising-name failure.
- Extended the adapter test to prove that clock and graffiti share exactly one
  dynamically registered effect. This remains a candidate pending hardware
  regression of the unified effect.

## 0.6.2-dev.1 - 2026-09-03 - Clock and configurable logical mapping

- Added confirmed `08 00 06 01 FLAGS R G B` clock-command decoding and ACK.
- Added the dedicated `iDotMatrix Clock` WLED effect and explicit display-mode
  ownership transitions between clock, graffiti, and `Solid`.
- Ported the eight reconstructed 16x16 clock styles from the standalone
  reference into the WLED-independent renderer.
- Added app-selected colour, 12/24-hour conversion, and the optional 30-second
  time / 5-second date cycle.
- Reused WLED `localTime`, NTP, timezone, and DST instead of creating another
  clock. The app time packet remains acknowledged but is not applied.
- Added a 16x16/32x32/64x64 profile dropdown, strict dimension matching by
  default, and optional nearest-neighbour logical-to-segment rescale.
- Added `IDM-` name normalization, a 15-byte advertising-safe maximum, and
  diagnostics when a name/profile change requires reboot.
- Extended protocol, renderer, and adapter host tests. The clock was subsequently
  validated on hardware and the effect model was simplified in 0.6.2-dev.2;
  stable 0.6.1 remains the fallback release.

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
