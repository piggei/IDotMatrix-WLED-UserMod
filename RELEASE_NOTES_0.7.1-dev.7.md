# 0.7.1-dev.7

This development build fixes a GIF/WLED ownership race found while validating full LZW12 decoding on classic ESP32.

A valid BLE GIF transfer is now only marked pending. The Usermod waits for filesystem promotion, decoder allocation, GIF open, and animation canvas setup to succeed before selecting the `iDotMatrix Display` WLED effect. If decoder preparation fails (for example `gif-ram-reserve`), WLED keeps its current effect and the failed transfer does not leave a false `content=gif` state.

`/json/info` reports `gifPending=1` while this asynchronous preparation is in progress.
