# iDotMatrix WLED UserMod 0.9.2-dev.4

## Usermod settings UI fix

- Fixes the Usermod settings page disappearing in dev.3.
- Keeps `appendConfigData()` below WLED's limited settings-script buffer.
- Replaces the large label-rewrite script with native `addInfo()` label overrides.
- Buzzer controls are ordered as section title, pin, type, relevant polarity option, then Test buzzer.
- Only the polarity option matching the selected buzzer type is shown.
- The hide/show logic can no longer hide the entire iDotMatrix Usermod container.
- Buzzer runtime/backend behavior is unchanged from dev.2.

Passive buzzer runtime behavior remains identical to the hardware-validated dev.2 backend.

Configuration keys remain `buzzerType`, `buzzerActiveHigh`, and `buzzerPassiveTrigger`.

## Clock presentation persistence

Style, 12/24-hour mode, date visibility and RGB colour are saved to NVS after stable app Clock commands and restored during Usermod setup. Selecting the iDotMatrix WLED effect before the phone app reconnects therefore restores the last Clock presentation rather than boot defaults.

The existing short protection against transient app `showDate=0` packets is retained. Device reset clears the stored Clock presentation preferences.
