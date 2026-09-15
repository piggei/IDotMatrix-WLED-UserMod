# iDotMatrix WLED Usermod 0.9.0-dev.9

Release: `0.9.0`  
Build: `0.9.0-dev.9`

## Focus

This development build corrects the Carousel transfer UI after physical 64x64 hardware testing of dev.7.

## Changes

- Corrected the progress model: the Device Assets configure count describes the 12-slot bank/order and is not the number of assets uploaded in the current session.
- Removed the misleading whole-Carousel percentage estimate based on `configuredCount`.
- During an active Carousel upload the bottom bar is now an indeterminate session indicator.
- When the existing Carousel quiet-period confirms that the complete upload session has ended, the bar becomes fully filled for a brief confirmation before playback begins.
- Replaced the previous packet/display artwork with the user-supplied 16x16 concept: a fixed blue receiving tray and a red arrow that translates downward only.
- The exact 16x16 pixel-art geometry is integer-scaled 2x and 4x for logical 32x32 and 64x64 profiles.
- No bitmap resources and no additional filesystem writes are introduced.

## Progress semantics

The protocol provides the byte length of the current asset but does not, in the observed Carousel upload flow, announce in advance how many assets will actually be sent or the total byte length of all future assets. A true global percentage is therefore not available during transfer. Build dev.9 deliberately uses an indeterminate global activity bar instead of displaying a false percentage.

## Scope

The indicator remains enabled for Carousel upload only. Long standalone/gallery GIF uploads remain a planned reuse of the same infrastructure after this UI is accepted on hardware.

## dev.9 diagnostic focus

Hardware feedback from dev.8 confirmed the 16x16 icon is readable but requested one logical pixel of separation from the bar. The indeterminate bar was also judged visually distracting. dev.9 therefore keeps only a static pending baseline until completion. It additionally exposes `carouselCfg=count:<n> order:<...>` and `carouselUpload=begin:<...> complete:<n>` in `/json/info` so one known-size upload can reveal whether the app's setup traffic contains the real session asset count or merely the 12-slot Carousel bank.
