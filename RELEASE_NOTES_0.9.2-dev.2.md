# iDotMatrix WLED Usermod 0.9.2-dev.2

Development build consolidating passive-buzzer hardware support and persistent Clock presentation, with a hardware-derived fix for passive low-level-trigger idle behavior.

## Passive buzzer

The Usermod settings expose:

- `buzzerType`: Active or Passive;
- `buzzerActiveHigh`: static polarity for active/self-oscillating buzzers;
- `buzzerPassiveTrigger`: High or Low trigger for passive buzzer modules.

Passive output uses ESP32 LEDC at 2 kHz. Low-level-trigger modules are held HIGH while silent so their transistor/buzzer is not DC-biased at rest. Existing non-blocking Test/Alarm/Program sound patterns are unchanged.

### Low-trigger idle fix

Hardware testing of 0.9.2-dev.1 exposed a passive-buzzer idle bug on Arduino-ESP32 3.x. `ledcWriteTone()` reconfigures the LEDC channel to 10-bit resolution. The dev.1 backend had attached the channel at 8 bits and therefore wrote `255` as the trigger-low idle duty after a tone. Once the tone API had switched the channel to 10 bits, that value represented roughly 25% PWM instead of a constant HIGH level, so a low-level-trigger passive buzzer could continue sounding after the three-beep test.

0.9.2-dev.2 uses a 10-bit LEDC configuration and a matching full-scale idle duty (`1023`) for trigger-low modules. Trigger-high behavior remains a constant LOW idle.

## Usermod settings layout

Audio Source is kept separate and the buzzer controls now form the final settings group. **Test buzzer** is shown at the bottom of that group and explicitly states that it uses the saved buzzer type and polarity, avoiding the impression that the test belongs only to the active-buzzer option.

## Clock presentation persistence

Style, 12/24-hour mode, date visibility and RGB colour are saved to NVS after stable app Clock commands and restored during Usermod setup. Selecting the iDotMatrix WLED effect before the phone app reconnects therefore restores the last Clock presentation rather than boot defaults.

The existing short protection against transient app `showDate=0` packets is retained. Device reset clears the stored Clock presentation preferences.

## Protocol

No BLE packet format changes are introduced. Buzzer backend selection and Clock persistence are local implementation behavior.
