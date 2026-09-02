#include "IDotMatrixWLEDAdapter.h"

#include "wled.h"

void IDotMatrixWLEDAdapter::onScreenPower(bool on) {
  screenOn_ = on;
  if ((bri > 0) == on) return;

  // WLED's own toggle preserves briLast while switching off and restores it
  // when switching back on.
  toggleOnOff();
  stateUpdated(CALL_MODE_DIRECT_CHANGE);
}

void IDotMatrixWLEDAdapter::onBrightnessPercent(uint8_t percent) {
  if (percent > 100) percent = 100;
  const uint8_t target = static_cast<uint8_t>(
    (static_cast<uint16_t>(percent) * 255u + 50u) / 100u
  );

  if (!screenOn_) {
    // Preserve WLED's OFF state, but remember the requested level for the next ON.
    if (target > 0 && briLast != target) {
      briLast = target;
      stateUpdated(CALL_MODE_DIRECT_CHANGE);
    }
    return;
  }

  if (bri == target) return;

  if (bri == 0 && target > 0) strip.restartRuntime();
  bri = target;
  // Keep the last usable ON level. A protocol value of 0 produces black while
  // retaining a non-zero level that WLED can restore after an OFF/ON cycle.
  if (target > 0) briLast = target;
  stateUpdated(CALL_MODE_DIRECT_CHANGE);
}

void IDotMatrixWLEDAdapter::onSolidColor(
  uint8_t red,
  uint8_t green,
  uint8_t blue
) {
  // Follow WLED's normal global-colour path. It applies the static effect and
  // primary colour to all active, selected segments and updates every interface.
  effectCurrent = FX_MODE_STATIC;
  colPri[0] = red;
  colPri[1] = green;
  colPri[2] = blue;
  colPri[3] = 0;
  colorUpdated(CALL_MODE_DIRECT_CHANGE);
}
