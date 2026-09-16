# iDotMatrix WLED Usermod 0.9.0-dev.13

Release: `0.9.0`  
Build: `0.9.0-dev.13`

## Summary

This build finalises the Carousel transfer-status behaviour after hardware diagnostics proved that the app does not announce the real number of assets in an upload session before transfer completion.

## Changes

- Keeps the periodic WLED refresh scheduling introduced in dev.12, so the indeterminate Carousel activity bar continues to animate independently of BLE chunk arrival.
- Removes the final full-width / 100% bar state; the activity segment stays indeterminate until the transfer UI is retired.
- Keeps the hardware-approved 16x16 and native 64x64 transfer artwork unchanged.

- Keeps the validated 16x16 red-arrow / blue-tray artwork and the dedicated native 64x64 artwork from dev.10.
- Replaces the static pending status line with a deliberately indeterminate activity segment that sweeps left and right.
- The activity bar does not claim percentage progress because `carouselCfg=count:12` describes the complete Carousel slot bank, while the real transfer count is only known from the assets that actually arrive.
- The indeterminate activity segment remains visually indeterminate through transfer completion; it disappears directly when Carousel playback resumes, avoiding a misleading 100% state.
- Keeps the dev.9/dev.10 `carouselCfg=...` and `carouselUpload=...` diagnostics for further protocol study.
- No changes to GIF decoding, Carousel storage, BLE framing, logical-profile scaling, TEXT, Clock, audio, or WLED/HUB75 ownership.

## Hardware evidence behind the change

A seven-image Carousel upload produced:

```text
carouselCfg=count:12 order:0,1,2,3,4,5,6,7,8,9,10,11
carouselUpload=begin:0,1,2,3,4,5,6 complete:7
```

Therefore the firmware cannot calculate a truthful global percentage before the session is complete.
