# Architecture

This document describes the stable 0.7.0 architecture.

## Design goals

The Usermod keeps BLE transport, reverse-engineered protocol semantics, media
handling, rendering, and WLED state changes separate. BLE callbacks must stay
short and bounded; filesystem access, CRC-heavy work, decoding, allocation, and
WLED rendering happen in normal WLED loop/effect context.

## Components

### `IDotMatrixBLEServer`

Owns NimBLE, the FA/AE GATT database, advertising, reconnection, delayed device
information notifications, short-command queuing, and FA02 logical-packet
reassembly. It requests MTU 517 so the official app can use large ATT payloads.
It does not render pixels or directly change WLED state.

### `IDotMatrixFA02Assembler`

Owns one bounded 4112-byte logical-packet buffer. The first two little-endian
bytes define the complete FA02 packet length. ATT fragments are appended until
the logical packet is complete. No protocol ACK is emitted for an ATT fragment.
An abandoned partial transfer is cleared after five seconds so later commands
cannot be poisoned by stale reassembly state.

### `IDotMatrixBulkTransfer`

Validates the common bulk header, enforces sequential chunks, calculates CRC32,
and exposes decoded chunk spans. TEXT is retained in a bounded buffer; RAW and
GIF are streamed onward chunk by chunk.

### `IDotMatrixProtocol`

Owns WLED-independent command validation, typed command decoding, TEXT record
parsing, ACK generation, device-information replies, and the media-sink
boundary. It is host-testable.

### `IDotMatrixRenderer`

Owns the logical RGB framebuffer, graffiti pixels, clock artwork, text glyphs,
and animation frame publication. The visible canvas uses three bytes per pixel:
768 bytes at 16x16, 3072 at 32x32, and 12288 at 64x64. RAW transfers use a
temporary canvas and are published atomically only after successful completion.

### `IDotMatrixMedia`

Owns filesystem-backed GIF reception/playback and compact PNG decoding. GIF
chunks stream to alternating RX files, a valid completed upload is promoted to
a PLAY file, and decoding begins in a later WLED loop iteration.

For 0.7.0 the repository patches AnimatedGIF 1.4.7 at PlatformIO build time for
the validated 16x16 target. The decoder uses a maximum width of 16, a 1024-byte
file buffer, a 1024-entry LZW dictionary, and reduced pixel workspace. The
`AnimatedGIF` object is then stored in fixed Usermod memory and reconstructed in
place for each animation, avoiding heap fragmentation without consuming the
~22 KiB required by the stock library configuration.

### `IDotMatrixWLEDAdapter`

Is the WLED boundary. It maps power, brightness, RGB, and app-content ownership
to WLED. It registers one effect named `iDotMatrix Display`, reads WLED local
time for clocks, and applies WLED's configured 2D mapping/rescale when emitting
pixels.

### `usermod_idotmatrix.cpp`

Owns Usermod lifecycle, settings, the I2S/RMT safety guard, Wi-Fi modem-sleep
requirement, delayed BLE startup, and compact runtime status under `/json/info`.

## Execution model

1. WLED initializes LEDs and Wi-Fi.
2. The Usermod allocates its logical renderer and waits five seconds.
3. NimBLE starts and advertises the iDotMatrix-compatible GATT database.
4. BLE callbacks only copy bounded writes or append FA02 fragments.
5. The WLED loop processes complete packets and sends replies.
6. Protocol events are applied through the WLED adapter.
7. `iDotMatrix Display` renders app-owned content from WLED effect context.
8. GIF filesystem/decoder work runs in normal loop context, never a BLE callback.

## State ownership

| State | Authority |
|---|---|
| Power / brightness | WLED |
| Solid colour | WLED segment state |
| BLE logical profile | Usermod configuration |
| Graffiti / RAW / PNG / GIF visible pixels | `IDotMatrixRenderer` |
| Clock time, timezone, DST | WLED time subsystem |
| Clock style / text style | last valid app command |
| Physical XY/serpentine mapping | WLED matrix configuration |

## Memory rules

- no blocking media decode in NimBLE callbacks;
- four 64-byte queue slots for complete short commands;
- one 4112-byte FA02 reassembly buffer for fragmented/large packets;
- one persistent logical framebuffer;
- RAW uses one temporary framebuffer only during reception;
- TEXT storage is bounded and allocated only outside BLE callbacks;
- GIF bytes are streamed to LittleFS rather than buffered completely in RAM;
- 0.7.0's 16x16 AnimatedGIF object is about 8 KiB and uses fixed placement
  storage to avoid heap fragmentation;
- PNG/miniz scratch memory is temporary;
- PSRAM is not required for the validated 16x16 target.

## Profile scope

The renderer and protocol retain 16x16, 32x32, and 64x64 logical profile
support. Stable 0.7.0 hardware validation is 16x16. The low-RAM GIF decoder is
specifically capped at 16x16, so larger-profile GIF support is intentionally a
future task rather than a 0.7.0 claim.
