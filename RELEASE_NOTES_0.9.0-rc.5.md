# iDotMatrix WLED Usermod 0.9.0-rc.5

RC5 fixes autonomous Preset/Carousel ownership when a live TEXT payload takes over the display.

## Fixed

- Live TEXT received from the app or automation now suspends autonomous Preset and Carousel playback before publishing the text. This prevents a still-running Preset dwell timer from replacing the newly selected TEXT roughly three seconds later.
- Stored TEXT played internally by Preset or Carousel explicitly keeps its owner active, so the ownership fix does not make a playlist suspend itself.

## Regression coverage

- Direct `processTextPayload()` is verified to suspend both autonomous players.
- Internal stored-TEXT parsing is verified not to suspend either player when invoked with `takeDisplayOwnership=false`.
- RC4 large-TEXT/64px support and all prior protocol/rendering behavior remain unchanged.

## Retained from RC4

- Full 16654-byte TEXT payload support remains unchanged, including 64 glyphs at 32x64/font-64 and dynamic/PSRAM-backed storage.
