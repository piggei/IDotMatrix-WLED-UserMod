#pragma once
#include <cstddef>
#include <cstdint>
class Preferences {
public:
  bool begin(const char*, bool) { return true; }
  void end() {}
  size_t getBytesLength(const char*) const { return 0; }
  size_t getBytes(const char*, void*, size_t) { return 0; }
  size_t putBytes(const char*, const void*, size_t n) { return n; }
  uint8_t getUChar(const char*, uint8_t v=0) const { return v; }
  size_t putUChar(const char*, uint8_t) { return 1; }
  bool remove(const char*) { return true; }
};
