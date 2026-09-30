#pragma once

#include <cstdint>

#if defined(__GNUC__)
#define IDOTMATRIX_WEAK_SYMBOL __attribute__((weak))
#else
#define IDOTMATRIX_WEAK_SYMBOL
#endif

// Optional C ABI exported by the standalone WLED Buzzer Usermod.
// Weak references keep iDotMatrix fully linkable when that usermod is absent.
extern "C" {
bool wledBuzzerServiceReady() IDOTMATRIX_WEAK_SYMBOL;
bool wledBuzzerServicePlaying() IDOTMATRIX_WEAK_SYMBOL;
bool wledBuzzerServicePlay(const char* soundId, bool loop) IDOTMATRIX_WEAK_SYMBOL;
void wledBuzzerServiceStop() IDOTMATRIX_WEAK_SYMBOL;
const char* wledBuzzerServiceCurrentSoundId() IDOTMATRIX_WEAK_SYMBOL;
}

namespace IDotMatrixBuzzerBridge {

inline bool installed() {
  return wledBuzzerServiceReady != nullptr &&
    wledBuzzerServicePlaying != nullptr &&
    wledBuzzerServicePlay != nullptr &&
    wledBuzzerServiceStop != nullptr &&
    wledBuzzerServiceCurrentSoundId != nullptr;
}

inline bool ready() {
  return installed() && wledBuzzerServiceReady();
}

inline bool playing() {
  return installed() && wledBuzzerServicePlaying();
}

inline bool play(const char* soundId, bool loop = false) {
  return installed() && wledBuzzerServicePlay(soundId, loop);
}

inline void stop() {
  if (installed()) wledBuzzerServiceStop();
}

inline const char* currentSoundId() {
  return installed() ? wledBuzzerServiceCurrentSoundId() : nullptr;
}

} // namespace IDotMatrixBuzzerBridge

#undef IDOTMATRIX_WEAK_SYMBOL
