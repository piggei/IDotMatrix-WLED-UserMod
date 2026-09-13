# Testing

This file defines the **0.8.2 regression procedure**. All 0.8.1 stable
regressions remain mandatory; this build adds source-selection tests for WLED
AudioReactive while preserving the separate physical
ESP32-C3 IDF5/shared-RMT validation line.

## Host regression tests

From the repository root with a C++11 compiler and zlib development files:

```sh
./run_host_tests.sh
```

The suite covers protocol framing/ACKs, power/brightness/RGB, DIY/graffiti,
clock/text rendering, all seven light effects, countdown, stopwatch, scoreboard,
Audio/Rhythm framing/rendering, buzzer timing, 2D mapping, bulk CRC42, RAW
publication, FA02 fragmentation, compact PNG decode, GIF RX/promotion/playback,
WLED ownership, repeat-GIF replacement, build-profile normalization, compact
LZW12/cache behavior, partition geometry, the NimBLE 1.x/2.x source bridge and
the supported C3 profile contract.

The current host suite also includes behavioural `IDotMatrixAutomation`
coverage, the 0.8.2 BLE-routing/ownership regressions, persistent Carousel-cache reuse, and
explicit failure injection. The host tests verify:

- GIF promotion by direct rename;
- rename failure with successful streamed-copy fallback;
- rename plus copy failure publishing `gif-cache-io`;
- successful GIF playback after a failed promotion;
- schedule create/replace persistence;
- temporary-file write failure;
- replacement rename failure with rollback to the previous valid program;
- rollback failure converging to an empty metadata/media state;
- NVS write failure rolling media back to the previous program;
- NVS rollback-metadata failure converging to an empty safe state;
- reboot/loadPersistence after failed replacement;
- missing media and corrupt metadata recovery;
- alarm metadata persistence;
- WLED-time authority, app-time fallback, weekday matching, and midnight-spanning schedules.

Where supported by the host compiler, run the sanitizer subset too:

```sh
./run_host_sanitizers.sh
```

It builds the protocol, renderer, bulk transfer, FA02 assembler, automation and
media regressions with AddressSanitizer and UndefinedBehaviorSanitizer.

## WLED build validation

0.8.2 retains two supported build families from the 0.8.1 baseline:

**Classic ESP32** — WLED 16.0.1, supplied legacy overrides, NimBLE 1.4.3.

```sh
cp ../wled-usermod-idotmatrix/platformio_override.ini.example platformio_override.ini
pio run -e esp32dev_idotmatrix_16x16 -t clean
pio run -e esp32dev_idotmatrix_16x16
```

**ESP32-C3** — pinned WLED commit `d55037f7510541eddc390c8f3d01afc5787aa44a`,
IDF5/shared-RMT, NimBLE 2.5.1.

```sh
cp ../wled-usermod-idotmatrix/platformio_override.ini.c3 platformio_override.ini
pio run -e esp32c3dev_idotmatrix_16x16 -t clean
pio run -e esp32c3dev_idotmatrix_16x16
```

Expected common facts for this release: `release=0.8.2`, `build=0.8.2`, AnimatedGIF 1.4.7, no-OTA partitioning,
only the iDotMatrix custom Usermod, and no dependency on `esp-nimble-cpp` or
`ESP32 BLE Arduino`. The C3 build must additionally prove ESP-IDF 5,
`WLED_USE_SHARED_RMT`, NimBLE 2.x and the pinned WLED base marker.

A successful compile proves build compatibility only. Physical support requires
the hardware procedures below. ESP32-S3, PSRAM/direct native 64x64 and HUB75
remain pending validation for the 0.9 line.

## Build profiles

| Logical GIF profile | Override | Hardware target set | Expected runtime decoder |
|---|---|---|---|
| 16x16 classic | `platformio_override.ini.example` | classic 4/8/16 MB + WROVER/S3 wrappers | `compact12/cache` without PSRAM; LZW12 decoder with UI capped to 16x16 |
| 16x16 C3 | `platformio_override.ini.c3` | supported ESP32-C3 4 MB | `compact12/cache`, IDF5/shared-RMT, NimBLE 2.x |
| 32x32 | `platformio_override.ini.32x32` | classic 4/8/16 MB + WROVER PSRAM + S3 8/16 MB PSRAM | `animatedgif11` |
| 64x64, normal WLED feature set | `platformio_override.ini.64x64` | classic 4/8/16 MB + WROVER PSRAM + S3 8/16 MB PSRAM | PSRAM direct or no-PSRAM cache depending hardware |
| 64x64, classic low-RAM | `platformio_override.ini.64x64-lite` | classic 4/8/16 MB only | `compact12/cache` without PSRAM |
| 64x64 + HUB75 | `platformio_override.ini.hub75` | WLED 16.0.1 HUB75 board/pinout wrappers | hardware-dependent; experimental |

