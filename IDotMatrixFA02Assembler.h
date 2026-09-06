#pragma once

#include <cstddef>
#include <cstdint>

class IDotMatrixFA02Assembler {
public:
  // BUILD 80 used an 8 KiB logical FA02 packet buffer for alarm/program media.
  // Keep the common 4 KiB bulk path inline, and allocate only the rare larger
  // packet on demand so classic ESP32 builds do not permanently lose another
  // ~4 KiB of DRAM.
  static constexpr size_t INLINE_PACKET_SIZE = 4096 + 16;
  static constexpr size_t MAX_PACKET_SIZE = 8192;

  enum class Result : uint8_t {
    Accumulating,
    Complete,
    Invalid,
    Busy
  };

  IDotMatrixFA02Assembler() = default;
  ~IDotMatrixFA02Assembler();
  IDotMatrixFA02Assembler(const IDotMatrixFA02Assembler&) = delete;
  IDotMatrixFA02Assembler& operator=(const IDotMatrixFA02Assembler&) = delete;

  // Call before entering the BLE queue critical section for the first fragment.
  // This keeps the potentially allocating large-packet path out of a spinlock.
  bool ensureCapacity(uint16_t declaredLength);
  Result append(const uint8_t* data, size_t length);
  void reset();

  bool complete() const { return complete_; }
  const uint8_t* data() const { return activeBuffer(); }
  uint16_t expected() const { return expected_; }
  uint16_t received() const { return received_; }
  bool usesDynamicBuffer() const { return dynamicPacket_ != nullptr; }

private:
  uint8_t* activeBuffer() { return dynamicPacket_ != nullptr ? dynamicPacket_ : inlinePacket_; }
  const uint8_t* activeBuffer() const { return dynamicPacket_ != nullptr ? dynamicPacket_ : inlinePacket_; }
  void releaseDynamic();

  uint8_t inlinePacket_[INLINE_PACKET_SIZE]{};
  uint8_t* dynamicPacket_ = nullptr;
  uint16_t dynamicCapacity_ = 0;
  uint16_t expected_ = 0;
  uint16_t received_ = 0;
  bool complete_ = false;
};
