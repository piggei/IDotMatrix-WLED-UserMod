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

void IDotMatrixWLEDAdapter::loop(uint32_t now) {
  // Countdown/stopwatch state belongs to the emulated device, not to the
  // currently selected WLED effect. Keep time moving even if the user has
  // temporarily switched the panel back to native WLED content.
  if (countdownRunning_) {
    const uint32_t elapsed = now - countdownStartMillis_;
    if (elapsed >= countdownRemainingMs_) {
      countdownRemainingMs_ = 0;
      countdownRunning_ = false;
      countdownPaused_ = false;
      countdownFinishPending_ = true;
      if (countdownActive_ && isDisplayEffectActive()) renderCountdown(now, true);
    }
  }
}

bool IDotMatrixWLEDAdapter::isDisplayEffectActive() const {
  if (!isDisplayEffectRegistered()) return false;
  return strip.getFirstSelectedSeg().mode == displayEffectId_;
}

void IDotMatrixWLEDAdapter::syncWLEDControl() {
  // Frame-cache preparation intentionally happens while a low-RAM WLED static
  // effect is selected.  Do not interpret that staging state as the user
  // taking ownership away from iDotMatrix.
  if (gifPending_ && gifPrecache_ && strip.getFirstSelectedSeg().mode == FX_MODE_STATIC) return;

  // iDotMatrix owns the framebuffer only while its dedicated WLED effect is
  // selected. A user/API can switch the segment to another WLED effect without
  // sending a BLE media command. In that case release all media state
  // immediately so ordinary WLED effects regain the RAM they need.
  if (isDisplayEffectActive()) return;

  const bool hadIDotContent = solidActive_ || lightEffectActive_ || audioActive_ || diySessionActive_ ||
    clockActive_ || countdownActive_ || stopwatchActive_ || scoreboardActive_ ||
    textActive_ || rawImageActive_ || gifActive_ || gifPending_ || gifStaging_ ||
    renderer_.isVisible();
  if (!hadIDotContent) return;

  clearContentState();
}

void IDotMatrixWLEDAdapter::clearContentState() {
  solidActive_ = false;
  lightEffectActive_ = false;
  audioActive_ = false;
  diySessionActive_ = false;
  clockActive_ = false;
  countdownActive_ = false;
  stopwatchActive_ = false;
  scoreboardActive_ = false;
  textActive_ = false;
  rawImageActive_ = false;
  gifActive_ = false;
  gifPending_ = false;
  gifStaging_ = false;
  gifPrecache_ = false;
  gifReplacingActiveGif_ = false;
  gifPreviousRendererVisible_ = false;
  textLoadReady_ = false;
  stopMediaPlayback();
  renderer_.setVisible(false);
}

void IDotMatrixWLEDAdapter::cancelAutomationContent() {
  clearContentState();
}

void IDotMatrixWLEDAdapter::restoreClockFallback() {
  onClock(clockSettings_);
}

void IDotMatrixWLEDAdapter::beginGifBlankStaging() {
  if (gifBlankStaging_) return;

  auto& segment = strip.getFirstSelectedSeg();
  gifStagingPrimaryColor_ = segment.colors[0];
  gifBlankStaging_ = true;

  // Frame-cache preparation still uses WLED Static because it has the lowest
  // runtime footprint on classic ESP32.  Temporarily make only the selected
  // segment's primary colour black so the staging phase is visually blank
  // without changing WLED's global brightness/power state or rewriting the
  // global primary-colour setting. The original segment colour is restored
  // before staging ends.
  segment.colors[0] = BLACK;
  segment.fill(BLACK);
}

void IDotMatrixWLEDAdapter::endGifBlankStaging() {
  if (!gifBlankStaging_) return;

  auto& segment = strip.getFirstSelectedSeg();
  segment.colors[0] = gifStagingPrimaryColor_;
  gifBlankStaging_ = false;
}

void IDotMatrixWLEDAdapter::stopMediaPlayback() {
  endGifBlankStaging();
  if (media_ != nullptr) media_->stopPlayback();
}

