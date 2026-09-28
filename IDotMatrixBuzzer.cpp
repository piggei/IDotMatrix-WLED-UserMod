#include "IDotMatrixBuzzer.h"

bool IDotMatrixBuzzer::beginThreadSafe() {
#if defined(ARDUINO_ARCH_ESP32)
  if (mutex_ != nullptr) return true;
  mutex_ = xSemaphoreCreateMutex();
  return mutex_ != nullptr;
#else
  return true;
#endif
}

bool IDotMatrixBuzzer::lock(bool wait) const {
#if defined(ARDUINO_ARCH_ESP32)
  if (mutex_ == nullptr) return true;
  return xSemaphoreTake(mutex_, wait ? portMAX_DELAY : 0) == pdTRUE;
#else
  (void)wait;
  return true;
#endif
}

void IDotMatrixBuzzer::unlock() const {
#if defined(ARDUINO_ARCH_ESP32)
  if (mutex_ != nullptr) xSemaphoreGive(mutex_);
#endif
}

bool IDotMatrixBuzzer::isPlaying() const {
  if (!lock(true)) return false;
  const bool value = patternRunning_;
  unlock();
  return value;
}

bool IDotMatrixBuzzer::outputOn() const {
  if (!lock(true)) return false;
  const bool value = outputOn_;
  unlock();
  return value;
}

uint32_t IDotMatrixBuzzer::lastLatenessMs() const {
  if (!lock(true)) return 0;
  const uint32_t value = lastLatenessMs_;
  unlock();
  return value;
}

uint32_t IDotMatrixBuzzer::maxLatenessMs() const {
  if (!lock(true)) return 0;
  const uint32_t value = maxLatenessMs_;
  unlock();
  return value;
}

void IDotMatrixBuzzer::setOutput(bool on) {
  if (outputOn_ == on) return;
  outputOn_ = on;
  if (outputCallback_ != nullptr) outputCallback_(outputContext_, on);
}

void IDotMatrixBuzzer::startPatternLocked(uint32_t now, uint8_t groups) {
  patternRunning_ = true;
  groupsRemaining_ = groups;
  pulseIndex_ = 0;
  lastLatenessMs_ = 0;
  maxLatenessMs_ = 0;
  setOutput(true);
  nextChangeAt_ = now + PULSE_ON_MS;
}

void IDotMatrixBuzzer::scheduleNextLocked(uint32_t now, uint32_t intervalMs) {
  // Preserve the original phase when service arrives slightly late so WLED/GIF
  // loop jitter does not accumulate from edge to edge. If an entire following
  // interval was already missed, rebase instead of emitting compressed edges.
  const uint32_t phaseNext = nextChangeAt_ + intervalMs;
  if (int32_t(now - phaseNext) >= 0) nextChangeAt_ = now + intervalMs;
  else nextChangeAt_ = phaseNext;
}

void IDotMatrixBuzzer::startTrill(uint32_t now) {
  if (!lock(true)) return;
  // 0 means repeat the three-pulse trill until stop() is called.
  startPatternLocked(now, 0);
  unlock();
}

void IDotMatrixBuzzer::startTest(uint32_t now) {
  if (!lock(true)) return;
  // Configuration-page wiring test: one group of three pulses.
  startPatternLocked(now, 1);
  unlock();
}

void IDotMatrixBuzzer::startCountdownAlert(uint32_t now) {
  if (!lock(true)) return;
  // Natural Countdown completion: one group of three pulses.
  startPatternLocked(now, 1);
  unlock();
}

void IDotMatrixBuzzer::startScheduleAlert(uint32_t now) {
  if (!lock(true)) return;
  // Program activation alert: three groups of three pulses, then silence.
  startPatternLocked(now, 3);
  unlock();
}

void IDotMatrixBuzzer::startConnectionBeep(uint32_t now) {
  if (!lock(true)) return;
  // Lowest-priority BLE connection notice: exactly one short pulse.
  patternRunning_ = true;
  groupsRemaining_ = 1;
  pulseIndex_ = TRILL_PULSES - 1;
  lastLatenessMs_ = 0;
  maxLatenessMs_ = 0;
  setOutput(true);
  nextChangeAt_ = now + PULSE_ON_MS;
  unlock();
}

void IDotMatrixBuzzer::stop() {
  if (!lock(true)) return;
  patternRunning_ = false;
  groupsRemaining_ = 0;
  pulseIndex_ = 0;
  nextChangeAt_ = 0;
  setOutput(false);
  unlock();
}

void IDotMatrixBuzzer::loop(uint32_t now) {
  // esp_timer callbacks must never block. If WLED is issuing a command at the
  // same instant, simply retry at the next 2 ms service tick.
  if (!lock(false)) return;
  if (!patternRunning_ || int32_t(now - nextChangeAt_) < 0) {
    unlock();
    return;
  }

  lastLatenessMs_ = uint32_t(now - nextChangeAt_);
  if (lastLatenessMs_ > maxLatenessMs_) maxLatenessMs_ = lastLatenessMs_;

  if (outputOn_) {
    setOutput(false);
    ++pulseIndex_;

    if (pulseIndex_ >= TRILL_PULSES) {
      if (groupsRemaining_ == 1) {
        patternRunning_ = false;
        groupsRemaining_ = 0;
        pulseIndex_ = 0;
        nextChangeAt_ = 0;
        unlock();
        return;
      }
      if (groupsRemaining_ > 1) --groupsRemaining_;
      scheduleNextLocked(now, TRILL_PAUSE_MS);
    } else {
      scheduleNextLocked(now, PULSE_GAP_MS);
    }
    unlock();
    return;
  }

  if (pulseIndex_ >= TRILL_PULSES) pulseIndex_ = 0;
  setOutput(true);
  scheduleNextLocked(now, PULSE_ON_MS);
  unlock();
}
