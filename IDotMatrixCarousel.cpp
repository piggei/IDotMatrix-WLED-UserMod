#include "IDotMatrixCarousel.h"

#include "IDotMatrixProtocol.h"
#include "IDotMatrixWLEDAdapter.h"
#include "wled.h"
#include <cstring>
#include <cstdio>

namespace {
constexpr char MANIFEST_PATH[] = "/idot_car.bin";
constexpr char MANIFEST_TMP[] = "/idot_car.tmp";
constexpr char MANIFEST_BACKUP[] = "/idot_car.bak";
constexpr char ASSET_RX[] = "/idot_asset.tmp";
constexpr uint8_t MANIFEST_MAGIC[4] = {'I','D','C','2'};
constexpr uint8_t MANIFEST_VERSION = 1;
File carouselRxFile;

bool validType(uint8_t type) {
  return type == IDotMatrixCarousel::TYPE_GIF || type == IDotMatrixCarousel::TYPE_TEXT;
}
}

IDotMatrixCarousel::IDotMatrixCarousel(
  IDotMatrixProtocol& protocol,
  IDotMatrixWLEDAdapter& adapter
) : protocol_(protocol), adapter_(adapter) {
  resetManifest();
}

void IDotMatrixCarousel::resetManifest() {
  manifest_ = Manifest{};
  memcpy(manifest_.magic, MANIFEST_MAGIC, sizeof(MANIFEST_MAGIC));
  manifest_.version = MANIFEST_VERSION;
  manifest_.configuredCount = SLOT_COUNT;
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) manifest_.order[i] = i;
  configuredCount_ = SLOT_COUNT;
  resumeOnBoot_ = false;
}

bool IDotMatrixCarousel::loadManifest() {
  File file = WLED_FS.open(MANIFEST_PATH, "r");
  if (!file || file.size() != sizeof(Manifest)) {
    if (file) file.close();
    return false;
  }
  Manifest loaded{};
  const size_t got = file.read(reinterpret_cast<uint8_t*>(&loaded), sizeof(loaded));
  file.close();
  if (got != sizeof(loaded) || memcmp(loaded.magic, MANIFEST_MAGIC, 4) != 0 ||
      loaded.version != MANIFEST_VERSION || loaded.configuredCount > SLOT_COUNT) return false;
  for (uint8_t i = 0; i < loaded.configuredCount; ++i) {
    if (loaded.order[i] >= SLOT_COUNT) return false;
  }
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) {
    if (loaded.slots[i].valid && !validType(loaded.slots[i].type)) return false;
  }
  manifest_ = loaded;
  configuredCount_ = manifest_.configuredCount;
  resumeOnBoot_ = manifest_.resumeOnBoot != 0;
  return true;
}

bool IDotMatrixCarousel::saveManifest() {
  manifest_.configuredCount = configuredCount_;
  manifest_.resumeOnBoot = resumeOnBoot_ ? 1 : 0;
  WLED_FS.remove(MANIFEST_TMP);
  File file = WLED_FS.open(MANIFEST_TMP, "w");
  if (!file) return false;
  const size_t wrote = file.write(reinterpret_cast<const uint8_t*>(&manifest_), sizeof(manifest_));
  file.flush();
  file.close();
  if (wrote != sizeof(manifest_)) {
    WLED_FS.remove(MANIFEST_TMP);
    return false;
  }
  WLED_FS.remove(MANIFEST_BACKUP);
  const bool hadManifest = WLED_FS.exists(MANIFEST_PATH);
  if (hadManifest && !WLED_FS.rename(MANIFEST_PATH, MANIFEST_BACKUP)) {
    WLED_FS.remove(MANIFEST_TMP);
    return false;
  }
  bool promoted = WLED_FS.rename(MANIFEST_TMP, MANIFEST_PATH);
  if (!promoted) {
    promoted = copyFile(MANIFEST_TMP, MANIFEST_PATH);
    if (promoted) WLED_FS.remove(MANIFEST_TMP);
  }
  if (!promoted) {
    WLED_FS.remove(MANIFEST_PATH);
    if (hadManifest) WLED_FS.rename(MANIFEST_BACKUP, MANIFEST_PATH);
    WLED_FS.remove(MANIFEST_TMP);
    return false;
  }
  WLED_FS.remove(MANIFEST_BACKUP);
  return true;
}

