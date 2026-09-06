#include "IDotMatrixAutomation.h"

#include "IDotMatrixBuzzer.h"
#include "IDotMatrixFA02Assembler.h"
#include "IDotMatrixMedia.h"
#include "IDotMatrixRenderer.h"
#include "IDotMatrixWLEDAdapter.h"
#include "wled.h"

#include <Preferences.h>
#include <cstdlib>
#include <cstring>
#include <new>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

namespace {
void alarmPath(uint8_t slot, char* buffer, size_t length) {
  snprintf(buffer, length, "/idot_a%u.bin", unsigned(slot));
}

void schedulePath(uint8_t index, char* buffer, size_t length) {
  snprintf(buffer, length, "/idot_s%u.bin", unsigned(index));
}

void scheduleTempPath(uint8_t index, char* buffer, size_t length) {
  snprintf(buffer, length, "/idot_t%u.bin", unsigned(index));
}
}

IDotMatrixAutomation::IDotMatrixAutomation(
  IDotMatrixRenderer& renderer,
  IDotMatrixWLEDAdapter& adapter,
  IDotMatrixMedia& media,
  IDotMatrixBuzzer& buzzer
) : renderer_(renderer), adapter_(adapter), media_(media), buzzer_(buzzer) {}

IDotMatrixAutomation::~IDotMatrixAutomation() {
  cancelScheduleUpload();
  if (alarmPrefs_ != nullptr) {
    alarmPrefs_->end();
    delete alarmPrefs_;
    alarmPrefs_ = nullptr;
  }
  if (schedulePrefs_ != nullptr) {
    schedulePrefs_->end();
    delete schedulePrefs_;
    schedulePrefs_ = nullptr;
  }
}

void IDotMatrixAutomation::begin() {
  if (begun_) return;
  loadPersistence();
  begun_ = true;
}

void IDotMatrixAutomation::loadPersistence() {
  alarmPrefs_ = new (std::nothrow) Preferences();
  schedulePrefs_ = new (std::nothrow) Preferences();
  if (alarmPrefs_ == nullptr || schedulePrefs_ == nullptr) {
    lastError_ = Error::Preferences;
    return;
  }

  if (!alarmPrefs_->begin("idot-alarm", false) ||
      !schedulePrefs_->begin("idot-sched", false)) {
    lastError_ = Error::Preferences;
    return;
  }

  for (uint8_t slot = 0; slot < IDotMatrixAlarmSettings::SLOT_COUNT; ++slot) {
    char key[8];
    snprintf(key, sizeof(key), "a%u", unsigned(slot));
    if (alarmPrefs_->getBytesLength(key) == sizeof(AlarmSlot)) {
      alarmPrefs_->getBytes(key, &alarms_[slot], sizeof(AlarmSlot));
    }
    alarms_[slot].lastTriggerMinuteKey = 0xFFFFFFFFu;
  }

  scheduleGlobalFlags_ = schedulePrefs_->getUChar("flags", 0);
  for (uint8_t index = 0; index < IDotMatrixScheduleActivitySettings::MAX_ACTIVITIES; ++index) {
    char key[8];
    snprintf(key, sizeof(key), "s%u", unsigned(index));
    if (schedulePrefs_->getBytesLength(key) == sizeof(ScheduleActivity)) {
      schedulePrefs_->getBytes(key, &scheduleActivities_[index], sizeof(ScheduleActivity));
    }
  }
}

void IDotMatrixAutomation::saveAlarmMeta(uint8_t slot) {
  if (alarmPrefs_ == nullptr || slot >= IDotMatrixAlarmSettings::SLOT_COUNT) return;
  char key[8];
  snprintf(key, sizeof(key), "a%u", unsigned(slot));
  alarmPrefs_->putBytes(key, &alarms_[slot], sizeof(AlarmSlot));
}

void IDotMatrixAutomation::saveScheduleGlobal() {
  if (schedulePrefs_ != nullptr) schedulePrefs_->putUChar("flags", scheduleGlobalFlags_);
}

