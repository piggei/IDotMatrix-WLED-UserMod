#include "../IDotMatrixPreset.h"
#include "preset_stub/IDotMatrixPresetDeps.h"
#include "wled.h"

#include <cassert>
#include <cstdint>
#include <vector>

TestFS WLED_FS;

class DummyEvents final : public IDotMatrixProtocolEvents {
public:
  void onDeviceReset() override {}
  void onScreenPower(bool) override {}
  void onBrightnessPercent(uint8_t) override {}
  void onSolidColor(uint8_t, uint8_t, uint8_t) override {}
  void onLightEffect(const IDotMatrixLightEffectSettings&) override {}
  void onAudio(const IDotMatrixAudioSettings&) override {}
  void onGraffitiMode(bool) override {}
  void onGraffitiPixels(uint8_t, uint8_t, uint8_t, const uint8_t*, size_t) override {}
  bool onGraffitiRasterBegin(size_t) override { return true; }
  bool onGraffitiRasterData(size_t, const uint8_t*, size_t) override { return true; }
  bool onGraffitiRasterComplete(bool valid) override { return valid; }
  void onClock(const IDotMatrixClockSettings&) override {}
  void onCountdown(const IDotMatrixCountdownSettings&) override {}
  void onStopwatch(uint8_t) override {}
  void onScoreboard(uint16_t, uint16_t) override {}
  bool takeCountdownFinished() override { return false; }
  int textBegins = 0;
  int textGlyphs = 0;
  IDotMatrixTextSettings lastText{};
  bool onTextBegin(const IDotMatrixTextSettings& settings) override { ++textBegins; lastText = settings; return true; }
  void onTextGlyph(uint8_t, const uint8_t*, size_t) override { ++textGlyphs; }
  void onTextComplete() override {}
  bool onRawImageBegin(size_t) override { return true; }
  bool onRawImageData(size_t, const uint8_t*, size_t) override { return true; }
  bool onRawImageComplete(bool ok) override { return ok; }
  bool onPngImage(const uint8_t*, size_t) override { return true; }
  bool onGifBegin(size_t) override { return true; }
  bool onGifData(size_t, const uint8_t*, size_t) override { return true; }
  bool onGifComplete(bool ok) override { return ok; }
};


static std::vector<uint8_t> readFile(const char* path) {
  File f = WLED_FS.open(path, "r");
  assert(f);
  std::vector<uint8_t> out(f.size());
  if (!out.empty()) assert(f.read(out.data(), out.size()) == out.size());
  f.close();
  return out;
}

static bool stageGif(IDotMatrixPreset& preset, uint8_t protocolSlot, uint8_t marker) {
  const uint8_t bytes[] = {marker, uint8_t(marker + 1u), uint8_t(marker + 2u)};
  if (!preset.onPresetAssetBegin(IDotMatrixPreset::TYPE_GIF, protocolSlot, 5, sizeof(bytes))) return false;
  if (!preset.onPresetAssetData(0, bytes, sizeof(bytes))) return false;
  return preset.onPresetAssetComplete(true);
}

static void activate(IDotMatrixPreset& preset, const std::vector<uint8_t>& slots) {
  preset.onPresetActivate(slots.data(), uint8_t(slots.size()));
}

static void expectBytes(const char* path, uint8_t marker) {
  const auto bytes = readFile(path);
  assert(bytes.size() == 3);
  assert(bytes[0] == marker && bytes[1] == uint8_t(marker + 1u) && bytes[2] == uint8_t(marker + 2u));
}

static std::vector<uint8_t> makeMaxFont64Text() {
  constexpr size_t header = 14;
  constexpr size_t record = 260;
  std::vector<uint8_t> data(header + 64u * record, 0);
  data[0] = 64;
  data[4] = 0; // non-scroll effect
  data[5] = 50;
  data[6] = 1;
  data[7] = 0xFF;
  for (size_t glyph = 0; glyph < 64u; ++glyph) {
    const size_t base = header + glyph * record;
    data[base] = 0x08; // 32x64 family marker
    data[base + 4u + (glyph % 256u)] = uint8_t(glyph + 1u);
  }
  assert(data.size() == 16654u);
  return data;
}

