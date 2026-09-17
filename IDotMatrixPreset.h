#pragma once

#include <cstddef>
#include <cstdint>
#include "IDotMatrixProtocol.h"

class IDotMatrixWLEDAdapter;

class IDotMatrixPreset final : public IDotMatrixPresetEvents {
public:
  static constexpr uint8_t SLOT_COUNT = 6;
  static constexpr uint8_t PROTOCOL_SLOT_FIRST = 14;
  static constexpr uint8_t PROTOCOL_SLOT_LAST = 19;
  static constexpr uint8_t TYPE_GIF = 0x01;
  static constexpr uint8_t TYPE_TEXT = 0x03;

  IDotMatrixPreset(IDotMatrixProtocol& protocol, IDotMatrixWLEDAdapter& adapter);

  void begin();
  void loop(uint32_t now);
  void reset();
  void suspend();
  bool playing() const { return playing_; }
  int8_t currentProtocolSlot() const;
  uint8_t activeCount() const { return activeCount_; }
  uint8_t pendingCount() const;
  uint32_t currentHoldMs() const { return currentHoldMs_; }
  bool uploadIndicatorActive() const { return uploadIndicatorActive_; }

  void onPresetReset() override { reset(); }
  void onPresetActivate(const uint8_t* slots, uint8_t count) override;
  void onPresetSuspend() override { suspend(); }
  bool onPresetAssetBegin(uint8_t type, uint8_t protocolSlot, uint16_t timeSign, size_t totalLength) override;
  bool onPresetAssetData(size_t offset, const uint8_t* data, size_t length) override;
  bool onPresetAssetComplete(bool crcValid) override;
  void onPresetAssetCancel() override;

private:
  struct SlotMeta {
    bool valid = false;
    uint8_t type = 0;
    uint16_t timeSign = 0;
    uint32_t bytes = 0;
  };

  static bool protocolSlotToLocal(uint8_t protocolSlot, uint8_t& localSlot);
  static void activePath(uint8_t localSlot, char* out, size_t outSize);
  static void pendingPath(uint8_t localSlot, char* out, size_t outSize);
  static void cachePath(uint8_t localSlot, char* out, size_t outSize);
  static void backupPath(uint8_t localSlot, char* out, size_t outSize);
  void clearFiles();
  bool activateTransactional(const uint8_t* mapped, uint8_t count);
  bool playPosition(uint8_t position, uint32_t now);
  bool playCurrent(uint32_t now);
  void finishUploadIndicator(uint32_t now, bool resumeCurrent);

  IDotMatrixProtocol& protocol_;
  IDotMatrixWLEDAdapter& adapter_;
  SlotMeta active_[SLOT_COUNT]{};
  SlotMeta pending_[SLOT_COUNT]{};
  uint8_t order_[SLOT_COUNT]{};
  uint8_t activeCount_ = 0;
  int8_t position_ = -1;
  bool playing_ = false;
  uint32_t entryStartedAt_ = 0;
  uint32_t currentHoldMs_ = 0;

  bool rxOpen_ = false;
  uint8_t rxSlot_ = 0;
  uint8_t rxType_ = 0;
  uint16_t rxTimeSign_ = 0;
  size_t rxExpected_ = 0;
  size_t rxWritten_ = 0;

  static constexpr uint32_t UPLOAD_IDLE_TIMEOUT_MS = 5000u;
  bool uploadIndicatorActive_ = false;
  uint32_t uploadLastProgressAt_ = 0;
};