`tests/test_platformio_profiles.py` statically validates the environment matrix,
base-environment names, profile flags, pinned dependencies, no-OTA policy, and
4/8/16/32 MB partition geometry on every host test run.

## Stable 16x16 hardware regression

Use a classic ESP32, a WLED 16x16 2D matrix, and I2S LED output.

1. Flash by USB/serial and reboot.
2. Confirm WLED remains reachable over Wi-Fi for at least 15 seconds.
3. Confirm `/json/info` reports `release=0.8.2`, `build=0.8.2`, and BLE advertising.
4. Connect with the official iDotMatrix app.
5. Verify power OFF/ON and brightness changes.
6. Verify red, green, blue, white, and black full-screen colours.
7. Enter DIY/Graffiti, draw pixels, and display a saved Graffiti image.
8. Test a clock style and verify WLED/NTP time is shown.
9. Send scrolling text with the speed slider at both extremes and verify an obvious slow/fast difference.
10. Browse/send multiple cloud/static images.
11. Send at least ten GIF animations in sequence.
12. While a GIF is playing, return to clock and then to a static colour/WLED effect.
13. Disconnect and verify BLE advertising resumes.
14. Reconnect and repeat one image and one GIF transfer.

## 32x32 logical profile regression

Classic ESP32, physical 16x16 matrix, no HUB75:

1. use `platformio_override.ini.32x32`;
2. set `screenType=32x32` and `rescale=true`;
3. confirm `/json/info` reports `profile=32x32`, `canvas=16x16`, and `gifDecoder=animatedgif11`;
4. test clock and text;
5. send at least ten GIFs consecutively, including a complex animation;
6. return directly to normal WLED effects and confirm decoder RAM is released;
7. verify no WLED Error 8, reboot, stale GIF, or BLE reconnect requirement.

Recorded hardware result during development:

- `gifDecoderBytes=9372`;
- minimum heap 7904 B during the stress sequence;
- return to WLED recovered about 33.5 KiB free heap and a 28.6 KiB largest block.

## 64x64 logical / classic ESP32 no-PSRAM regression

This remains the key larger-profile memory regression carried into 0.8.1.

Use:

- `platformio_override.ini.64x64-lite`;
- physical WLED matrix 16x16;
- `screenType=64x64`;
- `rescale=true`;
- no HUB75.

After reboot, confirm:

```text
release=0.8.2
build=0.8.2
profile=64x64
canvas=16x16
gifDecoder=compact12/cache
gifDecoderBytes=16128
```

Then test in this order:

1. normal WLED effect -> GIF;
2. clock/date -> GIF;
3. static/cloud image -> GIF;
4. one light GIF;
5. one large/complex GIF;
6. one long animation (100-frame class if available);
7. alternate two known-good GIFs at least ten times **without** manually selecting Solid;
8. GIF -> clock -> GIF -> static image -> GIF -> normal WLED effect;
9. repeatedly open the Web UI and `/json/info` during cache preparation and playback;
10. leave a representative cached GIF looping for at least 15-20 minutes and recheck heap/network responsiveness.

During cache preparation the physical panel should go black rather than visibly
showing WLED Solid. The original WLED primary colour must be restored after
success, failure, or a Web UI/API takeover.

A successful cached GIF should report `gifCachedFrames=N` and `content=gif`.
`gifCacheWaits=N low=M guard=9216` is allowed and means the cache builder yielded
to transient WLED/network allocations.

A cache build should fail with `mediaError=gif-ram-reserve` only after free heap
remains below the guard continuously for roughly two seconds. The Web UI/BLE
must remain alive after such a failure and the next valid GIF must be able to
start without a manual Solid reset.

