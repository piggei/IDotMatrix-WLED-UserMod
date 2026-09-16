# 0.9.0-dev.23

## Preset / Default upload feedback

This build adds the already validated Carousel-style indeterminate transfer animation to long Preset / Default uploads. The indicator starts when Bulk media for slots 14..19 begin arriving, remains visible across consecutive Preset assets, and is replaced directly by the newly activated playlist when `06/02` arrives.

A 5-second idle timeout clears an abandoned upload indicator and restores the prior active Preset where applicable. Transport, multipart ACK, CRC, pending staging, and `06/02` activation behavior are unchanged from dev.22.
