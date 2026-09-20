# Deferred / post-0.9.1 work

The 0.9.1 release is complete. The items below are intentionally deferred to future releases.

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
