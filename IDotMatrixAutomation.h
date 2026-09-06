#pragma once

#include "IDotMatrixProtocol.h"

#include <cstddef>
#include <cstdint>

class IDotMatrixBuzzer;
class IDotMatrixMedia;
class IDotMatrixRenderer;
class IDotMatrixWLEDAdapter;
class Preferences;

// Persistent alarm/program engine for the iDotMatrix protocol.  It deliberately
// lives outside the renderer: schedules own time, metadata, filesystem payloads
// and the optional buzzer, then reuse the same iDotMatrix Display paths as a
// normal app command when content actually has to be shown.
class IDotMatrixAutomation final : public IDotMatrixAutomationEvents {
public:
  IDotMatrixAutomation(
    IDotMatrixRenderer& renderer,
    IDotMatrixWLEDAdapter& adapter,
    IDotMatrixMedia& media,
    IDotMatrixBuzzer& buzzer
  );
  ~IDotMatrixAutomation();

  IDotMatrixAutomation(const IDotMatrixAutomation&) = delete;
  IDotMatrixAutomation& operator=(const IDotMatrixAutomation&) = delete;

  void attachProtocol(IDotMatrixProtocol* protocol) { protocol_ = protocol; }
  void begin();
  void loop(uint32_t now);

  void onTimeSync(const IDotMatrixTimeSyncSettings& settings) override;
  bool onAlarm(
    const IDotMatrixAlarmSettings& settings,
    const uint8_t* media,
    size_t mediaLength
  ) override;
  void onScheduleGlobal(uint8_t flags) override;
  bool onScheduleActivity(
    const IDotMatrixScheduleActivitySettings& settings,
    const uint8_t* media,
    size_t mediaLength
  ) override;

  bool timeValid() const { return appTimeValid_; }
  bool alarmActive() const { return alarmActive_; }
  uint8_t activeAlarmSlot() const { return activeAlarmSlot_; }
  uint8_t configuredAlarmCount() const;
  bool scheduleEnabled() const { return (scheduleGlobalFlags_ & 0x01u) != 0; }
  bool scheduleSoundEnabled() const { return (scheduleGlobalFlags_ & 0x02u) != 0; }
  uint8_t scheduleFlags() const { return scheduleGlobalFlags_; }
  int8_t activeScheduleIndex() const { return scheduleActiveIndex_; }
  uint8_t configuredScheduleCount() const;
  bool scheduleUploadOpen() const { return scheduleUploadOpen_; }
  const char* lastErrorText() const;

private:
  static constexpr uint8_t ALARM_CONTENT_GIF = 0x01;
  static constexpr uint8_t ALARM_CONTENT_RAW = 0x02;
  static constexpr uint16_t SCHEDULE_CONTENT_GIF = 0x01;
  static constexpr uint16_t SCHEDULE_CONTENT_IMAGE = 0x02;
  static constexpr uint16_t SCHEDULE_CONTENT_TEXT = 0x03;
  static constexpr uint32_t SCHEDULE_COMMIT_DELAY_MS = 900u;
  static constexpr uint32_t ALARM_CHECK_INTERVAL_MS = 500u;

  struct AlarmSlot {
    uint8_t configured = 0;
    uint8_t flags = 0;
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t durationSeconds = 10;
    uint8_t reserved1 = 0;
    uint8_t contentType = 0;
    uint8_t buzzer = 0;
    uint8_t reserved2 = 0;
    uint32_t mediaSize = 0;
    uint32_t mediaCRC = 0;
    uint16_t reserved3 = 0;
    uint8_t mediaId = 0;
    uint32_t lastTriggerMinuteKey = 0xFFFFFFFFu;
  };

  struct ScheduleActivity {
    uint8_t configured = 0;
    uint8_t flags = 0;
    uint8_t startHour = 0;
    uint8_t startMinute = 0;
    uint8_t endHour = 0;
    uint8_t endMinute = 0;
    uint16_t contentType = 0;
    uint32_t mediaSize = 0;
    uint32_t mediaCRC = 0;
    uint16_t reserved = 0;
    uint8_t mediaId = 0;
  };