void IDotMatrixAutomation::saveScheduleMeta(uint8_t index) {
  if (schedulePrefs_ == nullptr || index >= IDotMatrixScheduleActivitySettings::MAX_ACTIVITIES) return;
  char key[8];
  snprintf(key, sizeof(key), "s%u", unsigned(index));
  schedulePrefs_->putBytes(key, &scheduleActivities_[index], sizeof(ScheduleActivity));
}

void IDotMatrixAutomation::clearScheduleMeta(uint8_t index) {
  if (index >= IDotMatrixScheduleActivitySettings::MAX_ACTIVITIES) return;
  if (schedulePrefs_ != nullptr) {
    char key[8];
    snprintf(key, sizeof(key), "s%u", unsigned(index));
    schedulePrefs_->remove(key);
  }
  scheduleActivities_[index] = ScheduleActivity{};
  char path[20];
  schedulePath(index, path, sizeof(path));
  WLED_FS.remove(path);
}

void IDotMatrixAutomation::onTimeSync(const IDotMatrixTimeSyncSettings& settings) {
  if (settings.year < 2000 || settings.month < 1 || settings.month > 12 ||
      settings.day < 1 || settings.day > daysInMonth(settings.year, settings.month) ||
      settings.hour > 23 || settings.minute > 59 || settings.second > 59) {
    return;
  }
  appTime_ = settings;
  appTimeSyncMillis_ = millis();
  appTimeValid_ = true;
}

bool IDotMatrixAutomation::onAlarm(
  const IDotMatrixAlarmSettings& settings,
  const uint8_t* media,
  size_t mediaLength
) {
  if (settings.slot >= IDotMatrixAlarmSettings::SLOT_COUNT) return false;

  AlarmSlot next = alarms_[settings.slot];
  next.configured = 1;
  next.flags = settings.flags;
  next.hour = settings.hour;
  next.minute = settings.minute;
  next.durationSeconds = settings.durationSeconds;
  if (settings.packetLength > 9) next.reserved1 = settings.reserved1;
  if (settings.packetLength > 10) next.contentType = settings.contentType;
  if (settings.packetLength > 11) next.buzzer = settings.buzzer;

  if (settings.fullHeader) {
    next.reserved2 = settings.reserved2;
    next.mediaSize = settings.mediaSize;
    next.mediaCRC = settings.mediaCRC;
    next.reserved3 = settings.reserved3;
    next.mediaId = settings.mediaId;

    if (mediaLength != settings.mediaSize ||
        crc32(media, mediaLength) != settings.mediaCRC) return false;

    char path[20];
    alarmPath(settings.slot, path, sizeof(path));
    if (mediaLength == 0) {
      WLED_FS.remove(path);
    } else if (!writeFile(path, media, mediaLength)) {
      lastError_ = Error::FileWrite;
      return false;
    }
  }

  next.lastTriggerMinuteKey = 0xFFFFFFFFu;
  alarms_[settings.slot] = next;
  saveAlarmMeta(settings.slot);
  lastError_ = Error::None;
  return true;
}

bool IDotMatrixAutomation::ensureScheduleStaging() {
  if (scheduleStaging_ != nullptr) return true;
  scheduleStaging_ = new (std::nothrow)
    ScheduleActivity[IDotMatrixScheduleActivitySettings::MAX_ACTIVITIES]();
  if (scheduleStaging_ == nullptr) {
    lastError_ = Error::StagingOom;
    return false;
  }
  return true;
}

void IDotMatrixAutomation::beginScheduleUpload(uint8_t flags) {
  scheduleGlobalFlags_ = flags;
  saveScheduleGlobal();
  cancelScheduleUpload();
  if (!ensureScheduleStaging()) return;

  scheduleUploadOpen_ = true;
  scheduleUploadDirty_ = false;
  scheduleFailedIndex_ = -1;
  scheduleReceivedMask_ = 0;
  scheduleLastRxMs_ = millis();
  for (uint8_t index = 0; index < IDotMatrixScheduleActivitySettings::MAX_ACTIVITIES; ++index) {
    char path[20];
    scheduleTempPath(index, path, sizeof(path));
    WLED_FS.remove(path);
  }
}

