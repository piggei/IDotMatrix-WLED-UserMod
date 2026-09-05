# 0.8.0-dev.7

This development build fixes a RAM-retention regression in the LZW11/32x32 build.

When iDotMatrix GIF playback is active and the user selects a normal WLED effect from the Web UI or API, WLED changes the segment mode without sending a BLE content command. Earlier builds therefore left the GIF decoder allocated and `content=gif` active, reducing the largest free heap block until memory-hungry WLED effects could fail with `Error 8: Effect RAM depleted!`.

0.8.0-dev.7 detects that the dedicated `iDotMatrix Display` effect is no longer selected and immediately:

- stops GIF playback;
- frees dynamic decoder storage;
- clears all iDotMatrix content ownership flags;
- hides the iDotMatrix canvas;
- returns full ownership to the selected WLED effect.

The 16x16 static LZW10 behavior and the 32x32 on-demand LZW11 allocation strategy are otherwise unchanged.