void IDotMatrixCarousel::begin() {
  if (!loadManifest()) {
    resetManifest();
    saveManifest();
  }
  // Prune metadata whose backing file disappeared.
  bool dirty = false;
  for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
    if (!manifest_.slots[slot].valid) continue;
    char path[24];
    slotPath(slot, manifest_.slots[slot].type, path, sizeof(path));
    File file = WLED_FS.open(path, "r");
    const bool ok = file && file.size() == manifest_.slots[slot].bytes;
    if (file) file.close();
    if (!ok) {
      manifest_.slots[slot] = SlotMeta{};
      dirty = true;
    }
  }
  if (dirty) saveManifest();
  // Boot playback is deliberately decided by the WLED Usermod layer.  A
  // stored Carousel starts at boot only when the dedicated iDotMatrix Display
  // effect is the WLED boot effect (or is selected manually later).
}

void IDotMatrixCarousel::clearFiles() {
  for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
    char path[24];
    slotPath(slot, TYPE_GIF, path, sizeof(path)); WLED_FS.remove(path);
    slotPath(slot, TYPE_TEXT, path, sizeof(path)); WLED_FS.remove(path);
    backupPath(slot, TYPE_GIF, path, sizeof(path)); WLED_FS.remove(path);
    backupPath(slot, TYPE_TEXT, path, sizeof(path)); WLED_FS.remove(path);
  }
  WLED_FS.remove(ASSET_RX);
}

void IDotMatrixCarousel::resetPersistent() {
  cancelAsset();
  suspend();
  clearFiles();
  WLED_FS.remove(MANIFEST_PATH);
  WLED_FS.remove(MANIFEST_TMP);
  WLED_FS.remove(MANIFEST_BACKUP);
  resetManifest();
  // Keep an explicit empty manifest so a later reboot cannot resurrect stale
  // metadata even if the reset was the last command received before power loss.
  saveManifest();
}

void IDotMatrixCarousel::configure(const uint8_t* slots, uint8_t count) {
  cancelAsset();
  playing_ = false;
  autoStartPending_ = false;
  resumeOnBoot_ = false;
  currentOrderPos_ = -1;
  currentSlot_ = -1;
  clearFiles();
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) manifest_.slots[i] = SlotMeta{};
  configuredCount_ = count > SLOT_COUNT ? SLOT_COUNT : count;
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) manifest_.order[i] = i;
  if (slots != nullptr) {
    for (uint8_t i = 0; i < configuredCount_; ++i) {
      manifest_.order[i] = slots[i] < SLOT_COUNT ? slots[i] : i;
    }
  }
  saveManifest();
}

void IDotMatrixCarousel::enter() {
  if (!hasAssets()) return;
  autoStartPending_ = false;
  playing_ = true;
  resumeOnBoot_ = true;
  currentOrderPos_ = -1;
  currentSlot_ = -1;
  nextSwitchAt_ = 0;
  saveManifest();
}

void IDotMatrixCarousel::suspend() {
  playing_ = false;
  autoStartPending_ = false;
  currentOrderPos_ = -1;
  currentSlot_ = -1;
  nextSwitchAt_ = 0;
}

void IDotMatrixCarousel::requestAutoStart(uint32_t now, uint32_t delayMs) {
  if (!hasAssets()) return;
  autoStartPending_ = true;
  autoStartAt_ = now + delayMs;
}

bool IDotMatrixCarousel::beginAsset(
  uint8_t type,
  uint8_t slot,
  uint16_t dwellSeconds,
  size_t totalLength
) {
  cancelAsset();
  // A new asset belongs to the same app-side page upload.  Do not let the
  // previous slot's quiet-period timer start playback between two transfers.
  autoStartPending_ = false;
  if (!validType(type) || slot >= SLOT_COUNT || totalLength == 0) return false;
  WLED_FS.remove(ASSET_RX);
  carouselRxFile = WLED_FS.open(ASSET_RX, "w");
  if (!carouselRxFile) return false;
  rxOpen_ = true;
  rxType_ = type;
  rxSlot_ = slot;
  rxDwell_ = dwellSeconds;
  rxExpected_ = totalLength;
  rxWritten_ = 0;
  return true;
}

