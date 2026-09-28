#pragma once

#include <cstddef>
#include <cstdint>

extern uint32_t testMillis;

class IDotMatrixRenderer {};
class IDotMatrixMedia {};

class IDotMatrixBuzzer {
public:
  void startScheduleAlert(uint32_t now) { playing_ = true; lastScheduleStart_ = now; }
  void startTrill(uint32_t now) { playing_ = true; lastTrillStart_ = now; }
  void stop() { playing_ = false; }
  bool isPlaying() const { return playing_; }

  uint32_t lastScheduleStart_ = 0;
  uint32_t lastTrillStart_ = 0;
private:
  bool playing_ = false;
};

class IDotMatrixWLEDAdapter {
public:
  uint32_t mediaLoadDelayMs = 0;

  uint8_t displayEffectId() const { return 200; }
  bool isGifPending() const { return false; }
  void cancelAutomationContent() {}
  void restoreClockFallback() {}
  bool onRawImageBegin(size_t) { return true; }
  bool onRawImageData(size_t, const uint8_t*, size_t) { return true; }
  bool onRawImageComplete(bool ok) { testMillis += mediaLoadDelayMs; return ok; }
  bool onGifBegin(size_t) { return true; }
  bool onGifData(size_t, const uint8_t*, size_t) { return true; }
  bool onGifComplete(bool ok) { testMillis += mediaLoadDelayMs; return ok; }
  bool onPngImage(const uint8_t*, size_t) { testMillis += mediaLoadDelayMs; return true; }
};
