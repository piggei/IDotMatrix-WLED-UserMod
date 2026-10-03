# TODO

## Post-0.9.4 development

Release `0.9.4`, build `0.9.4`, is stable. The 0.9.4 qualification gates are closed: Waveshare and MatrixPortal native-S3 media paths are qualified, Waveshare automatic 16x16/32x32 -> 64x64 scaling is qualified, rc.2 Automation regression fixes passed host coverage, and the final Waveshare reboot/persistence/Alarm execution smoke passed.

Future work must start from the stable 0.9.4 baseline. Do not silently change the qualified Waveshare or MatrixPortal PSRAM budgets; any new optimization requires a new development build and fresh measurement evidence.

## Deferred audit hardening

The rc.1 audit also identified broader improvements that are intentionally deferred unless new release-blocking evidence appears:

- formalize Schedule upload-session/end-of-list semantics beyond the existing quiet-time heuristic;
- verify original-device failure ACK semantics before changing compact-PNG, Alarm, Preset or Carousel wire responses;
- add bounded BLE initialization retry/cleanup and immediate transaction abort on RX-queue overflow;
- introduce explicit renderer capability/error state instead of fail-open visual ACK behavior;
- version/migrate Automation persistence rather than relying indefinitely on raw struct blobs;
- add CRC verification for persistent Automation/Carousel media at boot or first use;
- make Carousel bank replacement fully incoming-vs-committed transactional;
- add Preset filesystem preflight and stronger double-failure rollback convergence;
- document or redesign the low-memory in-place RAW fallback transactional degradation;
- consider an opt-in authenticated BLE mode without breaking original-app compatibility.

# Deferred / future work

The items below are intentionally deferred to releases after 0.9.4.

- **Progressive Carousel/Preset item pipeline:** allow playback to begin after the first complete and validated item is received while later items continue transferring in the background. Preserve transactional safety with separate incoming and committed state: early display is allowed only for complete items, persistent promotion remains atomic after the full transfer validates, and an interrupted transfer must leave the previous committed content recoverable. Start with Carousel/C3 measurements before generalizing to Preset or other targets.
- Validate a physical 32x32 panel when hardware is available. The logical 32x32 path is already exercised through the 64x64 scaler.
- Revisit iOS compatibility only if needed; iOS work remains isolated on its dedicated branch.
- Consider filesystem-backed streaming for very large Alarm/Program media only if future hardware exposes sustained heap/PSRAM fragmentation under extreme use.
- Keep future controller-specific sensors in separate Usermods rather than coupling them to the iDotMatrix protocol layer.
- Do not add further Waveshare or MatrixPortal media optimizations unless new measurements show a real bottleneck.
