#pragma once

#include <cstddef>
#include <cstdint>
#include <ctime>

extern uint8_t bri;
extern uint8_t briLast;
extern uint32_t stateUpdateCount;
extern uint8_t effectCurrent;
extern uint8_t colPri[4];
extern uint32_t colorUpdateCount;
extern uint32_t stripTriggerCount;
extern time_t localTime;
extern uint32_t testMillis;
extern uint8_t testHour;
extern uint8_t testMinute;
extern uint8_t testDay;
extern uint8_t testMonth;

#define PROGMEM

constexpr uint8_t CALL_MODE_DIRECT_CHANGE = 1;
constexpr uint8_t FX_MODE_STATIC = 0;
constexpr uint32_t BLACK = 0;

constexpr uint32_t RGBW32(uint8_t red, uint8_t green, uint8_t blue, uint8_t white) {
  return (uint32_t(white) << 24) | (uint32_t(red) << 16) |
    (uint32_t(green) << 8) | blue;
}

class Segment {
public:
  bool is2D() const { return true; }
  Segment& setMode(uint8_t effect, bool = false) {
    mode = effect;
    return *this;
  }

  void fill(uint32_t color) {
    for (auto& pixel : colors) pixel = color;
  }

  void setPixelColorXY(uint16_t x, uint16_t y, uint32_t color) {
    if (x < 64 && y < 64) colors[size_t(y) * 64 + x] = color;
  }

  uint32_t colorAt(uint16_t x, uint16_t y) const {
    return (x < 64 && y < 64) ? colors[size_t(y) * 64 + x] : 0;
  }

  uint8_t mode = FX_MODE_STATIC;
  uint16_t width = 16;
  uint16_t height = 16;

private:
  uint32_t colors[64 * 64]{};
};

class TestStrip {
public:
  using ModeFunction = void (*)();

  void restartRuntime() { ++restartCount; }
  void trigger() { ++stripTriggerCount; }
  uint8_t addEffect(uint8_t, ModeFunction function, const char*) {
    effectFunctions[effectCount] = function;
    return framebufferEffectId + effectCount++;
  }
  Segment& getFirstSelectedSeg() { return segment; }
  uint32_t restartCount = 0;
  Segment& segmentRef() { return segment; }
  void renderEffect() {
    const uint8_t index = segment.mode - framebufferEffectId;
    if (index < effectCount && effectFunctions[index]) effectFunctions[index]();
  }
  uint8_t registeredEffectCount() const { return effectCount; }

  bool isMatrix = true;
  uint8_t framebufferEffectId = 200;

private:
  Segment segment;
  ModeFunction effectFunctions[4]{};
  uint8_t effectCount = 0;
};

extern TestStrip strip;

#define SEGMENT (strip.segmentRef())
#define SEG_W (strip.segmentRef().width)
#define SEG_H (strip.segmentRef().height)

inline uint32_t millis() { return testMillis; }
inline uint8_t hour(time_t) { return testHour; }
inline uint8_t minute(time_t) { return testMinute; }
inline uint8_t day(time_t) { return testDay; }
inline uint8_t month(time_t) { return testMonth; }

void toggleOnOff();
void stateUpdated(uint8_t callMode);
void colorUpdated(uint8_t callMode);
