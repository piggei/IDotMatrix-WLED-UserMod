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
int16_t currentPlaylist = -1;
uint32_t unloadPlaylistCount = 0;
uint32_t applyPresetCount = 0;
uint8_t pendingPreset = 0;
TestStrip strip;

class TestMediaSink final : public IDotMatrixMediaSink {
public:
  bool decodePng(const uint8_t*, size_t) override { return true; }
  bool beginGif(size_t) override { return true; }
  bool writeGif(size_t, const uint8_t*, size_t) override { return true; }
  bool completeGif(bool crcValid) override { return crcValid; }
  bool gifUsesFrameCache() const override { return cacheMode; }
  bool queueStoredGif(const char*, const char* = nullptr) override {
    ++storedQueueCount;
    return queueStoredOk;
  }
  void stopPlayback() override { ++stopCount; }
  bool cacheMode = false;
  bool queueStoredOk = true;
  uint32_t storedQueueCount = 0;
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

void unloadPlaylist() {
  ++unloadPlaylistCount;
  currentPlaylist = -1;
}

bool applyPreset(uint8_t index, uint8_t callMode) {
  assert(callMode == CALL_MODE_DIRECT_CHANGE);
  ++applyPresetCount;
  unloadPlaylist();
  pendingPreset = index;
  return true;
}

static void simulateHandlePresets() {
  if (pendingPreset == 0) return;
  pendingPreset = 0;
  strip.segmentRef().setMode(42);
  effectCurrent = 42;
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

  // WLED processes playlist presets asynchronously after the Usermod loop.
  // Reproduce the real failure mode: a playlist preset is already queued while
  // the panel still shows a native WLED effect. Explicit iDotMatrix ownership
  // must both stop the playlist and cancel that pending preset before selecting
  // the framebuffer effect.
  const uint32_t triggerCountBeforePlaylistRegression = stripTriggerCount;
  strip.segmentRef().setMode(42);
  effectCurrent = 42;
  currentPlaylist = 7;
  pendingPreset = 23;
  adapter.onSolidColor(0x12, 0x34, 0x56);
  assert(currentPlaylist == -1);
  assert(unloadPlaylistCount == 1);
  assert(applyPresetCount == 1);
  assert(pendingPreset == 0);
  assert(adapter.isDisplayEffectSelected());
  assert(effectCurrent == 200);
  // The later WLED handlePresets() pass now has nothing left to apply and must
  // not steal the segment back.
  simulateHandlePresets();
  assert(adapter.isDisplayEffectSelected());
  assert(effectCurrent == 200);
  // Once stopped, later iDotMatrix frames/content updates do not touch playlist
  // state again.
  adapter.onSolidColor(0x12, 0x34, 0x56);
  assert(unloadPlaylistCount == 1);
  assert(applyPresetCount == 1);
  // Keep legacy trigger-count assertions below focused on their original paths.
  stripTriggerCount = triggerCountBeforePlaylistRegression;

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

  // Every app-originated visual mode remains under iDotMatrix.
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

  // Local audio override keeps the visualizer family/mode selected by BLE but
  // replaces phone amplitude data with the local provider sample.
  adapter.setAudioDataOverride(true);
  const uint8_t localBands[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  adapter.updateAudioSample(7, localBands);
  IDotMatrixAudioSettings noisyPhone{};
  noisyPhone.fft = true;
  noisyPhone.mode = 4;
  noisyPhone.level = 12;
  for (uint8_t& band : noisyPhone.bands) band = 12;
  adapter.onAudio(noisyPhone);
  assert(adapter.audioDataOverride());
  assert(adapter.audioUsesFFT());
  assert(adapter.audioMode() == 4);
  assert(adapter.audioLevel() == 7);
  assert(adapter.audioBand(0) == 1 && adapter.audioBand(7) == 8);

  // Returning to phone mode restores the original protocol semantics.
  adapter.setAudioDataOverride(false);
  noisyPhone.fft = false;
  noisyPhone.mode = 2;
  noisyPhone.level = 11;
  adapter.onAudio(noisyPhone);
  assert(!adapter.audioDataOverride());
  assert(!adapter.audioUsesFFT());
  assert(adapter.audioMode() == 2);
  assert(adapter.audioLevel() == 11);

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
  // The callback heartbeat intentionally owns the framebuffer for a short
  // grace period so a newly started Carousel/Clock is not cleared while WLED
  // still exposes the previous native Segment::mode. Once the heartbeat has
  // expired, native WLED ownership is authoritative again.
  testMillis += 301;
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

  // Countdown uses the same iDotMatrix framebuffer. Its state keeps
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
  // The callback heartbeat intentionally owns the framebuffer for a short
  // grace period so a newly started Carousel/Clock is not cleared while WLED
  // still exposes the previous native Segment::mode. Once the heartbeat has
  // expired, native WLED ownership is authoritative again.
  testMillis += 301;
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
  // The callback heartbeat intentionally owns the framebuffer for a short
  // grace period so a newly started Carousel/Clock is not cleared while WLED
  // still exposes the previous native Segment::mode. Once the heartbeat has
  // expired, native WLED ownership is authoritative again.
  testMillis += 301;
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

  // 0.9 native-matrix behavior: logical and physical dimensions are
  // independent. Every 16/32/64 combination scales automatically.
  adapter.setRescaleEnabled(false);

  // 64 -> 32: box-average a 2x2 source block into one destination pixel.
  assert(renderer.begin(0x04));
  strip.segmentRef().width = 32;
  strip.segmentRef().height = 32;
  const uint8_t down64to32[] = {2, 2};
  adapter.onGraffitiPixels(100, 80, 60, down64to32, sizeof(down64to32));
  strip.renderEffect();
  assert(!adapter.dimensionsMatch());
  assert(!adapter.autoUpscaleActive());
  assert(adapter.autoDownscaleActive());
  assert(strip.segmentRef().colorAt(1, 1) == RGBW32(25, 20, 15, 0));

  // 64 -> 16: a 4x4 source block is reduced to one destination pixel.
  strip.segmentRef().width = 16;
  strip.segmentRef().height = 16;
  strip.renderEffect();
  assert(adapter.autoDownscaleActive());
  assert(strip.segmentRef().colorAt(0, 0) == RGBW32(6, 5, 3, 0));

  // 32 -> 16: verify the remaining physical-downscale combination.
  assert(renderer.begin(0x03));
  const uint8_t down32to16[] = {2, 2};
  adapter.onGraffitiPixels(100, 80, 60, down32to16, sizeof(down32to16));
  strip.renderEffect();
  assert(adapter.autoDownscaleActive());
  assert(strip.segmentRef().colorAt(1, 1) == RGBW32(25, 20, 15, 0));

  // 16 -> 64: nearest-neighbour expansion produces a crisp 4x4 block.
  assert(renderer.begin(0x01));
  strip.segmentRef().width = 64;
  strip.segmentRef().height = 64;
  const uint8_t upscaledPixel[] = {1, 2};
  adapter.onGraffitiPixels(10, 20, 30, upscaledPixel, sizeof(upscaledPixel));
  strip.renderEffect();
  assert(!adapter.dimensionsMatch());
  assert(adapter.autoUpscaleActive());
  assert(!adapter.autoDownscaleActive());
  for (uint16_t y = 8; y < 12; ++y) {
    for (uint16_t x = 4; x < 8; ++x) {
      assert(strip.segmentRef().colorAt(x, y) == RGBW32(10, 20, 30, 0));
    }
  }

  // 16 -> 32 and 32 -> 64 use the same automatic upscale path.
  strip.segmentRef().width = 32;
  strip.segmentRef().height = 32;
  strip.renderEffect();
  assert(adapter.autoUpscaleActive());
  assert(strip.segmentRef().colorAt(2, 4) == RGBW32(10, 20, 30, 0));

  assert(renderer.begin(0x03));
  strip.segmentRef().width = 64;
  strip.segmentRef().height = 64;
  const uint8_t up32to64[] = {3, 5};
  adapter.onGraffitiPixels(40, 50, 60, up32to64, sizeof(up32to64));
  strip.renderEffect();
  assert(adapter.autoUpscaleActive());
  assert(strip.segmentRef().colorAt(6, 10) == RGBW32(40, 50, 60, 0));

  strip.segmentRef().width = 16;
  strip.segmentRef().height = 16;

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
  // LZW12/no-PSRAM frame-cache preparation must keep iDotMatrix
  // selected while the decoder/cache is prepared, so WLED UI/presets retain
  // the dedicated effect ID throughout the transient blank staging phase.
  TestMediaSink cacheMedia;
  cacheMedia.cacheMode = true;
  IDotMatrixWLEDAdapter cacheAdapter(renderer, &cacheMedia);
  assert(cacheAdapter.registerDisplayEffect());
  strip.segmentRef().setMode(42);
  strip.segmentRef().colors[0] = RGBW32(0x12, 0x34, 0x56, 0);
  assert(cacheAdapter.onGifComplete(true));
  assert(!cacheAdapter.isGifActive());
  assert(cacheAdapter.isDisplayEffectActive());
  assert(strip.segmentRef().mode == cacheAdapter.displayEffectId());
  // Cached-GIF staging temporarily makes the physical panel black while the
  // frame cache is being built, without changing the public WLED effect.
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
  testMillis += 301;
  cacheAdapter.syncWLEDControl();
  assert(!cacheAdapter.isGifActive());
  assert(cacheMedia.stopCount == 1);

  // A transient failure while replacing an already active cached GIF must not
  // restore an empty iDotMatrix effect.  The old cache is retired once
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
  assert(strip.segmentRef().mode == replaceAdapter.displayEffectId());
  assert(strip.segmentRef().colors[0] == BLACK);
  replaceAdapter.syncGifPlayback(false, true);
  assert(!replaceAdapter.isGifActive());
  assert(!replaceAdapter.isDisplayEffectActive());
  assert(strip.segmentRef().mode == FX_MODE_STATIC);
  // Even failure recovery must undo the temporary black staging colour.
  assert(strip.segmentRef().colors[0] == RGBW32(0x21, 0x43, 0x65, 0));

  // The next replacement attempt must still be able to stage and publish.
  assert(replaceAdapter.onGifComplete(true));
  assert(strip.segmentRef().mode == replaceAdapter.displayEffectId());
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

  // Entering Carousel must take framebuffer ownership immediately.  A cold
  // stored-GIF cache can take noticeable time to build; Clock must not keep
  // rendering during that asynchronous staging window.
  clockAdapter.beginCarouselPlayback();
  assert(!clockAdapter.isClockActive());
  assert(!renderer.isVisible());
  clockAdapter.restoreClockFallback();
  assert(clockAdapter.isClockActive());
  assert(renderer.isVisible());

  // RC6 regression: replacing one Carousel with another must retain logical
  // iDotMatrix ownership while the old bank has been removed and the first new
  // asset is still uploading. Otherwise the standalone policy sees an empty
  // iDotMatrix effect and flashes Clock for the upload gap.
  // Prime the selection edge first so only a true empty-content regression can
  // request another standalone activation.
  clockAdapter.pollDisplayEffectSelection();
  (void)clockAdapter.takeDisplayEffectActivationRequest();
  clockAdapter.beginCarouselUpdateHold();
  assert(clockAdapter.isCarouselUpdateHoldActive());
  assert(clockAdapter.hasLogicalContent());
  assert(!clockAdapter.isClockActive());
  assert(!renderer.isVisible());

  // 0.9.0-dev.6: Carousel uploads get a delayed procedural status frame.
  // Short assets must not flash the indicator; after the threshold it becomes
  // visible and progresses without surrendering iDotMatrix ownership.
  testMillis = 1000;
  clockAdapter.beginTransferIndicator(1000, 250);
  assert(clockAdapter.isTransferIndicatorActive());
  clockAdapter.updateTransferIndicator(500, 1000);
  strip.renderEffect();
  assert(!renderer.isVisible());
  testMillis = 1300;
  strip.renderEffect();
  assert(renderer.isVisible());
  bool transferPixelSeen = false;
  for (uint8_t y = 0; y < renderer.height() && !transferPixelSeen; ++y) {
    for (uint8_t x = 0; x < renderer.width(); ++x) {
      const auto* pixel = renderer.pixel(x, y);
      if (pixel && (pixel->red || pixel->green || pixel->blue)) {
        transferPixelSeen = true;
        break;
      }
    }
  }
  assert(transferPixelSeen);

  // 0.9.0-dev.11: setup count is not the real session total. The 16x16
  // status bar is therefore indeterminate: a short segment sweeps left/right
  // until the quiet-period confirms completion. The canonical artwork keeps
  // the blue tray and downward-only red arrow separated from the bar.
  clockAdapter.endTransferIndicator();
  assert(renderer.begin(0x01));
  testMillis = 2000;
  clockAdapter.beginTransferIndicator(1000, 0, 1, 12);
  clockAdapter.updateTransferIndicator(500, 1000);
  strip.renderEffect();
  // 0.9.0-dev.12 regression: the adapter loop must keep requesting WLED
  // redraws while the activity UI is visible; BLE chunks are not the clock.
  const uint32_t refreshTriggersBefore = stripTriggerCount;
  testMillis = 2071;
  clockAdapter.loop(testMillis);
  assert(stripTriggerCount > refreshTriggersBefore);
  testMillis = 2000;
  const auto* trayPixel = renderer.pixel(2, 10);
  assert(trayPixel && trayPixel->blue == 235 && trayPixel->red == 35);
  bool redArrowSeen = false;
  for (uint8_t y = 0; y < 15 && !redArrowSeen; ++y) {
    for (uint8_t x = 0; x < 16; ++x) {
      const auto* pixel = renderer.pixel(x, y);
      if (pixel && pixel->red == 236 && pixel->green == 28 && pixel->blue == 36) {
        redArrowSeen = true;
        break;
      }
    }
  }
  assert(redArrowSeen);
  uint8_t activeBarPixelsA = 0;
  uint8_t firstActiveA = 0xFF;
  for (uint8_t x = 1; x <= 14; ++x) {
    const auto* pixel = renderer.pixel(x, 15);
    assert(pixel);
    if (pixel->green == 220) {
      if (firstActiveA == 0xFF) firstActiveA = x;
      ++activeBarPixelsA;
    }
  }
  assert(activeBarPixelsA == 4);
  testMillis = 2450;
  strip.renderEffect();
  uint8_t activeBarPixelsB = 0;
  uint8_t firstActiveB = 0xFF;
  for (uint8_t x = 1; x <= 14; ++x) {
    const auto* pixel = renderer.pixel(x, 15);
    assert(pixel);
    if (pixel->green == 220) {
      if (firstActiveB == 0xFF) firstActiveB = x;
      ++activeBarPixelsB;
    }
  }
  assert(activeBarPixelsB == 4);
  assert(firstActiveA != firstActiveB);
  // Completion is represented by retiring the indeterminate indicator; there
  // is intentionally no separate determinate/100% rendering state.

  // 0.9.0-dev.10: native 64x64 transfer art must use the dedicated
  // smoother rendering rather than a raw 4x copy of the 16x16 glyph.
  clockAdapter.endTransferIndicator();
  assert(renderer.begin(0x04));
  testMillis = 3000;
  clockAdapter.beginTransferIndicator(1000, 0, 0, 0);
  clockAdapter.updateTransferIndicator(500, 1000);
  strip.renderEffect();
  const auto* tray64 = renderer.pixel(16, 47);
  assert(tray64 && tray64->blue == 255 && tray64->green == 176);
  bool red64Seen = false;
  for (uint8_t y = 0; y < 39 && !red64Seen; ++y) {
    for (uint8_t x = 20; x < 44; ++x) {
      const auto* pixel = renderer.pixel(x, y);
      if (pixel && pixel->red >= 238 && pixel->green <= 92) {
        red64Seen = true;
        break;
      }
    }
  }
  assert(red64Seen);
  uint8_t active64 = 0;
  for (uint8_t x = 8; x <= 55; ++x) {
    const auto* pixel = renderer.pixel(x, 59);
    if (pixel && pixel->green == 220) ++active64;
  }
  assert(active64 == 12);
  // Starting the next Carousel slot resets current bytes but must not hide an
  // already-visible indicator or restart the anti-flash delay.
  clockAdapter.beginTransferIndicator(2000, 250);
  clockAdapter.updateTransferIndicator(100, 2000);
  strip.renderEffect();
  assert(renderer.isVisible());
  clockAdapter.endTransferIndicator();
  assert(!clockAdapter.isTransferIndicatorActive());

  clockAdapter.pollDisplayEffectSelection();
  assert(!clockAdapter.takeDisplayEffectActivationRequest());
  clockAdapter.endCarouselUpdateHold();
  assert(!clockAdapter.isCarouselUpdateHoldActive());
  assert(!clockAdapter.hasLogicalContent());
  clockAdapter.pollDisplayEffectSelection();
  assert(clockAdapter.takeDisplayEffectActivationRequest());
  clockAdapter.restoreClockFallback();
  assert(clockAdapter.isClockActive());
  assert(renderer.isVisible());

  // Every stored GIF staging transition must take exclusive ownership of the
  // framebuffer, not only the initial Carousel entry.  This reproduces the
  // hardware regression where an animated/rainbow text item immediately
  // before a GIF kept rendering while the GIF cache was built, contaminating
  // the cached frames with text pixels.
  IDotMatrixTextSettings stagingText{};
  stagingText.glyphCount = 1;
  stagingText.glyphWidth = 8;
  stagingText.glyphHeight = 16;
  stagingText.glyphBytes = 16;
  stagingText.colorMode = 2;
  uint8_t stagingGlyph[16];
  for (uint8_t& value : stagingGlyph) value = 0xFF;
  assert(clockAdapter.onTextBegin(stagingText));
  clockAdapter.onTextGlyph(0, stagingGlyph, sizeof(stagingGlyph));
  clockAdapter.onTextComplete();
  assert(clockAdapter.isTextActive());
  assert(renderer.isVisible());
  assert(clockAdapter.playStoredGif("/slot.gif", "/slot.cache"));
  assert(clockMedia.storedQueueCount == 1);
  assert(clockAdapter.isGifPending());
  assert(!clockAdapter.isTextActive());
  assert(!clockAdapter.isClockActive());
  assert(!renderer.isVisible());
  // A preparation failure restores the logical previous owner.
  clockAdapter.syncGifPlayback(false, true);
  assert(clockAdapter.isTextActive());
  assert(renderer.isVisible());

  // RC5 regression: Carousel storage mutation must close a currently queued
  // GIF/cache before the Carousel deletes its files, without disturbing an
  // unrelated live Clock/Text renderer when no GIF owns the media backend.
  clockAdapter.onClock(clockSettings);
  const uint32_t stopBeforeClockMutation = clockMedia.stopCount;
  clockAdapter.releaseCarouselMediaForStorageMutation();
  assert(clockMedia.stopCount == stopBeforeClockMutation);
  assert(clockAdapter.isClockActive());
  assert(renderer.isVisible());

  assert(clockAdapter.playStoredGif("/reset-slot.gif", "/reset-slot.cache"));
  assert(clockAdapter.isGifPending());
  const uint32_t stopBeforeGifMutation = clockMedia.stopCount;
  clockAdapter.releaseCarouselMediaForStorageMutation();
  assert(clockMedia.stopCount == stopBeforeGifMutation + 1);
  assert(!clockAdapter.isGifPending());
  assert(!clockAdapter.isGifActive());
  assert(!renderer.isVisible());

  clockAdapter.onClock(clockSettings);
  assert(clockAdapter.isClockActive());
  assert(clockAdapter.onGifComplete(true));
  assert(strip.segmentRef().mode == clockAdapter.displayEffectId());
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
  assert(strip.segmentRef().mode == lightAdapter.displayEffectId());
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
  assert(strip.segmentRef().mode == cancelAdapter.displayEffectId());
  assert(strip.segmentRef().colors[0] == BLACK);
  strip.segmentRef().setMode(43);
  cancelAdapter.syncWLEDControl();
  assert(strip.segmentRef().mode == 43);
  assert(strip.segmentRef().colors[0] == RGBW32(0x41, 0x42, 0x43, 0));
  assert(cancelMedia.stopCount == 1);

}
