#pragma once

#include <cstddef>
#include <cstdint>

class IDotMatrixFA02Assembler {
public:
  static constexpr size_t MAX_PACKET_SIZE = 4096 + 16;

  enum class Result : uint8_t {
    Accumulating,
    Complete,
    Invalid,
    Busy
  };

  Result append(const uint8_t* data, size_t length);
  void reset();

  bool complete() const { return complete_; }
  const uint8_t* data() const { return packet_; }
  uint16_t expected() const { return expected_; }
  uint16_t received() const { return received_; }

private:
  uint8_t packet_[MAX_PACKET_SIZE]{};
  uint16_t expected_ = 0;
  uint16_t received_ = 0;
  bool complete_ = false;
};
