#include "IDotMatrixFA02Assembler.h"

#include <cstdlib>
#include <cstring>

IDotMatrixFA02Assembler::~IDotMatrixFA02Assembler() {
  releaseDynamic();
}

void IDotMatrixFA02Assembler::releaseDynamic() {
  if (dynamicPacket_ != nullptr) {
    free(dynamicPacket_);
    dynamicPacket_ = nullptr;
  }
  dynamicCapacity_ = 0;
}

bool IDotMatrixFA02Assembler::ensureCapacity(uint16_t declaredLength) {
  if (declaredLength == 0 || declaredLength > MAX_PACKET_SIZE) return false;
  if (declaredLength <= INLINE_PACKET_SIZE) {
    // A stale large allocation can only exist before a new packet after reset.
    if (expected_ == 0) releaseDynamic();
    return true;
  }
  if (dynamicPacket_ != nullptr && dynamicCapacity_ >= declaredLength) return true;
  if (expected_ != 0 || received_ != 0 || complete_) return false;

  releaseDynamic();
  dynamicPacket_ = static_cast<uint8_t*>(malloc(declaredLength));
  if (dynamicPacket_ == nullptr) return false;
  dynamicCapacity_ = declaredLength;
  return true;
}

IDotMatrixFA02Assembler::Result IDotMatrixFA02Assembler::append(
  const uint8_t* data,
  size_t length
) {
  if (complete_) return Result::Busy;
  if (data == nullptr || length == 0) return Result::Invalid;

  if (expected_ == 0) {
    if (length < 2) return Result::Invalid;
    expected_ = uint16_t(data[0]) | (uint16_t(data[1]) << 8);
    if (expected_ == 0 || expected_ > MAX_PACKET_SIZE ||
        (expected_ > INLINE_PACKET_SIZE &&
         (dynamicPacket_ == nullptr || dynamicCapacity_ < expected_))) {
      reset();
      return Result::Invalid;
    }
  }

  const size_t remaining = expected_ - received_;
  const size_t copyLength = length < remaining ? length : remaining;
  memcpy(activeBuffer() + received_, data, copyLength);
  received_ += uint16_t(copyLength);
  if (received_ < expected_) return Result::Accumulating;

  complete_ = true;
  return Result::Complete;
}

void IDotMatrixFA02Assembler::reset() {
  expected_ = 0;
  received_ = 0;
  complete_ = false;
  releaseDynamic();
}
