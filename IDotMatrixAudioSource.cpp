#include "IDotMatrixAudioSource.h"

IDotMatrixAudioSourceMode IDotMatrixAudioSource::normalizeMode(uint8_t value) {
  switch (value) {
    case static_cast<uint8_t>(IDotMatrixAudioSourceMode::AudioReactive):
      return IDotMatrixAudioSourceMode::AudioReactive;
    case static_cast<uint8_t>(IDotMatrixAudioSourceMode::Auto):
      return IDotMatrixAudioSourceMode::Auto;
    case static_cast<uint8_t>(IDotMatrixAudioSourceMode::Phone):
    default:
      return IDotMatrixAudioSourceMode::Phone;
  }
}

const char* IDotMatrixAudioSource::modeText(IDotMatrixAudioSourceMode mode) {
  switch (mode) {
    case IDotMatrixAudioSourceMode::AudioReactive: return "audioreactive";
    case IDotMatrixAudioSourceMode::Auto: return "auto";
    case IDotMatrixAudioSourceMode::Phone:
    default: return "phone";
  }
}

uint8_t IDotMatrixAudioSource::scaleLegacyLevel(float value) {
  if (!(value > 0.0f)) return 0;
  if (value >= 255.0f) return 12;
  const float scaled = value * (12.0f / 255.0f);
  const uint8_t rounded = static_cast<uint8_t>(scaled + 0.5f);
  return rounded > 12 ? 12 : rounded;
}

uint8_t IDotMatrixAudioSource::scaleLegacyBand(uint8_t value) {
  const uint16_t scaled = static_cast<uint16_t>(value) * 12u + 127u;
  const uint8_t result = static_cast<uint8_t>(scaled / 255u);
  return result > 12 ? 12 : result;
}

void IDotMatrixAudioSource::mapAudioReactive(
  float volumeSmoothed,
  const uint8_t fftBands16[16],
  bool fft,
  uint8_t mode,
  IDotMatrixAudioSettings& output
) {
  output = IDotMatrixAudioSettings{};
  output.fft = fft;
  output.mode = mode > 4 ? 4 : mode;
  output.level = scaleLegacyLevel(volumeSmoothed);

  if (fftBands16 == nullptr) return;

  // WLED AudioReactive exports 16 GEQ bands in the 0..255 range while the
  // iDotMatrix protocol/renderers use eight 0..12 bands. Merge adjacent WLED
  // channels so the full frequency range is retained instead of discarding the
  // upper half of the spectrum.
  for (uint8_t i = 0; i < 8; ++i) {
    const uint16_t pair = static_cast<uint16_t>(fftBands16[i * 2]) +
      static_cast<uint16_t>(fftBands16[i * 2 + 1]);
    const uint8_t average = static_cast<uint8_t>((pair + 1u) / 2u);
    output.bands[i] = scaleLegacyBand(average);
  }
}
