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
time_t localTime = 0;
uint32_t testMillis = 0;
uint8_t testHour = 23;
uint8_t testMinute = 45;
uint8_t testDay = 2;
uint8_t testMonth = 9;
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
  assert(adapter.registerDisplayEffect());
  assert(adapter.displayEffectId() == 200);
  assert(strip.registeredEffectCount() == 1);

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
  assert(adapter.isDisplayEffectActive());
  assert(effectCurrent == 200);

  const uint8_t coordinates[] = {1, 2, 15, 14, 16, 0, 9};
  adapter.onGraffitiPixels(0x21, 0x43, 0x65, coordinates, sizeof(coordinates));
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
  assert(!adapter.isDisplayEffectActive());
  assert(colorUpdateCount == 2);

  // A valid pixel packet is sufficient to reclaim the effect even if an app
  // version does not send the DIY-state command first.
  const uint8_t directPixel[] = {3, 4};
  adapter.onGraffitiPixels(7, 8, 9, directPixel, sizeof(directPixel));
  assert(adapter.isDiySessionActive());
  assert(adapter.isDisplayEffectActive());
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(3, 4) == RGBW32(7, 8, 9, 0));

  IDotMatrixClockSettings clock;
  clock.style = 3;
  clock.use24Hour = true;
  clock.red = 10;
  clock.green = 20;
  clock.blue = 30;
  adapter.onClock(clock);
  assert(adapter.isClockActive());
  assert(adapter.isDisplayEffectActive());
  assert(strip.segmentRef().mode == adapter.displayEffectId());
  assert(!adapter.isDiySessionActive());
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(0, 0) == RGBW32(10, 20, 30, 0));
  assert(strip.segmentRef().colorAt(6, 2) == BLACK);

  clock.showDate = true;
  testMillis = 0;
  adapter.onClock(clock);
  testMillis = 30000;
  strip.renderEffect();
  // In style 3 the date slash is black over the selected background.
  assert(strip.segmentRef().colorAt(3, 9) == BLACK);

  IDotMatrixTextSettings text;
  text.glyphCount = 1;
  text.glyphWidth = 8;
  text.glyphHeight = 16;
  text.glyphBytes = 16;
  text.red = 11;
  text.green = 22;
  text.blue = 33;
  assert(adapter.onTextBegin(text));
  uint8_t glyph[16]{};
  glyph[0] = 0x01;
  adapter.onTextGlyph(0, glyph, sizeof(glyph));
  adapter.onTextComplete();
  assert(adapter.isTextActive());
  assert(!adapter.isClockActive());
  assert(adapter.isDisplayEffectActive());
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(0, 0) == RGBW32(11, 22, 33, 0));

  uint8_t rawImage[16 * 16 * 3]{};
  rawImage[0] = 41;
  rawImage[1] = 42;
  rawImage[2] = 43;
  const size_t lastPixel = sizeof(rawImage) - 3;
  rawImage[lastPixel] = 91;
  rawImage[lastPixel + 1] = 92;
  rawImage[lastPixel + 2] = 93;
  assert(adapter.onRawImageBegin(sizeof(rawImage)));
  assert(adapter.onRawImageData(0, rawImage, 400));
  assert(adapter.onRawImageData(400, rawImage + 400, sizeof(rawImage) - 400));
  assert(adapter.onRawImageComplete(true));
  assert(adapter.isRawImageActive());
  assert(!adapter.isTextActive());
  assert(!adapter.isClockActive());
  assert(adapter.isDisplayEffectActive());
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(0, 0) == RGBW32(41, 42, 43, 0));
  assert(strip.segmentRef().colorAt(15, 15) == RGBW32(91, 92, 93, 0));

  // A profile/segment mismatch is black in strict mode and nearest-neighbour
  // sampled only when rescale is explicitly enabled.
  assert(renderer.begin(0x04));
  const uint8_t scaledPixel[] = {4, 8};
  adapter.onGraffitiPixels(90, 80, 70, scaledPixel, sizeof(scaledPixel));
  strip.renderEffect();
  assert(!adapter.dimensionsMatch());
  assert(strip.segmentRef().colorAt(1, 2) == BLACK);
  adapter.setRescaleEnabled(true);
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(1, 2) == RGBW32(90, 80, 70, 0));
}
