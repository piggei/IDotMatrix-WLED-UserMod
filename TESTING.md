# Testing

## 0.9.3-rc.1 optional Buzzer bridge and disconnect notification

This release candidate changes the sound architecture but deliberately keeps the
iDotMatrix wire protocol unchanged. The primary 0.9.3 gate is therefore consumer
integration rather than GPIO/LEDC qualification inside this repository.

Automated checks must verify:

- `IDotMatrixBuzzer.*` is absent from the source package;
- no iDotMatrix buzzer GPIO, Active/Passive, polarity, LEDC, `esp_timer` or local
  buzzer-test implementation remains;
- the package compiles and links without the Buzzer Usermod even when its repository exists beside the WLED tree;
- with WLED Buzzer Usermod rc.7 present, calls use the documented weak C bridge without including the external C++ header;
- Alarm, Program/Schedule, Countdown, BLE connection and BLE disconnection still produce logical sound requests;
- the settings UI contains only **Buzzer → Enable** and the orange
  **Requires the WLED Buzzer Usermod.** note;
- without the external Usermod, the Enable control is visibly disabled.

Hardware qualification completed before RC1 promotion:

- firmware without WLED Buzzer Usermod: build PASS; Buzzer Enable is disabled;
- firmware with WLED Buzzer Usermod: Buzzer Enable is available;
- BLE connection: `connect` PASS;
- BLE disconnection: `disconnect` PASS;
- natural Countdown completion: one `triple_beep` PASS;
- Alarm: looping `triple_beep` PASS, including the RC7 550 ms inter-repeat silence;
- Program/Schedule: one `notification` PASS;
- the broader reboot, silent-Alarm, no-sound Program and repeated-event checks were also reported PASS.

Stable 0.9.2 remains the previous stable baseline. RC1 is a version/documentation promotion of the physically tested runtime; no functional runtime change was introduced during the renumbering; the app-visible release byte was restored from temporary 0x0A to the 0.9 line value 0x09 from the temporary 0.10.0 development identifiers.


## Historical 0.9.2 final buzzer timing qualification

The final 0.9.2 release is promoted directly from `0.9.2-dev.11`, after physical
hardware requalification of the runtime buzzer paths. The final promotion does
not alter runtime code beyond the build identifier.

Hardware validation confirmed:

- manual **Test buzzer**: one regular three-pulse trill;
- BLE connection: one clean short beep;
- natural Countdown completion: one regular three-pulse trill;
- Alarm with protocol sound enabled: stable repeating rhythm while media is visible;
- silent Alarm remains silent;
- Program/Schedule with global sound enabled: stable finite notification groups;
- Alarm and Program remain regular with heavier GIF/image media;
- Passive low-level-trigger operation returns to its safe silent HIGH idle state.

The qualified ESP32 timing path is `buzzerTiming=esp_timer-2ms`. The diagnostic
`lateMax` remains available in `/json/info` for future regression work; a
`buzzerTiming=wled-loop` value indicates the compatibility fallback rather than
the qualified real-time service path.


This document is the consolidated validation plan and current evidence for
iDotMatrix WLED Usermod **release 0.9.3 / build 0.9.3-rc.1**. Stable 0.9.2 remains the previous hardware-qualified baseline.

## Automated host regression

From the repository root:

```sh
./run_host_tests.sh
```

The suite covers protocol framing and ACKs, BLE FA02 assembly, Bulk/multipart
media, Alarm and Program/Schedule transactions, Carousel and Preset routing,
clock/text/timer/scoreboard/audio rendering, GIF/media handling, WLED ownership,
build-profile normalization, partition geometry and package consistency.

Important regressions include:

- complete 16654-byte 64-glyph / 32x64 TEXT objects through Bulk, Carousel and
  Preset paths;
- transactional Preset/Carousel failure paths;
- live TEXT ownership over active Preset/Carousel playback;
- original-app Graffiti full-raster multipart transfer using the exact 64x64
  `4096 + 4096 + 4096 = 12288` byte sequence and command-specific ACKs;
- compact PNG and generic Bulk RAW remaining separate from the Graffiti path;
- the validated C3 dual-slot OTA partition geometry and current override layout.

Where supported by the host compiler:

```sh
./run_host_sanitizers.sh
```

This builds the main protocol, renderer, media and automation regressions with
AddressSanitizer and UndefinedBehaviorSanitizer.

## MatrixPortal / HUB75 qualification

Validated on Adafruit MatrixPortal S3 + 64x64 HUB75 with WLED 17.0.0-devV5
native HUB75 backend, qualification commit
`06ae26db67107cb3f6a3d107a92340035991a063`:

- logical profiles 16x16, 32x32 and 64x64 through the universal scaler;
- native 64x64 TEXT/font path including large glyph payloads and scrolling;
- GIF playback and cached Carousel playback;
- persistent Carousel uploads and transfer indicator;
- Alarm multi-packet media upload and execution;
- Program/Schedule multi-packet upload, flow-control ACKs and execution;
- Preset / Default volatile six-slot bank and mixed media playback;
- WLED <-> iDotMatrix ownership transitions;
- AudioReactive/phone audio visualizers;
- Countdown, Stopwatch and Scoreboard graphics;
- combined 64x64 time/date layouts for clock styles 0 and 3;
- on native 64x64 styles 0 and 3, HH:MM stays unchanged while the date slash and month field are shifted two physical LEDs right and the day stays fixed.

## ESP32-C3 4 MB OTA qualification

