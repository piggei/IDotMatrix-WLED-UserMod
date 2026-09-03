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
};

class IDotMatrixBulkTransfer {
public:
  static constexpr size_t HEADER_SIZE = 16;
  static constexpr size_t MAX_TEXT_PAYLOAD = 4096;

  bool processPacket(
    const uint8_t* data,
    size_t length,
    IDotMatrixBulkResult& result
  );
  void reset();

  bool textReady() const { return textReady_; }
  const uint8_t* textPayload() const { return textPayload_; }
  size_t textPayloadLength() const { return textPayloadLength_; }
  uint32_t acceptedChunks() const { return acceptedChunks_; }
  uint32_t completedTransfers() const { return completedTransfers_; }
  uint32_t crcErrors() const { return crcErrors_; }
  uint32_t rejectedTransfers() const { return rejectedTransfers_; }

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
  uint32_t acceptedChunks_ = 0;
  uint32_t completedTransfers_ = 0;
  uint32_t crcErrors_ = 0;
  uint32_t rejectedTransfers_ = 0;
};
