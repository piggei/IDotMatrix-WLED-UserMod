# 0.7.1-dev.14

Final development candidate before 0.7.1 release-candidate testing.

## Changes

- Keeps the validated dev.13 64x64 classic-ESP32 architecture unchanged: compact-safe full 4096-code LZW12 semantics, downscaled LittleFS frame cache, repeat-GIF cleanup, and low-heap wait/retry behavior.
- Frame-cache preparation still uses WLED `Static` internally for minimal RAM, but the selected segment primary colour is now temporarily black. The panel therefore appears off/black while the cache is being built instead of flashing/showing the previous Solid colour.
- Saves and restores the original segment primary colour without toggling WLED global power/brightness. Restoration occurs on successful GIF activation, cache-build failure, return to clock/image/text/DIY content, and Web UI/API ownership changes during staging.
- Adds host tests covering successful black staging, GIF-to-GIF replacement failure/retry, clock recovery, and external cancellation while the black staging state is active.

## Expected classic-ESP32 64x64 flow

`previous content -> black panel (internal Static staging) -> iDotMatrix Display -> cached GIF`

`/json/info` continues to expose `gifDecoder=compact12/cache`, `gifDecoderBytes=16128` on a 16x16 physical canvas, optional `gifCacheWaits=...`, and the existing heap/reset diagnostics.

## Release intent

If the dev.14 hardware pass confirms black staging, preserved WLED colour, repeated GIF replacement, clock/static/WLED transitions, Web UI responsiveness, and no memory regression, the next build should be the 0.7.1 release candidate/stable cleanup rather than another development architecture change.