int main() {
  DummyEvents events;
  IDotMatrixProtocol protocol(events);
  IDotMatrixWLEDAdapter adapter;
  IDotMatrixPreset preset(protocol, adapter);

  // Normal multi-slot activation.
  WLED_FS.clear();
  preset.begin();
  assert(stageGif(preset, 14, 10));
  assert(stageGif(preset, 15, 20));
  assert(stageGif(preset, 16, 30));
  activate(preset, {14, 15, 16});
  assert(preset.playing());
  assert(preset.activeCount() == 3);
  assert(preset.pendingCount() == 0);
  expectBytes("/pre0.bin", 10);
  expectBytes("/pre1.bin", 20);
  expectBytes("/pre2.bin", 30);

  // Backup failure after an earlier slot was already backed up: old bank must
  // be restored completely and all new files must remain pending.
  assert(stageGif(preset, 14, 40));
  assert(stageGif(preset, 15, 50));
  assert(stageGif(preset, 16, 60));
  WLED_FS.failRename("/pre1.bin", "/pre1.bak");
  activate(preset, {14, 15, 16});
  assert(preset.activeCount() == 3);
  assert(preset.pendingCount() == 3);
  expectBytes("/pre0.bin", 10);
  expectBytes("/pre1.bin", 20);
  expectBytes("/pre2.bin", 30);
  expectBytes("/pre0.tmp", 40);
  expectBytes("/pre1.tmp", 50);
  expectBytes("/pre2.tmp", 60);
  assert(!WLED_FS.exists("/pre0.bak"));

  // Intermediate promotion failure: already promoted new files return to
  // pending names and every old active file is restored.
  WLED_FS.resetFailures();
  WLED_FS.failRename("/pre1.tmp", "/pre1.bin");
  activate(preset, {14, 15, 16});
  assert(preset.activeCount() == 3);
  assert(preset.pendingCount() == 3);
  expectBytes("/pre0.bin", 10);
  expectBytes("/pre1.bin", 20);
  expectBytes("/pre2.bin", 30);
  expectBytes("/pre0.tmp", 40);
  expectBytes("/pre1.tmp", 50);
  expectBytes("/pre2.tmp", 60);
  assert(!WLED_FS.exists("/pre0.bak"));
  assert(!WLED_FS.exists("/pre1.bak"));
  assert(!WLED_FS.exists("/pre2.bak"));

  // Retry after the transient failure must commit the complete new bank.
  WLED_FS.resetFailures();
  activate(preset, {14, 15, 16});
  assert(preset.pendingCount() == 0);
  expectBytes("/pre0.bin", 40);
  expectBytes("/pre1.bin", 50);
  expectBytes("/pre2.bin", 60);

  // Write failure and CRC failure never touch the active bank.
  const uint8_t one[] = {99};
  assert(preset.onPresetAssetBegin(IDotMatrixPreset::TYPE_GIF, 14, 5, sizeof(one)));
  WLED_FS.failWrite("/pre0.tmp");
  assert(!preset.onPresetAssetData(0, one, sizeof(one)));
  preset.onPresetAssetCancel();
  expectBytes("/pre0.bin", 40);

  assert(preset.onPresetAssetBegin(IDotMatrixPreset::TYPE_GIF, 14, 5, sizeof(one)));
  assert(preset.onPresetAssetData(0, one, sizeof(one)));
  assert(!preset.onPresetAssetComplete(false));
  expectBytes("/pre0.bin", 40);


  // Full original-device 64-glyph 32x64 TEXT must stage and play from Preset.
  WLED_FS.resetFailures();
  const auto largeText = makeMaxFont64Text();
  assert(preset.onPresetAssetBegin(IDotMatrixPreset::TYPE_TEXT, 14, 5, largeText.size()));
  assert(preset.onPresetAssetData(0, largeText.data(), largeText.size()));
  assert(preset.onPresetAssetComplete(true));
  const int beginsBefore = events.textBegins;
  const int glyphsBefore = events.textGlyphs;
  activate(preset, {14});
  assert(events.textBegins == beginsBefore + 1);
  assert(events.textGlyphs == glyphsBefore + 64);
  assert(events.lastText.glyphWidth == 32 && events.lastText.glyphHeight == 64);
  assert(events.lastText.glyphBytes == 256);
  assert(!preset.onPresetAssetBegin(IDotMatrixPreset::TYPE_TEXT, 15, 5, largeText.size() + 1u));

  // Preset is intentionally volatile. begin() models reboot/startup and must
  // clean active, pending, cache and transaction-backup files rather than
  // recover the previous session.
  preset.begin();
  assert(!preset.playing());
  assert(preset.activeCount() == 0);
  assert(preset.pendingCount() == 0);
  for (int i = 0; i < 6; ++i) {
    const std::string n = std::to_string(i);
    assert(!WLED_FS.exists(("/pre" + n + ".bin").c_str()));
    assert(!WLED_FS.exists(("/pre" + n + ".tmp").c_str()));
    assert(!WLED_FS.exists(("/pre" + n + ".bak").c_str()));
    assert(!WLED_FS.exists(("/pre" + n + ".cac").c_str()));
  }

  return 0;
}
