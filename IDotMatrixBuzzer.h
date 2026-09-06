#pragma once

#include <cstdint>

// Small non-blocking pattern engine for the optional iDotMatrix active buzzer.
// Hardware ownership/polarity are handled by the WLED usermod; this class only
// emits logical ON/OFF transitions.  Keeping the pattern engine independent
// from GPIO makes it straightforward to add a passive/PWM backend later.
class IDotMatrixBuzzer {
public:
  using OutputCallback = void (*)(void* context, bool on);

  void attach(OutputCallback callback, void* context) {
    outputCallback_ = callback;
    outputContext_ = context;
  }

  void startTrill(uint32_t now);
  void startTest(uint32_t now);
  void startScheduleAlert(uint32_t now);
  void stop();
  void loop(uint32_t now);

  bool isPlaying() const { return patternRunning_; }
  bool outputOn() const { return outputOn_; }

private:
  void setOutput(bool on);
  void startPattern(uint32_t now, uint8_t groups);

  static constexpr uint32_t PULSE_ON_MS = 90u;
  static constexpr uint32_t PULSE_GAP_MS = 70u;
  static constexpr uint32_t TRILL_PAUSE_MS = 550u;
  static constexpr uint8_t TRILL_PULSES = 3u;

  OutputCallback outputCallback_ = nullptr;
  void* outputContext_ = nullptr;
  bool outputOn_ = false;
  bool patternRunning_ = false;
  uint8_t pulseIndex_ = 0;
  uint32_t nextChangeAt_ = 0;
  // 0 = repeat indefinitely, otherwise number of three-pulse groups left.
  uint8_t groupsRemaining_ = 0;
};
