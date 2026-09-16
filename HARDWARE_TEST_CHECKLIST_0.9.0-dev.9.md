# Hardware Test Checklist - 0.9.0-dev.9

Release: `0.9.0`  
Build: `0.9.0-dev.9`

- [ ] `/json/info` reports `release=0.9.0` and `build=0.9.0-dev.9`.
- [ ] Start a Carousel upload lasting longer than 250 ms: the transfer indicator appears.
- [ ] On logical 16x16 the icon matches the reference concept: fixed blue tray, red arrow moving downward only.
- [ ] The arrow never bounces, reverses direction, squashes or visually behaves like a hammer/press.
- [ ] On 32x32 and 64x64 the same icon remains crisp and proportionally identical.
- [ ] During upload the bottom bar is clearly indeterminate and does not pretend to show a false percentage.
- [ ] The bar remains continuous across consecutive Carousel assets.
- [ ] After the final upload quiet-period the bar briefly reaches a fully filled state.
- [ ] Carousel playback begins after the completion confirmation without Clock/native-WLED flash.
- [ ] Quick regression: 64x64, 32->64 and 16->64 logical profiles still render correctly.
- [ ] Quick regression: TEXT, GIF playback and Snowflake remain unchanged.

## Carousel count diagnostic

- [ ] Upload a Carousel with a known number of assets (recommended: 5).
- [ ] Confirm arrow/tray artwork is one logical pixel above the static status bar.
- [ ] Confirm the pending bar does not move or wave during transfer.
- [ ] Capture `/json/info` immediately after the upload.
- [ ] Record `carouselCfg=count:... order:...`.
- [ ] Record `carouselUpload=begin:... complete:...`.
- [ ] Compare configured count/order with the known number of assets actually transferred.
