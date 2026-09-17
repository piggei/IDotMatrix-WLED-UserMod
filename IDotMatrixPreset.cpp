#include "IDotMatrixPreset.h"
#if defined(IDOT_PRESET_HOST_TEST)
#include "tests/preset_stub/IDotMatrixPresetDeps.h"
#else
#include "IDotMatrixWLEDAdapter.h"
#endif
#include "wled.h"

#include <cstdio>

IDotMatrixPreset::IDotMatrixPreset(IDotMatrixProtocol& protocol, IDotMatrixWLEDAdapter& adapter)
  : protocol_(protocol), adapter_(adapter) {}

bool IDotMatrixPreset::protocolSlotToLocal(uint8_t protocolSlot, uint8_t& localSlot) {
  if (protocolSlot < PROTOCOL_SLOT_FIRST || protocolSlot > PROTOCOL_SLOT_LAST) return false;
  localSlot = uint8_t(protocolSlot - PROTOCOL_SLOT_FIRST);
  return true;
}

void IDotMatrixPreset::activePath(uint8_t localSlot, char* out, size_t outSize) {
  snprintf(out, outSize, "/pre%u.bin", unsigned(localSlot));
}
void IDotMatrixPreset::pendingPath(uint8_t localSlot, char* out, size_t outSize) {
  snprintf(out, outSize, "/pre%u.tmp", unsigned(localSlot));
}
void IDotMatrixPreset::cachePath(uint8_t localSlot, char* out, size_t outSize) {
  snprintf(out, outSize, "/pre%u.cac", unsigned(localSlot));
}
void IDotMatrixPreset::backupPath(uint8_t localSlot, char* out, size_t outSize) {
  snprintf(out, outSize, "/pre%u.bak", unsigned(localSlot));
}

void IDotMatrixPreset::clearFiles() {
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) {
    char path[20];
    activePath(i, path, sizeof(path)); WLED_FS.remove(path);
    pendingPath(i, path, sizeof(path)); WLED_FS.remove(path);
    cachePath(i, path, sizeof(path)); WLED_FS.remove(path);
    backupPath(i, path, sizeof(path)); WLED_FS.remove(path);
  }
}

void IDotMatrixPreset::begin() {
  // Preset/Default is intentionally volatile: no boot restore, journaling or NVS.
  // reset() removes active, pending, cache and transaction-backup files so a
  // reboot always starts with an empty Preset bank.
  reset();
}

void IDotMatrixPreset::reset() {
  finishUploadIndicator(millis(), false);
  suspend();
  clearFiles();
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) {
    active_[i] = SlotMeta{};
    pending_[i] = SlotMeta{};
    order_[i] = 0;
  }
  activeCount_ = 0;
  rxOpen_ = false;
  rxExpected_ = rxWritten_ = 0;
}

void IDotMatrixPreset::suspend() {
  finishUploadIndicator(millis(), false);
  playing_ = false;
  position_ = -1;
  entryStartedAt_ = 0;
  currentHoldMs_ = 0;
}

void IDotMatrixPreset::finishUploadIndicator(uint32_t now, bool resumeCurrent) {
  if (!uploadIndicatorActive_) return;
  adapter_.endTransferIndicator();
  uploadIndicatorActive_ = false;
  uploadLastProgressAt_ = 0;
  if (resumeCurrent && playing_ && activeCount_ > 0 && position_ >= 0) {
    playCurrent(now);
  }
}

uint8_t IDotMatrixPreset::pendingCount() const {
  uint8_t count = 0;
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) if (pending_[i].valid) ++count;
  return count;
}

int8_t IDotMatrixPreset::currentProtocolSlot() const {
  if (!playing_ || position_ < 0 || uint8_t(position_) >= activeCount_) return -1;
  return int8_t(PROTOCOL_SLOT_FIRST + order_[uint8_t(position_)]);
}

bool IDotMatrixPreset::onPresetAssetBegin(uint8_t type, uint8_t protocolSlot, uint16_t timeSign, size_t totalLength) {
  uint8_t local = 0;
  if (!protocolSlotToLocal(protocolSlot, local) || (type != TYPE_GIF && type != TYPE_TEXT) || totalLength == 0) return false;
  char path[20];
  pendingPath(local, path, sizeof(path));
  WLED_FS.remove(path);
  File file = WLED_FS.open(path, "w");
  if (!file) return false;
  file.close();
  pending_[local] = SlotMeta{};
  rxOpen_ = true;
  rxSlot_ = local;
  rxType_ = type;
  rxTimeSign_ = timeSign;
  rxExpected_ = totalLength;
  rxWritten_ = 0;
  uploadIndicatorActive_ = true;
  uploadLastProgressAt_ = millis();
  adapter_.beginTransferIndicator(totalLength, 250u, 0, 0);
  return true;
}

