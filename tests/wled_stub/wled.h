#pragma once

#include <cstdint>

extern uint8_t bri;
extern uint8_t briLast;
extern uint32_t stateUpdateCount;
extern uint8_t effectCurrent;
extern uint8_t colPri[4];
extern uint32_t colorUpdateCount;

constexpr uint8_t CALL_MODE_DIRECT_CHANGE = 1;
constexpr uint8_t FX_MODE_STATIC = 0;

class TestStrip {
public:
  void restartRuntime() { ++restartCount; }
  uint32_t restartCount = 0;
};

extern TestStrip strip;

void toggleOnOff();
void stateUpdated(uint8_t callMode);
void colorUpdated(uint8_t callMode);
