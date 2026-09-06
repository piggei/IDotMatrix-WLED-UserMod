#include "../IDotMatrixBuildProfile.h"

#include <cassert>

#ifndef EXPECTED_SCREEN_MAX_DIM
#error EXPECTED_SCREEN_MAX_DIM is required
#endif

#ifndef EXPECTED_DECODER_MAX_DIM
#error EXPECTED_DECODER_MAX_DIM is required
#endif

int main() {
  using namespace IDotMatrixBuildProfile;
  static_assert(maxDimension() == EXPECTED_SCREEN_MAX_DIM, "unexpected screen profile");
  static_assert(decoderMaxDimension() == EXPECTED_DECODER_MAX_DIM, "unexpected decoder profile");
  assert(supportsScreenType(0x01));
  assert(supportsScreenType(0x03) == (EXPECTED_SCREEN_MAX_DIM >= 32));
  assert(supportsScreenType(0x04) == (EXPECTED_SCREEN_MAX_DIM >= 64));
  assert(supportsRescale() == (EXPECTED_SCREEN_MAX_DIM > 16));
  assert(normalizeScreenType(0xFF) == 0x01);
  assert(normalizeScreenType(0x03) == (EXPECTED_SCREEN_MAX_DIM >= 32 ? 0x03 : 0x01));
  assert(normalizeScreenType(0x04) ==
    (EXPECTED_SCREEN_MAX_DIM >= 64 ? 0x04 : EXPECTED_SCREEN_MAX_DIM >= 32 ? 0x03 : 0x01));
}
