#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
using std::size_t;
inline uint32_t millis() { return 0; }
inline size_t strlcpy(char* dst, const char* src, size_t size) {
  if (!dst || size == 0) return src ? std::strlen(src) : 0;
  const size_t n = src ? std::strlen(src) : 0;
  const size_t copy = n < size - 1 ? n : size - 1;
  if (src && copy) std::memcpy(dst, src, copy);
  dst[copy] = '\0';
  return n;
}
using portMUX_TYPE = int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) do { (void)(x); } while (0)
#define portEXIT_CRITICAL(x) do { (void)(x); } while (0)