A cache exceeding 512 KiB must fail cleanly with `mediaError=gif-cache-full`.

### Release hardware result

The compact-cache sequence first validated in 0.8.0 and retained by 0.8.1 passed:

- 10+ A/B GIF replacements;
- large and 100-frame animations;
- WLED effect -> GIF;
- clock/date -> GIF;
- static image -> GIF;
- return to WLED;
- live Web UI and `/json/info` access;
- black staging with primary-colour restoration.

Representative final GIF status:

```text
gifDecoder=compact12/cache
gifDecoderBytes=16128
gifProbe=31848 largest=26612 reserve=10240
gifCachedFrames=20
heap=33076 min=7468 largest=26612
content=gif
```

The historical `min` heap is expected to be lower than current playback heap;
cache predecode is the high-pressure phase.

## Light-effect and Usermod policy regression (carried forward from 0.8.0)

The seven app light effects are intentionally rendered by the Usermod and must
remain under `iDotMatrix`. For the stable hardware regression:

1. start from a normal WLED effect and verify `/json/info` reports `release=0.8.2` and `build=0.8.2`;
2. in the iDotMatrix app select Solid and verify WLED shows `iDotMatrix`,
   not native WLED `Solid`;
3. select each of the seven light effects and compare motion, palette and speed
   against the standalone ESP32 reference; for effects 3, 4 and 5 specifically,
   verify every visible scroll step is exactly one pixel with no multi-pixel catch-up jump;
4. for a multi-colour effect, change palette and speed repeatedly and verify the
   WLED Web UI remains responsive;
5. while an iDotMatrix light effect is running, select a normal WLED effect and
   verify it takes over immediately;
6. start a WLED playlist, then return to the iDotMatrix app and change/select an
   effect; verify the playlist is terminated, `iDotMatrix` becomes selected
   immediately, remains selected past the playlist's former next-step time, and
   no already queued or later playlist preset steals the display;
7. restart the playlist manually from WLED and confirm normal WLED playlist
   playback resumes until the next explicit iDotMatrix content command;
8. finish with the existing GIF/clock/image replacement sequence to confirm the
   stable media lifecycle and memory behaviour are unchanged;
9. for every supplied override, confirm `custom_usermods` contains only the
   external iDotMatrix symlink and does not inherit `${env:<base>.custom_usermods}`;
9. if additional Usermods are added manually, repeat the memory/media stress test
   because the distributed profiles intentionally avoid unvalidated heap pressure.

Expected diagnostics while a light effect is active include:

```text
release=0.8.2
build=0.8.2
lightEffect=<0..6> speed=<0..100> colors=<n>
content=light
```

The effect engine adds no new heap allocation, so free-heap behaviour should stay
close to the established non-GIF baseline.

## Countdown / stopwatch / scoreboard regression (carried forward from 0.8.0)

After the light-effect test, verify the three app tools while WLED
continues to show the single `iDotMatrix` effect:

1. confirm `/json/info` reports `release=0.8.2` and `build=0.8.2`;
2. start a countdown longer than five seconds and verify the orange timer icon above white `MM:SS`;
3. let it enter the final five seconds and verify the digits/separator turn red;
4. pause and resume the countdown and verify the remaining time is preserved;
5. let a countdown finish and verify the app receives the completion event;
6. start the stopwatch, pause it, wait, and resume it; paused time must not be counted;
7. while countdown or stopwatch is running, select a normal WLED effect, wait a
   few seconds, then issue a timer command from the iDotMatrix app; the timer
   must have continued in the background and `iDotMatrix` must reclaim
   the panel;
8. set scoreboard values for both sides and verify team A is blue, the separator
   white, and team B red;
9. try values above 99 and verify the display shows the last two decimal digits,
   matching the standalone reference;
10. finish with light effects and a GIF replacement to confirm no regression in
    the stable light-effect/media paths.

Expected diagnostics include one of:

```text
content=countdown
countdown=run remain=<n>s

content=stopwatch
stopwatch=run elapsed=<n>s

content=scoreboard
score=<a>:<b>
```

## Buzzer / timer icon regression (carried forward from 0.8.0)

