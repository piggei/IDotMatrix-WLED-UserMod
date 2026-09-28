# iDotMatrix WLED UserMod 0.9.2

Release `0.9.2`, build `0.9.2` is the stable release following 0.9.1. It promotes the hardware-validated `0.9.2-dev.8` runtime baseline without introducing new functional changes during final promotion.

## Highlights

- Adds configurable **Active / Passive** buzzer support.
- Drives passive buzzers through ESP32 LEDC at **2 kHz** while preserving the existing non-blocking Test/Alarm/Program sound patterns.
- Correctly supports **low-level-trigger passive modules** with a true constant-HIGH idle after tones.
- Persists Clock style, 12/24-hour mode, date visibility and RGB colour in NVS and restores them before standalone Clock fallback.
- Finalizes the WLED Usermod settings UI with one conditional polarity control and one independent **Test buzzer** action.
- Preserves the complete qualified 0.9.1 feature set, including Graffiti multipart, ESP32-C3 OTA, MatrixPortal/HUB75, TEXT, GIF, Carousel, Preset, Alarm and Program/Schedule.

## Active and passive buzzer support

The Buzzer settings are ordered as:

```text
Buzzer
Buzzer Pin
Buzzer Type
<type-specific polarity>
Test buzzer
Save first: test uses saved settings.
```

`Buzzer Type` (`buzzerType`) selects either an active/self-oscillating buzzer or a passive buzzer. Active polarity is stored as `buzzerActiveHigh`; passive trigger polarity is stored as `buzzerPassiveTrigger`. Active mode exposes **Active buzzer active-high:**. Passive mode exposes **Passive buzzer trigger:** with High/Low trigger selection. Only the polarity control relevant to the selected type is visible.

Passive buzzer output uses ESP32 LEDC at approximately 2 kHz. Arduino-ESP32 3.x `ledcWriteTone()` uses 10-bit resolution, so a trigger-low module's inactive HIGH state uses full-scale duty `1023`; trigger-high uses LOW idle. This prevents residual PWM/DC bias after the tone. The low-trigger path was hardware-validated with three expected tones followed by complete silence and no idle heating.

The **Test buzzer** action uses the saved configuration, hence the explicit save-first warning. Alarm and Program/Schedule retain the established non-blocking sound timing.

## Clock presentation persistence

The Usermod persists these Clock presentation values in NVS:

- Clock style;
- 12/24-hour mode;
- date visibility;
- RGB colour settings.

They are loaded during Usermod startup before a standalone Clock fallback can render. Selecting the iDotMatrix WLED effect before the official app reconnects therefore restores the previously selected Clock appearance instead of boot defaults. The existing protection against transient application `showDate=0` updates remains unchanged. Device reset clears the persisted Clock presentation preferences.

## Compatibility retained from 0.9.1

0.9.2 retains the hardware-qualified 0.9.1 Graffiti full-raster multipart path, ESP32-C3 4 MB AudioReactive + dual-slot OTA profile, native MatrixPortal S3 / 64x64 HUB75 output, logical 16x16/32x32/64x64 scaling, 16654-byte TEXT path, GIF/image playback, persistent Carousel, volatile transactional Preset/Default, Alarm and Program/Schedule multipart media, Audio/Rhythm visualizers, timers, scoreboard, WLED ownership transitions and scoped LittleFS cleanup.

Qualified WLED baselines remain:

```text
MatrixPortal S3 / HUB75:
WLED 17.0.0-devV5
06ae26db67107cb3f6a3d107a92340035991a063

ESP32-C3:
WLED 17.0.0-devV5
d55037f7510541eddc390c8f3d01afc5787aa44a
```

The `devV5` suffix above belongs to the qualified upstream **WLED** version and is unrelated to the final iDotMatrix build identifier.

## Final qualification

The final release was promoted from `0.9.2-dev.8` after physical confirmation that the completed buzzer backend and settings UI operate correctly. Host regression, PlatformIO profile checks, package-consistency checks and documentation/link checks are part of the release gate.

## Release promotion

The final `0.9.2` runtime differs from the qualified `0.9.2-dev.8` baseline only in release/build identification. No BLE protocol, renderer, media, automation, buzzer timing or Clock persistence behavior is intentionally changed by the final promotion.
