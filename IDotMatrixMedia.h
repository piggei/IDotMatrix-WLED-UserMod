#pragma once

#include <cstddef>
#include <cstdint>
#include <AnimatedGIF.h>
#include "IDotMatrixMediaSink.h"
#include "IDotMatrixCompactGif.h"
#include "IDotMatrixBuildProfile.h"

#ifndef IDOT_GIF_BITS
#define IDOT_GIF_BITS 10
#endif
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
  bool gifUsesFrameCache() const override { return useFrameCache(); }
  void stopPlayback() override;

  bool gifActive() const { return gifPlaying_; }
  const char* lastErrorText() const;
  static constexpr uint8_t gifDecoderBits() { return IDOT_GIF_BITS; }
  size_t gifDecoderBytes() const;
  const char* gifDecoderModeText() const;
  size_t gifProbeFree() const { return gifProbeFree_; }
  size_t gifProbeLargest() const { return gifProbeLargest_; }
  static constexpr size_t gifDramReserve() { return 10240u; }
  static constexpr size_t gifCacheRuntimeReserve() { return 9216u; }
  static constexpr uint32_t gifCacheLowHeapTimeoutMs() { return 2000u; }
  bool gifCaching() const { return cacheBuilding_; }
  uint32_t gifCachedFrames() const { return cachedFrames_; }
  uint32_t gifCacheWaitCount() const { return cacheLowHeapWaitCount_; }
  size_t gifCacheLowHeapMin() const { return cacheLowHeapMin_; }
  static constexpr uint8_t gifMaxDimension() { return IDOT_GIF_MAX_DIM; }

  enum class Error : uint8_t {
    None,
    GifInvalid,
    GifDecoderOom,
    GifPsramRequired,
    GifRamReserve,
    GifDecoderOpen,
    GifCanvasOom,
    GifCacheIo,
    GifCacheFull
  };
  Error lastError() const { return lastError_; }

private:
  bool promoteGif();
  bool openGif();
  bool openGifForCache();
  bool buildCacheFrame(uint32_t now);
  bool finalizeGifCache(uint32_t now);
  bool playCachedFrame(uint32_t now);
  void resetGifCache();
  void releasePlaybackResources();
  bool useFrameCache() const;
  bool inspectGifFile(const char* path);
  bool ensureDecoderStorage();
  void destroyDecoder(bool releaseStorage = false);
  static void* openFile(const char* name, int32_t* size);
  static void closeFile(void* handle);
  static int32_t readFile(GIFFILE* file, uint8_t* buffer, int32_t length);
  static int32_t seekFile(GIFFILE* file, int32_t position);
  static int32_t compactRead(void* context, uint8_t* buffer, size_t length);
  static bool compactSeek(void* context, uint32_t position);
  static void drawGif(GIFDRAW* draw);

  IDotMatrixRenderer& renderer_;
  AnimatedGIF* decoder_ = nullptr;
  IDotMatrixCompactGif compactDecoder_{};
  bool compactCache_ = false;
  void* decoderStorage_ = nullptr;
  int8_t rxSlot_ = -1;
  int8_t pendingSlot_ = -1;
  uint8_t nextRxSlot_ = 0;
  size_t gifExpected_ = 0;
  size_t gifWritten_ = 0;
  bool rxOpen_ = false;
  bool promotePending_ = false;
  bool openPending_ = false;
  uint8_t openDelayLoops_ = 0;
  bool cacheBuilding_ = false;
  bool cachePlayback_ = false;
  bool gifPlaying_ = false;
  bool restartPending_ = false;
  bool frameDrawn_ = false;
  uint32_t nextFrameAt_ = 0;
  size_t pendingGifBytes_ = 0;
  size_t cacheFrameBytes_ = 0;
  size_t cacheBytes_ = 0;
  uint32_t cachedFrames_ = 0;
  uint32_t cacheFrameIndex_ = 0;
  bool cacheLowHeapWaiting_ = false;
  uint32_t cacheLowHeapSince_ = 0;
  uint32_t cacheLowHeapWaitCount_ = 0;
  size_t cacheLowHeapMin_ = 0;
  Error lastError_ = Error::None;
  size_t gifProbeFree_ = 0;
  size_t gifProbeLargest_ = 0;
};
