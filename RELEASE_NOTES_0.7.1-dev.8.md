# 0.7.1-dev.8

This build fixes the GIF activation race seen in dev.7. The iDotMatrix display effect is staged first, then decoder allocation/open is attempted after WLED has had a loop to commit its own effect RAM. If preparation fails, the previous WLED effect is restored.
