#include "IDotMatrixBuzzerBridge.h"
#include <cassert>

int main() {
  assert(!IDotMatrixBuzzerBridge::installed());
  assert(!IDotMatrixBuzzerBridge::ready());
  assert(!IDotMatrixBuzzerBridge::playing());
  assert(!IDotMatrixBuzzerBridge::play("connect"));
  assert(IDotMatrixBuzzerBridge::currentSoundId() == nullptr);
  IDotMatrixBuzzerBridge::stop();
  return 0;
}
