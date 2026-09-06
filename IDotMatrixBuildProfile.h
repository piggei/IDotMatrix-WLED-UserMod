#pragma once

#include <cstdint>

#ifndef IDOT_GIF_MAX_DIM
#define IDOT_GIF_MAX_DIM 16
#endif

static_assert(
  IDOT_GIF_MAX_DIM == 16 || IDOT_GIF_MAX_DIM == 32 || IDOT_GIF_MAX_DIM == 64,
  "IDOT_GIF_MAX_DIM must be 16, 32 or 64"
);

namespace IDotMatrixBuildProfile {

constexpr uint8_t maxDimension() { return IDOT_GIF_MAX_DIM; }
constexpr bool supportsRescale() { return IDOT_GIF_MAX_DIM > 16; }

constexpr bool supportsScreenType(uint8_t screenType) {
  return screenType == 0x01 ||
    (screenType == 0x03 && IDOT_GIF_MAX_DIM >= 32) ||
    (screenType == 0x04 && IDOT_GIF_MAX_DIM >= 64);
}

// Preserve the nearest supported profile when an older configuration was
// written by a larger build. Unknown values still use the 16x16 baseline.
constexpr uint8_t normalizeScreenType(uint8_t screenType) {
  return screenType == 0x04
    ? (IDOT_GIF_MAX_DIM >= 64 ? 0x04 : IDOT_GIF_MAX_DIM >= 32 ? 0x03 : 0x01)
    : screenType == 0x03
      ? (IDOT_GIF_MAX_DIM >= 32 ? 0x03 : 0x01)
      : 0x01;
}

}
