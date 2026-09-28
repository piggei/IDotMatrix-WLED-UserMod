# iDotMatrix WLED UserMod 0.9.2-dev.5

> **Follow-up:** physical UI validation found one remaining wrapper-order defect: selecting Active could hide both polarity controls. Superseded by 0.9.2-dev.6.

## Buzzer settings UI correction

- Fixes the remaining 0.9.2-dev.4 settings-page defect where both Active and Passive polarity controls could remain visible and the **Test buzzer** action could appear twice.
- Matches WLED's actual Usermod Settings DOM: configuration fields are emitted as a flat sequence separated by `<br>`, not as independent per-field row containers.
- Wraps only the two generated polarity lines after WLED has built the page, then toggles those wrappers from **Buzzer Type**:
  - **Active** shows only **Active buzzer active-high**;
  - **Passive** shows only **Passive buzzer trigger**.
- Creates one independent **Test buzzer** control after the conditional polarity lines, so it remains visible for both types and cannot be duplicated by the show/hide logic.
- The test warning now explicitly states that the test uses saved settings and the user must save first.
- Persistent configuration keys remain `buzzerType`, `buzzerActiveHigh`, and `buzzerPassiveTrigger`; no configuration migration is introduced.
- Keeps the generated `appendConfigData()` script below the repository's 2800-byte regression budget.

The buzzer backend is unchanged from the hardware-validated implementation: passive output remains 2 kHz LEDC with 10-bit idle-duty handling, and Alarm / Program / Test timing is unchanged.

## Clock presentation persistence

No change from the previous 0.9.2 development builds. Clock style, 12/24-hour mode, date visibility and RGB colour remain persisted in NVS and restored before standalone Clock fallback. The existing transient `showDate=0` protection remains unchanged. Device reset still clears the stored Clock presentation preferences.
