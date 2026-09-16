# 0.9.0-dev.24 release notes

## Scope

Consolidation release. No new iDotMatrix protocol feature is introduced.

## Changes

- Reconciles the primary documentation with the hardware-validated state of Alarm, Program/Schedule and Preset / Default.
- Keeps the dev.21 multipart Alarm/Schedule protocol behavior unchanged.
- Keeps the dev.22/dev.23 Preset / Default protocol and playback behavior unchanged.
- Removes obsolete Carousel transfer-completion state associated with an abandoned determinate/100% status design. The shared Carousel/Preset indicator remains indeterminate until it disappears.
- Records the ESP32-C3 burn-test OFF investigation as externally caused by Home Assistant grouping, not by the iDotMatrix firmware path.
- Updates release/package checks to the dev.24 baseline.

## Hardware status carried forward

- MatrixPortal S3 + WLED native HUB75 + 64x64: validated.
- Logical 16/32/64 rendering to physical 64x64: validated.
- Alarm multi-packet transfer and trigger: validated.
- Program/Schedule multi-packet transfer and runtime: validated.
- Preset / Default transfer and playback: validated.
- Carousel/Preset indeterminate upload feedback: validated.

Physical 32x32 output combinations remain awaiting real 32x32 hardware validation.
