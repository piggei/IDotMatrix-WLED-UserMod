#include "../IDotMatrixWLEDAdapter.h"

#include <cassert>

#include "wled.h"

uint8_t bri = 128;
uint8_t briLast = 128;
uint32_t stateUpdateCount = 0;
uint8_t effectCurrent = 42;
uint8_t colPri[4] = {0, 0, 0, 255};
uint32_t colorUpdateCount = 0;
uint32_t stripTriggerCount = 0;
time_t localTime = 0;
uint32_t testMillis = 0;
uint8_t testHour = 23;
uint8_t testMinute = 45;
uint8_t testDay = 2;
uint8_t testMonth = 9;
TestStrip strip;

class TestMediaSink final : public IDotMatrixMediaSink {
public:
  bool decodePng(const uint8_t*, size_t) override { return true; }
  bool beginGif(size_t) override { return true; }
  bool writeGif(size_t, const uint8_t*, size_t) override { return true; }
  bool completeGif(bool crcValid) override { return crcValid; }
  bool gifUsesFrameCache() const override { return cacheMode; }
  void stopPlayback() override { ++stopCount; }
  bool cacheMode = false;
  uint32_t stopCount = 0;
};

void toggleOnOff() {
  if (bri == 0) {
    bri = briLast;
    strip.restartRuntime();
  } else {
    briLast = bri;
    bri = 0;
  }
}

void stateUpdated(uint8_t callMode) {
  assert(callMode == CALL_MODE_DIRECT_CHANGE);
  ++stateUpdateCount;
}

void colorUpdated(uint8_t callMode) {
  assert(callMode == CALL_MODE_DIRECT_CHANGE);
  strip.segmentRef().colors[0] = RGBW32(colPri[0], colPri[1], colPri[2], colPri[3]);
  ++colorUpdateCount;
}

