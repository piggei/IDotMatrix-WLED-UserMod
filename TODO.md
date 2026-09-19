# Deferred / post-0.9 work

The 0.9.0 feature set is complete. Items below are intentionally deferred to later releases.

- Validate a physical 32x32 panel when hardware is available. The 32x32 logical
  path is already exercised through the 64x64 scaler.
- Revisit iOS compatibility only if needed; iOS work remains isolated on its
  dedicated branch.
- Consider filesystem-backed streaming for very large Alarm/Program media if
  future hardware exposes sustained heap/PSRAM fragmentation under extreme use.
- Consider further diagnostic reduction only after the 0.9 qualification cycle;
  retain useful runtime health/status information for field support.