void IDotMatrixAutomation::cancelScheduleUpload() {
  scheduleUploadOpen_ = false;
  scheduleUploadDirty_ = false;
  scheduleReceivedMask_ = 0;
  if (scheduleStaging_ != nullptr) {
    delete[] scheduleStaging_;
    scheduleStaging_ = nullptr;
  }
  for (uint8_t index = 0; index < IDotMatrixScheduleActivitySettings::MAX_ACTIVITIES; ++index) {
    char path[20];
    scheduleTempPath(index, path, sizeof(path));
    WLED_FS.remove(path);
  }
}

void IDotMatrixAutomation::onScheduleGlobal(uint8_t flags) {
  if ((flags & 0x01u) == 0) {
    scheduleGlobalFlags_ = flags;
    saveScheduleGlobal();
    cancelScheduleUpload();
    scheduleFailedIndex_ = -1;
    if (scheduleActiveIndex_ >= 0) stopScheduleActivity(true);
    refreshBuzzer(millis());
    return;
  }
  beginScheduleUpload(flags);
}

bool IDotMatrixAutomation::onScheduleActivity(
  const IDotMatrixScheduleActivitySettings& settings,
  const uint8_t* media,
  size_t mediaLength
) {
  if (settings.index >= IDotMatrixScheduleActivitySettings::MAX_ACTIVITIES ||
      media == nullptr || mediaLength == 0 || mediaLength != settings.mediaSize ||
      crc32(media, mediaLength) != settings.mediaCRC) return false;

  if (!scheduleUploadOpen_) {
    scheduleGlobalFlags_ |= 0x01u;
    saveScheduleGlobal();
    if (!ensureScheduleStaging()) return false;
    scheduleUploadOpen_ = true;
    scheduleUploadDirty_ = false;
    scheduleReceivedMask_ = 0;
  }
  if (!ensureScheduleStaging()) return false;

  char tempPath[20];
  scheduleTempPath(settings.index, tempPath, sizeof(tempPath));
  if (!writeFile(tempPath, media, mediaLength)) {
    lastError_ = Error::FileWrite;
    return false;
  }

  ScheduleActivity activity;
  activity.configured = 1;
  activity.flags = settings.flags;
  activity.startHour = settings.startHour;
  activity.startMinute = settings.startMinute;
  activity.endHour = settings.endHour;
  activity.endMinute = settings.endMinute;
  activity.contentType = settings.contentType;
  activity.mediaSize = settings.mediaSize;
  activity.mediaCRC = settings.mediaCRC;
  activity.reserved = settings.reserved;
  activity.mediaId = settings.mediaId;
  scheduleStaging_[settings.index] = activity;
  scheduleReceivedMask_ |= (1UL << settings.index);
  scheduleUploadDirty_ = true;
  scheduleLastRxMs_ = millis();
  lastError_ = Error::None;
  return true;
}

void IDotMatrixAutomation::commitScheduleUpload() {
  if (!scheduleUploadOpen_) return;
  if (!scheduleUploadDirty_) {
    cancelScheduleUpload();
    return;
  }

  // Reload an activity even when its index remains the same after the new list
  // is committed. Without this, edited media would not appear until the next
  // time window.
  const bool hadActive = scheduleActiveIndex_ >= 0;
  if (hadActive) stopScheduleActivity(false);

  for (uint8_t index = 0; index < IDotMatrixScheduleActivitySettings::MAX_ACTIVITIES; ++index) {
    const bool received = (scheduleReceivedMask_ & (1UL << index)) != 0;
    if (received) {
      scheduleActivities_[index] = scheduleStaging_[index];
      char tempPath[20], finalPath[20];
      scheduleTempPath(index, tempPath, sizeof(tempPath));
      schedulePath(index, finalPath, sizeof(finalPath));
      WLED_FS.remove(finalPath);
      if (!WLED_FS.rename(tempPath, finalPath)) {
        lastError_ = Error::FileWrite;
        scheduleActivities_[index] = ScheduleActivity{};
        continue;
      }
      saveScheduleMeta(index);
    } else if (scheduleActivities_[index].configured) {
      clearScheduleMeta(index);
    }
  }

  scheduleUploadOpen_ = false;
  scheduleUploadDirty_ = false;
  scheduleReceivedMask_ = 0;
  delete[] scheduleStaging_;
  scheduleStaging_ = nullptr;
  scheduleFailedIndex_ = -1;
}