void IDotMatrixWLEDAdapter::activateDisplayEffect() {
  if (!isDisplayEffectRegistered()) return;

  // Any transition back to the framebuffer effect ends the temporary black
  // Static staging state.  Restore the user's WLED primary colour first; the
  // display effect itself ignores it, but later WLED effects must see the
  // exact colour that was active before the GIF transfer.
  endGifBlankStaging();

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
  // App-originated solid colour is iDotMatrix content, not WLED's own Static
  // effect.  Keeping it on the dedicated framebuffer effect prevents the WLED
  // UI and the one-way iDotMatrix app from appearing to share/synchronise a
  // colour state that cannot actually be round-tripped.
  solidActive_ = true;
  lightEffectActive_ = false;
  audioActive_ = false;
  diySessionActive_ = false;
  clockActive_ = false;
  countdownActive_ = false;
  stopwatchActive_ = false;
  scoreboardActive_ = false;
  textActive_ = false;
  rawImageActive_ = false;
  gifActive_ = false;
  gifPending_ = false;
  gifPrecache_ = false;
  gifStaging_ = false;
  gifReplacingActiveGif_ = false;
  gifPreviousRendererVisible_ = false;
  stopMediaPlayback();
  renderer_.fill(red, green, blue);
  renderer_.setVisible(true);
  activateDisplayEffect();
}

void IDotMatrixWLEDAdapter::onLightEffect(
  const IDotMatrixLightEffectSettings& settings
) {
  IDotMatrixRenderer::Pixel colors[IDotMatrixLightEffectSettings::MAX_COLORS]{};
  for (uint8_t i = 0; i < settings.colorCount; ++i) {
    colors[i] = IDotMatrixRenderer::Pixel{
      settings.colors[i].red,
      settings.colors[i].green,
      settings.colors[i].blue
    };
  }

  if (!renderer_.beginLightEffect(
        settings.effect,
        settings.speed,
        settings.colorCount,
        colors,
        millis())) return;

  solidActive_ = false;
  lightEffectActive_ = true;
  audioActive_ = false;
  diySessionActive_ = false;
  clockActive_ = false;
  countdownActive_ = false;
  stopwatchActive_ = false;
  scoreboardActive_ = false;
  textActive_ = false;
  rawImageActive_ = false;
  gifActive_ = false;
  gifPending_ = false;
  gifPrecache_ = false;
  gifStaging_ = false;
  gifReplacingActiveGif_ = false;
  gifPreviousRendererVisible_ = false;
  stopMediaPlayback();
  renderer_.setVisible(true);
  activateDisplayEffect();
}

void IDotMatrixWLEDAdapter::onAudio(const IDotMatrixAudioSettings& settings) {
  solidActive_ = false;
  lightEffectActive_ = false;
  audioActive_ = true;
  diySessionActive_ = false;
  clockActive_ = false;
  countdownActive_ = false;
  stopwatchActive_ = false;
  scoreboardActive_ = false;
  textActive_ = false;
  rawImageActive_ = false;
  gifActive_ = false;
  gifPending_ = false;
  gifPrecache_ = false;
  gifStaging_ = false;
  gifReplacingActiveGif_ = false;
  gifPreviousRendererVisible_ = false;
  stopMediaPlayback();
  audioSettings_ = settings;
  audioLastRenderMillis_ = millis();
  renderer_.renderAudio(settings.fft, settings.mode, settings.level, settings.bands,
                        audioLastRenderMillis_);
  renderer_.setVisible(true);
  activateDisplayEffect();
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
    solidActive_ = false;
    lightEffectActive_ = false;
    audioActive_ = false;
    clockActive_ = false;
    countdownActive_ = false;
    stopwatchActive_ = false;
    scoreboardActive_ = false;
    textActive_ = false;
    rawImageActive_ = false;
    gifActive_ = false;
  gifPending_ = false;
    stopMediaPlayback();
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
  solidActive_ = false;
  lightEffectActive_ = false;
  audioActive_ = false;
  diySessionActive_ = true;
  clockActive_ = false;
  countdownActive_ = false;
  stopwatchActive_ = false;
  scoreboardActive_ = false;
  textActive_ = false;
  rawImageActive_ = false;
  gifActive_ = false;
  gifPending_ = false;
  gifPrecache_ = false;
  gifStaging_ = false;
  stopMediaPlayback();
  activateDisplayEffect();
}

void IDotMatrixWLEDAdapter::onClock(const IDotMatrixClockSettings& settings) {
  solidActive_ = false;
  lightEffectActive_ = false;
  audioActive_ = false;
  diySessionActive_ = false;
  clockActive_ = true;
  countdownActive_ = false;
  stopwatchActive_ = false;
  scoreboardActive_ = false;
  textActive_ = false;
  rawImageActive_ = false;
  gifActive_ = false;
  gifPending_ = false;
  gifPrecache_ = false;
  gifStaging_ = false;
  stopMediaPlayback();
  clockSettings_ = settings;
  clockCycleStartedAt_ = millis();
  renderer_.setVisible(true);
  activateDisplayEffect();
}

