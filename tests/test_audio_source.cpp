#include "../IDotMatrixAudioSource.h"

#include <cassert>
#include <cstdint>

int main() {
  assert(IDotMatrixAudioSource::normalizeMode(0) == IDotMatrixAudioSourceMode::Phone);
  assert(IDotMatrixAudioSource::normalizeMode(1) == IDotMatrixAudioSourceMode::AudioReactive);
  assert(IDotMatrixAudioSource::normalizeMode(2) == IDotMatrixAudioSourceMode::Auto);
  assert(IDotMatrixAudioSource::normalizeMode(99) == IDotMatrixAudioSourceMode::Phone);

  assert(IDotMatrixAudioSource::scaleLegacyLevel(-1.0f) == 0);
  assert(IDotMatrixAudioSource::scaleLegacyLevel(0.0f) == 0);
  assert(IDotMatrixAudioSource::scaleLegacyLevel(255.0f) == 12);
  assert(IDotMatrixAudioSource::scaleLegacyLevel(1000.0f) == 12);
  assert(IDotMatrixAudioSource::scaleLegacyBand(0) == 0);
  assert(IDotMatrixAudioSource::scaleLegacyBand(255) == 12);

  uint8_t fft[16]{};
  for (uint8_t i = 0; i < 16; ++i) fft[i] = static_cast<uint8_t>(i * 17u);

  IDotMatrixAudioSettings settings;
  IDotMatrixAudioSource::mapAudioReactive(127.5f, fft, true, 3, settings);
  assert(settings.fft);
  assert(settings.mode == 3);
  assert(settings.level == 6);
  assert(settings.bands[0] == IDotMatrixAudioSource::scaleLegacyBand(9));
  assert(settings.bands[7] == IDotMatrixAudioSource::scaleLegacyBand(247));

  IDotMatrixAudioSource::mapAudioReactive(255.0f, nullptr, false, 9, settings);
  assert(!settings.fft);
  assert(settings.mode == 4);
  assert(settings.level == 12);
  for (uint8_t value : settings.bands) assert(value == 0);

  return 0;
}
