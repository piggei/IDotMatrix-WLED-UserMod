# iDotMatrix WLED UserMod 0.9.2

**Release:** 0.9.2  
**Build:** 0.9.2  
**Status:** stable release

## Summary

0.9.2 adds configurable Active/Passive buzzer support, persistent Clock
presentation preferences, and a hardware-qualified real-time buzzer scheduler.
It is promoted directly from the physically validated `0.9.2-dev.11` build. The
final promotion changes only the build identifier and release documentation; no
runtime behavior differs from dev.11.

## Buzzer hardware and configuration

- Added `buzzerType` with Active and Passive backends.
- Active mode retains configurable `buzzerActiveHigh` polarity.
- Passive mode adds configurable `buzzerPassiveTrigger` polarity.
- Passive tone generation uses the qualified 2 kHz LEDC backend.
- Trigger-low Passive modules use the safe silent HIGH idle state after playback.
- The settings UI conditionally displays only the option relevant to the selected
  buzzer type and provides one **Test buzzer** control.
- Existing saved configuration remains compatible; no migration is required.

## Runtime buzzer routing

The runtime notification paths are explicitly connected to the same buzzer
engine used by the manual test:

- BLE connection: one short low-priority beep;
- natural Countdown completion: one three-pulse trill;
- Alarm: repeating trill for the configured Alarm lifetime when the protocol
  `buzzer` field is enabled; silent Alarm remains silent;
- Program/Schedule: finite notification groups when the global sound flag is
  enabled.

The audible pattern remains 90 ms ON, 70 ms inter-pulse gap, three pulses per
trill and 550 ms inter-group pause.

## Real-time buzzer timing

Alarm and Program media loading can involve filesystem and decoder work, and the
WLED main loop can have variable cadence while GIF, BLE or rendering work is
active. During 0.9.2 development two timing defects were removed:

1. Alarm/Program now start their audible interval from a fresh post-media-load
   timestamp, so synchronous media preparation cannot shorten the first beep.
2. On ESP32, buzzer-envelope transitions are serviced by ESP-IDF `esp_timer` at
   2 ms cadence instead of depending on the WLED main loop. Absolute edge
   deadlines prevent small service delays from accumulating across a trill.

A guarded WLED-loop fallback remains available if the real-time timer cannot be
created. `/json/info` exposes `buzzerTiming`, `lateLast` and `lateMax` for future
regression diagnosis.

## Clock presentation persistence

Clock presentation settings are stored in NVS and restored before standalone
Clock fallback:

- Clock style;
- 12/24-hour mode;
- date visibility;
- RGB colour.

The existing protection against transient application `showDate=0` updates is
retained, and iDotMatrix device reset clears the stored Clock preferences.

## Hardware qualification

The final buzzer runtime path was physically revalidated after dev.11. Test
buzzer, BLE connection, natural Countdown completion, Alarm and Program/Schedule
all operated correctly, including the previously problematic Alarm/Program
rhythm under media load. The Passive low-level-trigger path retains its earlier
validation: expected tones, clean return to silence, safe idle level and no
residual heating.

## Compatibility

The qualified WLED baselines and existing protocol/media behavior are unchanged.
0.9.2 retains Carousel, Alarm, Program/Schedule, Preset / Default, Graffiti,
GIF/media playback, AudioReactive integration, WLED ownership switching and the
0.9.1 hardware/profile foundation.
