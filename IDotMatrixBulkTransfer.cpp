#include "IDotMatrixBulkTransfer.h"

#include <cstring>

namespace {
constexpr uint8_t TEXT_TYPE = 0x03;
constexpr uint8_t RAW_TYPE = 0x02;
constexpr uint8_t GIF_TYPE = 0x01;
constexpr uint8_t ACK_CONTINUE = 0x01;
constexpr uint8_t ACK_COMPLETE = 0x03;

uint32_t readLE32(const uint8_t* data) {
  return uint32_t(data[0]) |
    (uint32_t(data[1]) << 8) |
    (uint32_t(data[2]) << 16) |
    (uint32_t(data[3]) << 24);
}
}

bool IDotMatrixBulkTransfer::processPacket(
  const uint8_t* data,
  size_t length,
  IDotMatrixBulkResult& result
) {
  result = IDotMatrixBulkResult{};
  if (data == nullptr || length < HEADER_SIZE) return false;

  const uint16_t declaredLength = uint16_t(data[0]) | (uint16_t(data[1]) << 8);
  const uint8_t type = data[2];
  if (declaredLength != length || data[3] != 0x00 ||
      (type != TEXT_TYPE && type != RAW_TYPE && type != GIF_TYPE)) {
    return false;
  }

  result.handled = true;
  result.type = type;
  const uint32_t totalSize = readLE32(data + 5);
  const uint32_t expectedCRC = readLE32(data + 9);
  result.totalLength = totalSize;
  result.expectedCRC = expectedCRC;

  const uint32_t maximumSize = type == TEXT_TYPE ? MAX_TEXT_PAYLOAD :
    type == RAW_TYPE ? MAX_RAW_PAYLOAD : MAX_GIF_PAYLOAD;
  if (totalSize == 0 || totalSize > maximumSize) {
    resetActive();
    result.replyAvailable = true;
    result.status = ACK_COMPLETE;
    result.completed = true;
    return true;
  }

  if (!active_) {
    active_ = true;
    activeType_ = type;
    expectedSize_ = totalSize;
    expectedCRC_ = expectedCRC;
    runningCRC_ = 0xFFFFFFFFu;
    receivedSize_ = 0;
    if (type == TEXT_TYPE) {
      textPayloadLength_ = 0;
      textReady_ = false;
    }
    result.began = true;
  } else if (type != activeType_ || totalSize != expectedSize_ ||
             expectedCRC != expectedCRC_) {
    resetActive();
    result.aborted = true;
    return true;
  }

  const size_t payloadLength = length - HEADER_SIZE;
  const uint32_t remaining = expectedSize_ - receivedSize_;
  const size_t useful = payloadLength < remaining ? payloadLength : remaining;
  result.chunkData = data + HEADER_SIZE;
  result.chunkOffset = receivedSize_;
  result.chunkLength = useful;
  if (useful > 0) {
    if (type == TEXT_TYPE) {
      memcpy(textPayload_ + receivedSize_, data + HEADER_SIZE, useful);
    }
    runningCRC_ = updateCRC32(runningCRC_, data + HEADER_SIZE, useful);
    receivedSize_ += useful;
    if (type == TEXT_TYPE) textPayloadLength_ = receivedSize_;
  }

  result.replyAvailable = true;
  if (receivedSize_ < expectedSize_) {
    result.status = ACK_CONTINUE;
    return true;
  }

  result.status = ACK_COMPLETE;
  result.completed = true;
  result.calculatedCRC = runningCRC_ ^ 0xFFFFFFFFu;
  result.crcValid = result.calculatedCRC == expectedCRC_;
  if (result.crcValid) {
    if (type == TEXT_TYPE) textReady_ = true;
  } else {
    if (type == TEXT_TYPE) {
      textReady_ = false;
      textPayloadLength_ = 0;
    }
  }
  resetActive();
  return true;
}

uint32_t IDotMatrixBulkTransfer::updateCRC32(
  uint32_t crc,
  const uint8_t* data,
  size_t length
) {
  for (size_t index = 0; index < length; ++index) {
    crc ^= data[index];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
    }
  }
  return crc;
}

void IDotMatrixBulkTransfer::resetActive() {
  active_ = false;
  activeType_ = 0;
  expectedSize_ = 0;
  expectedCRC_ = 0;
  runningCRC_ = 0xFFFFFFFFu;
  receivedSize_ = 0;
}

void IDotMatrixBulkTransfer::reset() {
  resetActive();
  textPayloadLength_ = 0;
  textReady_ = false;
}
