#pragma once

#include <cstddef>
#include <cstdint>

struct IDotMatrixBulkResult {
  bool handled = false;
  bool replyAvailable = false;
  uint8_t type = 0;
  uint8_t status = 0;
  bool completed = false;
  bool crcValid = false;
  bool began = false;
  bool aborted = false;
  const uint8_t* chunkData = nullptr;
  size_t chunkOffset = 0;
  size_t chunkLength = 0;
  uint32_t totalLength = 0;
  uint32_t expectedCRC = 0;
  uint32_t calculatedCRC = 0;
};

class IDotMatrixBulkTransfer {
public:
  static constexpr size_t HEADER_SIZE = 16;
  static constexpr size_t MAX_TEXT_PAYLOAD = 4096;
  static constexpr size_t MAX_RAW_PAYLOAD = 64u * 64u * 3u;
  static constexpr size_t MAX_GIF_PAYLOAD = 2u * 1024u * 1024u;

  bool processPacket(
    const uint8_t* data,
    size_t length,
    IDotMatrixBulkResult& result
  );
  void reset();

  bool textReady() const { return textReady_; }
  const uint8_t* textPayload() const { return textPayload_; }
  size_t textPayloadLength() const { return textPayloadLength_; }
  uint8_t activeType() const { return activeType_; }

private:
  static uint32_t updateCRC32(uint32_t crc, const uint8_t* data, size_t length);
  void resetActive();

  bool active_ = false;
  uint8_t activeType_ = 0;
  uint32_t expectedSize_ = 0;
  uint32_t expectedCRC_ = 0;
  uint32_t runningCRC_ = 0xFFFFFFFFu;
  uint32_t receivedSize_ = 0;
  uint8_t textPayload_[MAX_TEXT_PAYLOAD]{};
  size_t textPayloadLength_ = 0;
  bool textReady_ = false;
};
