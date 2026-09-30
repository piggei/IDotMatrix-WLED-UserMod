#include "IDotMatrixBuzzerBridge.h"
#include <cassert>
#include <cstring>

namespace {
bool ready = true;
bool playing = false;
const char* current = nullptr;
}

extern "C" bool wledBuzzerServiceReady() { return ready; }
extern "C" bool wledBuzzerServicePlaying() { return playing; }
extern "C" bool wledBuzzerServicePlay(const char* soundId, bool) {
  if (!ready || soundId == nullptr) return false;
  current = soundId;
  playing = true;
  return true;
}
extern "C" void wledBuzzerServiceStop() {
  playing = false;
  current = nullptr;
}
extern "C" const char* wledBuzzerServiceCurrentSoundId() { return current; }

int main() {
  assert(IDotMatrixBuzzerBridge::installed());
  assert(IDotMatrixBuzzerBridge::ready());
  assert(!IDotMatrixBuzzerBridge::playing());
  assert(IDotMatrixBuzzerBridge::play("disconnect"));
  assert(IDotMatrixBuzzerBridge::playing());
  assert(std::strcmp(IDotMatrixBuzzerBridge::currentSoundId(), "disconnect") == 0);
  IDotMatrixBuzzerBridge::stop();
  assert(!IDotMatrixBuzzerBridge::playing());
  return 0;
}
