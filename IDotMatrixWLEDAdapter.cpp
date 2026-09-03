#include "IDotMatrixWLEDAdapter.h"

#include "wled.h"

namespace {
IDotMatrixWLEDAdapter* activeAdapter = nullptr;

const char DISPLAY_EFFECT_DATA[] PROGMEM =
  "iDotMatrix Display@;;;2";

void modeIDotMatrixDisplay() {
  if (activeAdapter != nullptr) {
    activeAdapter->renderDisplayEffectFrame();
  } else {
    SEGMENT.fill(BLACK);
  }
}
}

bool IDotMatrixWLEDAdapter::registerDisplayEffect() {
  activeAdapter = this;
  displayEffectId_ = strip.addEffect(
    0xFF,
    &modeIDotMatrixDisplay,
    DISPLAY_EFFECT_DATA
  );
  return displayEffectId_ != 0xFF;
}

bool IDotMatrixWLEDAdapter::isDisplayEffectActive() const {
  if (!isDisplayEffectRegistered()) return false;
  return strip.getFirstSelectedSeg().mode == displayEffectId_;
}

void IDotMatrixWLEDAdapter::activateDisplayEffect() {
  if (!isDisplayEffectRegistered()) return;

  auto& segment = strip.getFirstSelectedSeg();
  if (segment.mode != displayEffectId_) {
    segment.setMode(displayEffectId_);
    effectCurrent = displayEffectId_;
    stateUpdated(CALL_MODE_DIRECT_CHANGE);
  }
  strip.trigger();
}

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
  // A solid-colour command selects a new display mode in the reference and
  // therefore releases ownership of the framebuffer effect.
  diySessionActive_ = false;
  clockActive_ = false;
  textActive_ = false;
  renderer_.setVisible(false);

  auto& segment = strip.getFirstSelectedSeg();
  if (segment.mode != FX_MODE_STATIC) segment.setMode(FX_MODE_STATIC);

  // Follow WLED's normal global-colour path. It applies the static effect and
  // primary colour to all active, selected segments and updates every interface.
  effectCurrent = FX_MODE_STATIC;
  colPri[0] = red;
  colPri[1] = green;
  colPri[2] = blue;
  colPri[3] = 0;
  colorUpdated(CALL_MODE_DIRECT_CHANGE);
}

void IDotMatrixWLEDAdapter::onGraffitiMode(bool enter) {
  // The reference clears only when entering a new DIY session. Leaving DIY
  // does not blank the display; the last canvas remains visible until another
  // content command takes ownership.
  if (enter && !diySessionActive_) {
    renderer_.clear();
    renderer_.setVisible(true);
  }
  diySessionActive_ = enter;
  if (enter) {
    clockActive_ = false;
    textActive_ = false;
  }
  if (enter) activateDisplayEffect();
}

void IDotMatrixWLEDAdapter::onGraffitiPixels(
  uint8_t red,
  uint8_t green,
  uint8_t blue,
  const uint8_t* coordinates,
  size_t coordinateBytes
) {
  if (!renderer_.isReady() || coordinates == nullptr) return;

  renderer_.setVisible(true);
  for (size_t offset = 0; offset + 1 < coordinateBytes; offset += 2) {
    renderer_.setPixel(
      coordinates[offset],
      coordinates[offset + 1],
      red,
      green,
      blue
    );
  }
  // Some app versions may send pixel data without a preceding DIY-state
  // command. A valid pixel packet therefore also takes ownership of the
  // selected WLED segment.
  diySessionActive_ = true;
  clockActive_ = false;
  textActive_ = false;
  activateDisplayEffect();
}

void IDotMatrixWLEDAdapter::onClock(const IDotMatrixClockSettings& settings) {
  diySessionActive_ = false;
  clockActive_ = true;
  textActive_ = false;
  clockSettings_ = settings;
  clockCycleStartedAt_ = millis();
  renderer_.setVisible(true);
  activateDisplayEffect();
}

