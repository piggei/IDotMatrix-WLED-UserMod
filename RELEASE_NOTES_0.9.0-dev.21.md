# 0.9.0-dev.21 release notes

## Purpose

Complete the 64x64 Program/Schedule multipart protocol fix using the behavior hardware-validated in the standalone iDotMatrix emulator.

## Changes

- Schedule activity `contentType` is now parsed from byte 10 as an 8-bit value.
- Byte 11 is retained separately as `chunkMarker` (`0x00` first chunk, observed `0x02` continuation) and is excluded from media identity.
- In-progress Schedule media identity is index + content type + total media size + total media CRC.
- Accepted incomplete Schedule chunks reply `05 00 05 80 01` so the official app continues transmitting.
- `05 00 05 80 03` is sent only after the complete object is assembled, its full CRC is valid, and persistence accepts the activity.
- Failed/rejected completion replies with status `0x02`.
- Alarm ACK semantics and generic FA02 reassembly are unchanged.
- Diagnostics now expose Schedule marker and ACK status.

## Compatibility

Single-packet Schedule activities still complete with status `0x03`. Existing Alarm multipart behavior from dev.20 is preserved.
