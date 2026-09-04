#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
enum SeekMode { SeekSet };
class File {
public:
  File() = default;
  explicit operator bool() const { return true; }
  size_t write(const uint8_t*, size_t n) { return n; }
  size_t read(uint8_t*, size_t) { return 0; }
  size_t size() const { return 0; }
  size_t position() const { return 0; }
  bool seek(uint32_t, SeekMode) { return true; }
  void flush() {}
  void close() {}
};
class TestFS {
public:
  File open(const char*, const char*) { return File(); }
  bool remove(const char*) { return true; }
  bool exists(const char*) { return true; }
  bool rename(const char*, const char*) { return true; }
};
extern TestFS WLED_FS;
inline uint32_t millis() { return 0; }
