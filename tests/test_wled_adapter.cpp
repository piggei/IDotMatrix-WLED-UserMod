#include "../IDotMatrixWLEDAdapter.h"

#include <cassert>

#include "wled.h"

uint8_t bri = 128;
uint8_t briLast = 128;
uint32_t stateUpdateCount = 0;
uint8_t effectCurrent = 42;
uint8_t colPri[4] = {0, 0, 0, 255};
uint32_t colorUpdateCount = 0;
TestStrip strip;

void toggleOnOff() {
  if (bri == 0) {
    bri = briLast;
    strip.restartRuntime();
  } else {
    briLast = bri;
    bri = 0;
  }
}

void stateUpdated(uint8_t callMode) {
  assert(callMode == CALL_MODE_DIRECT_CHANGE);
  ++stateUpdateCount;
}

void colorUpdated(uint8_t callMode) {
  assert(callMode == CALL_MODE_DIRECT_CHANGE);
  ++colorUpdateCount;
}

int main() {
  IDotMatrixWLEDAdapter adapter;

  adapter.onScreenPower(true);
  assert(bri == 128 && briLast == 128);

  adapter.onBrightnessPercent(100);
  assert(bri == 255 && briLast == 255);

  adapter.onScreenPower(false);
  assert(bri == 0 && briLast == 255);

  adapter.onBrightnessPercent(25);
  assert(bri == 0 && briLast == 64);

  adapter.onScreenPower(true);
  assert(bri == 64 && briLast == 64);

  adapter.onBrightnessPercent(0);
  assert(bri == 0 && briLast == 64);

  adapter.onBrightnessPercent(50);
  assert(bri == 128 && briLast == 128);
  assert(strip.restartCount == 2);
  assert(stateUpdateCount == 6);

  adapter.onSolidColor(0x12, 0x34, 0x56);
  assert(effectCurrent == FX_MODE_STATIC);
  assert(colPri[0] == 0x12 && colPri[1] == 0x34 && colPri[2] == 0x56);
  assert(colPri[3] == 0);
  assert(colorUpdateCount == 1);
}
