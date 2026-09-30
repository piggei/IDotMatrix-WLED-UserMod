# WLED iDotMatrix Usermod 0.9.3

**Release:** 0.9.3  
**Build:** 0.9.3  
**Status:** stable; promoted from hardware-qualified 0.9.3-rc.1 without functional runtime changes

## Purpose

0.9.3 moves buzzer hardware ownership out of iDotMatrix and delegates optional sound playback to the standalone **WLED Buzzer Usermod**. The iDotMatrix BLE protocol and all non-audio behavior remain aligned with the qualified 0.9.2 baseline.

The integration was first exercised under temporary internal `0.10.0-dev.1` / `0.10.0-dev.2` identifiers and was then renumbered to the 0.9.3 line. The final build is promoted directly from hardware-qualified `0.9.3-rc.1`; runtime behavior is unchanged apart from the build identifier.

## External buzzer integration

The internal `IDotMatrixBuzzer` backend is removed. iDotMatrix no longer owns:

- buzzer GPIO allocation;
- Active/Passive buzzer selection;
- trigger polarity;
- LEDC/tone generation;
- buzzer scheduling/timing;
- a local buzzer test endpoint or test button.

Sound is optional and is requested through `IDotMatrixBuzzerBridge.h`, a weak-linked C ABI. The package compiles and runs both with and without the external Buzzer Usermod.

The settings UI exposes only **Buzzer -> Enable**, followed by the orange note **Requires the WLED Buzzer Usermod.** If the external service is absent, the checkbox remains visible but disabled.

## Event mapping

| iDotMatrix event | External sound request |
|---|---|
| Alarm with sound enabled | `triple_beep`, looped until Alarm stop |
| Program / Schedule with sound flag | `notification`, once |
| Natural Countdown completion | `triple_beep`, once |
| BLE application connection | `connect`, once |
| BLE application disconnection | `disconnect`, once |

Alarm remains the highest-priority iDotMatrix sound request. Lower-priority notifications are skipped rather than queued while an Alarm owns the iDotMatrix sound policy.

## External provider

The final hardware qualification used **WLED Buzzer Usermod release 0.1.0 / build final**. Its `triple_beep` retains the qualified three-pulse one-shot and inserts a 550 ms trailing gap only when another repeat follows, giving Alarm a clear `beep-beep-beep / silence / repeat` cadence without changing Countdown one-shot playback.

The iDotMatrix consumer bridge deliberately needs only `ready`, `playing`, named-sound `play`, `stop` and `currentSoundId`. `playRepeat`, raw `beep` and raw `tone` are not required by the current predefined-sound mapping.

## Hardware qualification

Physical validation reported PASS for:

- a WLED firmware built without WLED Buzzer Usermod, with **Buzzer -> Enable** disabled;
- a WLED firmware built with WLED Buzzer Usermod, with **Buzzer -> Enable** available;
- BLE connection and disconnection sounds;
- natural Countdown completion;
- Alarm sound, repeat gap and clean stop;
- Program/Schedule sound;
- reboot/persistence, silent Alarm, no-sound Program and repeated-event checks;
- continued BLE/display operation while Carousel/GIF activity, Alarm and Program operations were exercised;
- an extended ESP32-C3 soak pass with no progressive heap degradation observed.

The final 0.9.3 promotion changes no functional runtime path from `0.9.3-rc.1`.
