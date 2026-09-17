#include "../IDotMatrixCarousel.h"
#include "carousel_stub/IDotMatrixCarouselDeps.h"
#include "wled.h"
#include <cassert>
#include <cstdint>
#include <vector>

TestFS WLED_FS;

class DummyEvents final : public IDotMatrixProtocolEvents {
public:
  void onDeviceReset() override {} void onScreenPower(bool) override {} void onBrightnessPercent(uint8_t) override {}
  void onSolidColor(uint8_t,uint8_t,uint8_t) override {} void onLightEffect(const IDotMatrixLightEffectSettings&) override {}
  void onAudio(const IDotMatrixAudioSettings&) override {} void onGraffitiMode(bool) override {}
  void onGraffitiPixels(uint8_t,uint8_t,uint8_t,const uint8_t*,size_t) override {} void onClock(const IDotMatrixClockSettings&) override {}
  void onCountdown(const IDotMatrixCountdownSettings&) override {} void onStopwatch(uint8_t) override {}
  void onScoreboard(uint16_t,uint16_t) override {} bool takeCountdownFinished() override { return false; }
  bool onTextBegin(const IDotMatrixTextSettings&) override { return true; } void onTextGlyph(uint8_t,const uint8_t*,size_t) override {}
  void onTextComplete() override {} bool onRawImageBegin(size_t) override { return true; }
  bool onRawImageData(size_t,const uint8_t*,size_t) override { return true; } bool onRawImageComplete(bool ok) override { return ok; }
  bool onPngImage(const uint8_t*,size_t) override { return true; } bool onGifBegin(size_t) override { return true; }
  bool onGifData(size_t,const uint8_t*,size_t) override { return true; } bool onGifComplete(bool ok) override { return ok; }
};

static bool uploadGif(IDotMatrixCarousel& c, uint8_t slot, uint8_t marker) {
  const uint8_t data[] = {marker, uint8_t(marker+1), uint8_t(marker+2)};
  if (!c.beginAsset(IDotMatrixCarousel::TYPE_GIF, slot, 5, sizeof(data))) return false;
  if (!c.writeAsset(0, data, sizeof(data))) return false;
  return c.completeAsset(true);
}

static uint8_t firstByte(const char* path) {
  File f = WLED_FS.open(path,"r"); assert(f); uint8_t b=0; assert(f.read(&b,1)==1); f.close(); return b;
}

int main() {
  DummyEvents events; IDotMatrixProtocol protocol(events); IDotMatrixWLEDAdapter adapter; IDotMatrixCarousel c(protocol, adapter);
  WLED_FS.clear(); c.begin();
  const uint8_t order[] = {0,1}; c.configure(order,2); assert(c.lastManifestSaveOk());
  assert(uploadGif(c,0,10)); assert(c.storedCount()==1); assert(firstByte("/idot_a0.gif")==10);

  // Manifest write failure is surfaced and replacement rolls back to old asset.
  const uint8_t data[] = {20,21,22};
  assert(c.beginAsset(IDotMatrixCarousel::TYPE_GIF,0,5,sizeof(data)));
  assert(c.writeAsset(0,data,sizeof(data)));
  WLED_FS.failWrite("/idot_car.tmp");
  assert(!c.completeAsset(true));
  assert(!c.lastManifestSaveOk());
  assert(firstByte("/idot_a0.gif")==10);

  // A subsequent successful replacement clears the manifest failure state.
  WLED_FS.resetFailures(); assert(uploadGif(c,0,30)); assert(c.lastManifestSaveOk()); assert(firstByte("/idot_a0.gif")==30);

  // Configure also reports manifest persistence failure instead of silently ignoring it.
  WLED_FS.failWrite("/idot_car.tmp"); c.configure(order,2); assert(!c.lastManifestSaveOk());
  return 0;
}