void IDotMatrixWLEDAdapter::onCountdown(
  const IDotMatrixCountdownSettings& settings
) {
  const uint32_t now = millis();
  const uint32_t requestedMs =
    (uint32_t(settings.minutes) * 60u + uint32_t(settings.seconds)) * 1000u;

  switch (settings.mode) {
    case 0:
      countdownRunning_ = false;
      countdownPaused_ = false;
      countdownRemainingMs_ = 0;
      countdownFinishPending_ = false;
      break;
    case 1:
      countdownRemainingMs_ = requestedMs;
      countdownStartMillis_ = now;
      countdownRunning_ = true;
      countdownPaused_ = false;
      countdownFinishPending_ = false;
      break;
    case 2:
      if (countdownRunning_) {
        const uint32_t elapsed = now - countdownStartMillis_;
        countdownRemainingMs_ = elapsed < countdownRemainingMs_
          ? countdownRemainingMs_ - elapsed
          : 0;
      }
      countdownRunning_ = false;
      countdownPaused_ = true;
      break;
    case 3:
      if (countdownRemainingMs_ != 0) {
        countdownStartMillis_ = now;
        countdownRunning_ = true;
        countdownPaused_ = false;
      }
      break;
    default:
      break;
  }

  solidActive_ = false;
  lightEffectActive_ = false;
  audioActive_ = false;
  diySessionActive_ = false;
  clockActive_ = false;
  countdownActive_ = true;
  stopwatchActive_ = false;
  scoreboardActive_ = false;
  textActive_ = false;
  rawImageActive_ = false;
  gifActive_ = false;
  gifPending_ = false;
  gifPrecache_ = false;
  gifStaging_ = false;
  gifReplacingActiveGif_ = false;
  gifPreviousRendererVisible_ = false;
  stopMediaPlayback();
  renderer_.setVisible(true);
  countdownLastRenderMillis_ = now;
  renderCountdown(now, true);
  activateDisplayEffect();
}

void IDotMatrixWLEDAdapter::onStopwatch(uint8_t mode) {
  const uint32_t now = millis();
  switch (mode) {
    case 0:
      stopwatchRunning_ = false;
      stopwatchElapsedMs_ = 0;
      break;
    case 1:
      stopwatchElapsedMs_ = 0;
      stopwatchStartMillis_ = now;
      stopwatchRunning_ = true;
      break;
    case 2:
      if (stopwatchRunning_) stopwatchElapsedMs_ += now - stopwatchStartMillis_;
      stopwatchRunning_ = false;
      break;
    case 3:
      if (!stopwatchRunning_) {
        stopwatchStartMillis_ = now;
        stopwatchRunning_ = true;
      }
      break;
    default:
      break;
  }

  solidActive_ = false;
  lightEffectActive_ = false;
  audioActive_ = false;
  diySessionActive_ = false;
  clockActive_ = false;
  countdownActive_ = false;
  stopwatchActive_ = true;
  scoreboardActive_ = false;
  textActive_ = false;
  rawImageActive_ = false;
  gifActive_ = false;
  gifPending_ = false;
  gifPrecache_ = false;
  gifStaging_ = false;
  gifReplacingActiveGif_ = false;
  gifPreviousRendererVisible_ = false;
  stopMediaPlayback();
  renderer_.setVisible(true);
  stopwatchLastRenderMillis_ = now;
  renderStopwatch(now, true);
  activateDisplayEffect();
}

void IDotMatrixWLEDAdapter::onScoreboard(uint16_t scoreA, uint16_t scoreB) {
  scoreA_ = scoreA;
  scoreB_ = scoreB;

  solidActive_ = false;
  lightEffectActive_ = false;
  audioActive_ = false;
  diySessionActive_ = false;
  clockActive_ = false;
  countdownActive_ = false;
  stopwatchActive_ = false;
  scoreboardActive_ = true;
  textActive_ = false;
  rawImageActive_ = false;
  gifActive_ = false;
  gifPending_ = false;
  gifPrecache_ = false;
  gifStaging_ = false;
  gifReplacingActiveGif_ = false;
  gifPreviousRendererVisible_ = false;
  stopMediaPlayback();
  renderer_.renderScoreboard(scoreA_, scoreB_);
  renderer_.setVisible(true);
  activateDisplayEffect();
}

bool IDotMatrixWLEDAdapter::takeCountdownFinished() {
  if (!countdownFinishPending_) return false;
  countdownFinishPending_ = false;
  return true;
}

uint32_t IDotMatrixWLEDAdapter::countdownRemainingMillis(uint32_t now) const {
  uint32_t remaining = countdownRemainingMs_;
  if (countdownRunning_) {
    const uint32_t elapsed = now - countdownStartMillis_;
    remaining = elapsed < remaining ? remaining - elapsed : 0;
  }
  return remaining;
}

