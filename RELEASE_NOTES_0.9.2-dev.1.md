# iDotMatrix WLED Usermod 0.9.2-dev.1

Development build adding passive-buzzer hardware support and persistent Clock presentation.

## Passive buzzer

The Usermod settings now expose:

- `buzzerType`: Active or Passive;
- `buzzerActiveHigh`: static polarity for active/self-oscillating buzzers;
- `buzzerPassiveTrigger`: High or Low trigger for passive buzzer modules.

Passive output uses ESP32 LEDC at 2 kHz. Low-level-trigger modules are held HIGH while silent so their transistor/buzzer is not DC-biased at rest. Existing non-blocking Test/Alarm/Program sound patterns are unchanged.

## Clock presentation persistence

Style, 12/24-hour mode, date visibility and RGB colour are now saved to NVS after stable app Clock commands and restored during Usermod setup. Selecting the iDotMatrix WLED effect before the phone app reconnects therefore restores the last Clock presentation rather than boot defaults.

The existing short protection against transient app `showDate=0` packets is retained. Device reset clears the stored Clock presentation preferences.

## Protocol

No BLE packet format changes are introduced. Buzzer backend selection and Clock persistence are local implementation behavior.
