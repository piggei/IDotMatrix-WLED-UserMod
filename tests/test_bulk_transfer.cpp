#include "../IDotMatrixBulkTransfer.h"

#include <cassert>
#include <cstring>

static size_t makePacket(
  uint8_t* packet,
  uint8_t type,
  const uint8_t* payload,
  size_t payloadLength,
  uint32_t totalLength,
  uint32_t crc
) {
  const size_t packetLength = IDotMatrixBulkTransfer::HEADER_SIZE + payloadLength;
  memset(packet, 0, packetLength);
  packet[0] = static_cast<uint8_t>(packetLength & 0xFF);
  packet[1] = static_cast<uint8_t>((packetLength >> 8) & 0xFF);
  packet[2] = type;
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
  size_t packetLength = makePacket(packet, 0x03, first, sizeof(first), 5, 0x3610A686u);
  assert(transfer.processPacket(packet, packetLength, result));
  assert(result.handled && result.replyAvailable);
  assert(result.type == 0x03 && result.status == 0x01);
  assert(!result.completed && !transfer.textReady());

  const uint8_t second[] = {'l', 'l', 'o'};
  packetLength = makePacket(packet, 0x03, second, sizeof(second), 5, 0x3610A686u);
  assert(transfer.processPacket(packet, packetLength, result));
  assert(result.status == 0x03 && result.completed && result.crcValid);
  assert(transfer.textReady());
  assert(transfer.textPayloadLength() == 5);
  assert(memcmp(transfer.textPayload(), "hello", 5) == 0);

  const uint8_t bad[] = {'x'};
  packetLength = makePacket(packet, 0x03, bad, sizeof(bad), 1, 0x12345678u);
  assert(transfer.processPacket(packet, packetLength, result));
  assert(result.completed && !result.crcValid);
  assert(!transfer.textReady());

  packetLength = makePacket(
    packet,
    0x03,
    bad,
    sizeof(bad),
    IDotMatrixBulkTransfer::MAX_TEXT_PAYLOAD + 1,
    0
  );
  assert(transfer.processPacket(packet, packetLength, result));
  assert(result.status == 0x03 && result.completed && !result.crcValid);

  packetLength = makePacket(packet, 0x02, first, sizeof(first), 5, 0x3610A686u);
  assert(transfer.processPacket(packet, packetLength, result));
  assert(result.type == 0x02 && result.began);
  assert(result.totalLength == 5);
  assert(result.chunkOffset == 0 && result.chunkLength == 2);
  assert(result.chunkData != nullptr && result.chunkData[0] == 'h');
  assert(result.status == 0x01 && !result.completed);

  packetLength = makePacket(packet, 0x02, second, sizeof(second), 5, 0x3610A686u);
  assert(transfer.processPacket(packet, packetLength, result));
  assert(!result.began && result.completed && result.crcValid);
  assert(result.chunkOffset == 2 && result.chunkLength == 3);
  assert(result.status == 0x03);

  const uint8_t gif[] = {'G', 'I', 'F', '8', '9', 'a'};
  packetLength = makePacket(packet, 0x01, gif, sizeof(gif), sizeof(gif), 0x564ACEF2u);
  assert(transfer.processPacket(packet, packetLength, result));
  assert(result.type == 0x01 && result.began && result.completed);
  assert(result.expectedCRC == 0x564ACEF2u);
  assert(result.calculatedCRC == 0x564ACEF2u);
  assert(result.crcValid);

  packet[0] ^= 1;
  assert(!transfer.processPacket(packet, packetLength, result));
}