uint8_t IDotMatrixAutomation::daysInMonth(uint16_t yearValue, uint8_t monthValue) {
  static const uint8_t days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (monthValue < 1 || monthValue > 12) return 31;
  uint8_t value = days[monthValue - 1];
  const bool leap = (yearValue % 4u == 0u && yearValue % 100u != 0u) ||
    (yearValue % 400u == 0u);
  if (monthValue == 2 && leap) value = 29;
  return value;
}

uint8_t IDotMatrixAutomation::weekdayBit(uint16_t yearValue, uint8_t monthValue, uint8_t dayValue) {
  // Sakamoto: 0=Sunday. iDotMatrix flags use bit1=Monday ... bit7=Sunday.
  static const uint8_t table[] = {0,3,2,5,0,3,5,1,4,6,2,4};
  uint16_t y = yearValue;
  if (monthValue < 3) --y;
  const uint8_t dow = uint8_t((y + y/4u - y/100u + y/400u +
    table[monthValue - 1] + dayValue) % 7u);
  return dow == 0 ? 0x80u : uint8_t(1u << dow);
}

bool IDotMatrixAutomation::currentDateTime(uint32_t now, DateTimeParts& value) const {
  // Prefer WLED's persisted/NTP local clock whenever it is valid.  The BLE app
  // sync remains a useful fallback for isolated installations with no network.
  if (year(localTime) >= 2020) {
    value.year = uint16_t(year(localTime));
    value.month = uint8_t(month(localTime));
    value.day = uint8_t(day(localTime));
    value.hour = uint8_t(hour(localTime));
    value.minute = uint8_t(minute(localTime));
    value.second = uint8_t(second(localTime));
    return true;
  }
  if (!appTimeValid_) return false;

  value.year = appTime_.year;
  value.month = appTime_.month;
  value.day = appTime_.day;
  uint32_t total = uint32_t(appTime_.hour) * 3600u +
    uint32_t(appTime_.minute) * 60u + appTime_.second +
    uint32_t(now - appTimeSyncMillis_) / 1000u;
  uint32_t days = total / 86400u;
  total %= 86400u;
  value.hour = uint8_t(total / 3600u);
  total %= 3600u;
  value.minute = uint8_t(total / 60u);
  value.second = uint8_t(total % 60u);

  while (days-- > 0) {
    if (++value.day > daysInMonth(value.year, value.month)) {
      value.day = 1;
      if (++value.month > 12) {
        value.month = 1;
        ++value.year;
      }
    }
  }
  return true;
}

bool IDotMatrixAutomation::scheduleTimeInside(
  const ScheduleActivity& activity,
  uint16_t nowMinutes
) {
  const uint16_t start = uint16_t(activity.startHour) * 60u + activity.startMinute;
  const uint16_t end = uint16_t(activity.endHour) * 60u + activity.endMinute;
  if (start == end) return true;
  if (start < end) return nowMinutes >= start && nowMinutes < end;
  return nowMinutes >= start || nowMinutes < end;
}

