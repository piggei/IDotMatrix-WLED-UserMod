# TODO

## 0.9.3-rc.1 current gate

Completed before RC1 packaging:

- hardware build with WLED Buzzer Usermod 0.1.0-rc.7;
- hardware build without the Buzzer Usermod;
- optional Buzzer Enable presence/absence behavior;
- BLE connect and disconnect sounds;
- natural Countdown `triple_beep`;
- Alarm looping `triple_beep` with the RC7 repeat gap and clean stop;
- Program/Schedule `notification`;
- reboot/persistence, silent Alarm, no-sound Program and repeated-event smoke checks.

Remaining release gate:

- perform a short RC1 smoke test after flashing the renumbered package;
- if no regression is found, promote the exact RC1 runtime to stable 0.9.3 without functional changes.

# Deferred / post-0.9.2 work

Release 0.9.2 is complete. The items below are intentionally deferred to future releases.

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