bool IDotMatrixPreset::onPresetAssetData(size_t offset, const uint8_t* data, size_t length) {
  if (!rxOpen_ || data == nullptr || offset != rxWritten_ || rxWritten_ + length > rxExpected_) return false;
  char path[20]; pendingPath(rxSlot_, path, sizeof(path));
  File file = WLED_FS.open(path, "a");
  if (!file) return false;
  const size_t written = file.write(data, length);
  file.close();
  if (written != length) return false;
  rxWritten_ += written;
  uploadLastProgressAt_ = millis();
  adapter_.updateTransferIndicator(rxWritten_, rxExpected_);
  return true;
}

bool IDotMatrixPreset::onPresetAssetComplete(bool crcValid) {
  if (!rxOpen_) return false;
  const uint8_t slot = rxSlot_;
  const bool ok = crcValid && rxWritten_ == rxExpected_;
  if (ok) {
    pending_[slot].valid = true;
    pending_[slot].type = rxType_;
    pending_[slot].timeSign = rxTimeSign_;
    pending_[slot].bytes = uint32_t(rxWritten_);
  } else {
    char path[20]; pendingPath(slot, path, sizeof(path)); WLED_FS.remove(path);
    pending_[slot] = SlotMeta{};
  }
  rxOpen_ = false;
  rxExpected_ = rxWritten_ = 0;
  uploadLastProgressAt_ = millis();
  if (!ok) finishUploadIndicator(uploadLastProgressAt_, true);
  return ok;
}

void IDotMatrixPreset::onPresetAssetCancel() {
  if (rxOpen_) {
    char path[20]; pendingPath(rxSlot_, path, sizeof(path)); WLED_FS.remove(path);
    pending_[rxSlot_] = SlotMeta{};
  }
  rxOpen_ = false;
  rxExpected_ = rxWritten_ = 0;
  finishUploadIndicator(millis(), true);
}

bool IDotMatrixPreset::activateTransactional(const uint8_t* mapped, uint8_t count) {
  bool replace[SLOT_COUNT]{};
  bool hadActive[SLOT_COUNT]{};

  for (uint8_t i = 0; i < count; ++i) {
    const uint8_t slot = mapped[i];
    if (slot >= SLOT_COUNT) return false;
    for (uint8_t j = 0; j < i; ++j) if (mapped[j] == slot) return false;
    if (!pending_[slot].valid && !active_[slot].valid) return false;
    replace[slot] = pending_[slot].valid;
  }

  // Phase 1: move every active file that will be replaced to a transaction
  // backup. Pending files are not touched until all backups are ready.
  for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
    if (!replace[slot]) continue;
    char active[20], backup[20];
    activePath(slot, active, sizeof(active));
    backupPath(slot, backup, sizeof(backup));
    WLED_FS.remove(backup);
    hadActive[slot] = active_[slot].valid && WLED_FS.exists(active);
    if (hadActive[slot] && !WLED_FS.rename(active, backup)) {
      // Restore any backups already prepared. Pending files remain staged.
      for (uint8_t restore = 0; restore < slot; ++restore) {
        if (!replace[restore] || !hadActive[restore]) continue;
        char oldActive[20], oldBackup[20];
        activePath(restore, oldActive, sizeof(oldActive));
        backupPath(restore, oldBackup, sizeof(oldBackup));
        if (WLED_FS.exists(oldBackup)) WLED_FS.rename(oldBackup, oldActive);
      }
      return false;
    }
  }

  // Phase 2: promote all pending files. Metadata is deliberately left untouched
  // until every filesystem rename succeeds.
  bool promoted[SLOT_COUNT]{};
  bool promotionFailed = false;
  for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
    if (!replace[slot]) continue;
    char pending[20], active[20];
    pendingPath(slot, pending, sizeof(pending));
    activePath(slot, active, sizeof(active));
    if (!WLED_FS.rename(pending, active)) {
      promotionFailed = true;
      break;
    }
    promoted[slot] = true;
  }

  if (promotionFailed) {
    bool rollbackOk = true;
    // Return any newly promoted file to its pending name so a retry remains
    // possible. If that rename fails, discard only the new file; the old bank
    // is still restored below and remains the authoritative state.
    for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
      if (!promoted[slot]) continue;
      char pending[20], active[20];
      pendingPath(slot, pending, sizeof(pending));
      activePath(slot, active, sizeof(active));
      WLED_FS.remove(pending);
      if (!WLED_FS.rename(active, pending)) {
        WLED_FS.remove(active);
        pending_[slot] = SlotMeta{};
        rollbackOk = false;
      }
    }
    for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
      if (!replace[slot] || !hadActive[slot]) continue;
      char active[20], backup[20];
      activePath(slot, active, sizeof(active));
      backupPath(slot, backup, sizeof(backup));
      WLED_FS.remove(active);
      if (!WLED_FS.rename(backup, active)) rollbackOk = false;
    }
    (void)rollbackOk; // The previous logical bank remains selected either way.
    return false;
  }

  // Commit metadata only after the complete filesystem transaction succeeded.
  for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
    if (!replace[slot]) continue;
    char backup[20], cache[20];
    backupPath(slot, backup, sizeof(backup));
    cachePath(slot, cache, sizeof(cache));
    WLED_FS.remove(backup);
    WLED_FS.remove(cache);
    active_[slot] = pending_[slot];
    pending_[slot] = SlotMeta{};
  }
  return true;
}

