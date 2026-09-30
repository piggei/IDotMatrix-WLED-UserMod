# WLED iDotMatrix Usermod 0.9.3-rc.1

**Release:** 0.9.3  
**Build:** 0.9.3-rc.1  
**Status:** release candidate; hardware-tested integration promoted without functional runtime changes

## Purpose

0.9.3-rc.1 moves buzzer hardware ownership out of iDotMatrix and delegates sound playback to the optional standalone **WLED Buzzer Usermod**. The change keeps the iDotMatrix BLE protocol and all non-audio behavior aligned with stable 0.9.2.

The functionality was first exercised under temporary internal `0.10.0-dev.1` / `0.10.0-dev.2` identifiers. Before release-candidate packaging the release line was intentionally renumbered to 0.9.3 because the user-visible change is an architectural extraction rather than a new major feature set. RC1 contains no functional runtime change relative to the hardware-tested integration build; only release/build identity, the app-visible 0.9 release byte, and documentation were promoted.

## External buzzer integration

The internal `IDotMatrixBuzzer` backend has been removed. iDotMatrix no longer owns:

- buzzer GPIO allocation;
- Active/Passive buzzer selection;
- trigger polarity;
- LEDC/tone generation;
- buzzer scheduling/timing;
- a local buzzer test endpoint or test button.

Sound is optional and is requested through `IDotMatrixBuzzerBridge.h`, a weak-linked C ABI. The iDotMatrix package therefore compiles and links both with and without the external Buzzer Usermod.

The settings UI exposes only **Buzzer -> Enable**, followed by the orange note **Requires the WLED Buzzer Usermod.** If the external service is absent, the checkbox remains visible but disabled.

## Event mapping

| iDotMatrix event | External sound request |
|---|---|
| Alarm with sound enabled | `triple_beep`, looped until Alarm stop |
| Program / Schedule with sound flag | `notification`, once |
| Natural Countdown completion | `triple_beep`, once |
| BLE application connection | `connect`, once |
| BLE application disconnection | `disconnect`, once |

Alarm remains the highest-priority iDotMatrix sound request. Lower-priority notifications are skipped rather than queued while another iDotMatrix-owned sound is active.

## Required external version

For sound support use **WLED Buzzer Usermod release 0.1.0 / build rc.7** or newer.

RC7 retains the existing three-pulse `triple_beep` one-shot and adds a 550 ms trailing gap that is applied only when another repeat follows. This gives the looping Alarm a clear `beep-beep-beep / silence / repeat` cadence without changing Countdown one-shot playback.

The current iDotMatrix consumer bridge deliberately needs only `ready`, `playing`, named-sound `play`, `stop` and `currentSoundId`. `playRepeat`, raw `beep` and raw `tone` are not required by the current predefined-sound mapping.

## Hardware qualification before RC1

Physical testing reported PASS for:

- a WLED firmware built without WLED Buzzer Usermod, with **Buzzer -> Enable** disabled;
- a WLED firmware built with WLED Buzzer Usermod, with **Buzzer -> Enable** available;
- BLE connection sound;
- BLE disconnection sound;
- natural Countdown completion;
- Alarm sound and repeated `triple_beep` cadence with the RC7 pause;
- Program/Schedule sound;
- reboot/persistence and negative sound-policy checks exercised during the final integration pass.

Stable 0.9.2 remains the previous stable release until 0.9.3 is promoted from this candidate.