uint32_t IDotMatrixWLEDAdapter::countdownRemainingSeconds(uint32_t now) const {
  return (countdownRemainingMillis(now) + 999u) / 1000u;
}

uint32_t IDotMatrixWLEDAdapter::stopwatchElapsedMillis(uint32_t now) const {
  uint32_t elapsed = stopwatchElapsedMs_;
  if (stopwatchRunning_) elapsed += now - stopwatchStartMillis_;
  return elapsed;
}

uint32_t IDotMatrixWLEDAdapter::stopwatchElapsedSeconds(uint32_t now) const {
  return stopwatchElapsedMillis(now) / 1000u;
}

void IDotMatrixWLEDAdapter::renderCountdown(uint32_t now, bool force) {
  if (!countdownActive_) return;
  if (!force && uint32_t(now - countdownLastRenderMillis_) < 200u) return;
  countdownLastRenderMillis_ = now;
  renderer_.renderCountdown(countdownRemainingMillis(now));
}

void IDotMatrixWLEDAdapter::renderStopwatch(uint32_t now, bool force) {
  if (!stopwatchActive_) return;
  if (!force && uint32_t(now - stopwatchLastRenderMillis_) < 200u) return;
  stopwatchLastRenderMillis_ = now;
  renderer_.renderStopwatch(stopwatchElapsedMillis(now));
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
  solidActive_ = false;
  lightEffectActive_ = false;
  audioActive_ = false;
  diySessionActive_ = false;
  clockActive_ = false;
  countdownActive_ = false;
  stopwatchActive_ = false;
  scoreboardActive_ = false;
  textActive_ = true;
  rawImageActive_ = false;
  gifActive_ = false;
  gifPending_ = false;
  gifPrecache_ = false;
  gifStaging_ = false;
  stopMediaPlayback();
  renderer_.setVisible(true);
  activateDisplayEffect();
}

bool IDotMatrixWLEDAdapter::onRawImageBegin(size_t byteLength) {
  return renderer_.beginRawImage(byteLength);
}

bool IDotMatrixWLEDAdapter::onRawImageData(
  size_t offset,
  const uint8_t* data,
  size_t length
) {
  return renderer_.writeRawImage(offset, data, length);
}

bool IDotMatrixWLEDAdapter::onRawImageComplete(bool crcValid) {
  if (!renderer_.completeRawImage(crcValid)) return false;
  solidActive_ = false;
  lightEffectActive_ = false;
  audioActive_ = false;
  diySessionActive_ = false;
  clockActive_ = false;
  countdownActive_ = false;
  stopwatchActive_ = false;
  scoreboardActive_ = false;
  textActive_ = false;
  rawImageActive_ = true;
  gifActive_ = false;
  gifPending_ = false;
  gifPrecache_ = false;
  gifStaging_ = false;
  stopMediaPlayback();
  activateDisplayEffect();
  return true;
}

bool IDotMatrixWLEDAdapter::onPngImage(const uint8_t* data, size_t length) {
  if (media_ == nullptr) return false;
  if (!media_->decodePng(data, length)) return false;
  stopMediaPlayback();
  solidActive_ = false;
  lightEffectActive_ = false;
  audioActive_ = false;
  diySessionActive_ = false;
  clockActive_ = false;
  countdownActive_ = false;
  stopwatchActive_ = false;
  scoreboardActive_ = false;
  textActive_ = false;
  rawImageActive_ = true;
  gifActive_ = false;
  gifPending_ = false;
  renderer_.setVisible(true);
  activateDisplayEffect();
  return true;
}

bool IDotMatrixWLEDAdapter::onGifBegin(size_t byteLength) {
  return media_ != nullptr && media_->beginGif(byteLength);
}

bool IDotMatrixWLEDAdapter::onGifData(
  size_t offset, const uint8_t* data, size_t length
) {
  return media_ != nullptr && media_->writeGif(offset, data, length);
}