void IDotMatrixPreset::onPresetActivate(const uint8_t* slots, uint8_t count) {
  if (slots == nullptr || count == 0 || count > SLOT_COUNT) return;
  uint8_t mapped[SLOT_COUNT]{};
  for (uint8_t i = 0; i < count; ++i) {
    if (!protocolSlotToLocal(slots[i], mapped[i])) return;
  }

  // The upload indicator intentionally spans the full Preset replacement and
  // disappears only when 06/02 activates the new bank. This avoids a black or
  // unrelated frame between consecutive large assets.
  finishUploadIndicator(millis(), false);
  // Stop any decoder using the old bank before filesystem mutation. Pending
  // media remains private until activateTransactional() commits the whole bank.
  adapter_.beginCarouselPlayback();
  if (!activateTransactional(mapped, count)) {
    // The active metadata was never committed on failure. Restart the previous
    // item if there was one; Preset remains volatile and no reboot recovery is
    // attempted or desired.
    if (playing_ && activeCount_ > 0 && position_ >= 0) playCurrent(millis());
    return;
  }
  for (uint8_t i = 0; i < count; ++i) order_[i] = mapped[i];
  activeCount_ = count;
  position_ = 0;
  playing_ = true;
  entryStartedAt_ = 0;
  currentHoldMs_ = 0;
  playCurrent(millis());
}

bool IDotMatrixPreset::playPosition(uint8_t position, uint32_t now) {
  if (position >= activeCount_) return false;
  const uint8_t slot = order_[position];
  if (slot >= SLOT_COUNT || !active_[slot].valid) return false;
  char path[20]; activePath(slot, path, sizeof(path));
  bool shown = false;
  uint32_t hold = 3000u;
  if (active_[slot].type == TYPE_GIF) {
    char cache[20]; cachePath(slot, cache, sizeof(cache));
    shown = adapter_.playStoredGif(path, cache);
  } else if (active_[slot].type == TYPE_TEXT) {
    File file = WLED_FS.open(path, "r");
    if (file && file.size() <= 4096u) {
      static uint8_t textBuffer[4096];
      const size_t bytes = file.size();
      shown = file.read(textBuffer, bytes) == bytes && protocol_.processTextPayload(textBuffer, bytes);
      if (shown) hold = adapter_.textPresentationDurationMs();
    }
    if (file) file.close();
  }
  if (!shown) return false;
  position_ = int8_t(position);
  entryStartedAt_ = active_[slot].type == TYPE_GIF ? 0u : now;
  currentHoldMs_ = hold < 250u ? 250u : hold;
  return true;
}

bool IDotMatrixPreset::playCurrent(uint32_t now) {
  if (!playing_ || activeCount_ == 0 || position_ < 0) return false;
  return playPosition(uint8_t(position_), now);
}

void IDotMatrixPreset::loop(uint32_t now) {
  if (uploadIndicatorActive_ && uploadLastProgressAt_ != 0 &&
      uint32_t(now - uploadLastProgressAt_) >= UPLOAD_IDLE_TIMEOUT_MS) {
    if (rxOpen_) onPresetAssetCancel();
    else finishUploadIndicator(now, true);
  }
  if (!playing_ || activeCount_ == 0) return;
  if (position_ < 0) {
    if (!playPosition(0, now)) suspend();
    return;
  }
  // For a cold-cache GIF, start the 3 s visible dwell only after playback is active.
  const uint8_t slot = order_[uint8_t(position_)];
  if (active_[slot].type == TYPE_GIF && adapter_.isGifPending()) return;
  if (active_[slot].type == TYPE_GIF && entryStartedAt_ == 0 && adapter_.isGifActive()) entryStartedAt_ = now;
  if (entryStartedAt_ == 0) entryStartedAt_ = now;
  if (uint32_t(now - entryStartedAt_) < currentHoldMs_) return;
  const uint8_t next = uint8_t((uint8_t(position_) + 1u) % activeCount_);
  if (!playPosition(next, now)) suspend();
}