void IDotMatrixAutomation::updateAlarms(uint32_t now, bool& alarmEnded) {
  alarmEnded = false;
  if (alarmActive_) {
    if (int32_t(now - alarmEndsAt_) >= 0) {
      stopAlarm(true);
      alarmEnded = true;
    }
    return;
  }
  if (uint32_t(now - lastAlarmCheckAt_) < ALARM_CHECK_INTERVAL_MS) return;
  lastAlarmCheckAt_ = now;

  DateTimeParts current;
  if (!currentDateTime(now, current)) return;
  const uint32_t minuteKey = (uint32_t(current.year) << 20) |
    (uint32_t(current.month) << 16) | (uint32_t(current.day) << 11) |
    (uint32_t(current.hour) << 6) | current.minute;
  const uint8_t dayBit = weekdayBit(current.year, current.month, current.day);

  for (uint8_t slot = 0; slot < IDotMatrixAlarmSettings::SLOT_COUNT; ++slot) {
    AlarmSlot& alarm = alarms_[slot];
    if (!alarm.configured || (alarm.flags & 0x01u) == 0 ||
        alarm.hour != current.hour || alarm.minute != current.minute ||
        alarm.lastTriggerMinuteKey == minuteKey) continue;
    const uint8_t days = alarm.flags & 0xFEu;
    if (days != 0 && (days & dayBit) == 0) continue;

    alarm.lastTriggerMinuteKey = minuteKey;
    if (days == 0) {
      alarm.flags &= uint8_t(~0x01u);
      saveAlarmMeta(slot);
    }
    startAlarm(slot, now);
    break;
  }
}

void IDotMatrixAutomation::startAlarm(uint8_t slot, uint32_t now) {
  if (slot >= IDotMatrixAlarmSettings::SLOT_COUNT || alarmActive_) return;

  const bool scheduleWasActive = scheduleActiveIndex_ >= 0;
  const bool scheduleOwnedDisplay = scheduleWasActive && automationOwnsDisplay();
  uint8_t returnEffect = selectedEffect();
  if (scheduleWasActive) {
    if (scheduleOwnedDisplay && scheduleReturnValid_) returnEffect = scheduleReturnEffect_;
    stopScheduleActivity(false);
  }

  alarmReturnEffect_ = returnEffect;
  alarmReturnValid_ = true;
  alarmActive_ = true;
  activeAlarmSlot_ = slot;
  alarmEndsAt_ = now + uint32_t(alarms_[slot].durationSeconds) * 1000u;
  if (!loadAlarmMedia(slot)) lastError_ = Error::MediaLoad;
  refreshBuzzer(now);
}

void IDotMatrixAutomation::stopAlarm(bool deferRestore) {
  if (!alarmActive_) return;
  alarmActive_ = false;
  activeAlarmSlot_ = 0xFF;
  // When the normal loop ends an alarm, keep its framebuffer ownership alive
  // until updateSchedule() has had the chance to resume an interrupted program.
  // If no program takes over, loop() restores the captured WLED/clock state.
  if (!deferRestore) {
    restoreEffect(alarmReturnEffect_, alarmReturnValid_);
    alarmReturnValid_ = false;
  }
}

void IDotMatrixAutomation::updateSchedule(uint32_t now) {
  if (scheduleUploadOpen_ && scheduleUploadDirty_ &&
      uint32_t(now - scheduleLastRxMs_) >= SCHEDULE_COMMIT_DELAY_MS) {
    commitScheduleUpload();
  }

  if ((scheduleGlobalFlags_ & 0x01u) == 0) {
    if (scheduleActiveIndex_ >= 0) stopScheduleActivity(true);
    return;
  }
  if (alarmActive_) {
    if (scheduleActiveIndex_ >= 0) stopScheduleActivity(false);
    return;
  }

  DateTimeParts current;
  if (!currentDateTime(now, current)) return;
  const uint8_t dayBit = weekdayBit(current.year, current.month, current.day);
  const uint16_t nowMinutes = uint16_t(current.hour) * 60u + current.minute;
  int8_t wanted = -1;
  for (uint8_t index = 0; index < IDotMatrixScheduleActivitySettings::MAX_ACTIVITIES; ++index) {
    const ScheduleActivity& activity = scheduleActivities_[index];
    if (!activity.configured || (activity.flags & 0x01u) == 0) continue;
    if ((activity.flags & dayBit) == 0) continue;
    if (scheduleTimeInside(activity, nowMinutes)) {
      wanted = int8_t(index);
      break;
    }
  }

  if (wanted < 0) {
    scheduleFailedIndex_ = -1;
    if (scheduleActiveIndex_ >= 0) stopScheduleActivity(true);
    else if (scheduleReturnValid_) {
      // A schedule interrupted by an alarm may have left its return snapshot
      // alive while no activity is currently valid.
      scheduleReturnValid_ = false;
    }
  } else if (scheduleActiveIndex_ != wanted && scheduleFailedIndex_ != wanted) {
    startScheduleActivity(uint8_t(wanted), now);
  }
}

