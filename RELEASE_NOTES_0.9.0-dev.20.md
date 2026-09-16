# 0.9.0-dev.20 release notes

## Purpose

Correct Alarm and Program/Schedule media uploads on 64x64 devices when the official app splits one media asset across multiple complete logical packets.

## Root cause

The previous parser assumed `mediaSize` described the bytes carried after the header in the current logical packet. A captured 64x64 Alarm disproved that assumption: total media size 6706 bytes arrived as two `00/80` packets containing 4096 and 2610 media bytes, each with its own repeated 24-byte Alarm header. Program/Schedule uses the same model.

## Changes

- Alarm and Program now have independent multipart assembly transactions.
- `mediaSize`/`mediaCRC` describe the complete asset; each packet contributes only its post-header chunk.
- Stable metadata + total size + CRC + media ID associate chunks. Reserved continuation fields are not required for assembly.
- Complete CRC is checked only after all bytes have arrived.
- Existing valid Alarm/Program data is replaced only after successful complete-media validation. Alarm storage now uses temp + backup + promote/rollback, matching the transactional safety already used by Program storage.
- Incomplete transactions expire after 5 seconds.
- Defensive maximum assembled asset size: 512 KiB.
- Metadata mismatch, overflow, allocation failure, timeout and bad final CRC abort the in-progress replacement without touching the previous committed asset.
- `/json/info` retains `alarmRx` and adds `programRx`, including chunk and cumulative byte counters.

## Compatibility

Single-packet Alarm and Program activity packets remain supported. This build does not rely on observed `reserved2=00/02` (Alarm) or analogous reserved values to determine first/final chunks, so it also supports 3+ chunks when metadata and CRC remain consistent.
