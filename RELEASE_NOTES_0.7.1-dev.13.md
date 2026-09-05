# 0.7.1-dev.13

This build targets the intermittent repeated-GIF failure observed after dev.12 first proved the compact 64x64 decoder/cache path on classic ESP32. The LZW12 decoder itself is unchanged: all 4096 legal codes remain supported and the 16x16 physical-canvas workspace remains 16128 bytes.

Changes:

- A valid no-PSRAM replacement transfer retires the previous cached GIF before the new precache begins; invalid/CRC-failed transfers preserve the current GIF.
- The 9 KiB predecode guard now waits and retries through short-lived RAM pressure. Only a continuous 2-second low-heap condition aborts with `gif-ram-reserve`.
- If a replacement GIF fails after the old GIF has been retired, recovery uses WLED Static instead of reactivating an empty iDotMatrix display effect. This removes the state that previously required manually selecting a solid colour before later GIFs would work again.
- Restorable clock/text/image/DIY canvas visibility is restored after a failed GIF preparation.
- `/json/info` adds `gifCacheWaits=N low=M guard=9216` when cache construction has yielded because of transient heap pressure.
- Host tests now cover twelve consecutive cached-GIF replacements plus GIF-to-GIF failure/retry and clock restoration.

Recommended hardware test: alternate the same two GIFs that reproduced the dev.12 issue for at least ten A/B cycles, then test a large 80-100 frame GIF while repeatedly loading `/json/info`.
