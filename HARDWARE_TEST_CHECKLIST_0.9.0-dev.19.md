# Hardware Test Checklist - 0.9.0-dev.19

1. Connect the official app.
2. Configure one Alarm 1-2 minutes ahead.
3. Immediately fetch `/json/info`; there is no need to wait for the target minute.
4. Preserve the complete `u.iDotMatrix` array, especially `alarmRx=`, `alarmSlot=` and `alarmDiag=`.
5. If `alarmRx` reports `result:committed`, verify that `alarmSlot` shows the newly selected time.
6. If it reports `bad-size`, `bad-crc`, `bad-slot`, `rejected`, or `not-called`, preserve that exact line for diagnosis.