void IDotMatrixAutomation::startScheduleActivity(uint8_t index, uint32_t now) {
  if (index >= IDotMatrixScheduleActivitySettings::MAX_ACTIVITIES || alarmActive_) return;
  if (scheduleActiveIndex_ == int8_t(index)) return;
  if (scheduleActiveIndex_ >= 0) stopScheduleActivity(false);

  const bool capturedHere = !scheduleReturnValid_;
  if (capturedHere) {
    scheduleReturnEffect_ = selectedEffect();
    scheduleReturnValid_ = true;
  }

  if (loadScheduleMedia(index)) {
    scheduleActiveIndex_ = int8_t(index);
    scheduleFailedIndex_ = -1;
    if ((scheduleGlobalFlags_ & 0x02u) != 0) {
      // A program sound is an activation notification, not an alarm.  Emit
      // three finite groups of three short trills and then stay silent for
      // the remainder of the activity.
      buzzer_.startScheduleAlert(now);
      scheduleBuzzerOwned_ = true;
      alarmBuzzerOwned_ = false;
    }
  } else {
    scheduleFailedIndex_ = int8_t(index);
    lastError_ = Error::MediaLoad;
    if (capturedHere) {
      restoreEffect(scheduleReturnEffect_, scheduleReturnValid_);
      scheduleReturnValid_ = false;
    }
  }
  refreshBuzzer(millis());
}

void IDotMatrixAutomation::stopScheduleActivity(bool restoreOutput) {
  if (scheduleActiveIndex_ < 0) return;
  scheduleActiveIndex_ = -1;
  if (scheduleBuzzerOwned_) {
    buzzer_.stop();
    scheduleBuzzerOwned_ = false;
  }
  if (restoreOutput) {
    restoreEffect(scheduleReturnEffect_, scheduleReturnValid_);
    scheduleReturnValid_ = false;
  } else {
    adapter_.cancelAutomationContent();
  }
  refreshBuzzer(millis());
}

bool IDotMatrixAutomation::writeFile(const char* path, const uint8_t* data, size_t length) {
  if (path == nullptr || data == nullptr || length == 0) return false;
  File file = WLED_FS.open(path, "w");
  if (!file) return false;
  const size_t written = file.write(data, length);
  file.flush();
  file.close();
  return written == length;
}

bool IDotMatrixAutomation::streamRawFile(const char* path, uint32_t size) {
  File file = WLED_FS.open(path, "r");
  if (!file || uint32_t(file.size()) != size) { if (file) file.close(); return false; }
  if (!adapter_.onRawImageBegin(size)) { file.close(); return false; }

  uint8_t buffer[256];
  size_t offset = 0;
  bool ok = true;
  while (ok && offset < size) {
    const size_t wanted = (size - offset) < sizeof(buffer) ? size - offset : sizeof(buffer);
    const size_t got = file.read(buffer, wanted);
    if (got == 0) { ok = false; break; }
    ok = adapter_.onRawImageData(offset, buffer, got);
    offset += got;
  }
  file.close();
  return adapter_.onRawImageComplete(ok && offset == size);
}