bool IDotMatrixCarousel::writeAsset(size_t offset, const uint8_t* data, size_t length) {
  if (!rxOpen_ || !carouselRxFile || data == nullptr || offset != rxWritten_ ||
      length > rxExpected_ - rxWritten_) return false;
  const size_t wrote = carouselRxFile.write(data, length);
  rxWritten_ += wrote;
  return wrote == length;
}

bool IDotMatrixCarousel::completeAsset(bool crcValid) {
  if (rxOpen_) {
    carouselRxFile.flush();
    carouselRxFile.close();
    rxOpen_ = false;
  }
  if (!crcValid || rxWritten_ != rxExpected_) {
    WLED_FS.remove(ASSET_RX);
    return false;
  }

  char finalPath[24], backup[24];
  slotPath(rxSlot_, rxType_, finalPath, sizeof(finalPath));
  backupPath(rxSlot_, rxType_, backup, sizeof(backup));
  WLED_FS.remove(backup);
  const bool hadOld = WLED_FS.exists(finalPath);
  bool backedUp = !hadOld || WLED_FS.rename(finalPath, backup);
  if (!backedUp) {
    WLED_FS.remove(ASSET_RX);
    return false;
  }

  bool promoted = WLED_FS.rename(ASSET_RX, finalPath);
  if (!promoted) {
    promoted = copyFile(ASSET_RX, finalPath);
    if (promoted) WLED_FS.remove(ASSET_RX);
  }
  if (!promoted) {
    WLED_FS.remove(finalPath);
    if (hadOld) WLED_FS.rename(backup, finalPath);
    WLED_FS.remove(ASSET_RX);
    return false;
  }

  const SlotMeta previous = manifest_.slots[rxSlot_];
  const bool previousResume = resumeOnBoot_;
  char oldOther[24] = {0};
  char oldOtherBackup[24] = {0};
  bool backedUpOther = false;
  if (previous.valid && previous.type != rxType_) {
    slotPath(rxSlot_, previous.type, oldOther, sizeof(oldOther));
    backupPath(rxSlot_, previous.type, oldOtherBackup, sizeof(oldOtherBackup));
    WLED_FS.remove(oldOtherBackup);
    if (WLED_FS.exists(oldOther)) {
      backedUpOther = WLED_FS.rename(oldOther, oldOtherBackup);
      if (!backedUpOther) {
        WLED_FS.remove(finalPath);
        if (hadOld) WLED_FS.rename(backup, finalPath);
        return false;
      }
    }
  }

  manifest_.slots[rxSlot_].valid = 1;
  manifest_.slots[rxSlot_].type = rxType_;
  manifest_.slots[rxSlot_].dwellSeconds = rxDwell_ == 0 ? 5 : rxDwell_;
  manifest_.slots[rxSlot_].bytes = static_cast<uint32_t>(rxWritten_);
  // A successfully downloaded Device Assets slot proves that a Carousel bank
  // exists.  The official app does not emit a separate, reliable "play now"
  // command after every page upload, so arm autonomous playback after the
  // transfer stream becomes quiet.  beginAsset() below keeps extending this
  // boundary while the page is still being downloaded.
  resumeOnBoot_ = true;
  if (!saveManifest()) {
    manifest_.slots[rxSlot_] = previous;
    resumeOnBoot_ = previousResume;
    WLED_FS.remove(finalPath);
    if (hadOld) WLED_FS.rename(backup, finalPath);
    if (backedUpOther) WLED_FS.rename(oldOtherBackup, oldOther);
    return false;
  }
  WLED_FS.remove(backup);
  if (backedUpOther) WLED_FS.remove(oldOtherBackup);
  requestAutoStart(millis());
  return true;
}

void IDotMatrixCarousel::cancelAsset() {
  if (rxOpen_) {
    carouselRxFile.close();
    rxOpen_ = false;
  }
  WLED_FS.remove(ASSET_RX);
  rxExpected_ = rxWritten_ = 0;
}

