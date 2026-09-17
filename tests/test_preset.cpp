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
  void onClock(const IDotMatrixClockSettings&) override {}
  void onCountdown(const IDotMatrixCountdownSettings&) override {}
  void onStopwatch(uint8_t) override {}
  void onScoreboard(uint16_t, uint16_t) override {}
  bool takeCountdownFinished() override { return false; }
  bool onTextBegin(const IDotMatrixTextSettings&) override { return true; }
  void onTextGlyph(uint8_t, const uint8_t*, size_t) override {}
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