bool IDotMatrixAutomation::streamGifFile(const char* path, uint32_t size) {
  File file = WLED_FS.open(path, "r");
  if (!file || uint32_t(file.size()) != size) { if (file) file.close(); return false; }
  if (!adapter_.onGifBegin(size)) { file.close(); return false; }

  uint8_t buffer[384];
  size_t offset = 0;
  bool ok = true;
  while (ok && offset < size) {
    const size_t wanted = (size - offset) < sizeof(buffer) ? size - offset : sizeof(buffer);
    const size_t got = file.read(buffer, wanted);
    if (got == 0) { ok = false; break; }
    ok = adapter_.onGifData(offset, buffer, got);
    offset += got;
  }
  file.close();
  return adapter_.onGifComplete(ok && offset == size);
}

void* IDotMatrixAutomation::allocateTemporary(size_t size) {
#if defined(ARDUINO_ARCH_ESP32)
  if (psramFound()) {
    void* memory = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (memory != nullptr) return memory;
  }
#endif
  return malloc(size);
}

void IDotMatrixAutomation::freeTemporary(void* memory) {
  free(memory);
}

bool IDotMatrixAutomation::loadTextFile(const char* path, uint32_t size) {
  if (protocol_ == nullptr || size == 0 || size > 4096u) return false;
  File file = WLED_FS.open(path, "r");
  if (!file || uint32_t(file.size()) != size) { if (file) file.close(); return false; }
  uint8_t* buffer = static_cast<uint8_t*>(allocateTemporary(size));
  if (buffer == nullptr) { file.close(); lastError_ = Error::MediaOom; return false; }
  const size_t got = file.read(buffer, size);
  file.close();
  const bool ok = got == size && protocol_->processTextPayload(buffer, size);
  freeTemporary(buffer);
  return ok;
}

bool IDotMatrixAutomation::loadPngFile(const char* path, uint32_t size) {
  if (size < 33 || size > IDotMatrixFA02Assembler::MAX_PACKET_SIZE) return false;
  File file = WLED_FS.open(path, "r");
  if (!file || uint32_t(file.size()) != size) { if (file) file.close(); return false; }
  uint8_t* buffer = static_cast<uint8_t*>(allocateTemporary(size));
  if (buffer == nullptr) { file.close(); lastError_ = Error::MediaOom; return false; }
  const size_t got = file.read(buffer, size);
  file.close();
  const bool ok = got == size && adapter_.onPngImage(buffer, size);
  freeTemporary(buffer);
  return ok;
}

bool IDotMatrixAutomation::loadAlarmMedia(uint8_t slot) {
  if (slot >= IDotMatrixAlarmSettings::SLOT_COUNT) return false;
  const AlarmSlot& alarm = alarms_[slot];
  if (alarm.mediaSize == 0) return false;
  char path[20];
  alarmPath(slot, path, sizeof(path));
  if (alarm.contentType == ALARM_CONTENT_RAW) return streamRawFile(path, alarm.mediaSize);
  if (alarm.contentType == ALARM_CONTENT_GIF) return streamGifFile(path, alarm.mediaSize);
  return false;
}

bool IDotMatrixAutomation::loadScheduleMedia(uint8_t index) {
  if (index >= IDotMatrixScheduleActivitySettings::MAX_ACTIVITIES) return false;
  const ScheduleActivity& activity = scheduleActivities_[index];
  if (!activity.configured || activity.mediaSize == 0) return false;
  char path[20];
  schedulePath(index, path, sizeof(path));
  if (activity.contentType == SCHEDULE_CONTENT_GIF) return streamGifFile(path, activity.mediaSize);
  if (activity.contentType == SCHEDULE_CONTENT_IMAGE) return loadPngFile(path, activity.mediaSize);
  if (activity.contentType == SCHEDULE_CONTENT_TEXT) return loadTextFile(path, activity.mediaSize);
  return false;
}

