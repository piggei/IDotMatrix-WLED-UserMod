#include "../IDotMatrixBuildProfile.h"

#include <cassert>

#ifndef EXPECTED_MAX_DIM
#error EXPECTED_MAX_DIM is required
#endif

int main() {
  using namespace IDotMatrixBuildProfile;
  static_assert(maxDimension() == EXPECTED_MAX_DIM, "unexpected build profile");
  assert(supportsScreenType(0x01));
  assert(supportsScreenType(0x03) == (EXPECTED_MAX_DIM >= 32));
  assert(supportsScreenType(0x04) == (EXPECTED_MAX_DIM >= 64));
  assert(supportsRescale() == (EXPECTED_MAX_DIM > 16));
  assert(normalizeScreenType(0xFF) == 0x01);
  assert(normalizeScreenType(0x03) == (EXPECTED_MAX_DIM >= 32 ? 0x03 : 0x01));
  assert(normalizeScreenType(0x04) ==
    (EXPECTED_MAX_DIM >= 64 ? 0x04 : EXPECTED_MAX_DIM >= 32 ? 0x03 : 0x01));
}
