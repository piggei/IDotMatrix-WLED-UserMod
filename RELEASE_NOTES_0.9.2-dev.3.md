# iDotMatrix WLED Usermod 0.9.2-dev.3

## Usermod settings UI cleanup

This build keeps the Passive buzzer backend from 0.9.2-dev.2 unchanged and refines only the Usermod settings layout.

- Adds a clear **Buzzer** section heading.
- Orders buzzer controls as **Buzzer Pin**, then **Buzzer Type** (`buzzerType`).
- Shows **Active buzzer active-high** (`buzzerActiveHigh`) only when `Active` is selected.
- Shows **Passive buzzer trigger** (`buzzerPassiveTrigger`) only when `Passive` is selected.
- Removes the redundant **Buzzer test** subheading.
- Keeps **Test buzzer** at the bottom of the buzzer controls.

No buzzer timing, frequency, polarity, Alarm, Program/Schedule, BLE, renderer, or wire-protocol behavior is changed.

## Clock presentation persistence

The Clock presentation persistence introduced in 0.9.2-dev.1 remains unchanged: style, 12/24-hour mode, date visibility and RGB color are stored in NVS and restored before the app reconnects. The existing transient `showDate=0` protection is retained.