bool IDotMatrixWLEDAdapter::onTextBegin(const IDotMatrixTextSettings& settings) {
  textLoadReady_ = renderer_.beginText(
    settings.glyphCount,
    settings.glyphWidth,
    settings.glyphHeight,
    settings.glyphBytes,
    settings.motionEffect,
    settings.speed,
    settings.colorMode,
    settings.red,
    settings.green,
    settings.blue,
    settings.backgroundEnabled,
    settings.backgroundRed,
    settings.backgroundGreen,
    settings.backgroundBlue,
    millis()
  );
  return textLoadReady_;
}

void IDotMatrixWLEDAdapter::onTextGlyph(
  uint8_t index,
  const uint8_t* bitmap,
  size_t bitmapLength
) {
  if (!textLoadReady_) return;
  if (!renderer_.setTextGlyph(index, bitmap, bitmapLength)) textLoadReady_ = false;
}

void IDotMatrixWLEDAdapter::onTextComplete() {
  if (!textLoadReady_) return;
  diySessionActive_ = false;
  clockActive_ = false;
  textActive_ = true;
  renderer_.setVisible(true);
  activateDisplayEffect();
}

void IDotMatrixWLEDAdapter::renderDisplayEffectFrame() {
  ++renderedFrames_;
  if (clockActive_) {
    const uint32_t elapsed = millis() - clockCycleStartedAt_;
    const bool renderDate = clockSettings_.showDate && (elapsed % 35000u) >= 30000u;
    renderer_.renderClock(
      static_cast<uint8_t>(hour(localTime)),
      static_cast<uint8_t>(minute(localTime)),
      static_cast<uint8_t>(day(localTime)),
      static_cast<uint8_t>(month(localTime)),
      clockSettings_.style,
      clockSettings_.use24Hour,
      renderDate,
      clockSettings_.red,
      clockSettings_.green,
      clockSettings_.blue,
      millis()
    );
  } else if (textActive_) {
    renderer_.renderText(millis());
  }
  renderCanvasToSegment();
}

void IDotMatrixWLEDAdapter::renderCanvasToSegment() {
  targetWidth_ = SEG_W;
  targetHeight_ = SEG_H;
  dimensionsMatch_ = renderer_.width() == targetWidth_ &&
    renderer_.height() == targetHeight_;

  if (!renderer_.isReady() || !renderer_.isVisible() ||
      targetWidth_ == 0 || targetHeight_ == 0 ||
      !strip.isMatrix || !SEGMENT.is2D()) {
    SEGMENT.fill(BLACK);
    return;
  }

  // The callback runs inside WLED's normal effect service, where SEGMENT and
  // its virtual XY dimensions are valid. WLED therefore remains responsible
  // for serpentine wiring, panel layout, rotation, mirroring and grouping.
  SEGMENT.fill(BLACK);
  const IDotMatrixRenderer::Pixel* pixels = renderer_.pixels();

  if (!dimensionsMatch_ && !rescaleEnabled_) return;

  if (rescaleEnabled_) {
    for (uint16_t y = 0; y < targetHeight_; ++y) {
      const uint16_t sourceY = uint32_t(y) * renderer_.height() / targetHeight_;
      for (uint16_t x = 0; x < targetWidth_; ++x) {
        const uint16_t sourceX = uint32_t(x) * renderer_.width() / targetWidth_;
        const IDotMatrixRenderer::Pixel& pixel =
          pixels[size_t(sourceY) * renderer_.width() + sourceX];
        SEGMENT.setPixelColorXY(x, y, RGBW32(pixel.red, pixel.green, pixel.blue, 0));
      }
    }
    return;
  }

  const uint16_t drawWidth = renderer_.width() < SEG_W ? renderer_.width() : SEG_W;
  const uint16_t drawHeight = renderer_.height() < SEG_H ? renderer_.height() : SEG_H;
  for (uint16_t y = 0; y < drawHeight; ++y) {
    for (uint16_t x = 0; x < drawWidth; ++x) {
      const IDotMatrixRenderer::Pixel& pixel = pixels[size_t(y) * renderer_.width() + x];
      SEGMENT.setPixelColorXY(x, y, RGBW32(pixel.red, pixel.green, pixel.blue, 0));
    }
  }
}
