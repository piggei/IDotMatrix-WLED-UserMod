# Hardware Test Checklist - 0.9.0-dev.11

Release: `0.9.0`  
Build: `0.9.0-dev.11`

- [ ] `/json/info` reports `release=0.9.0` and `build=0.9.0-dev.11`.
- [ ] Start a multi-image Carousel upload.
- [ ] The red arrow moves downward only into the blue tray.
- [ ] The status bar is visually separated from the tray.
- [ ] While upload is active, a short green segment sweeps left and right continuously.
- [ ] The moving segment is clearly an activity indicator and does not imply percentage completion.
- [ ] At the end of the transfer session, the status bar becomes fully green briefly before playback starts.
- [ ] Carousel playback starts normally and all uploaded assets are present.
- [ ] No regression in GIF playback, TEXT, Clock, WLED effects, or logical profile scaling.
