# iDotMatrix WLED Usermod 0.9.1-dev.3

Release `0.9.1`, build `0.9.1-dev.3` is a narrowly scoped renderer-adjustment build based on the hardware-validated `0.9.1-dev.2`.

## Native 64x64 clock spacing

For clock styles 0 and 3 on the native 64x64 combined HH:MM + DD/MM layout:

- the hour field is unchanged;
- the blinking HH:MM separator moves two physical LEDs to the right;
- both minute digits move two physical LEDs to the right;
- the lower DD/MM row is unchanged;
- 16x16 and 32x32 rendering is unchanged.

The change addresses the physical 64x64 observation that the separator was visually too close to the hour digits.

## Regression coverage

Host renderer tests now verify the new separator and minute positions for both style 0 (rainbow frame) and style 3 (solid selected background with black foreground), including clearing of their previous positions.

## Unchanged validated work

The previously validated Graffiti framing remains unchanged: marker `0x00` starts a full-raster transfer and marker `0x02` carries continuations until the final completion ACK.

The following dev.1/dev.2 work is intentionally unchanged:

- hardware-validated 64x64 Graffiti full-raster multipart transport;
- hardware-validated ESP32-C3 4 MB dual-slot OTA profile;
- `overrides/` and `partitions/` repository organization;
- Alarm, Program/Schedule, Carousel, Preset, TEXT/font-64, AudioReactive and display-ownership behavior.
