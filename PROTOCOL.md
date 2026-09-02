# Implemented iDotMatrix protocol subset

This document describes only WLED iDotMatrix Usermod 0.6.0. The standalone
`IDotMatrix-ESP32-Emulator` repository remains the primary, complete protocol
reference.

- **Confirmed protocol**: derived from the reference and experiments.
- **WLED mapping**: an integration decision, not a wire-protocol fact.
- **Limitation**: unknown or unimplemented behavior.

## BLE transport

| Role | UUID | Properties |
|---|---|---|
| FA service | `000000fa-0000-1000-8000-00805f9b34fb` | Service |
| FA02 | `0000fa02-0000-1000-8000-00805f9b34fb` | Write, write without response |
| FA03 | `0000fa03-0000-1000-8000-00805f9b34fb` | Read, notify |
| AE service | `0000ae00-0000-1000-8000-00805f9b34fb` | Service |
| AE01 | `0000ae01-0000-1000-8000-00805f9b34fb` | Write, write without response |
| AE02 | `0000ae02-0000-1000-8000-00805f9b34fb` | Read, notify |

Manufacturer data: `54 52 00 70 SCREEN_TYPE`.

| Screen type | Logical resolution |
|---|---|
| `01` | 16x16 |
| `03` | 32x32 |
| `04` | 64x64 |

## Framing and ACK

FA02 commands start with a 16-bit little-endian total length. Version 0.6.0
requires one complete command per queued BLE write and validates declared length
against received length.

Standard ACK on FA03:

```text
05 00 COMMAND SUBCOMMAND 01
```

Status `01` is confirmed for the commands below; it is not a universal bulk ACK.

## Device information

Request: `04 00 01 80`

16x16 response: `09 00 01 80 04 0E 01 01 00`

Offset 7 becomes `03` or `04` for the other profiles. The final byte remains the
confirmed fixed `00`. Encoding WLED power there did not change the app switch and
was reverted.

For compatibility, the Usermod sends this response unsolicited at about 1.2 and
2.5 seconds after connection. The second identical send is a WLED timing
workaround, not a protocol variant.

## Time synchronization

Command:

```text
0B 00 01 80 YEAR MONTH DAY DOW HOUR MINUTE SECOND
```

Response: `05 00 01 80 01`

Version 0.6.0 acknowledges but does not apply the time fields.

## Screen power

**Confirmed protocol:** `05 00 07 01 STATE`

- `00`: off;
- non-zero: on.

ACK: `05 00 07 01 01`

**WLED mapping:** use WLED's normal power path, preserving/restoring the previous
non-zero brightness. A new app connection also emits ON, matching the standalone
reference.

## Brightness

**Confirmed protocol:** `05 00 04 80 PERCENT`

Input is clamped to `0..100`. ACK: `05 00 04 80 01`.

**WLED mapping:** rounded conversion to master brightness:

```text
WLED_BRIGHTNESS = (PERCENT * 255 + 50) / 100
```

While logically OFF, a non-zero command updates the remembered WLED level but
does not power on. Zero produces black while retaining the last usable non-zero
level for a later OFF/ON cycle.

**Limitation:** synchronization is app to WLED only. No confirmed device-to-app
brightness-state message is known, so WLED changes do not update the app slider.

## Full-screen RGB

**Confirmed protocol:** `07 00 02 02 RED GREEN BLUE`

ACK: `05 00 02 02 01`

**WLED mapping:** select static effect, set primary RGB, clear the white channel,
and use WLED's global colour path. Active, selected segments are affected.

## Unsupported in 0.6.0

- fragmented commands or writes longer than 64 bytes;
- AE bulk/media processing;
- graffiti/pixels and framebuffer;
- text and 8x16/16x32 bitmap glyphs;
- clock/date rendering;
- GIF/cloud media;
- countdown, stopwatch, scoreboard, alarms, schedules;
- energy saving, rotation, effects, and reset.

These remain documented in the standalone reference and will be added without
placing blocking or bulk work in BLE callbacks.