bool IDotMatrixWLEDAdapter::onGifComplete(bool crcValid) {
  if (media_ == nullptr) return false;

  // Capture the content that owns the segment before completeGif() releases
  // an existing frame-cache playback.  A valid replacement may safely retire
  // the old GIF, but an invalid/CRC-failed transfer must leave it untouched.
  auto& segment = strip.getFirstSelectedSeg();
  const uint8_t previousEffect = segment.mode;
  const bool previousRendererVisible = renderer_.isVisible();
  const bool replacingActiveGif = gifActive_ && previousEffect == displayEffectId_;

  if (!media_->completeGif(crcValid)) return false;

  gifPreviousEffect_ = previousEffect;
  gifPreviousRendererVisible_ = previousRendererVisible;
  gifReplacingActiveGif_ = replacingActiveGif;
  gifPending_ = true;
  gifPrecache_ = media_->gifUsesFrameCache();
  gifStaging_ = !gifPrecache_;
  if (gifReplacingActiveGif_) gifActive_ = false;
  renderer_.setVisible(false);

  if (gifPrecache_) {
    // LZW12/no-PSRAM builds predecode into LittleFS before iDotMatrix Display
    // is allowed to own the segment.  WLED Static still provides the smallest
    // runtime footprint. Its primary colour is blanked temporarily so
    // the user sees an OFF/black panel instead of a distracting solid colour
    // while the cache is being prepared.
    beginGifBlankStaging();
    if (segment.mode != FX_MODE_STATIC) {
      segment.setMode(FX_MODE_STATIC);
      effectCurrent = FX_MODE_STATIC;
      stateUpdated(CALL_MODE_DIRECT_CHANGE);
    }
    strip.trigger();
  } else {
    // Validated 10/11-bit direct playback keeps the established staged effect
    // ordering used by the stable 16x16/32x32 paths.
    activateDisplayEffect();
  }
  return true;
}

void IDotMatrixWLEDAdapter::syncGifPlayback(bool playing, bool failed) {
  if (playing) {
    if (!gifPending_ && gifActive_) return;
    solidActive_ = false;
    lightEffectActive_ = false;
    audioActive_ = false;
    diySessionActive_ = false;
    clockActive_ = false;
    countdownActive_ = false;
    stopwatchActive_ = false;
    scoreboardActive_ = false;
    textActive_ = false;
    rawImageActive_ = false;
    gifActive_ = true;
    gifPending_ = false;
    gifStaging_ = false;
    if (gifPrecache_) activateDisplayEffect();
    gifPrecache_ = false;
    gifReplacingActiveGif_ = false;
    gifPreviousRendererVisible_ = false;
    renderer_.setVisible(true);
    return;
  }

  if (!gifPending_ && !gifStaging_ && !gifPrecache_) return;
  if (!failed) return;

  // Decoder/cache preparation failed.  If this was replacing an active GIF,
  // the old cache/decoder has already been retired and cannot be restored; in
  // that case leave WLED on Static instead of reviving an empty
  // iDotMatrix Display effect.  That empty effect was the reason one transient
  // failure could poison all following GIF replacements until the user
  // manually selected a solid colour.  Non-GIF content (clock/text/image/DIY)
  // remains restorable because its renderer canvas/state is still present.
  gifPending_ = false;
  gifStaging_ = false;
  const bool wasPrecache = gifPrecache_;
  const bool replacingActiveGif = gifReplacingActiveGif_;
  const bool restorePreviousCanvas = !replacingActiveGif &&
    gifPreviousEffect_ == displayEffectId_ && gifPreviousRendererVisible_;
  gifPrecache_ = false;
  gifReplacingActiveGif_ = false;
  gifPreviousRendererVisible_ = false;
  gifActive_ = false;
  renderer_.setVisible(false);
  stopMediaPlayback();

  auto& segment = strip.getFirstSelectedSeg();
  const uint8_t restoreEffect = replacingActiveGif ? FX_MODE_STATIC : gifPreviousEffect_;
  if (segment.mode == displayEffectId_ || (wasPrecache && segment.mode == FX_MODE_STATIC)) {
    if (segment.mode != restoreEffect) segment.setMode(restoreEffect);
    effectCurrent = restoreEffect;
    stateUpdated(CALL_MODE_DIRECT_CHANGE);
    strip.trigger();
  }
  if (restorePreviousCanvas) renderer_.setVisible(true);
}

void IDotMatrixWLEDAdapter::renderDisplayEffectFrame() {
  if (lightEffectActive_) {
    renderer_.renderLightEffect(millis());
  } else if (audioActive_) {
    const uint32_t now = millis();
    if (uint32_t(now - audioLastRenderMillis_) >= 80u) {
      audioLastRenderMillis_ = now;
      renderer_.renderAudio(audioSettings_.fft, audioSettings_.mode,
                            audioSettings_.level, audioSettings_.bands, now);
    }
  } else if (clockActive_) {
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
  } else if (countdownActive_) {
    renderCountdown(millis());
  } else if (stopwatchActive_) {
    renderStopwatch(millis());
  } else if (scoreboardActive_) {
    // Scoreboard content is static and is rendered immediately on command.
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
