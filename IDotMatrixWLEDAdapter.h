#pragma once

#include "IDotMatrixProtocol.h"
#include "IDotMatrixRenderer.h"
#include "IDotMatrixMediaSink.h"

class IDotMatrixWLEDAdapter final : public IDotMatrixProtocolEvents {
public:
  explicit IDotMatrixWLEDAdapter(
    IDotMatrixRenderer& renderer,
    IDotMatrixMediaSink* media = nullptr
  ) : renderer_(renderer), media_(media) {}

  bool registerDisplayEffect();
  void setRescaleEnabled(bool enabled) { rescaleEnabled_ = enabled; }
  void loop(uint32_t now);

  void onScreenPower(bool on) override;
  void onBrightnessPercent(uint8_t percent) override;
  void onSolidColor(uint8_t red, uint8_t green, uint8_t blue) override;
  void onLightEffect(const IDotMatrixLightEffectSettings& settings) override;
  void onAudio(const IDotMatrixAudioSettings& settings) override;
  void onGraffitiMode(bool enter) override;
  void onGraffitiPixels(
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    const uint8_t* coordinates,
    size_t coordinateBytes
  ) override;
  void onClock(const IDotMatrixClockSettings& settings) override;
  void onCountdown(const IDotMatrixCountdownSettings& settings) override;
  void onStopwatch(uint8_t mode) override;
  void onScoreboard(uint16_t scoreA, uint16_t scoreB) override;
  bool takeCountdownFinished() override;
  bool onTextBegin(const IDotMatrixTextSettings& settings) override;
  void onTextGlyph(
    uint8_t index,
    const uint8_t* bitmap,
    size_t bitmapLength
  ) override;
  void onTextComplete() override;
  bool onRawImageBegin(size_t byteLength) override;
  bool onRawImageData(
    size_t offset,
    const uint8_t* data,
    size_t length
  ) override;
  bool onRawImageComplete(bool crcValid) override;
  bool onPngImage(const uint8_t* data, size_t length) override;
  bool onGifBegin(size_t byteLength) override;
  bool onGifData(size_t offset, const uint8_t* data, size_t length) override;
  bool onGifComplete(bool crcValid) override;

  void renderDisplayEffectFrame();
  bool isDiySessionActive() const { return diySessionActive_; }
  bool isSolidActive() const { return solidActive_; }
  bool isLightEffectActive() const { return lightEffectActive_; }
  bool isAudioActive() const { return audioActive_; }
  bool audioUsesFFT() const { return audioSettings_.fft; }
  uint8_t audioMode() const { return audioSettings_.mode; }
  uint8_t lightEffectId() const { return renderer_.lightEffectId(); }
  uint8_t lightEffectSpeed() const { return renderer_.lightEffectSpeed(); }
  uint8_t lightEffectColorCount() const { return renderer_.lightEffectColorCount(); }
  bool isDisplayEffectRegistered() const { return displayEffectId_ != 0xFF; }
  bool isDisplayEffectActive() const;
  void syncWLEDControl();
  void syncGifPlayback(bool playing, bool failed);
  // Automation (alarm/program) playback temporarily reuses the normal iDot
  // display paths. These helpers let the scheduler release that content
  // explicitly, including a GIF still in low-RAM precache staging.
  void cancelAutomationContent();
  void restoreClockFallback();
  uint8_t displayEffectId() const { return displayEffectId_; }
  bool isClockActive() const { return clockActive_; }
  bool isCountdownActive() const { return countdownActive_; }
  bool isStopwatchActive() const { return stopwatchActive_; }
  bool isScoreboardActive() const { return scoreboardActive_; }
  bool isCountdownRunning() const { return countdownRunning_; }
  bool isCountdownPaused() const { return countdownPaused_; }
  uint32_t countdownRemainingMillis(uint32_t now) const;
  uint32_t countdownRemainingSeconds(uint32_t now) const;
  bool isStopwatchRunning() const { return stopwatchRunning_; }
  uint32_t stopwatchElapsedMillis(uint32_t now) const;
  uint32_t stopwatchElapsedSeconds(uint32_t now) const;
  uint16_t scoreA() const { return scoreA_; }
  uint16_t scoreB() const { return scoreB_; }
  bool isTextActive() const { return textActive_; }
  bool isRawImageActive() const { return rawImageActive_; }
  bool isGifActive() const { return gifActive_; }
  bool isGifPending() const { return gifPending_; }
  uint8_t clockStyle() const { return clockSettings_.style; }
  bool clockUses24Hour() const { return clockSettings_.use24Hour; }
  bool clockShowsDate() const { return clockSettings_.showDate; }
  uint8_t textGlyphCount() const { return renderer_.textGlyphCount(); }
  uint8_t textGlyphWidth() const { return renderer_.textGlyphWidth(); }
  uint8_t textGlyphHeight() const { return renderer_.textGlyphHeight(); }
  uint8_t textSpeed() const { return renderer_.textSpeed(); }
  bool rescaleEnabled() const { return rescaleEnabled_; }
  bool dimensionsMatch() const { return dimensionsMatch_; }

private:
  void activateDisplayEffect();
  void clearContentState();
  void renderCountdown(uint32_t now, bool force = false);
  void renderStopwatch(uint32_t now, bool force = false);
  void beginGifBlankStaging();
  void endGifBlankStaging();
  void stopMediaPlayback();
  void renderCanvasToSegment();

  IDotMatrixRenderer& renderer_;
  IDotMatrixMediaSink* media_ = nullptr;
  bool screenOn_ = false;
  bool solidActive_ = false;
  bool lightEffectActive_ = false;
  bool audioActive_ = false;
  bool diySessionActive_ = false;
  bool clockActive_ = false;
  bool countdownActive_ = false;
  bool stopwatchActive_ = false;
  bool scoreboardActive_ = false;
  bool textActive_ = false;
  bool rawImageActive_ = false;
  bool gifActive_ = false;
  bool gifPending_ = false;
  bool gifStaging_ = false;
  bool gifPrecache_ = false;
  bool gifReplacingActiveGif_ = false;
  bool gifPreviousRendererVisible_ = false;
  bool gifBlankStaging_ = false;
  uint32_t gifStagingPrimaryColor_ = 0;
  uint8_t gifPreviousEffect_ = 0;
  bool textLoadReady_ = false;
  bool rescaleEnabled_ = false;
  bool dimensionsMatch_ = false;
  uint8_t displayEffectId_ = 0xFF;
  IDotMatrixClockSettings clockSettings_{};
  IDotMatrixAudioSettings audioSettings_{};
  uint32_t audioLastRenderMillis_ = 0;
  uint32_t clockCycleStartedAt_ = 0;
  bool countdownRunning_ = false;
  bool countdownPaused_ = false;
  bool countdownFinishPending_ = false;
  uint32_t countdownRemainingMs_ = 0;
  uint32_t countdownStartMillis_ = 0;
  uint32_t countdownLastRenderMillis_ = 0;
  bool stopwatchRunning_ = false;
  uint32_t stopwatchElapsedMs_ = 0;
  uint32_t stopwatchStartMillis_ = 0;
  uint32_t stopwatchLastRenderMillis_ = 0;
  uint16_t scoreA_ = 0;
  uint16_t scoreB_ = 0;
  uint16_t targetWidth_ = 0;
  uint16_t targetHeight_ = 0;
};