1. In Config → Usermods → iDotMatrix verify the buzzer GPIO selector, `buzzerActiveHigh`, and the **Test buzzer** button are visible.
2. Leave the buzzer GPIO unassigned and press **Test buzzer**; the page should report that the GPIO must be configured and saved.
3. Set a free GPIO and the correct polarity, save/apply the config, then press **Test buzzer**. Exactly one three-short-beep trill must play and then stop.
4. `/json/info` should return to `buzzer=active gpio=<n> ... idle` after the one-shot test finishes.
5. Set a GPIO already used by WLED; the module must report it unavailable, the test request must fail, and the pin must not be driven.
6. Countdown: verify the orange timer icon appears above MM:SS and its red hand changes position while running; last five seconds remain red.
7. Stopwatch: verify the same timer icon appears, the hand advances from elapsed milliseconds, and pause/resume preserves elapsed time.
8. Re-run the 0.8.0 light-effect scroll tests and the stable GIF replacement/cache regression sequence.

The audible button is also a direct hardware validation path independent of alarm/program triggers.

## Program activation sound regression (carried forward from 0.8.0)

1. Configure a valid active-buzzer GPIO and verify **Test buzzer** still emits one group of three short beeps.
2. Create/enable a program whose global sound option is enabled and whose time window includes the current time.
3. When the activity first becomes active, count exactly **three groups of three short trills** (nine short beeps total), separated by the normal longer inter-group pause.
4. Leave the program active for several minutes. The buzzer must remain silent after the finite activation notice; it must not restart on subsequent automation loop iterations.
5. End/disable the program while its notice is still playing; the remaining notice must stop.
6. Trigger an alarm with its buzzer option enabled while a program is active. The alarm must take priority and use the repeating trill for its configured alarm duration.
7. After the alarm ends, a resumed program may emit its normal finite activation notice when the activity is actually started again; it must never become a continuous schedule buzzer.

## Audio / Rhythm regression (carried forward from 0.8.0)

1. Start from the clock and select Audio/Rhythm effect 1. WLED must switch to
   `iDotMatrix`, the breakdancer must appear, and `/json/info` must show
   `content=audio LEVEL mode=1`.
2. Test all five LEVEL modes and verify that their animation reacts to sound.
3. Test all five FFT modes; `/json/info` must report modes 1 through 5 and the
   display must continue updating across BLE write boundaries without returning
   to the clock.
4. Select a normal WLED effect and verify that it takes control. Generate new
   audio data and verify that `iDotMatrix` is selected again.
5. Repeat at least one LEVEL and one FFT mode with logical profiles 32x32 and
   64x64-lite mapped to the physical 16x16 matrix.
6. Re-run a GIF from the stable regression set and both manual and scheduled
   buzzer tests to confirm that audio added no media or timing regression.

## Audio source selection regression (0.8.2)

Use the normal C3 profile first, then the `platformio_override.ini.c3-audio`
profile on the same pinned WLED commit.

1. **Normal C3 build, Phone / BLE:** verify all ten Audio/Rhythm visualizers
   behave exactly as in 0.8.1. `/json/info` must show
   `audioSource=phone active=phone` and `audioReactive=absent`.
2. **Audio profile, Phone / BLE:** enable WLED AudioReactive and confirm the
   iDotMatrix visualizer still follows the phone stream. Diagnostics may report
   AudioReactive as present, but the active source must remain `phone`.
3. **Audio profile, WLED AudioReactive:** configure and enable the WLED
   microphone, select the local source, then exercise at least one LEVEL and one
   FFT visualizer. Cover all five visualizer modes if practical. The app must
   still select the family/mode while the local microphone controls amplitude and
   spectrum. Diagnostics must show `active=audioreactive` and
   `audioReactive=data`.
4. Stop/disable WLED AudioReactive while the explicit local source is selected.
   The visualizer must become silent; it must **not** silently start following
   phone amplitude. Re-enable AudioReactive and verify local response resumes.
5. Select **Auto**. With AudioReactive running, verify local response and
   `active=audioreactive`. Disable AudioReactive and verify the next phone audio
   frames are used with `active=phone`; re-enable it and verify automatic return
   to local data.
6. While local audio is active, switch repeatedly between WLED effects, GIFs,
   and Audio/Rhythm. Confirm normal WLED ownership/reclaim semantics and no LED
   spikes, BLE disconnects, watchdogs or media errors.
