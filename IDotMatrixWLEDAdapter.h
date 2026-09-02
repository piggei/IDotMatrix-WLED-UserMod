#pragma once

#include "IDotMatrixProtocol.h"

class IDotMatrixWLEDAdapter final : public IDotMatrixProtocolEvents {
public:
  void onScreenPower(bool on) override;
  void onBrightnessPercent(uint8_t percent) override;
  void onSolidColor(uint8_t red, uint8_t green, uint8_t blue) override;

private:
  bool screenOn_ = false;
};
