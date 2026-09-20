# iDotMatrix WLED Usermod 0.9.1-dev.1

Release `0.9.1`, build `0.9.1-dev.1` is the first development build after the stable 0.9.0 release. It is intentionally narrow: it adds the original-app Graffiti full-raster multipart path, validates a practical ESP32-C3 OTA layout, and reorganizes build-support files without changing the established 0.9.0 feature set.

## Graffiti full-raster multipart protocol

A Bluetooth HCI capture against original 64x64 iDotMatrix hardware, cross-checked with standalone emulator B171, confirms a dedicated Graffiti transport that is separate from compact inline PNG and the generic 16-byte GIF/RAW/TEXT Bulk format.

Each logical FA02 packet uses a 9-byte header:

```text
0..1  logical packet length, little-endian
2     type 0x00
3     0x00
4     marker: 0x00 first, 0x02 continuation
5..8  complete raster byte count, little-endian
9..   raw row-major RGB bytes for this chunk
```

For a native 64x64 RGB canvas the complete raster is 12288 bytes. The captured original-app transfer is three complete logical packets carrying 4096 RGB bytes each:

```text
first  4096 bytes, marker 0x00 -> ACK 05 00 00 00 02
next   4096 bytes, marker 0x02 -> ACK 05 00 00 00 02
final  4096 bytes, marker 0x02 -> ACK 05 00 00 00 01
```

Status `0x02` means accepted but incomplete for this command family; status `0x01` is the completion ACK. This is deliberately not interpreted using the generic Bulk `0x01`/`0x03` status rule. No CRC field is present in the confirmed 9-byte Graffiti envelope.

The WLED implementation keeps the two fragmentation layers separate: ATT writes are first reassembled into one FA02 logical packet, then multiple complete Graffiti packets are accumulated into one raster transaction. The existing 8192-byte FA02 limit therefore remains unchanged. RGB chunks stream directly into the renderer staging sink, avoiding an additional 12 KiB protocol buffer on 64x64.

Partial Graffiti transactions are cancelled on timeout, BLE disconnect, protocol reset, a new first marker, or replacement by an incompatible transfer. Completion publishes the display as Graffiti/DIY ownership. Compact PNG and generic Bulk RAW remain independent paths.

## ESP32-C3 OTA profile

A new `overrides/esp32c3-16x16-audio-ota.ini` profile is included for the qualified 4 MB ESP32-C3 / WS2812B 16x16 / AudioReactive combination. It uses `partitions/WLED_ESP32_4MB_IDOT_OTA.csv` with:

```text
app0      0x1A0000
app1      0x1A0000
LittleFS  0x0A0000 (640 KiB nominal)
coredump  0x010000
```

The tested OTA-enabled firmware.bin was approximately 1.55 MB, leaving about 149 KiB per application slot. Hardware qualification completed three consecutive WLED OTA updates with a populated filesystem and simultaneous iDotMatrix load: 12 Carousel assets, six Preset assets, a three-activity Schedule, BLE, shared-RMT output, GIF cache and AudioReactive. The legacy no-OTA C3 profiles remain available.

## Repository layout cleanup

Build-support files are now grouped by purpose:

```text
overrides/
partitions/
```

All supplied PlatformIO profiles reference partition tables through the new `partitions/` paths. Documentation and static profile tests have been updated accordingly.

## Documentation corrections

- Protocol documentation now distinguishes Graffiti full-raster type-0 multipart traffic from compact inline PNG and generic Bulk RAW.
- The 64x64 captured marker and ACK sequence is documented explicitly.
- C3 profile comments identify 0.8.2 as a historical qualification baseline rather than the current build.
- Countdown documentation is aligned with the released renderer: normal seconds are orange and the final ten seconds are red.

## Validation status

Host regressions reproduce the captured three-packet Graffiti transfer byte-for-byte and verify timeout/cancellation behavior. The C3 OTA profile is hardware-validated. The new Graffiti full-raster WLED path remains **protocol-validated / host-tested** until this development build is exercised against the official app on physical hardware.