7. Stress C3 + AudioReactive with a heavy GIF and Web UI/WebSocket activity, then
   record `freeheap`, `min`, `largest`, `gifProbe`, `gifCachedFrames`, reset
   reason and observed FPS. Compare with the 0.8.1 baseline rather than judging
   free heap alone.

The host test `tests/test_audio_source.cpp` covers mode normalization and the
16-band to 8-band scaling. `tests/test_wled_adapter.cpp` verifies that local
audio overrides level/bands without changing BLE-selected visualizer family/mode
and that returning to Phone mode restores the original semantics.

## Usermod settings regression (carried forward from 0.8.0)

1. Verify that `ScreenType` has no inline description and shows an orange
   reboot/app-reconnection warning on the following line.
2. Verify that `DeviceName` renders as fixed text `IDM-` followed by a suffix-only
   input and the same orange warning below it. Save, reboot, and confirm that
   `/json/info` and BLE advertising expose the complete `IDM-<suffix>` name.
3. Verify that the complete rescale sentence labels the checkbox and that no
   separate side description remains.
4. Verify that **Test buzzer** remains functional and that `Save before testing.`
   appears below it in orange rather than beside the button.
5. Load a legacy configuration containing a full `IDM-...` value and confirm
   that 0.8.1 displays only its suffix without duplicating the prefix.
6. Verify that every visible field label ends with `:` and that the rescale row
   reads `Scale the logical profile to the selected WLED 2D segment:` instead
   of `Rescale`.

## Build-aware resolution regression (carried forward from 0.8.0)

1. Compile `platformio_override.ini.example`: the dropdown must contain only
   16x16, and the Rescale row must not exist.
2. Compile `.32x32`: the dropdown must contain only 16x16 and 32x32, and
   Rescale must be visible.
3. Compile `.64x64` or `.64x64-lite`: the dropdown must contain 16x16, 32x32
   and 64x64, and Rescale must be visible.
4. Save 64x64, then install the 32x32 build while preserving configuration:
   the effective profile must become 32x32 after boot.
5. Save 32x32 or 64x64, then install the standard build: the effective profile
   must become 16x16 and Rescale must be forced off.
6. Confirm `/json/info` reports the normalized profile and the decoder backend
   corresponding to the compiled override.

## Recovery tests

- Interrupt/cancel a media transfer and then send a normal command.
- Disconnect during a transfer, reconnect, and verify the next valid command.
- Send a CRC-invalid replacement GIF and confirm the currently playing GIF is not destroyed before validation.
- Force a post-validation GIF preparation failure; recovery must not select an empty `iDotMatrix`.
- Change WLED effect from the Web UI while GIF staging is active; the saved primary colour must be restored.
- Change `deviceName` or `screenType`; verify `/json/info` reports that a restart is required until reboot.
- Configure a digital RMT bus and verify the Usermod refuses to start BLE rather than entering the known Bluetooth/RMT reboot loop.

## Memory-safety checks

For 64x64 no-PSRAM testing:

- never treat a truncated 12-bit dictionary as an acceptable optimization;
- `largest` must be large enough for the compact workspace before allocation;
- free heap after admission must preserve the 10 KiB reserve;
- the 9 KiB runtime guard is a wait/yield threshold, not a one-sample abort;
- Web UI responsiveness is a release criterion, not merely absence of reboot;
- after cached playback starts, the compact decoder workspace must have been released.

The detailed rationale and failed experiments are in `ARCHITECTURE.md`.

## ESP32-C3 0.8.1 release validation

Supported C3 release profile:

```sh
git clone https://github.com/wled/WLED.git WLED-idot-c3
cd WLED-idot-c3
git checkout d55037f7510541eddc390c8f3d01afc5787aa44a
cp ../wled-usermod-idotmatrix/platformio_override.ini.c3 platformio_override.ini
pio run -e esp32c3dev_idotmatrix_16x16 -t clean
pio run -e esp32c3dev_idotmatrix_16x16
```

The release hardware test used a 4 MB ESP32-C3, a physical 16x16 matrix on
GPIO4, Wi-Fi, BLE, and the standard compact12/cache 16x16 profile. Required
`/json/info` markers:

