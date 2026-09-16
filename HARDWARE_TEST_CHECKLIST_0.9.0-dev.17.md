# Hardware Test Checklist - 0.9.0-dev.17

## Alarm regression

- Connect with the original iDotMatrix app and allow the normal time synchronization.
- Configure a one-shot Alarm 1-2 minutes in the future with visible media and buzzer if available.
- Confirm the Alarm triggers at the time shown by the app, regardless of WLED/NTP timezone configuration.
- Confirm the Alarm stops after its configured duration and prior display content resumes.
- Repeat with a weekday-enabled Alarm if convenient.

## Smoke test

- Clock still renders normally.
- GIF playback works.
- Carousel playback works.
- WLED <-> iDotMatrix ownership transitions still work.
