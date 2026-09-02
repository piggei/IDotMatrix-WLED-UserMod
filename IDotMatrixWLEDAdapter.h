#pragma once

#include "IDotMatrixProtocol.h"
#include "IDotMatrixRenderer.h"

class IDotMatrixWLEDAdapter final : public IDotMatrixProtocolEvents {
public:
  explicit IDotMatrixWLEDAdapter(IDotMatrixRenderer& renderer) : renderer_(renderer) {}

  bool registerFramebufferEffect();

  void onScreenPower(bool on) override;
  void onBrightnessPercent(uint8_t percent) override;
  void onSolidColor(uint8_t red, uint8_t green, uint8_t blue) override;
  void onGraffitiMode(bool enter) override;
  void onGraffitiPixels(
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    const uint8_t* coordinates,
    size_t coordinateBytes
  ) override;

  void renderEffectFrame();
  bool isDiySessionActive() const { return diySessionActive_; }
  bool isFramebufferEffectRegistered() const { return framebufferEffectId_ != 0xFF; }
  bool isFramebufferEffectActive() const;
  uint8_t framebufferEffectId() const { return framebufferEffectId_; }
  uint32_t renderedFrames() const { return renderedFrames_; }
  uint16_t targetWidth() const { return targetWidth_; }
  uint16_t targetHeight() const { return targetHeight_; }

private:
  void activateFramebufferEffect();

  IDotMatrixRenderer& renderer_;
  bool screenOn_ = false;
  bool diySessionActive_ = false;
  uint8_t framebufferEffectId_ = 0xFF;
  uint32_t renderedFrames_ = 0;
  uint16_t targetWidth_ = 0;
  uint16_t targetHeight_ = 0;
};
