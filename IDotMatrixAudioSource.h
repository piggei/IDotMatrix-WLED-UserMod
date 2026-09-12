#pragma once

#include "IDotMatrixProtocol.h"

#include <cstdint>

enum class IDotMatrixAudioSourceMode : uint8_t {
  Phone = 0,
  AudioReactive = 1,
  Auto = 2,
};

class IDotMatrixAudioSource {
public:
  static IDotMatrixAudioSourceMode normalizeMode(uint8_t value);
  static const char* modeText(IDotMatrixAudioSourceMode mode);
  static uint8_t scaleLegacyLevel(float value);
  static uint8_t scaleLegacyBand(uint8_t value);
  static void mapAudioReactive(
    float volumeSmoothed,
    const uint8_t fftBands16[16],
    bool fft,
    uint8_t mode,
    IDotMatrixAudioSettings& output
  );
};
