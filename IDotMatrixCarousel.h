#pragma once

#include <cstddef>
#include <cstdint>
#include "IDotMatrixProtocol.h"

class IDotMatrixWLEDAdapter;

class IDotMatrixCarousel final : public IDotMatrixCarouselEvents {
public:
  static constexpr uint8_t SLOT_COUNT = 12;
  static constexpr uint8_t TYPE_GIF = 0x01;
  static constexpr uint8_t TYPE_TEXT = 0x03;

  IDotMatrixCarousel(IDotMatrixProtocol& protocol, IDotMatrixWLEDAdapter& adapter);

  void begin();
  void loop(uint32_t now);
  void onPlaybackFailure(uint32_t now);

  void resetPersistent();
  void configure(const uint8_t* slots, uint8_t count);
  void enter();
  void suspend();
  void requestAutoStart(uint32_t now, uint32_t delayMs = 750);

  void onCarouselReset() override { resetPersistent(); }
  void onCarouselConfigure(const uint8_t* slots, uint8_t count) override { configure(slots, count); }
  void onCarouselEnter() override { enter(); }
  void onCarouselSuspend() override { suspend(); }

  bool beginAsset(uint8_t type, uint8_t slot, uint16_t dwellSeconds, size_t totalLength);
  bool writeAsset(size_t offset, const uint8_t* data, size_t length);
  bool completeAsset(bool crcValid);
  void cancelAsset();
  bool onCarouselAssetBegin(uint8_t type, uint8_t slot, uint16_t dwellSeconds, size_t totalLength) override { return beginAsset(type, slot, dwellSeconds, totalLength); }
  bool onCarouselAssetData(size_t offset, const uint8_t* data, size_t length) override { return writeAsset(offset, data, length); }
  bool onCarouselAssetComplete(bool crcValid) override { return completeAsset(crcValid); }
  void onCarouselAssetCancel() override { cancelAsset(); }

  bool hasAssets() const;
  bool playing() const { return playing_; }
  bool resumeOnBoot() const { return resumeOnBoot_; }
  uint8_t configuredCount() const { return configuredCount_; }
  uint8_t storedCount() const;
  int8_t currentSlot() const { return currentSlot_; }
  uint16_t currentDwellSeconds() const;
  bool autoStartPending() const { return autoStartPending_; }
  bool updateHoldActive() const { return updateHoldActive_; }
  uint16_t failedMask() const { return failedMask_; }
  int8_t lastFailedSlot() const { return lastFailedSlot_; }
  bool lastResetOk() const { return lastResetOk_; }

private:
  struct SlotMeta {
    uint8_t valid = 0;
    uint8_t type = 0;
    uint16_t dwellSeconds = 0;
    uint32_t bytes = 0;
  };

  struct Manifest {
    uint8_t magic[4];
    uint8_t version;
    uint8_t configuredCount;
    uint8_t order[SLOT_COUNT];
    uint8_t resumeOnBoot;
    uint8_t reserved[3];
    SlotMeta slots[SLOT_COUNT];
  };

  bool loadManifest();
  bool saveManifest();
  void resetManifest();
  void clearFiles();
  bool playNext(uint32_t now, bool first);
  bool playSlot(uint8_t slot, uint32_t now);
  void startUpdateHold(uint32_t now);
  void touchUpdateHold(uint32_t now);
  void endUpdateHold();
  int8_t findNextPlayable(int8_t from) const;
  static void slotPath(uint8_t slot, uint8_t type, char* out, size_t outSize);
  static void backupPath(uint8_t slot, uint8_t type, char* out, size_t outSize);
  static void cachePath(uint8_t slot, char* out, size_t outSize);
  static bool copyFile(const char* from, const char* to);

  IDotMatrixProtocol& protocol_;
  IDotMatrixWLEDAdapter& adapter_;
  Manifest manifest_{};
  bool playing_ = false;
  bool resumeOnBoot_ = false;
  uint8_t configuredCount_ = 0;
  int8_t currentOrderPos_ = -1;
  int8_t currentSlot_ = -1;
  uint32_t nextSwitchAt_ = 0;
  bool autoStartPending_ = false;
  uint32_t autoStartAt_ = 0;
  bool updateHoldActive_ = false;
  uint32_t updateHoldDeadline_ = 0;
  static constexpr uint32_t UPDATE_HOLD_TIMEOUT_MS = 8000u;

  bool rxOpen_ = false;
  uint8_t rxType_ = 0;
  uint8_t rxSlot_ = 0;
  uint16_t rxDwell_ = 0;
  size_t rxExpected_ = 0;
  size_t rxWritten_ = 0;
  uint16_t failedMask_ = 0;
  int8_t lastFailedSlot_ = -1;
  bool lastResetOk_ = true;
};
