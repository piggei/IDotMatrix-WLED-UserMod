#pragma once

#include <cstddef>
#include <cstdint>

class IDotMatrixRenderer {};
class IDotMatrixMedia {};

class IDotMatrixBuzzer {
public:
  void startScheduleAlert(uint32_t) { playing_ = true; }
  void startTrill(uint32_t) { playing_ = true; }
  void stop() { playing_ = false; }
  bool isPlaying() const { return playing_; }
private:
  bool playing_ = false;
};

class IDotMatrixWLEDAdapter {
public:
  uint8_t displayEffectId() const { return 200; }
  bool isGifPending() const { return false; }
  void cancelAutomationContent() {}
  void restoreClockFallback() {}
  bool onRawImageBegin(size_t) { return true; }
  bool onRawImageData(size_t, const uint8_t*, size_t) { return true; }
  bool onRawImageComplete(bool ok) { return ok; }
  bool onGifBegin(size_t) { return true; }
  bool onGifData(size_t, const uint8_t*, size_t) { return true; }
  bool onGifComplete(bool ok) { return ok; }
  bool onPngImage(const uint8_t*, size_t) { return true; }
};
