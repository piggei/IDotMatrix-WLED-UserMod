# TODO

## 0.9.4 release-candidate freeze

Release `0.9.4`, build `0.9.4-rc.1`, targets WLED 17.0.0-devV5. Stable 0.9.3 remains the behavioral baseline until final promotion. The rc.1 runtime is promoted from hardware-qualified dev.16 with no new functional feature.

Qualification status required for 0.9.4 is closed:

- Waveshare ESP32-S3-RGB-Matrix / native 64x64: PASS;
- Waveshare 16x16 -> 64x64 and 32x32 -> 64x64 automatic output scaling: PASS with legacy Rescale disabled;
- Waveshare transient staging, persistent source cache, one-item prefetch, invalidation/recovery and dev.12 peak telemetry: PASS;
- Adafruit MatrixPortal S3 / native 64x64: PASS;
- MatrixPortal 256 KiB transient staging / 1 MiB reserve: PASS;
- MatrixPortal 384 KiB persistent cache / 256 KiB entry / 8 metadata entries: PASS;
- MatrixPortal one-item / 250 ms prefetch: PASS;
- MatrixPortal mixed Carousel/Preset soak: PASS with `fallback:0`, `gifPrefetch fail:0`, LRU eviction and invalidation exercised, and PSRAM invariants valid;
- ESP32-C3 and classic ESP32 qualification baselines remain unchanged.

During the release-candidate cycle, accept only regression fixes, documentation corrections, packaging fixes, or build-profile corrections. Do not add new runtime features or increase PSRAM budgets.

# Deferred / future work

The items below are intentionally deferred to releases after 0.9.4.

- **Progressive Carousel/Preset item pipeline:** allow playback to begin after the first complete and validated item is received while later items continue transferring in the background. Preserve transactional safety with separate incoming and committed state: early display is allowed only for complete items, persistent promotion remains atomic after the full transfer validates, and an interrupted transfer must leave the previous committed content recoverable. Start with Carousel/C3 measurements before generalizing to Preset or other targets.
- Validate a physical 32x32 panel when hardware is available. The logical 32x32 path is already exercised through the 64x64 scaler.
- Revisit iOS compatibility only if needed; iOS work remains isolated on its dedicated branch.
- Consider filesystem-backed streaming for very large Alarm/Program media only if future hardware exposes sustained heap/PSRAM fragmentation under extreme use.
- Keep future controller-specific sensors in separate Usermods rather than coupling them to the iDotMatrix protocol layer.
- Do not add further Waveshare or MatrixPortal media optimizations unless new measurements show a real bottleneck.
