#include "IDotMatrixBuzzer.h"

void IDotMatrixBuzzer::setOutput(bool on) {
  if (outputOn_ == on) return;
  outputOn_ = on;
  if (outputCallback_ != nullptr) outputCallback_(outputContext_, on);
}

void IDotMatrixBuzzer::startPattern(uint32_t now, uint8_t groups) {
  patternRunning_ = true;
  groupsRemaining_ = groups;
  pulseIndex_ = 0;
  setOutput(true);
  nextChangeAt_ = now + PULSE_ON_MS;
}

void IDotMatrixBuzzer::startTrill(uint32_t now) {
  // 0 means repeat the three-pulse trill until stop() is called.
  startPattern(now, 0);
}

void IDotMatrixBuzzer::startTest(uint32_t now) {
  // Configuration-page wiring test: one group of three pulses.
  startPattern(now, 1);
}

void IDotMatrixBuzzer::startScheduleAlert(uint32_t now) {
  // Program activation alert: three groups of three pulses, then silence.
  startPattern(now, 3);
}

void IDotMatrixBuzzer::stop() {
  patternRunning_ = false;
  groupsRemaining_ = 0;
  pulseIndex_ = 0;
  nextChangeAt_ = 0;
  setOutput(false);
}

void IDotMatrixBuzzer::loop(uint32_t now) {
  if (!patternRunning_ || int32_t(now - nextChangeAt_) < 0) return;

  if (outputOn_) {
    setOutput(false);
    ++pulseIndex_;

    if (pulseIndex_ >= TRILL_PULSES) {
      if (groupsRemaining_ == 1) {
        patternRunning_ = false;
        groupsRemaining_ = 0;
        pulseIndex_ = 0;
        nextChangeAt_ = 0;
        return;
      }
      if (groupsRemaining_ > 1) --groupsRemaining_;
      nextChangeAt_ = now + TRILL_PAUSE_MS;
    } else {
      nextChangeAt_ = now + PULSE_GAP_MS;
    }
    return;
  }

  if (pulseIndex_ >= TRILL_PULSES) pulseIndex_ = 0;
  setOutput(true);
  nextChangeAt_ = now + PULSE_ON_MS;
}
