# iDotMatrix WLED Usermod 0.9.0-dev.18

Diagnostic build for the Alarm regression investigation.

No Alarm scheduling behaviour is intentionally changed from dev.17. `/json/info` now exposes the automation clock source/current time and every configured Alarm slot, including flags, trigger time, duration, media metadata and the last-trigger minute key. This is intended to identify whether the failure is in command storage, date/day matching, trigger execution, or media/display activation on hardware.
