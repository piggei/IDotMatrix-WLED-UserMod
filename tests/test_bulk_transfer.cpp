#include "../IDotMatrixBulkTransfer.h"

#include <cassert>
#include <cstring>

static size_t makePacket(
  uint8_t* packet,
  const uint8_t* payload,
  size_t payloadLength,
  uint32_t totalLength,
  uint32_t crc
) {
  const size_t packetLength = IDotMatrixBulkTransfer::HEADER_SIZE + payloadLength;
  memset(packet, 0, packetLength);
  packet[0] = static_cast<uint8_t>(packetLength & 0xFF);
  packet[1] = static_cast<uint8_t>((packetLength >> 8) & 0xFF);
  packet[2] = 0x03;
  packet[5] = static_cast<uint8_t>(totalLength & 0xFF);
  packet[6] = static_cast<uint8_t>((totalLength >> 8) & 0xFF);
  packet[7] = static_cast<uint8_t>((totalLength >> 16) & 0xFF);
  packet[8] = static_cast<uint8_t>((totalLength >> 24) & 0xFF);
  packet[9] = static_cast<uint8_t>(crc & 0xFF);
  packet[10] = static_cast<uint8_t>((crc >> 8) & 0xFF);
  packet[11] = static_cast<uint8_t>((crc >> 16) & 0xFF);
  packet[12] = static_cast<uint8_t>((crc >> 24) & 0xFF);
  memcpy(packet + IDotMatrixBulkTransfer::HEADER_SIZE, payload, payloadLength);
  return packetLength;
}

int main() {
  IDotMatrixBulkTransfer transfer;
  IDotMatrixBulkResult result;
  uint8_t packet[32]{};

  const uint8_t first[] = {'h', 'e'};
  size_t packetLength = makePacket(packet, first, sizeof(first), 5, 0x3610A686u);
  assert(transfer.processPacket(packet, packetLength, result));
  assert(result.handled && result.replyAvailable);
  assert(result.type == 0x03 && result.status == 0x01);
  assert(!result.completed && !transfer.textReady());

  const uint8_t second[] = {'l', 'l', 'o'};
  packetLength = makePacket(packet, second, sizeof(second), 5, 0x3610A686u);
  assert(transfer.processPacket(packet, packetLength, result));
  assert(result.status == 0x03 && result.completed && result.crcValid);
  assert(transfer.textReady());
  assert(transfer.textPayloadLength() == 5);
  assert(memcmp(transfer.textPayload(), "hello", 5) == 0);
  assert(transfer.acceptedChunks() == 2);
  assert(transfer.completedTransfers() == 1);

  const uint8_t bad[] = {'x'};
  packetLength = makePacket(packet, bad, sizeof(bad), 1, 0x12345678u);
  assert(transfer.processPacket(packet, packetLength, result));
  assert(result.completed && !result.crcValid);
  assert(transfer.crcErrors() == 1);
  assert(!transfer.textReady());

  packetLength = makePacket(
    packet,
    bad,
    sizeof(bad),
    IDotMatrixBulkTransfer::MAX_TEXT_PAYLOAD + 1,
    0
  );
  assert(transfer.processPacket(packet, packetLength, result));
  assert(result.status == 0x03 && result.completed && !result.crcValid);
  assert(transfer.rejectedTransfers() == 1);

  packet[0] ^= 1;
  assert(!transfer.processPacket(packet, packetLength, result));
}
