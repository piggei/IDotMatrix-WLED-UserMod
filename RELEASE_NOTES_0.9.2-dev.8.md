# iDotMatrix WLED UserMod 0.9.2-dev.8

## Buzzer settings final spacing cleanup

This build changes only two presentation details in the already-working Buzzer settings UI:

- removes the small horizontal rule directly below the **iDotMatrix** Usermod title;
- removes the extra blank line between the **Buzzer** heading and **Buzzer Pin**.

The Active/Passive conditional UI is unchanged from dev.7. Persistent configuration keys remain `buzzerType`, `buzzerActiveHigh`, and `buzzerPassiveTrigger`, and the Passive buzzer backend remains unchanged.

## Clock presentation persistence

No change from the previous 0.9.2 development builds. Clock style, 12/24-hour mode, date visibility and RGB colour remain persisted in NVS and restored before standalone Clock fallback. The existing transient `showDate=0` protection remains unchanged.

No BLE protocol, buzzer runtime, timing or other functional behavior changed in this build.
