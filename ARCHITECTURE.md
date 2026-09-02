# Architecture

## Goal

The standalone ESP32 emulator is the behavioral reference, not the desired WLED
structure. BLE transport, protocol semantics, and WLED integration remain
separate so future features do not recreate a monolithic sketch.

## Components

### `IDotMatrixBLEServer`

Owns NimBLE, FA/AE GATT, advertising, reconnection, short callbacks, the bounded
receive queue, FA03 notifications, and connection-time device-info scheduling.
It does not interpret WLED commands or render pixels.

### `IDotMatrixProtocol`

Owns length validation, command recognition, field extraction, clamping, ACKs,
device-info responses, and typed events. It includes neither NimBLE nor WLED and
is host-testable.

### `IDotMatrixWLEDAdapter`

Is the only current WLED boundary. It maps typed events to power, master
brightness, static effect, and colour.

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

Brightness persistence is intentionally not duplicated.

## Memory and concurrency rules

- The protocol decoder uses fixed response storage.
- The callback queue is four 64-byte entries plus metadata.
- Large transfers must use a separate streaming path.
- Future media must stream to LittleFS using separate RX and PLAY files.
- AnimatedGIF must be recreated for each promoted GIF.
- PSRAM may optimize large profiles but cannot be required for 16x16.

## Future renderer

Graffiti/media will add `IDotMatrixRenderer`, owning a resolution-independent
16x16/32x32/64x64 logical framebuffer. The WLED adapter must translate logical
coordinates through WLED's configured 2D mapping rather than duplicate physical
wiring logic.

Bulk reassembly/streaming must be separate from the small-command queue before
text or GIF support is enabled.
