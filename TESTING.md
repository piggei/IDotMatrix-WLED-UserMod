# Testing

This file defines the **0.8.0 stable regression procedure** and records the
hardware configurations used to validate the release. The 0.7.1 media/memory
regressions remain mandatory because 0.8.0 builds directly on that baseline.

## Host regression tests

From the repository root with a C++11 compiler and zlib development files:

```sh
./run_host_tests.sh
```

The suite covers protocol framing/ACKs, power/brightness/RGB, DIY/graffiti,
clock/text rendering, all seven light effects, countdown, stopwatch, scoreboard,
Audio/Rhythm framing/rendering, alarm/program parsing, buzzer timing, 2D mapping,
bulk CRC32, RAW publication, FA02 fragmentation, compact PNG decode, GIF
RX/promotion/playback, WLED ownership, repeat-GIF replacement, build-profile
normalization, the compact 64x64 decoder/cache path, and the complete static
PlatformIO target/partition matrix shipped with the repository.

The compact-GIF tests include real 64x64 fixtures and stress the 4096-code
LZW12 implementation without using a truncated dictionary.

## WLED build validation

The supplied PlatformIO files target **WLED v16.0.1**. Before building, copy one
media profile to `platformio_override.ini` in the WLED source directory.

For the validated classic 4 MB baseline:

```powershell
pio run -e esp32dev_idotmatrix -t clean
pio run -e esp32dev_idotmatrix
```

The normal `example`, `32x32`, and `64x64` overrides should also be compiled for
each newly supported hardware target when release/build infrastructure is
available:

```text
esp32dev_idotmatrix
esp32dev_8M_idotmatrix
esp32dev_16M_idotmatrix
esp32_wrover_idotmatrix
esp32s3dev_8MB_opi_idotmatrix
esp32s3dev_8MB_qspi_idotmatrix
esp32s3dev_16MB_opi_idotmatrix
```

`64x64-lite` intentionally applies only to the three classic-ESP32 targets.
`platformio_override.ini.hub75` has its own board/pinout-specific environments;
see `BUILD_PROFILES.md` and compile only profiles matching real or intended
hardware.

Expected facts for every successful build:

- repository/library version: `0.8.0`;
- Espressif platform: `espressif32@~6.13.0`;
- `NimBLE-Arduino @ 1.4.3`;
- `AnimatedGIF @ 1.4.7`;
- default build patch banner: 10-bit / max 16x16;
- 32x32 build patch banner: 11-bit / max 32x32;
- 64x64/HUB75 build patch banner: 12-bit / max 64x64 / dict=4096 / filebuf=1024;
- `WLED_DISABLE_OTA` present and the matching single-app partition table selected;
- no `.dram0.bss` overflow;
- no dependency on `esp-nimble-cpp` or `ESP32 BLE Arduino`.

A PlatformIO compile proves **build compatibility**, not hardware compatibility.
ESP32-S3, PSRAM/direct 64x64, native physical 64x64, and HUB75 remain pending
physical validation.

## Build profiles

| Logical GIF profile | Override | Hardware target set | Expected runtime decoder |
|---|---|---|---|
| 16x16 | `platformio_override.ini.example` | classic 4/8/16 MB + WROVER PSRAM + S3 8/16 MB PSRAM | `animatedgif10` |
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
3. Confirm `/json/info` reports `build=0.8.0` and BLE advertising.
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

This remains the key larger-profile memory regression for 0.8.0.

Use:

- `platformio_override.ini.64x64-lite`;
- physical WLED matrix 16x16;
- `screenType=64x64`;
- `rescale=true`;
- no HUB75.

After reboot, confirm:

```text
build=0.8.0
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

The validated compact-cache sequence retained by 0.8.0 passed:

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

## Light-effect and Usermod-inheritance regression (0.8.0)

The seven app light effects are intentionally rendered by the Usermod and must
remain under `iDotMatrix Display`. For the stable hardware regression:

1. start from a normal WLED effect and verify `/json/info` reports `build=0.8.0`;
2. in the iDotMatrix app select Solid and verify WLED shows `iDotMatrix Display`,
   not native WLED `Solid`;
3. select each of the seven light effects and compare motion, palette and speed
   against the standalone ESP32 reference; for effects 3, 4 and 5 specifically,
   verify every visible scroll step is exactly one pixel with no multi-pixel catch-up jump;
4. for a multi-colour effect, change palette and speed repeatedly and verify the
   WLED Web UI remains responsive;
5. while an iDotMatrix light effect is running, select a normal WLED effect and
   verify it takes over immediately;
6. return to the iDotMatrix app and change/select an effect, verifying that
   `iDotMatrix Display` reclaims the matrix without reboot or stale WLED colour;
7. finish with the existing GIF/clock/image replacement sequence to confirm the
   stable media lifecycle and memory behaviour are unchanged;
8. with `platformio_override.ini.example`, `.32x32`, `.64x64`, or `.hub75`,
   confirm the WLED default/inherited Usermods for the selected base environment
   are still compiled alongside iDotMatrix;
9. with `platformio_override.ini.64x64-lite`, confirm inherited Usermods remain
   intentionally absent unless inheritance from the corresponding base WLED
   environment is explicitly re-enabled.

Expected diagnostics while a light effect is active include:

```text
build=0.8.0
lightEffect=<0..6> speed=<0..100> colors=<n>
content=light
```

The effect engine adds no new heap allocation, so free-heap behaviour should stay
close to the established non-GIF baseline.

## Countdown / stopwatch / scoreboard regression (0.8.0)

After the light-effect test, verify the three app tools while WLED
continues to show the single `iDotMatrix Display` effect:

1. confirm `/json/info` reports `build=0.8.0`;
2. start a countdown longer than five seconds and verify the orange timer icon above white `MM:SS`;
3. let it enter the final five seconds and verify the digits/separator turn red;
4. pause and resume the countdown and verify the remaining time is preserved;
5. let a countdown finish and verify the app receives the completion event;
6. start the stopwatch, pause it, wait, and resume it; paused time must not be counted;
7. while countdown or stopwatch is running, select a normal WLED effect, wait a
   few seconds, then issue a timer command from the iDotMatrix app; the timer
   must have continued in the background and `iDotMatrix Display` must reclaim
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

## Buzzer / timer icon regression (0.8.0)

1. In Config → Usermods → iDotMatrix verify the buzzer GPIO selector, `buzzerActiveHigh`, and the **Test buzzer** button are visible.
2. Leave the buzzer GPIO unassigned and press **Test buzzer**; the page should report that the GPIO must be configured and saved.
3. Set a free GPIO and the correct polarity, save/apply the config, then press **Test buzzer**. Exactly one three-short-beep trill must play and then stop.
4. `/json/info` should return to `buzzer=active gpio=<n> ... idle` after the one-shot test finishes.
5. Set a GPIO already used by WLED; the module must report it unavailable, the test request must fail, and the pin must not be driven.
6. Countdown: verify the orange timer icon appears above MM:SS and its red hand changes position while running; last five seconds remain red.
7. Stopwatch: verify the same timer icon appears, the hand advances from elapsed milliseconds, and pause/resume preserves elapsed time.
8. Re-run the 0.8.0 light-effect scroll tests and the stable GIF replacement/cache regression sequence.

The audible button is also a direct hardware validation path independent of alarm/program triggers.

## Program activation sound regression (0.8.0)

1. Configure a valid active-buzzer GPIO and verify **Test buzzer** still emits one group of three short beeps.
2. Create/enable a program whose global sound option is enabled and whose time window includes the current time.
3. When the activity first becomes active, count exactly **three groups of three short trills** (nine short beeps total), separated by the normal longer inter-group pause.
4. Leave the program active for several minutes. The buzzer must remain silent after the finite activation notice; it must not restart on subsequent automation loop iterations.
5. End/disable the program while its notice is still playing; the remaining notice must stop.
6. Trigger an alarm with its buzzer option enabled while a program is active. The alarm must take priority and use the repeating trill for its configured alarm duration.
7. After the alarm ends, a resumed program may emit its normal finite activation notice when the activity is actually started again; it must never become a continuous schedule buzzer.

## Audio / Rhythm regression (0.8.0)

1. Start from the clock and select Audio/Rhythm effect 1. WLED must switch to
   `iDotMatrix Display`, the breakdancer must appear, and `/json/info` must show
   `content=audio LEVEL mode=1`.
2. Test all five LEVEL modes and verify that their animation reacts to sound.
3. Test all five FFT modes; `/json/info` must report modes 1 through 5 and the
   display must continue updating across BLE write boundaries without returning
   to the clock.
4. Select a normal WLED effect and verify that it takes control. Generate new
   audio data and verify that `iDotMatrix Display` is selected again.
5. Repeat at least one LEVEL and one FFT mode with logical profiles 32x32 and
   64x64-lite mapped to the physical 16x16 matrix.
6. Re-run a GIF from the stable regression set and both manual and scheduled
   buzzer tests to confirm that audio added no media or timing regression.

## Usermod settings regression (0.8.0)

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
   that 0.8.0 displays only its suffix without duplicating the prefix.
6. Verify that every visible field label ends with `:` and that the rescale row
   reads `Scale the logical profile to the selected WLED 2D segment:` instead
   of `Rescale`.

## Build-aware resolution regression (0.8.0)

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
- Force a post-validation GIF preparation failure; recovery must not select an empty `iDotMatrix Display`.
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

## Pending validation after 0.8.0

- automatic PSRAM/full-AnimatedGIF 64x64 direct path on real hardware;
- native physical 64x64 output;
- HUB75 DMA + BLE/media coexistence;
- 24-hour Wi-Fi/BLE/content soak.

## Historical development notes

The `RELEASE_NOTES_0.7.1-dev.*.md` files preserve the individual experiments,
including the unsafe truncated-LZW12 branches. They are retained for engineering
history, not as recommended build targets. In particular, dev.4/dev.5 must not
be used as a model for future memory optimization because their physical LZW12
dictionaries were smaller than the legal 4096-code space.

## Final 0.8.0 packaging check

Before creating the release archive:

1. run `./run_host_tests.sh`;
2. verify `library.json` and `IDOTMATRIX_BUILD` both report `0.8.0`;
3. validate all local Markdown links;
4. confirm every text file ends with a newline and has no trailing whitespace;
5. confirm the repository contains no `.pio`, build outputs, editor backups,
   Python caches, core dumps, or other temporary artifacts;
6. create the archive with one top-level `wled-usermod-idotmatrix/` directory and
   inspect its file list before publishing.
