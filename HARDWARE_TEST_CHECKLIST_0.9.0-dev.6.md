# Hardware Test Checklist - 0.9.0-dev.6

Release: `0.9.0`  
Build: `0.9.0-dev.6`

- [ ] `/json/info` reports `release=0.9.0` and `build=0.9.0-dev.6`.
- [ ] Existing 64x64 Clock/TEXT/GIF/Carousel behavior remains unchanged outside upload.
- [ ] Start a Carousel upload that lasts longer than 250 ms: the transfer indicator appears.
- [ ] Arrow animation is legible and continuous.
- [ ] Matrix icon is legible at 64x64.
- [ ] Progress bar advances during the current asset transfer.
- [ ] Short transfers do not produce a distracting loading flash.
- [ ] Multiple Carousel assets can upload consecutively without Clock/WLED fallback between assets.
- [ ] At upload completion, Carousel playback starts without an intermediate fallback frame.
- [ ] Repeat with logical 32x32 -> physical 64x64.
- [ ] Repeat with logical 16x16 -> physical 64x64.
- [ ] Abort/disconnect during upload: the existing update-hold timeout eventually releases the indicator/fallback state.
