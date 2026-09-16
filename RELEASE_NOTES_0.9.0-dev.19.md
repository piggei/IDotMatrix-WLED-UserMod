# iDotMatrix WLED Usermod 0.9.0-dev.19

Diagnostic build focused on the Alarm `00/80` receive/commit path.

No Alarm scheduling or rendering behaviour is intentionally changed from dev.18. `/json/info` now exposes an `alarmRx=` line containing the most recent Alarm command length, decoded slot/flags/time, media size and availability, CRC validation, whether `onAlarm()` was called, whether the slot commit succeeded, and whether the compatibility ACK was emitted.

This build is intended to distinguish an input packet/validation failure from an automation storage failure.
