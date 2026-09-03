#include "IDotMatrixFA02Assembler.h"

#include <cstring>

IDotMatrixFA02Assembler::Result IDotMatrixFA02Assembler::append(
  const uint8_t* data,
  size_t length
) {
  if (complete_) return Result::Busy;
  if (data == nullptr || length == 0) return Result::Invalid;

  if (expected_ == 0) {
    if (length < 2) return Result::Invalid;
    expected_ = uint16_t(data[0]) | (uint16_t(data[1]) << 8);
    if (expected_ == 0 || expected_ > MAX_PACKET_SIZE) {
      reset();
      return Result::Invalid;
    }
  }

  const size_t remaining = expected_ - received_;
  const size_t copyLength = length < remaining ? length : remaining;
  memcpy(packet_ + received_, data, copyLength);
  received_ += copyLength;
  if (received_ < expected_) return Result::Accumulating;

  complete_ = true;
  return Result::Complete;
}

void IDotMatrixFA02Assembler::reset() {
  expected_ = 0;
  received_ = 0;
  complete_ = false;
}
