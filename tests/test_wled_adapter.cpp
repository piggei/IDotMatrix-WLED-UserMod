#include "../IDotMatrixWLEDAdapter.h"

#include <cassert>

#include "wled.h"

uint8_t bri = 128;
uint8_t briLast = 128;
uint32_t stateUpdateCount = 0;
uint8_t effectCurrent = 42;
uint8_t colPri[4] = {0, 0, 0, 255};
uint32_t colorUpdateCount = 0;
uint32_t stripTriggerCount = 0;
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
  IDotMatrixRenderer renderer;
  assert(renderer.begin(0x01));
  IDotMatrixWLEDAdapter adapter(renderer);
  assert(adapter.registerFramebufferEffect());
  assert(adapter.framebufferEffectId() == 200);

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

  adapter.onGraffitiMode(true);
  assert(adapter.isDiySessionActive());
  assert(renderer.isVisible());
  assert(stripTriggerCount == 1);
  assert(adapter.isFramebufferEffectActive());
  assert(effectCurrent == 200);

  const uint8_t coordinates[] = {1, 2, 15, 14, 16, 0, 9};
  adapter.onGraffitiPixels(0x21, 0x43, 0x65, coordinates, sizeof(coordinates));
  assert(renderer.acceptedPixelUpdates() == 2);
  assert(stripTriggerCount == 2);

  strip.renderEffect();
  assert(strip.segmentRef().colorAt(1, 2) == RGBW32(0x21, 0x43, 0x65, 0));
  assert(strip.segmentRef().colorAt(15, 14) == RGBW32(0x21, 0x43, 0x65, 0));
  assert(strip.segmentRef().colorAt(0, 0) == 0);

  // Leaving the editor preserves the last canvas, matching the reference.
  adapter.onGraffitiMode(false);
  assert(!adapter.isDiySessionActive());
  assert(renderer.isVisible());

  // A new content mode releases framebuffer ownership.
  adapter.onSolidColor(1, 2, 3);
  assert(!renderer.isVisible());
  assert(!adapter.isFramebufferEffectActive());
  assert(colorUpdateCount == 2);

  // A valid pixel packet is sufficient to reclaim the effect even if an app
  // version does not send the DIY-state command first.
  const uint8_t directPixel[] = {3, 4};
  adapter.onGraffitiPixels(7, 8, 9, directPixel, sizeof(directPixel));
  assert(adapter.isDiySessionActive());
  assert(adapter.isFramebufferEffectActive());
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(3, 4) == RGBW32(7, 8, 9, 0));
}
