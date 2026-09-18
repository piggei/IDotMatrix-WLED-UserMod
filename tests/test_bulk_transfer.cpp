#include "../IDotMatrixBulkTransfer.h"

#include <cassert>
#include <cstring>
#include <vector>

static size_t makePacket(
  uint8_t* packet,
  uint8_t type,
  const uint8_t* payload,
  size_t payloadLength,
  uint32_t totalLength,
  uint32_t crc,
  uint8_t option = 0,
  uint16_t timeSign = 0,
  uint8_t imageIndex = 12
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
  packet[4] = option;
  packet[9] = static_cast<uint8_t>(crc & 0xFF);
  packet[10] = static_cast<uint8_t>((crc >> 8) & 0xFF);
  packet[11] = static_cast<uint8_t>((crc >> 16) & 0xFF);
  packet[12] = static_cast<uint8_t>((crc >> 24) & 0xFF);
  packet[13] = static_cast<uint8_t>(timeSign & 0xFF);
  packet[14] = static_cast<uint8_t>((timeSign >> 8) & 0xFF);
  packet[15] = imageIndex;
  memcpy(packet + IDotMatrixBulkTransfer::HEADER_SIZE, payload, payloadLength);
  return packetLength;
}


static uint32_t crc32(const uint8_t* data, size_t length) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
  }
  return crc ^ 0xFFFFFFFFu;
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

  // Full 64-glyph 32x64 TEXT object: 14-byte global header plus
  // 64 x (4-byte glyph metadata + 256-byte bitmap) = 16654 bytes.
  std::vector<uint8_t> largeText(IDotMatrixBulkTransfer::MAX_TEXT_PAYLOAD);
  for (size_t i = 0; i < largeText.size(); ++i) largeText[i] = uint8_t(i & 0xFFu);
  const uint32_t largeCrc = crc32(largeText.data(), largeText.size());
  transfer.reset();
  size_t offset = 0;
  std::vector<uint8_t> largePacket(IDotMatrixBulkTransfer::HEADER_SIZE + 4096u);
  while (offset < largeText.size()) {
    const size_t chunk = (largeText.size() - offset) < 4096u ? (largeText.size() - offset) : 4096u;
    const size_t n = makePacket(largePacket.data(), 0x03, largeText.data() + offset, chunk,
                                uint32_t(largeText.size()), largeCrc);
    assert(transfer.processPacket(largePacket.data(), n, result));
    offset += chunk;
    if (offset < largeText.size()) {
      assert(result.status == 0x01 && !result.completed);
    } else {
      assert(result.status == 0x03 && result.completed && result.crcValid);
    }
  }
  assert(transfer.textReady());
  assert(transfer.textPayloadLength() == largeText.size());
  assert(memcmp(transfer.textPayload(), largeText.data(), largeText.size()) == 0);

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

  const uint8_t metaPayload[] = {'o','k'};
  packetLength = makePacket(packet, 0x01, metaPayload, sizeof(metaPayload), sizeof(metaPayload), 0x79DCDD47u, 2, 300, 7);
  assert(transfer.processPacket(packet, packetLength, result));
  assert(result.option == 2);
  assert(result.timeSign == 300);
  assert(result.imageIndex == 7);


  // Device Assets metadata is transaction metadata. Continuation chunks from
  // the app are allowed to carry different/zero values: the first chunk stays
  // authoritative and the transfer must not abort.
  transfer.reset();
  const uint8_t assetA[] = {'a', 'b'};
  const uint8_t assetB[] = {'c', 'd', 'e'};
  packetLength = makePacket(packet, 0x01, assetA, sizeof(assetA), 5, 0x8587D865u, 2, 30, 4);
  assert(transfer.processPacket(packet, packetLength, result));
  assert(result.began && !result.completed);
  assert(result.option == 2 && result.timeSign == 30 && result.imageIndex == 4);
  packetLength = makePacket(packet, 0x01, assetB, sizeof(assetB), 5, 0x8587D865u, 0, 0, 12);
  assert(transfer.processPacket(packet, packetLength, result));
  assert(result.completed && result.crcValid && !result.aborted);
  assert(result.option == 2 && result.timeSign == 30 && result.imageIndex == 4);

  packet[0] ^= 1;
  assert(!transfer.processPacket(packet, packetLength, result));
}
