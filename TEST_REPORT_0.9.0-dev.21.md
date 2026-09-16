# Test report - 0.9.0-dev.21

## Host validation

`./run_host_tests.sh` passes after the Schedule framing and flow-control changes.

The protocol suite covers:

- single-packet Schedule activity -> final ACK `0x03`;
- 9000-byte three-packet activity -> 4096 + 4096 + 808;
- first marker `0x00`, continuation marker `0x02`;
- intermediate ACK sequence `0x01`, `0x01`;
- final CRC-valid committed ACK `0x03`;
- automation-layer rejection -> ACK `0x02`;
- no reset of multipart accumulation when only the chunk marker changes;
- previously existing Alarm multipart, timeout, CRC and package/profile regressions.

Hardware validation on the official 64x64 app remains required.