bool IDotMatrixCarousel::hasAssets() const {
  for (uint8_t i = 0; i < configuredCount_; ++i) {
    const uint8_t slot = manifest_.order[i];
    if (slot < SLOT_COUNT && manifest_.slots[slot].valid) return true;
  }
  return false;
}

uint8_t IDotMatrixCarousel::storedCount() const {
  uint8_t count = 0;
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) if (manifest_.slots[i].valid) ++count;
  return count;
}

uint16_t IDotMatrixCarousel::currentDwellSeconds() const {
  return currentSlot_ >= 0 ? manifest_.slots[uint8_t(currentSlot_)].dwellSeconds : 0;
}

int8_t IDotMatrixCarousel::findNextPlayable(int8_t from) const {
  if (configuredCount_ == 0) return -1;
  for (uint8_t step = 1; step <= configuredCount_; ++step) {
    const uint8_t pos = uint8_t((int(from) + step + configuredCount_) % configuredCount_);
    const uint8_t slot = manifest_.order[pos];
    if (slot < SLOT_COUNT && manifest_.slots[slot].valid) return int8_t(pos);
  }
  return -1;
}

bool IDotMatrixCarousel::playSlot(uint8_t slot, uint32_t now) {
  if (slot >= SLOT_COUNT || !manifest_.slots[slot].valid) return false;
  char path[24];
  slotPath(slot, manifest_.slots[slot].type, path, sizeof(path));
  bool shown = false;
  if (manifest_.slots[slot].type == TYPE_GIF) {
    shown = adapter_.playStoredGif(path);
  } else if (manifest_.slots[slot].type == TYPE_TEXT) {
    File file = WLED_FS.open(path, "r");
    if (file && file.size() <= 4096u) {
      static uint8_t textBuffer[4096];
      const size_t bytes = file.size();
      shown = file.read(textBuffer, bytes) == bytes && protocol_.processTextPayload(textBuffer, bytes);
    }
    if (file) file.close();
  }
  if (shown) {
    currentSlot_ = int8_t(slot);
    const uint32_t dwell = manifest_.slots[slot].dwellSeconds == 0 ? 5u : manifest_.slots[slot].dwellSeconds;
    nextSwitchAt_ = now + dwell * 1000u;
  }
  return shown;
}

bool IDotMatrixCarousel::playNext(uint32_t now, bool first) {
  if (!playing_) return false;
  const int8_t next = findNextPlayable(first ? -1 : currentOrderPos_);
  if (next < 0) {
    playing_ = false;
    return false;
  }
  currentOrderPos_ = next;
  return playSlot(manifest_.order[uint8_t(next)], now);
}

void IDotMatrixCarousel::loop(uint32_t now) {
  if (!playing_ && autoStartPending_ && int32_t(now - autoStartAt_) >= 0) {
    enter();
  }
  if (!playing_) return;
  if (currentSlot_ < 0) {
    playNext(now, true);
    return;
  }
  if (int32_t(now - nextSwitchAt_) >= 0) playNext(now, false);
}

void IDotMatrixCarousel::slotPath(uint8_t slot, uint8_t type, char* out, size_t outSize) {
  snprintf(out, outSize, type == TYPE_TEXT ? "/idot_a%u.txt" : "/idot_a%u.gif", unsigned(slot));
}

void IDotMatrixCarousel::backupPath(uint8_t slot, uint8_t type, char* out, size_t outSize) {
  snprintf(out, outSize, type == TYPE_TEXT ? "/idot_b%u.txt" : "/idot_b%u.gif", unsigned(slot));
}

bool IDotMatrixCarousel::copyFile(const char* from, const char* to) {
  File source = WLED_FS.open(from, "r");
  File target = WLED_FS.open(to, "w");
  if (!source || !target) {
    if (source) source.close();
    if (target) target.close();
    return false;
  }
  uint8_t buffer[256];
  bool ok = true;
  const size_t size = source.size();
  while (source.position() < size) {
    const size_t remaining = size - source.position();
    const size_t got = source.read(buffer, remaining < sizeof(buffer) ? remaining : sizeof(buffer));
    if (got == 0 || target.write(buffer, got) != got) { ok = false; break; }
  }
  target.flush();
  source.close();
  target.close();
  if (!ok) WLED_FS.remove(to);
  return ok;
}