```text
release=0.8.1
build=0.8.1-audit-fix1
RMT+BLE=ESP32-C3 shared-RMT
framework=WLED IDF5/shared-RMT
wledBase=d55037f
nimble=2.x API
```

Recorded release-line evidence from the dev.3 hardware run:

- BLE advertising: stable, no LED spikes;
- BLE connected: stable, no LED spikes;
- static images and animated GIFs: working;
- roughly 100 media/animation changes plus about 20 WLED effects: no pixel spikes or reboot;
- WLED WebSocket remained active;
- final snapshot: free heap ~74 KB, minimum observed ~37.5 KB, largest block 64 KB, `reset=poweron`;
- a few transient red Web UI connection banners appeared during aggressive UI stress, while the requested WLED effect still started and the device remained responsive. Treat this as a Web UI/network observation, not an LED/BLE failure, unless it becomes reproducible outside stress.

The earlier IDF4 dev.1/dev.2 A/B experiments are retained in `HISTORY.md`; they
are intentionally absent from the stable package profiles.

## 0.8.2 packaging check

Before tagging the release:

1. run `./run_host_tests.sh`;
2. run `./run_host_sanitizers.sh` where ASan/UBSan are available;
3. confirm `library.json` reports release `0.8.2` and runtime diagnostics report
   `release=0.8.2` plus `build=0.8.2`;
4. confirm `platformio_override.ini.c3` remains the supported minimal C3 path and
   `platformio_override.ini.c3-audio` is clearly identified as the optional
   AudioReactive variant;
5. verify all local Markdown links;
6. verify the archive contains one root directory named `IDotMatrix-WLED-UserMod-0.8.2`;
7. exclude `.pio`, build products, caches and editor temporaries;
8. perform a short C3 smoke test after any release-only code change.

## Carousel hardware regression (0.8.2)

1. Upload a three-item Device Assets page and verify `carousel=playing` after the final transfer.
2. Repeat the six-GIF page that failed on dev.4; verify all six slots are stored and no app error occurs.
3. Upload all 12 slots with at least one TEXT item and verify mixed playback.
4. Disconnect BLE while Carousel is active; playback must continue.
5. Select a native WLED effect; Carousel must stop without deleting stored assets.
6. Select `iDotMatrix`; the stored Carousel must resume.
7. Configure `iDotMatrix` as the WLED boot effect, power-cycle, and verify stored Carousel startup without app reconnection.
8. Later repeat the same behavior on native 64x64 hardware.


## 0.8.2 transition/transport/storage qualification additions

These checks are stable-release qualification requirements and are distinct from host
syntax/build tests:

1. start LEVEL or FFT audio, then immediately send protocol reset (`03 80`);
   confirm the reset is processed rather than consumed as audio;
2. repeat audio -> Carousel configure (`02 01`) and audio -> Carousel enter
   (`0A 01`);
3. disconnect during fragmented FA02, reconnect, and verify the next complete
   command succeeds; confirm `bleRx` timeout/drop counters are sensible;
4. leave a fragmented FA02 or bulk transfer incomplete for more than five
   seconds and verify deterministic cleanup and subsequent recovery;
5. store at least two Carousel GIFs, rotate through them repeatedly, and verify
   `gifCacheStats` shows cache reuse rather than one rebuild per revisit;
6. make the first Carousel slot unplayable while a later slot is valid; playback
   must advance and `carouselFailed` must identify the failed slot without a hot
   loop;
7. replace an active cached GIF with a candidate that fails during cache
   preparation/commit; the previous known-good GIF must remain recoverable and
   `mediaError` must expose the failure;
8. reset the iDotMatrix device state and verify Carousel, alarms and schedules
   are absent afterward while WLED remains running; `protocolReset` must report
   `status:ok carousel:ok automation:ok` or an explicit partial failure identifying the failing subsystem;
9. power-cycle with deliberately created feature-owned `.tmp`/`.bak` states and
   verify boot recovery converges to the last committed manifest/program media;
10. for future rebuilds or platform changes, repeat an uninterrupted 24-48 hour C3 soak as final confidence evidence.

Test reports must label results as **host behavioural**, **syntax/build**,
**sanitizer**, **firmware compile**, **hardware functional**, or **hardware
soak**. A syntax-only Carousel compile is not behavioural Carousel coverage.
