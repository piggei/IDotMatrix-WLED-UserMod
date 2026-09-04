#pragma once

#include <cstddef>
#include <cstdint>

class IDotMatrixMediaSink {
public:
  virtual ~IDotMatrixMediaSink() = default;
  virtual bool decodePng(const uint8_t* data, size_t length) = 0;
  virtual bool beginGif(size_t byteLength) = 0;
  virtual bool writeGif(size_t offset, const uint8_t* data, size_t length) = 0;
  virtual bool completeGif(bool crcValid) = 0;
  virtual void stopPlayback() = 0;
};