The 0.9.1 release includes a separately hardware-validated C3 OTA profile:

```text
overrides/esp32c3-16x16-audio-ota.ini
env:esp32c3dev_idotmatrix_audio_16x16_ota
```

Hardware used:

```text
Controller : ESP32-C3 SuperMini, 4 MB flash
Display    : WS2812B ECO 16x16
WLED base  : d55037f7510541eddc390c8f3d01afc5787aa44a
Framework  : Arduino 3.3.8 / ESP-IDF 5.5.4, WLED shared-RMT
BLE        : NimBLE-Arduino 2.5.1
Filesystem : dual-slot OTA table, 640 KiB nominal LittleFS
```

Measured firmware sizes:

```text
without OTA, total image report : 1,541,496 bytes
with OTA, total image report    : 1,550,862 bytes
with OTA, firmware.bin          : 1,551,008 bytes
OTA slot size (0x1A0000)        : 1,703,936 bytes
remaining per slot              :   152,928 bytes (~149 KiB)
```

Hardware stress evidence:

- first install of the new partition table over USB/serial: PASS;
- three consecutive WLED OTA updates: PASS;
- 12 stored Carousel assets: PASS;
- six Preset assets and Preset playback: PASS;
- three Schedule activities: PASS;
- BLE + shared-RMT + AudioReactive + GIF frame cache in the same build: PASS;
- return from iDotMatrix-owned content to native WLED effects: PASS;
- persistent filesystem content remained available across OTA cycles: PASS.

Representative diagnostics after the heavy test and return to WLED:

```text
filesystem        : 73 KiB used / ~655 KiB reported total
free heap         : 62,384 bytes
historical minimum: 31,756 bytes
largest free block: 49,152 bytes
GIF cache stats   : build 28 / reuse 78
protocol reset    : status ok
```

While a Preset/GIF was active, free heap was about 57.5 KiB with a historical
minimum around 34 KiB and a largest free block around 45 KiB. No progressive
fragmentation, reboot or LED/BLE failure was observed during the reported test.

This evidence classifies the C3 AudioReactive + OTA profile as
**hardware-validated** for the stated controller/display/baseline combination.
The legacy no-OTA C3 profile remains available as a conservative option.

## Graffiti full-raster validation

The 0.9.1 Graffiti implementation is grounded in two independent project
sources:

1. a Bluetooth capture from the official 64x64 app showing three complete FA02
   packets with a repeated 9-byte type-0 header, markers `0x00/0x02`, complete
   size 12288 and 4096-byte RGB chunks;
2. standalone emulator B171, where the same protocol was already implemented and
   validated.

Host regression reproduces the exact transfer and requires:

```text
chunk 1 marker 00 -> ACK 05 00 00 00 02
chunk 2 marker 02 -> ACK 05 00 00 00 02
chunk 3 marker 02 -> ACK 05 00 00 00 01
```

It verifies byte-for-byte renderer staging, continuation rejection without an
active transfer and timeout cancellation. Compact PNG remains covered separately.

Hardware validation is now complete on the physical 64x64 MatrixPortal/HUB75
target using the official app. Several complex photographic images were sent
through Graffiti after flashing the 0.9.1 multipart implementation; all rendered
successfully. Together with the captured three-packet sequence, emulator B171 and
the byte-for-byte host regression, this classifies the Graffiti full-raster path
as **hardware-validated** for the qualified 64x64 target.

## Filesystem failure-path qualification

Host behavioral tests cover Preset and Carousel LittleFS transactions. Preset
activation is all-or-nothing within a running session and is tested for
intermediate backup/promotion failures, write failure and CRC rejection. Preset
remains intentionally volatile: boot removes active, pending and backup files
instead of recovering the previous session. Carousel tests cover manifest-save
failure rollback and reporting.

The regression suite also protects TEXT ownership: external live TEXT must
suspend active Preset/Carousel playback, while TEXT rendered internally by those
players must not self-suspend.

## Deferred physical coverage

A physical 32x32 panel is not required for the current stable release. The
32x32 logical path has been exercised through the 64x64 hardware/scaler.
Additional iOS-specific work remains isolated and deferred unless a new
compatibility requirement makes it necessary.

## Historical 0.9.2 buzzer / Clock regression evidence

The release retains the following qualified/required behaviors:

- existing active buzzer mode remains unchanged;
- passive direct/high-trigger mode emits the non-blocking trill and returns LOW while silent;
- passive low-level-trigger transistor module emits the trill and remains HIGH while silent;
- changing buzzer type/trigger from Usermod settings reinitializes hardware cleanly;
- **Test buzzer**, Alarm and Program/Schedule use the same logical timing as before;
- Clock style, 12/24-hour mode, date visibility and RGB colour survive reboot;
- selecting the iDotMatrix WLED effect before connecting the app restores those Clock preferences;
- app entry/style-change `showDate=0` transient protection still works;
- device reset clears the stored Clock presentation preferences.

### Passive low-trigger post-tone idle

The passive low-level-trigger backend is already hardware-validated: **Test buzzer** produces exactly three short tones, followed by silence with the GPIO held at its inactive HIGH level, with no residual tone or idle heating.

The final 0.9.2 settings UI was also physically validated: Active shows only **Active buzzer active-high**, Passive shows only **Passive buzzer trigger**, switching type updates the page immediately in both directions, the type-specific option follows the common Buzzer help text without an extra blank gap, and **Test buzzer** appears exactly once below it with a warning to save first.