uint8_t IDotMatrixAutomation::selectedEffect() const {
  return strip.getFirstSelectedSeg().mode;
}

bool IDotMatrixAutomation::automationOwnsDisplay() const {
  const uint8_t mode = selectedEffect();
  return mode == adapter_.displayEffectId() ||
    (adapter_.isGifPending() && mode == FX_MODE_STATIC);
}

void IDotMatrixAutomation::restoreEffect(uint8_t effect, bool valid) {
  const bool owned = automationOwnsDisplay();
  adapter_.cancelAutomationContent();
  if (!owned) return; // user/API deliberately took WLED ownership meanwhile

  if (!valid || effect == adapter_.displayEffectId()) {
    adapter_.restoreClockFallback();
    return;
  }

  auto& segment = strip.getFirstSelectedSeg();
  if (segment.mode != effect) segment.setMode(effect);
  effectCurrent = effect;
  stateUpdated(CALL_MODE_DIRECT_CHANGE);
  strip.trigger();
}

void IDotMatrixAutomation::refreshBuzzer(uint32_t now) {
  const bool alarmWanted = alarmActive_ &&
    activeAlarmSlot_ < IDotMatrixAlarmSettings::SLOT_COUNT &&
    alarms_[activeAlarmSlot_].buzzer != 0;

  if (alarmWanted) {
    if (!alarmBuzzerOwned_) {
      // An alarm has priority over a manual test or a finite program alert and
      // repeats for the configured alarm duration.
      buzzer_.startTrill(now);
      alarmBuzzerOwned_ = true;
      scheduleBuzzerOwned_ = false;
    }
    return;
  }

  if (alarmBuzzerOwned_) {
    buzzer_.stop();
    alarmBuzzerOwned_ = false;
  }

  if (scheduleBuzzerOwned_) {
    // Program audio is only a finite activation notice.  Never restart it just
    // because the activity remains active.  Stop it early only if the program
    // ends or its global sound option is disabled.
    if (scheduleActiveIndex_ < 0 || (scheduleGlobalFlags_ & 0x02u) == 0) {
      buzzer_.stop();
      scheduleBuzzerOwned_ = false;
    } else if (!buzzer_.isPlaying()) {
      scheduleBuzzerOwned_ = false;
    }
  }
}

void IDotMatrixAutomation::loop(uint32_t now) {
  if (!begun_) return;
  bool alarmEnded = false;
  updateAlarms(now, alarmEnded);
  updateSchedule(now);
  refreshBuzzer(now);

  if (alarmEnded) {
    if (scheduleActiveIndex_ < 0) {
      restoreEffect(alarmReturnEffect_, alarmReturnValid_);
    }
    alarmReturnValid_ = false;
  }
}

uint8_t IDotMatrixAutomation::configuredAlarmCount() const {
  uint8_t count = 0;
  for (uint8_t slot = 0; slot < IDotMatrixAlarmSettings::SLOT_COUNT; ++slot) {
    if (alarms_[slot].configured) ++count;
  }
  return count;
}

uint8_t IDotMatrixAutomation::configuredScheduleCount() const {
  uint8_t count = 0;
  for (uint8_t index = 0; index < IDotMatrixScheduleActivitySettings::MAX_ACTIVITIES; ++index) {
    if (scheduleActivities_[index].configured) ++count;
  }
  return count;
}

uint32_t IDotMatrixAutomation::crc32(const uint8_t* data, size_t length) {
  if (data == nullptr && length != 0) return 0;
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t index = 0; index < length; ++index) {
    crc ^= data[index];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
    }
  }
  return crc ^ 0xFFFFFFFFu;
}

const char* IDotMatrixAutomation::lastErrorText() const {
  switch (lastError_) {
    case Error::Preferences: return "preferences";
    case Error::FileWrite: return "file-write";
    case Error::StagingOom: return "schedule-staging-oom";
    case Error::MediaLoad: return "media-load";
    case Error::MediaOom: return "media-oom";
    case Error::None:
    default: return "none";
  }
}
