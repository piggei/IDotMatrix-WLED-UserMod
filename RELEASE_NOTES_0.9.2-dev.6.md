# iDotMatrix WLED UserMod 0.9.2-dev.6

## Buzzer settings UI follow-up

- Fixes the remaining dev.5 Active-mode visibility defect found on the physical WLED Usermod Settings page.
- Root cause: dev.5 wrapped the Active polarity line before the later Passive line. Because WLED emits fields as one flat `<br>`-separated sequence, the Passive backward scan then absorbed the already-created Active wrapper. Hiding the Passive wrapper therefore hid Active with it.
- Wraps the later **Passive buzzer trigger** line first and the earlier **Active buzzer active-high** line second, keeping the two wrappers independent.
- Switching **Buzzer Type** now works in both directions:
  - **Active** shows only **Active buzzer active-high**;
  - **Passive** shows only **Passive buzzer trigger**.
- Keeps one independent **Test buzzer** button with the warning that the test uses saved settings.
- Persistent configuration keys remain `buzzerType`, `buzzerActiveHigh`, and `buzzerPassiveTrigger`; no migration is introduced.
- Tightens the Buzzer layout by removing the extra top margin before the type-specific polarity option; the Active/Passive backend description remains part of the same Buzzer group.
- Adds a regression assertion for wrapper construction order so this DOM-nesting failure cannot silently return.

The hardware-validated buzzer backend is unchanged: passive output remains 2 kHz LEDC with 10-bit idle-duty handling, while Alarm / Program / Test timing and active-buzzer behavior are untouched.

## Clock presentation persistence

No change from the previous 0.9.2 development builds. Clock style, 12/24-hour mode, date visibility and RGB colour remain persisted in NVS and restored before standalone Clock fallback. The existing transient `showDate=0` protection remains unchanged. Device reset still clears the stored Clock presentation preferences.
