#pragma once
#include <cstddef>
#include <zlib.h>
struct tinfl_decompressor { int unused; };
enum tinfl_status { TINFL_STATUS_FAILED = -1, TINFL_STATUS_DONE = 0 };
#define TINFL_FLAG_PARSE_ZLIB_HEADER 1
#define TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF 2
inline void tinfl_init(tinfl_decompressor*) {}
inline tinfl_status tinfl_decompress(tinfl_decompressor*, const unsigned char* input,
  size_t* inputBytes, unsigned char*, unsigned char* output, size_t* outputBytes, int) {
  uLongf decoded = static_cast<uLongf>(*outputBytes);
  const int result = uncompress(
    output, &decoded, input, static_cast<uLong>(*inputBytes)
  );
  *outputBytes = static_cast<size_t>(decoded);
  return result == Z_OK ? TINFL_STATUS_DONE : TINFL_STATUS_FAILED;
}
