#pragma once

#include <cstddef>
#include <cstdint>
#include <AnimatedGIF.h>
#include "IDotMatrixMediaSink.h"

class IDotMatrixRenderer;

class IDotMatrixMedia final : public IDotMatrixMediaSink {
public:
  explicit IDotMatrixMedia(IDotMatrixRenderer& renderer);
  ~IDotMatrixMedia();

  IDotMatrixMedia(const IDotMatrixMedia&) = delete;
  IDotMatrixMedia& operator=(const IDotMatrixMedia&) = delete;

  void loop(uint32_t now);
  bool decodePng(const uint8_t* data, size_t length) override;
  bool beginGif(size_t byteLength) override;
  bool writeGif(size_t offset, const uint8_t* data, size_t length) override;
  bool completeGif(bool crcValid) override;
  void cancelGifReceive();
  void stopPlayback() override;

  bool gifActive() const { return gifPlaying_; }

private:
  bool promoteGif();
  bool openGif();
  bool inspectGifFile(const char* path);
  void destroyDecoder();
  static void* openFile(const char* name, int32_t* size);
  static void closeFile(void* handle);
  static int32_t readFile(GIFFILE* file, uint8_t* buffer, int32_t length);
  static int32_t seekFile(GIFFILE* file, int32_t position);
  static void drawGif(GIFDRAW* draw);

  IDotMatrixRenderer& renderer_;
  AnimatedGIF* decoder_ = nullptr;
  alignas(AnimatedGIF) uint8_t decoderStorage_[sizeof(AnimatedGIF)]{};
  int8_t rxSlot_ = -1;
  int8_t pendingSlot_ = -1;
  uint8_t nextRxSlot_ = 0;
  size_t gifExpected_ = 0;
  size_t gifWritten_ = 0;
  bool rxOpen_ = false;
  bool promotePending_ = false;
  bool openPending_ = false;
  bool gifPlaying_ = false;
  bool restartPending_ = false;
  bool frameDrawn_ = false;
  uint32_t nextFrameAt_ = 0;
  size_t pendingGifBytes_ = 0;
};
