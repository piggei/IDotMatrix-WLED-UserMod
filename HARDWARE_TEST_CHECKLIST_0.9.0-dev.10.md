# Hardware Test Checklist - 0.9.0-dev.10

Release: `0.9.0`  
Build: `0.9.0-dev.10`

- [ ] `/json/info` reports `release=0.9.0` and `build=0.9.0-dev.10`.
- [ ] On 16x16, the validated red-arrow/blue-tray artwork remains unchanged.
- [ ] On 64x64, the transfer indicator uses the smoother native artwork rather than a blocky 4x enlargement.
- [ ] The red arrow translates downward only; there is no bounce or hammer/press motion.
- [ ] The blue tray is visually distinct from the status bar.
- [ ] The status bar remains static during transfer and fills only when the upload session completes.
- [ ] Upload a Carousel with a known number of images (recommended: 5).
- [ ] Record `carouselCfg=count:... order:...` from `/json/info`.
- [ ] Record `carouselUpload=begin:... complete:...` from `/json/info`.
- [ ] Compare the configured count/order with the known number of assets actually transferred.
