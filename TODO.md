# TODO

## 0.9.4 Waveshare qualification

Stable 0.9.3 is closed and remains the behavioral baseline. Current work is release `0.9.4`, build `0.9.4-dev.5`, on WLED 17.0.0-devV5.

Hardware already verified on the Waveshare ESP32-S3-RGB-Matrix with one 64x64 HUB75 panel:

- full native 64x64 output and PSRAM visibility;
- BLE/original-app connection and reconnect;
- Clock, TEXT, static image, GIF and Carousel;
- reboot/persistence;
- Alarm and Program paths;
- Matrix Auto Rotation coexistence;
- onboard microphone through AudioReactive. The working configuration uses the Waveshare I2S pins (`SD 39`, `WS 38`, `SCK 43`, `MCLK 12`) with UDP Sound Sync receive mode disabled.

Remaining qualification items:

- verify 16x16 -> 64x64 logical scaling;
- verify 32x32 -> 64x64 logical scaling;
- run a 30-60 minute GIF/Carousel + AudioReactive soak and compare heap/PSRAM telemetry before and after.

After the soak passes, keep 64x64 as the physically qualified target and start memory work in this order:

- add PSRAM/cache telemetry without changing behavior;
- evaluate whole-file media staging in PSRAM;
- evaluate GIF prefetch/frame-cache improvements;
- evaluate Carousel preloading;
- preserve automatic fallback on low-memory targets such as ESP32-C3.

Do not begin 128x64/128x128 physical qualification until matching hardware is available.

Preserve the 0.9.3 external-buzzer boundary: iDotMatrix requests logical sounds while WLED Buzzer Usermod owns GPIO, hardware type, tone generation and timing.

# Deferred / future work

The items below are intentionally deferred to future releases.

- Validate a physical 32x32 panel when hardware is available. The 32x32 logical
  path is already exercised through the 64x64 scaler.
- Revisit iOS compatibility only if needed; iOS work remains isolated on its
  dedicated branch.
- Consider filesystem-backed streaming for very large Alarm/Program media only
  if future hardware exposes sustained heap/PSRAM fragmentation under extreme
  use.
- Keep future controller-specific sensors, such as MatrixPortal LIS3DH automatic
  orientation, in separate Usermods rather than coupling them to the iDotMatrix
  protocol layer.
