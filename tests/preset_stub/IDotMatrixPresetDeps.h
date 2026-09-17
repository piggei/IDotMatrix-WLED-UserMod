#pragma once

#include <cstddef>
#include <cstdint>

class IDotMatrixWLEDAdapter {
public:
  void beginTransferIndicator(size_t, uint32_t, uint8_t, uint8_t) { ++beginTransferCount; }
  void updateTransferIndicator(size_t, size_t) { ++updateTransferCount; }
  void endTransferIndicator() { ++endTransferCount; }
  void beginCarouselPlayback() { ++beginPlaybackCount; }
  bool playStoredGif(const char*, const char* = nullptr) { ++playStoredGifCount; return playStoredGifOk; }
  uint32_t textPresentationDurationMs() const { return 3000u; }
  bool isGifPending() const { return false; }
  bool isGifActive() const { return true; }

  bool playStoredGifOk = true;
  uint32_t beginTransferCount = 0;
  uint32_t updateTransferCount = 0;
  uint32_t endTransferCount = 0;
  uint32_t beginPlaybackCount = 0;
  uint32_t playStoredGifCount = 0;
};
