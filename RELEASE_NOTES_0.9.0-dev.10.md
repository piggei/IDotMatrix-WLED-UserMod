# iDotMatrix WLED Usermod 0.9.0-dev.10

Release: `0.9.0`  
Build: `0.9.0-dev.10`

## Focus

This build keeps the dev.9 Carousel upload diagnostics and introduces a dedicated native 64x64 transfer indicator rather than scaling the validated 16x16 artwork by 4x.

## Changes

- Preserves the validated 16x16 transfer artwork unchanged.
- Preserves exact 2x scaling for the 32x32 profile.
- Adds a native 64x64 transfer indicator with the same semantics: red arrow moving downward only, blue receiving tray, separate status bar.
- Uses additional 64x64 pixels for rounded tray corners, highlights and shaded arrow edges; no image asset or filesystem write is required.
- Keeps the status bar static while the real Carousel session asset count remains under protocol investigation.
- Keeps the dev.9 `/json/info` diagnostics: `carouselCfg=...` and `carouselUpload=...`.

## Hardware test goal

Upload a Carousel with a known number of images, visually validate the new 64x64 artwork, then capture `/json/info` so the true session-count semantics can be determined.