  struct DateTimeParts {
    uint16_t year = 2000;
    uint8_t month = 1;
    uint8_t day = 1;
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;
  };

  enum class Error : uint8_t {
    None,
    Preferences,
    FileWrite,
    StagingOom,
    MediaLoad,
    MediaOom
  };

  void loadPersistence();
  void saveAlarmMeta(uint8_t slot);
  void saveScheduleGlobal();
  void saveScheduleMeta(uint8_t index);
  void clearScheduleMeta(uint8_t index);

  bool ensureScheduleStaging();
  void beginScheduleUpload(uint8_t flags);
  void cancelScheduleUpload();
  void commitScheduleUpload();

  bool currentDateTime(uint32_t now, DateTimeParts& value) const;
  static uint8_t weekdayBit(uint16_t year, uint8_t month, uint8_t day);
  static uint8_t daysInMonth(uint16_t year, uint8_t month);
  static bool scheduleTimeInside(const ScheduleActivity& activity, uint16_t nowMinutes);

  void updateAlarms(uint32_t now, bool& alarmEnded);
  void updateSchedule(uint32_t now);
  void startAlarm(uint8_t slot, uint32_t now);
  void stopAlarm(bool deferRestore);
  void startScheduleActivity(uint8_t index, uint32_t now);
  void stopScheduleActivity(bool restoreOutput);

  bool loadAlarmMedia(uint8_t slot);
  bool loadScheduleMedia(uint8_t index);
  bool streamRawFile(const char* path, uint32_t size);
  bool streamGifFile(const char* path, uint32_t size);
  bool loadTextFile(const char* path, uint32_t size);
  bool loadPngFile(const char* path, uint32_t size);
  bool writeFile(const char* path, const uint8_t* data, size_t length);

  bool automationOwnsDisplay() const;
  uint8_t selectedEffect() const;
  void restoreEffect(uint8_t effect, bool valid);
  void refreshBuzzer(uint32_t now);

  static uint32_t crc32(const uint8_t* data, size_t length);
  static void* allocateTemporary(size_t size);
  static void freeTemporary(void* memory);

  IDotMatrixRenderer& renderer_;
  IDotMatrixWLEDAdapter& adapter_;
  IDotMatrixMedia& media_;
  IDotMatrixBuzzer& buzzer_;
  IDotMatrixProtocol* protocol_ = nullptr;

  AlarmSlot alarms_[IDotMatrixAlarmSettings::SLOT_COUNT]{};
  ScheduleActivity scheduleActivities_[IDotMatrixScheduleActivitySettings::MAX_ACTIVITIES]{};
  ScheduleActivity* scheduleStaging_ = nullptr;
  Preferences* alarmPrefs_ = nullptr;
  Preferences* schedulePrefs_ = nullptr;

  bool begun_ = false;
  bool appTimeValid_ = false;
  IDotMatrixTimeSyncSettings appTime_{};
  uint32_t appTimeSyncMillis_ = 0;
  uint32_t lastAlarmCheckAt_ = 0;

  bool alarmActive_ = false;
  uint8_t activeAlarmSlot_ = 0xFF;
  uint32_t alarmEndsAt_ = 0;
  uint8_t alarmReturnEffect_ = 0;
  bool alarmReturnValid_ = false;

  uint8_t scheduleGlobalFlags_ = 0;
  bool scheduleUploadOpen_ = false;
  bool scheduleUploadDirty_ = false;
  uint32_t scheduleLastRxMs_ = 0;
  uint32_t scheduleReceivedMask_ = 0;
  int8_t scheduleActiveIndex_ = -1;
  int8_t scheduleFailedIndex_ = -1;
  uint8_t scheduleReturnEffect_ = 0;
  bool scheduleReturnValid_ = false;

  bool alarmBuzzerOwned_ = false;
  bool scheduleBuzzerOwned_ = false;
  Error lastError_ = Error::None;
};
