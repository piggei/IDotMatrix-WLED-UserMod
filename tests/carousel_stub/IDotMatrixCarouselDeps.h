#pragma once
#include <cstddef>
#include <cstdint>
class IDotMatrixWLEDAdapter {
public:
  void releaseCarouselMediaForStorageMutation() { ++releaseCount; }
  void beginCarouselUpdateHold() { ++holdBeginCount; }
  void endCarouselUpdateHold() { ++holdEndCount; }
  void endTransferIndicator() { ++endTransferCount; }
  void beginCarouselPlayback() { ++playbackBeginCount; }
  void beginTransferIndicator(size_t, uint32_t, uint8_t, uint8_t) { ++beginTransferCount; }
  void updateTransferIndicator(size_t, size_t) { ++updateTransferCount; }
  bool playStoredGif(const char*, const char* = nullptr) { ++playGifCount; return playGifOk; }
  bool isGifPending() const { return false; }
  bool isGifActive() const { return true; }
  void restoreClockFallback() { ++fallbackCount; }
  bool playGifOk = true;
  uint32_t releaseCount = 0, holdBeginCount = 0, holdEndCount = 0, endTransferCount = 0;
  uint32_t playbackBeginCount = 0, beginTransferCount = 0, updateTransferCount = 0, playGifCount = 0, fallbackCount = 0;
};
