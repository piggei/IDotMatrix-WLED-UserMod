#include "../IDotMatrixBLEFraming.h"
#include <cassert>
#include <cstdint>

int main() {
  const uint8_t level[] = {0x06,0x00,0x00,0x02,0x05,0x01};
  const uint8_t fft[] = {0x21,0x00,0x01,0x02,0,0,0,0};
  const uint8_t reset[] = {0x05,0x00,0x03,0x80,0x00};
  const uint8_t configure[] = {0x08,0x00,0x02,0x01,0,0,0,0};
  const uint8_t enter[] = {0x05,0x00,0x0A,0x01,0x00};
  const uint8_t presetActivate[] = {0x07,0x00,0x06,0x02,0x02,0x0E,0x0F};
  const uint8_t random[] = {0x05,0x00,0x7F,0x7F,0x00};
  assert(idotStartsAudioFrame(level, sizeof(level)));
  assert(idotStartsAudioFrame(fft, sizeof(fft)));
  assert(!idotStartsKnownNonAudioFrame(level, sizeof(level)));
  assert(idotStartsKnownNonAudioFrame(reset, sizeof(reset)));
  assert(idotStartsKnownNonAudioFrame(configure, sizeof(configure)));
  assert(idotStartsKnownNonAudioFrame(enter, sizeof(enter)));
  assert(idotStartsKnownNonAudioFrame(presetActivate, sizeof(presetActivate)));
  assert(!idotStartsKnownNonAudioFrame(random, sizeof(random)));
  return 0;
}