int main() {
  IDotMatrixRenderer renderer;
  assert(renderer.begin(0x01));
  IDotMatrixWLEDAdapter adapter(renderer);
  assert(adapter.registerDisplayEffect());
  assert(adapter.displayEffectId() == 200);
  assert(strip.registeredEffectCount() == 1);

  adapter.onScreenPower(true);
  assert(bri == 128 && briLast == 128);

  adapter.onBrightnessPercent(100);
  assert(bri == 255 && briLast == 255);

  adapter.onScreenPower(false);
  assert(bri == 0 && briLast == 255);

  adapter.onBrightnessPercent(25);
  assert(bri == 0 && briLast == 64);

  adapter.onScreenPower(true);
  assert(bri == 64 && briLast == 64);

  adapter.onBrightnessPercent(0);
  assert(bri == 0 && briLast == 64);

  adapter.onBrightnessPercent(50);
  assert(bri == 128 && briLast == 128);
  assert(strip.restartCount == 2);
  assert(stateUpdateCount == 6);

  adapter.onSolidColor(0x12, 0x34, 0x56);
  assert(adapter.isSolidActive());
  assert(adapter.isDisplayEffectActive());
  assert(effectCurrent == 200);
  assert(renderer.isVisible());
  // App solid colour is framebuffer content in 0.8; it must not rewrite the
  // WLED primary colour or expose WLED Static as if the two apps were synced.
  assert(colPri[0] == 0 && colPri[1] == 0 && colPri[2] == 0 && colPri[3] == 255);
  assert(colorUpdateCount == 0);
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(0, 0) == RGBW32(0x12, 0x34, 0x56, 0));
  assert(strip.segmentRef().colorAt(15, 15) == RGBW32(0x12, 0x34, 0x56, 0));

  adapter.onGraffitiMode(true);
  assert(adapter.isDiySessionActive());
  assert(!adapter.isSolidActive());
  assert(renderer.isVisible());
  assert(stripTriggerCount == 2);
  assert(adapter.isDisplayEffectActive());
  assert(effectCurrent == 200);

  const uint8_t coordinates[] = {1, 2, 15, 14, 16, 0, 9};
  adapter.onGraffitiPixels(0x21, 0x43, 0x65, coordinates, sizeof(coordinates));
  assert(stripTriggerCount == 3);

  strip.renderEffect();
  assert(strip.segmentRef().colorAt(1, 2) == RGBW32(0x21, 0x43, 0x65, 0));
  assert(strip.segmentRef().colorAt(15, 14) == RGBW32(0x21, 0x43, 0x65, 0));
  assert(strip.segmentRef().colorAt(0, 0) == 0);

  // Leaving the editor preserves the last canvas, matching the reference.
  adapter.onGraffitiMode(false);
  assert(!adapter.isDiySessionActive());
  assert(renderer.isVisible());

  // Every app-originated visual mode remains under iDotMatrix Display.
  adapter.onSolidColor(1, 2, 3);
  assert(adapter.isSolidActive());
  assert(renderer.isVisible());
  assert(adapter.isDisplayEffectActive());
  assert(colorUpdateCount == 0);
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(0, 0) == RGBW32(1, 2, 3, 0));

  IDotMatrixAudioSettings audio{};
  audio.mode = 1;
  audio.level = 8;
  adapter.onAudio(audio);
  assert(adapter.isAudioActive());
  assert(!adapter.isSolidActive());
  assert(adapter.isDisplayEffectActive());
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(7, 7) != 0);

  audio.fft = true;
  audio.mode = 3;
  for (uint8_t i = 0; i < 8; ++i) audio.bands[i] = 12;
  adapter.onAudio(audio);
  assert(adapter.isAudioActive() && adapter.audioUsesFFT());
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(7, 0) != 0);

  IDotMatrixLightEffectSettings light{};
  light.effect = 3;
  light.speed = 50;
  light.colorCount = 2;
  light.colors[0].red = 255;
  light.colors[0].green = 0;
  light.colors[0].blue = 0;
  light.colors[1].red = 0;
  light.colors[1].green = 255;
  light.colors[1].blue = 0;
  testMillis = 1000;
  adapter.onLightEffect(light);
  assert(adapter.isLightEffectActive());
  assert(!adapter.isSolidActive());
  assert(adapter.isDisplayEffectActive());
  assert(adapter.lightEffectId() == 3);
  assert(adapter.lightEffectSpeed() == 50);
  assert(adapter.lightEffectColorCount() == 2);
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(0, 0) == RGBW32(255, 0, 0, 0));
  assert(strip.segmentRef().colorAt(4, 0) == RGBW32(0, 255, 0, 0));

  // A WLED-side effect selection is an explicit source change. It releases the
  // iDotMatrix canvas; the next iDotMatrix effect command can reclaim it.
  strip.segmentRef().setMode(42);
  adapter.syncWLEDControl();
  assert(!adapter.isLightEffectActive());
  assert(!renderer.isVisible());
  adapter.onLightEffect(light);
  assert(adapter.isLightEffectActive());
  assert(adapter.isDisplayEffectActive());
  assert(renderer.isVisible());

  // A valid pixel packet is sufficient to reclaim the effect even if an app
  // version does not send the DIY-state command first.
  const uint8_t directPixel[] = {3, 4};
  adapter.onGraffitiPixels(7, 8, 9, directPixel, sizeof(directPixel));
  assert(adapter.isDiySessionActive());
  assert(adapter.isDisplayEffectActive());
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(3, 4) == RGBW32(7, 8, 9, 0));

  IDotMatrixClockSettings clock;
  clock.style = 3;
  clock.use24Hour = true;
  clock.red = 10;
  clock.green = 20;
  clock.blue = 30;
  adapter.onClock(clock);
  assert(adapter.isClockActive());
  assert(adapter.isDisplayEffectActive());
  assert(strip.segmentRef().mode == adapter.displayEffectId());
  assert(!adapter.isDiySessionActive());
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(0, 0) == RGBW32(10, 20, 30, 0));
  assert(strip.segmentRef().colorAt(6, 2) == BLACK);

  clock.showDate = true;
  testMillis = 0;
  adapter.onClock(clock);
  testMillis = 30000;
  strip.renderEffect();
  // In style 3 the date slash is black over the selected background.
  assert(strip.segmentRef().colorAt(3, 9) == BLACK);

  // Countdown uses the same iDotMatrix Display framebuffer. Its state keeps
  // running even if WLED temporarily takes the panel, because the BLE app has
  // no reverse channel telling it that another source was selected.
  IDotMatrixCountdownSettings countdown{};
  countdown.mode = 1;
  countdown.minutes = 0;
  countdown.seconds = 3;
  testMillis = 10000;
  adapter.onCountdown(countdown);
  assert(adapter.isCountdownActive());
  assert(adapter.isCountdownRunning());
  assert(adapter.isDisplayEffectActive());
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(6, 0) == RGBW32(255, 145, 0, 0));
  assert(strip.segmentRef().colorAt(7, 4) == RGBW32(255, 220, 120, 0));
  assert(adapter.countdownRemainingMillis(testMillis) == 3000);

  testMillis = 11500;
  adapter.loop(testMillis);
  assert(adapter.countdownRemainingSeconds(testMillis) == 2);
  countdown.mode = 2;
  adapter.onCountdown(countdown);
  assert(!adapter.isCountdownRunning());
  assert(adapter.isCountdownPaused());
  assert(adapter.countdownRemainingSeconds(testMillis) == 2);

  testMillis = 13000;
  countdown.mode = 3;
  adapter.onCountdown(countdown);
  assert(adapter.isCountdownRunning());
  strip.segmentRef().setMode(42);
  adapter.syncWLEDControl();
  assert(!adapter.isCountdownActive());
  assert(adapter.isCountdownRunning());
  testMillis = 15000;
  adapter.loop(testMillis);
  assert(!adapter.isCountdownRunning());
  assert(adapter.countdownRemainingSeconds(testMillis) == 0);
  assert(adapter.takeCountdownFinished());
  assert(!adapter.takeCountdownFinished());

  // Stopwatch start/pause/resume semantics mirror the standalone emulator and
  // also retain time while native WLED owns the display.
  testMillis = 20000;
  adapter.onStopwatch(1);
  assert(adapter.isStopwatchActive());
  assert(adapter.isStopwatchRunning());
  assert(adapter.isDisplayEffectActive());
  testMillis = 22500;
  assert(adapter.stopwatchElapsedMillis(testMillis) == 2500);
  assert(adapter.stopwatchElapsedSeconds(testMillis) == 2);
  adapter.onStopwatch(2);
  assert(!adapter.isStopwatchRunning());
  assert(adapter.stopwatchElapsedSeconds(testMillis) == 2);
  testMillis = 24000;
  adapter.onStopwatch(3);
  assert(adapter.isStopwatchRunning());
  strip.segmentRef().setMode(42);
  adapter.syncWLEDControl();
  assert(!adapter.isStopwatchActive());
  assert(adapter.isStopwatchRunning());
  testMillis = 26000;
  assert(adapter.stopwatchElapsedSeconds(testMillis) == 4);
  adapter.onStopwatch(2);
  assert(adapter.isStopwatchActive());
  assert(!adapter.isStopwatchRunning());
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(6, 0) == RGBW32(255, 145, 0, 0));
  assert(strip.segmentRef().colorAt(7, 11) == RGBW32(255, 255, 255, 0));

  adapter.onScoreboard(7, 42);
  assert(adapter.isScoreboardActive());
  assert(adapter.scoreA() == 7 && adapter.scoreB() == 42);
  assert(adapter.isDisplayEffectActive());
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(3, 5) == RGBW32(0, 0, 255, 0));
  assert(strip.segmentRef().colorAt(9, 5) == RGBW32(255, 0, 0, 0));

  IDotMatrixTextSettings text;
  text.glyphCount = 1;
  text.glyphWidth = 8;
  text.glyphHeight = 16;
  text.glyphBytes = 16;
  text.red = 11;
  text.green = 22;
  text.blue = 33;
  assert(adapter.onTextBegin(text));
  uint8_t glyph[16]{};
  glyph[0] = 0x01;
  adapter.onTextGlyph(0, glyph, sizeof(glyph));
  adapter.onTextComplete();
  assert(adapter.isTextActive());
  assert(!adapter.isClockActive());
  assert(adapter.isDisplayEffectActive());
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(0, 0) == RGBW32(11, 22, 33, 0));

  uint8_t rawImage[16 * 16 * 3]{};
  rawImage[0] = 41;
  rawImage[1] = 42;
  rawImage[2] = 43;
  const size_t lastPixel = sizeof(rawImage) - 3;
  rawImage[lastPixel] = 91;
  rawImage[lastPixel + 1] = 92;
  rawImage[lastPixel + 2] = 93;
  assert(adapter.onRawImageBegin(sizeof(rawImage)));
  assert(adapter.onRawImageData(0, rawImage, 400));
  assert(adapter.onRawImageData(400, rawImage + 400, sizeof(rawImage) - 400));
  assert(adapter.onRawImageComplete(true));
  assert(adapter.isRawImageActive());
  assert(!adapter.isTextActive());
  assert(!adapter.isClockActive());
  assert(adapter.isDisplayEffectActive());
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(0, 0) == RGBW32(41, 42, 43, 0));
  assert(strip.segmentRef().colorAt(15, 15) == RGBW32(91, 92, 93, 0));

  // A profile/segment mismatch is black in strict mode and nearest-neighbour
  // sampled only when rescale is explicitly enabled.
  assert(renderer.begin(0x04));
  const uint8_t scaledPixel[] = {4, 8};
  adapter.onGraffitiPixels(90, 80, 70, scaledPixel, sizeof(scaledPixel));
  strip.renderEffect();
  assert(!adapter.dimensionsMatch());
  assert(strip.segmentRef().colorAt(1, 2) == BLACK);
  adapter.setRescaleEnabled(true);
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(1, 2) == RGBW32(90, 80, 70, 0));

  // Switching to a normal WLED effect through the UI/API must release any
  // active iDotMatrix media state even though no BLE content command arrived.
  TestMediaSink media;
  IDotMatrixWLEDAdapter mediaAdapter(renderer, &media);
  assert(mediaAdapter.registerDisplayEffect());

  // GIF reception is transactional: completing the BLE transfer stages the
  // dedicated WLED effect first, but the GIF remains invisible/inactive until
  // asynchronous decoder open succeeds.
  strip.segmentRef().setMode(42);
  assert(mediaAdapter.onGifComplete(true));
  assert(!mediaAdapter.isGifActive());
  assert(mediaAdapter.isDisplayEffectActive());
  assert(!renderer.isVisible());
  mediaAdapter.syncGifPlayback(true, false);
  assert(mediaAdapter.isGifActive());
  assert(mediaAdapter.isDisplayEffectActive());
  assert(renderer.isVisible());

  // Switching to a normal WLED effect through the UI/API must release any
  // active iDotMatrix media state even though no BLE content command arrived.
  strip.segmentRef().setMode(42);
  mediaAdapter.syncWLEDControl();
  assert(!mediaAdapter.isGifActive());
  assert(!renderer.isVisible());
  assert(media.stopCount == 1);

  // A decoder/open failure after a valid transfer must be terminal and must
  // restore the exact WLED effect that was active before staging.
  strip.segmentRef().setMode(42);
  assert(mediaAdapter.onGifComplete(true));
  assert(!mediaAdapter.isGifActive());
  assert(mediaAdapter.isDisplayEffectActive());
  mediaAdapter.syncGifPlayback(false, true);
  assert(!mediaAdapter.isGifActive());
  assert(!mediaAdapter.isDisplayEffectActive());
  assert(strip.segmentRef().mode == 42);
  // LZW12/no-PSRAM frame-cache preparation must keep iDotMatrix Display
  // inactive until the decoder has been released and the cache is ready.
  TestMediaSink cacheMedia;
  cacheMedia.cacheMode = true;
  IDotMatrixWLEDAdapter cacheAdapter(renderer, &cacheMedia);
  assert(cacheAdapter.registerDisplayEffect());
  strip.segmentRef().setMode(42);
  strip.segmentRef().colors[0] = RGBW32(0x12, 0x34, 0x56, 0);
  assert(cacheAdapter.onGifComplete(true));
  assert(!cacheAdapter.isGifActive());
  assert(!cacheAdapter.isDisplayEffectActive());
  assert(strip.segmentRef().mode == FX_MODE_STATIC);
  // Cached-GIF staging keeps Static internally for its low RAM cost, but temporarily makes
  // the physical panel black while the frame cache is being built.
  assert(strip.segmentRef().colors[0] == BLACK);
  strip.renderEffect();
  assert(strip.segmentRef().colorAt(0, 0) == BLACK);
  cacheAdapter.syncWLEDControl();
  assert(cacheMedia.stopCount == 0);
  cacheAdapter.syncGifPlayback(true, false);
  assert(cacheAdapter.isGifActive());
  assert(cacheAdapter.isDisplayEffectActive());
  assert(renderer.isVisible());
  // The user's WLED colour must survive the temporary blank staging phase.
  assert(strip.segmentRef().colors[0] == RGBW32(0x12, 0x34, 0x56, 0));

  strip.segmentRef().setMode(42);
  cacheAdapter.syncWLEDControl();
  assert(!cacheAdapter.isGifActive());
  assert(cacheMedia.stopCount == 1);

  // A transient failure while replacing an already active cached GIF must not
  // restore an empty iDotMatrix Display effect.  The old cache is retired once
  // the replacement transfer is valid, so failure recovery must land on
  // WLED Static.  A following GIF can then stage cleanly without the user
  // manually selecting a solid colour.
  TestMediaSink replaceMedia;
  replaceMedia.cacheMode = true;
  IDotMatrixWLEDAdapter replaceAdapter(renderer, &replaceMedia);
  assert(replaceAdapter.registerDisplayEffect());
  strip.segmentRef().setMode(42);
  strip.segmentRef().colors[0] = RGBW32(0x21, 0x43, 0x65, 0);
  assert(replaceAdapter.onGifComplete(true));
  assert(strip.segmentRef().colors[0] == BLACK);
  replaceAdapter.syncGifPlayback(true, false);
  assert(replaceAdapter.isGifActive());
  assert(replaceAdapter.isDisplayEffectActive());
  assert(strip.segmentRef().colors[0] == RGBW32(0x21, 0x43, 0x65, 0));

  assert(replaceAdapter.onGifComplete(true));
  assert(!replaceAdapter.isGifActive());
  assert(strip.segmentRef().mode == FX_MODE_STATIC);
  assert(strip.segmentRef().colors[0] == BLACK);
  replaceAdapter.syncGifPlayback(false, true);
  assert(!replaceAdapter.isGifActive());
  assert(!replaceAdapter.isDisplayEffectActive());
  assert(strip.segmentRef().mode == FX_MODE_STATIC);
  // Even failure recovery must undo the temporary black staging colour.
  assert(strip.segmentRef().colors[0] == RGBW32(0x21, 0x43, 0x65, 0));

  // The next replacement attempt must still be able to stage and publish.
  assert(replaceAdapter.onGifComplete(true));
  assert(strip.segmentRef().mode == FX_MODE_STATIC);
  assert(strip.segmentRef().colors[0] == BLACK);
  replaceAdapter.syncGifPlayback(true, false);
  assert(replaceAdapter.isGifActive());
  assert(replaceAdapter.isDisplayEffectActive());
  assert(renderer.isVisible());
  assert(strip.segmentRef().colors[0] == RGBW32(0x21, 0x43, 0x65, 0));

  // Non-GIF iDotMatrix content is restorable: a failed cache build after a
  // clock must return to the display effect and make the prior canvas visible.
  TestMediaSink clockMedia;
  clockMedia.cacheMode = true;
  IDotMatrixWLEDAdapter clockAdapter(renderer, &clockMedia);
  assert(clockAdapter.registerDisplayEffect());
  strip.segmentRef().colors[0] = RGBW32(0x31, 0x32, 0x33, 0);
  IDotMatrixClockSettings clockSettings{};
  clockAdapter.onClock(clockSettings);
  assert(clockAdapter.isClockActive());
  assert(clockAdapter.isDisplayEffectActive());
  assert(renderer.isVisible());
  assert(clockAdapter.onGifComplete(true));
  assert(strip.segmentRef().mode == FX_MODE_STATIC);
  assert(strip.segmentRef().colors[0] == BLACK);
  assert(!renderer.isVisible());
  clockAdapter.syncGifPlayback(false, true);
  assert(clockAdapter.isClockActive());
  assert(clockAdapter.isDisplayEffectActive());
  assert(renderer.isVisible());
  assert(strip.segmentRef().colors[0] == RGBW32(0x31, 0x32, 0x33, 0));

  // The new 0.8 light-effect state is equally restorable after a failed cached
  // GIF preparation; it must resume as iDotMatrix content, not native WLED state.
  TestMediaSink lightMedia;
  lightMedia.cacheMode = true;
  IDotMatrixWLEDAdapter lightAdapter(renderer, &lightMedia);
  assert(lightAdapter.registerDisplayEffect());
  IDotMatrixLightEffectSettings recoverLight{};
  recoverLight.effect = 4;
  recoverLight.speed = 30;
  recoverLight.colorCount = 2;
  recoverLight.colors[0].red = 200;
  recoverLight.colors[1].blue = 200;
  testMillis = 5000;
  lightAdapter.onLightEffect(recoverLight);
  assert(lightAdapter.isLightEffectActive());
  assert(lightAdapter.isDisplayEffectActive());
  assert(lightAdapter.onGifComplete(true));
  assert(strip.segmentRef().mode == FX_MODE_STATIC);
  assert(!renderer.isVisible());
  lightAdapter.syncGifPlayback(false, true);
  assert(lightAdapter.isLightEffectActive());
  assert(lightAdapter.isDisplayEffectActive());
  assert(renderer.isVisible());

  // If the user/API takes WLED ownership while a cache build is staged, the
  // temporary black primary colour must never leak into the user's WLED state.
  TestMediaSink cancelMedia;
  cancelMedia.cacheMode = true;
  IDotMatrixWLEDAdapter cancelAdapter(renderer, &cancelMedia);
  assert(cancelAdapter.registerDisplayEffect());
  strip.segmentRef().setMode(42);
  strip.segmentRef().colors[0] = RGBW32(0x41, 0x42, 0x43, 0);
  assert(cancelAdapter.onGifComplete(true));
  assert(strip.segmentRef().mode == FX_MODE_STATIC);
  assert(strip.segmentRef().colors[0] == BLACK);
  strip.segmentRef().setMode(43);
  cancelAdapter.syncWLEDControl();
  assert(strip.segmentRef().mode == 43);
  assert(strip.segmentRef().colors[0] == RGBW32(0x41, 0x42, 0x43, 0));
  assert(cancelMedia.stopCount == 1);

}
