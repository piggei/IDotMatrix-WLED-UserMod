# Architecture

## Goal

The standalone ESP32 emulator is the behavioral reference, not the desired WLED
structure. BLE transport, protocol semantics, and WLED integration remain
separate so future features do not recreate a monolithic sketch.

## Components

### `IDotMatrixBLEServer`

Owns NimBLE, FA/AE GATT, advertising, reconnection, short callbacks, the bounded
receive queue, FA03 notifications, and connection-time device-info scheduling.
It also owns bounded FA02 write-to-logical-packet reassembly. AE01 is retained
for GATT compatibility but is not the bulk path in BUILD 80. The server does
not interpret WLED commands or render pixels.

### `IDotMatrixFA02Assembler`

Owns the single bounded 4112-byte reconstruction buffer. It uses the first
write's little-endian packet length and accepts continuation writes until the
logical packet is complete. It has no BLE, protocol, or WLED dependency.

### `IDotMatrixBulkTransfer`

Owns WLED-independent bulk-header validation, bounded TEXT assembly, streaming
CRC32, and transfer-result state. A valid type-`0x03` payload is passed to the
protocol layer from WLED loop context.

### `IDotMatrixProtocol`

Owns length validation, command recognition, TEXT header/glyph-record parsing,
field extraction, clamping, ACKs, device-info responses, and typed events. It
includes neither NimBLE nor WLED and is host-testable.

### `IDotMatrixWLEDAdapter`

Is the only current WLED boundary. It maps typed events to power, master
brightness, static colour, and display-content ownership. It registers one
`iDotMatrix Display` effect and renders every app-driven canvas from that WLED
segment-service callback. It is also the only layer
that reads WLED local time or maps logical pixels to physical segment pixels.

### `IDotMatrixRenderer`

Owns the logical RGB canvas and no WLED or BLE APIs. It allocates three bytes per
pixel at runtime: 768 bytes for 16x16, 3072 bytes for 32x32, or 12288 bytes for
64x64. It validates logical coordinates and renders clock artwork and
app-rasterized text without WLED dependencies. Glyph storage is allocated in
loop context and reused when possible. The renderer is independently
host-testable.

### `usermod_idotmatrix.cpp`

Owns registration, settings, the I2S/RMT guard, Wi-Fi modem-sleep requirement,
delayed BLE startup, and `/json/info` diagnostics.

## Execution model

1. WLED initializes LED and Wi-Fi facilities.
2. The Usermod waits five seconds, then starts NimBLE.
3. A BLE callback copies bounded data into a four-entry queue.
4. The WLED loop dequeues the write.
5. The protocol validates and decodes it.
6. The adapter applies a typed event to WLED.
7. A bounded reply is notified through FA03.

For graffiti, the adapter updates the canvas during step 6 and selects the
registered `iDotMatrix Display` effect. WLED calls that effect during its
normal segment service, after establishing the current segment and its virtual
XY dimensions. The effect copies the logical canvas through
`SEGMENT.setPixelColorXY()`, so WLED retains physical mapping ownership.

For clock commands, the adapter stores typed options and selects the same
`iDotMatrix Display` effect. Its callback reads WLED `localTime`, asks the
renderer to update the logical canvas, and then uses the same bounded output
path. TEXT payloads are parsed in loop context, copied into bounded reusable
glyph storage, and rendered through this same effect. Future media and timers
should also use this single display effect.

BLE callbacks do not call WLED state/rendering APIs and contain no `delay()`.

## Connection sequence

1. The callback records connection state and queues an event.
2. The WLED loop applies logical screen ON.
3. Device info is sent after about 1.2 seconds.
4. The same packet is retried after about 2.5 seconds.

The retry handles app subscription/UI timing observed in WLED.

## State ownership

| State | Authority |
|---|---|
| Power | WLED, changed by app events or WLED interfaces |
| Brightness | WLED `bri`/`briLast` and WLED persistence |
| Solid colour/effect | WLED selected-segment state |
| App brightness UI | App-local and unreadable by the device |
| Emulated profile | Usermod configuration |
| Graffiti pixels | `IDotMatrixRenderer` logical framebuffer |
| Clock time, timezone, and DST | WLED time/NTP subsystem |
| Clock style/colour/date option | Last command received from the app |
| Text bitmap/style/motion | Last valid app-rasterized TEXT payload |
| Physical XY/serpentine mapping | WLED matrix configuration |

Brightness persistence is intentionally not duplicated.

## Memory and concurrency rules

- The protocol decoder uses fixed response storage.
- The callback queue is four 64-byte entries plus metadata.
- Fragmented or large FA02 traffic uses one 4112-byte reconstruction slot; the
  callback copies into it and loop context performs parsing, CRC, and replies.
- The canvas is allocated once during Usermod setup and never from a BLE
  callback.
- TEXT bitmap storage is allocated or grown only in loop context, is reused,
  and is capped at 4096 bytes.
- Large transfers must use a separate streaming path.
- Future media must stream to LittleFS using separate RX and PLAY files.
- AnimatedGIF must be recreated for each promoted GIF.
- PSRAM may optimize large profiles but cannot be required for 16x16.

## Renderer extension

Graffiti and clock use `IDotMatrixRenderer` with a resolution-independent
16x16/32x32/64x64 logical framebuffer. Future text and decoded media should draw
into the same canvas and explicitly take display-content ownership.

The advertised profile remains an explicit Usermod setting. The WLED segment is
the physical target. Strict native-size matching is the default; optional
nearest-neighbour rescale permits lossy 32/64-to-16 previews and unusual target
sizes without changing protocol coordinates.

Bulk reassembly/streaming must be separate from the small-command queue before
text or GIF support is enabled.
