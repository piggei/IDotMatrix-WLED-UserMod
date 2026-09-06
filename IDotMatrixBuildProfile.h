#pragma once

#include <cstdint>

#ifndef IDOT_GIF_MAX_DIM
#define IDOT_GIF_MAX_DIM 16
#endif

// The decoder workspace and the screen choices exposed by the WLED settings
// page are intentionally independent.  A 16x16 hardware/profile build can
// therefore use the stable LZW12 decoder while still exposing only 16x16 and
// hiding Rescale.
#ifndef IDOT_SCREEN_MAX_DIM
#define IDOT_SCREEN_MAX_DIM IDOT_GIF_MAX_DIM
#endif

static_assert(
  IDOT_GIF_MAX_DIM == 16 || IDOT_GIF_MAX_DIM == 32 || IDOT_GIF_MAX_DIM == 64,
  "IDOT_GIF_MAX_DIM must be 16, 32 or 64"
);
static_assert(
  IDOT_SCREEN_MAX_DIM == 16 || IDOT_SCREEN_MAX_DIM == 32 || IDOT_SCREEN_MAX_DIM == 64,
  "IDOT_SCREEN_MAX_DIM must be 16, 32 or 64"
);
static_assert(
  IDOT_SCREEN_MAX_DIM <= IDOT_GIF_MAX_DIM,
  "IDOT_SCREEN_MAX_DIM cannot exceed the compiled GIF decoder dimension"
);

namespace IDotMatrixBuildProfile {

constexpr uint8_t maxDimension() { return IDOT_SCREEN_MAX_DIM; }
constexpr uint8_t decoderMaxDimension() { return IDOT_GIF_MAX_DIM; }
constexpr bool supportsRescale() { return IDOT_SCREEN_MAX_DIM > 16; }

constexpr bool supportsScreenType(uint8_t screenType) {
  return screenType == 0x01 ||
    (screenType == 0x03 && IDOT_SCREEN_MAX_DIM >= 32) ||
    (screenType == 0x04 && IDOT_SCREEN_MAX_DIM >= 64);
}

// Preserve the nearest supported profile when an older configuration was
// written by a larger build. Unknown values still use the 16x16 baseline.
constexpr uint8_t normalizeScreenType(uint8_t screenType) {
  return screenType == 0x04
    ? (IDOT_SCREEN_MAX_DIM >= 64 ? 0x04 : IDOT_SCREEN_MAX_DIM >= 32 ? 0x03 : 0x01)
    : screenType == 0x03
      ? (IDOT_SCREEN_MAX_DIM >= 32 ? 0x03 : 0x01)
      : 0x01;
}

}
