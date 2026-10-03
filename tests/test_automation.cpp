#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#define private public
#include "../IDotMatrixAutomation.h"
#undef private

#include "automation_stub/IDotMatrixAutomationDeps.h"
#include "automation_stub/Preferences.h"
#include "automation_stub/wled.h"

TestFS WLED_FS;
time_t localTime = 0;
uint32_t testMillis = 0;
uint8_t effectCurrent = 0;
int testYear = 1970;
int testMonth = 1;
int testDay = 1;
int testHour = 0;
int testMinute = 0;
int testSecond = 0;
TestStrip strip;

class NullProtocolEvents final : public IDotMatrixProtocolEvents {
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
  bool onTextBegin(const IDotMatrixTextSettings&) override { return true; }
  void onTextGlyph(uint8_t, const uint8_t*, size_t) override {}
  void onTextComplete() override {}
  bool onRawImageBegin(size_t) override { return true; }
  bool onRawImageData(size_t, const uint8_t*, size_t) override { return true; }
  bool onRawImageComplete(bool valid) override { return valid; }
  bool onPngImage(const uint8_t*, size_t) override { return true; }
  bool onGifBegin(size_t) override { return true; }
  bool onGifData(size_t, const uint8_t*, size_t) override { return true; }
  bool onGifComplete(bool valid) override { return valid; }
};

static uint32_t crc32(const uint8_t* data, size_t length) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
    }
  }
  return crc ^ 0xFFFFFFFFu;
}

struct Fixture {
  IDotMatrixRenderer renderer;
  IDotMatrixWLEDAdapter adapter;
  IDotMatrixMedia media;
  IDotMatrixAutomation automation;

  Fixture() : automation(renderer, adapter, media) {}
};

static void resetState() {
  WLED_FS.clear();
  TestPreferencesStore::clear();
  testMillis = 0;
  localTime = 0;
  testYear = 1970;
  testMonth = 1;
  testDay = 1;
  testHour = 0;
  testMinute = 0;
  testSecond = 0;
  effectCurrent = 0;
}

static IDotMatrixScheduleActivitySettings scheduleSettings(
  uint8_t index, const std::vector<uint8_t>& media, uint8_t mediaId = 1
) {
  IDotMatrixScheduleActivitySettings settings{};
  settings.index = index;
  settings.flags = 0xFF;
  settings.startHour = 8;
  settings.startMinute = 0;
  settings.endHour = 18;
  settings.endMinute = 0;
  settings.contentType = 0x01;
  settings.mediaSize = static_cast<uint32_t>(media.size());
  settings.mediaCRC = crc32(media.data(), media.size());
  settings.mediaId = mediaId;
  return settings;
}

static IDotMatrixAlarmSettings fullAlarmSettings(
  uint8_t slot, const std::vector<uint8_t>& media, uint8_t hour, uint8_t minute, uint8_t mediaId
) {
  IDotMatrixAlarmSettings alarm{};
  alarm.slot = slot;
  alarm.flags = 0x01;
  alarm.hour = hour;
  alarm.minute = minute;
  alarm.durationSeconds = 10;
  alarm.contentType = 0x02;
  alarm.mediaSize = static_cast<uint32_t>(media.size());
  alarm.mediaCRC = crc32(media.data(), media.size());
  alarm.mediaId = mediaId;
  alarm.packetLength = IDotMatrixAlarmSettings::FULL_HEADER_SIZE;
  alarm.fullHeader = true;
  return alarm;
}

static void commitOne(Fixture& f, const std::vector<uint8_t>& media, uint8_t mediaId = 1) {
  f.automation.onScheduleGlobal(0x01);
  const auto settings = scheduleSettings(0, media, mediaId);
  assert(f.automation.onScheduleActivity(settings, media.data(), media.size()));
  f.automation.commitScheduleUpload();
}

static void testPersistenceAndSuccessfulReplacement() {
  resetState();
  const std::vector<uint8_t> first{1, 2, 3, 4};
  const std::vector<uint8_t> second{5, 6, 7, 8, 9};

  {
    Fixture f;
    f.automation.begin();
    commitOne(f, first, 10);
    assert(f.automation.configuredScheduleCount() == 1);
    assert(WLED_FS.get("/idot_s0.bin") == first);
    assert(TestPreferencesStore::bytesLength("idot-sched", "s0") ==
      sizeof(IDotMatrixAutomation::ScheduleActivity));
    commitOne(f, second, 11);
    assert(f.automation.configuredScheduleCount() == 1);
    assert(WLED_FS.get("/idot_s0.bin") == second);
    assert(std::string(f.automation.lastErrorText()) == "none");
  }

  // A new object models reboot/loadPersistence and must recover the committed
  // program from NVS plus its media file.
  {
    Fixture rebooted;
    rebooted.automation.begin();
    assert(rebooted.automation.configuredScheduleCount() == 1);
    assert(rebooted.automation.scheduleActivities_[0].mediaId == 11);
    assert(rebooted.automation.scheduleActivities_[0].mediaSize == second.size());
  }
}

static void testTemporaryWriteFailure() {
  resetState();
  const std::vector<uint8_t> media{1, 2, 3};
  Fixture f;
  f.automation.begin();
  f.automation.onScheduleGlobal(0x01);
  WLED_FS.failOpenWrite("/idot_t0.bin");
  const auto settings = scheduleSettings(0, media);
  assert(!f.automation.onScheduleActivity(settings, media.data(), media.size()));
  assert(std::string(f.automation.lastErrorText()) == "file-write");
  assert(!WLED_FS.exists("/idot_s0.bin"));
  assert(TestPreferencesStore::bytesLength("idot-sched", "s0") == 0);
}

static void testRenameFailurePreservesPreviousSchedule() {
  resetState();
  const std::vector<uint8_t> oldMedia{1, 2, 3, 4};
  const std::vector<uint8_t> newMedia{9, 8, 7, 6, 5};
  Fixture f;
  f.automation.begin();
  commitOne(f, oldMedia, 20);
  const auto oldMeta = TestPreferencesStore::bytes("idot-sched", "s0");

  f.automation.onScheduleGlobal(0x01);
  const auto settings = scheduleSettings(0, newMedia, 21);
  assert(f.automation.onScheduleActivity(settings, newMedia.data(), newMedia.size()));
  WLED_FS.failRename("/idot_t0.bin", "/idot_s0.bin");
  f.automation.commitScheduleUpload();

  assert(std::string(f.automation.lastErrorText()) == "file-write");
  assert(f.automation.configuredScheduleCount() == 1);
  assert(f.automation.scheduleActivities_[0].mediaId == 20);
  assert(WLED_FS.get("/idot_s0.bin") == oldMedia);
  assert(TestPreferencesStore::bytes("idot-sched", "s0") == oldMeta);
  assert(!WLED_FS.exists("/idot_t0.bin"));
  assert(!WLED_FS.exists("/idot_b0.bin"));

  Fixture rebooted;
  rebooted.automation.begin();
  assert(rebooted.automation.configuredScheduleCount() == 1);
  assert(rebooted.automation.scheduleActivities_[0].mediaId == 20);
}

static void testRollbackFailureConvergesToEmptyState() {
  resetState();
  const std::vector<uint8_t> oldMedia{1, 2, 3, 4};
  const std::vector<uint8_t> newMedia{9, 8, 7, 6};
  Fixture f;
  f.automation.begin();
  commitOne(f, oldMedia, 30);

  f.automation.onScheduleGlobal(0x01);
  const auto settings = scheduleSettings(0, newMedia, 31);
  assert(f.automation.onScheduleActivity(settings, newMedia.data(), newMedia.size()));
  WLED_FS.failRename("/idot_t0.bin", "/idot_s0.bin");
  WLED_FS.failRename("/idot_b0.bin", "/idot_s0.bin");
  f.automation.commitScheduleUpload();

  assert(f.automation.configuredScheduleCount() == 0);
  assert(!WLED_FS.exists("/idot_s0.bin"));
  assert(!WLED_FS.exists("/idot_b0.bin"));
  assert(TestPreferencesStore::bytesLength("idot-sched", "s0") == 0);

  Fixture rebooted;
  rebooted.automation.begin();
  assert(rebooted.automation.configuredScheduleCount() == 0);
}

static void testNvsFailureRollsBackMedia() {
  resetState();
  const std::vector<uint8_t> oldMedia{4, 3, 2, 1};
  const std::vector<uint8_t> newMedia{5, 5, 5, 5, 5};
  Fixture f;
  f.automation.begin();
  commitOne(f, oldMedia, 40);
  const auto oldMeta = TestPreferencesStore::bytes("idot-sched", "s0");

  f.automation.onScheduleGlobal(0x01);
  const auto settings = scheduleSettings(0, newMedia, 41);
  assert(f.automation.onScheduleActivity(settings, newMedia.data(), newMedia.size()));
  TestPreferencesStore::failNextPut("idot-sched/s0");
  f.automation.commitScheduleUpload();

  assert(std::string(f.automation.lastErrorText()) == "preferences");
  assert(f.automation.configuredScheduleCount() == 1);
  assert(f.automation.scheduleActivities_[0].mediaId == 40);
  assert(WLED_FS.get("/idot_s0.bin") == oldMedia);
  assert(TestPreferencesStore::bytes("idot-sched", "s0") == oldMeta);
}

static void testNvsRollbackMetadataFailureConvergesToEmptyState() {
  resetState();
  const std::vector<uint8_t> oldMedia{6, 6, 6, 6};
  const std::vector<uint8_t> newMedia{7, 7, 7, 7, 7};
  Fixture f;
  f.automation.begin();
  commitOne(f, oldMedia, 45);

  f.automation.onScheduleGlobal(0x01);
  const auto settings = scheduleSettings(0, newMedia, 46);
  assert(f.automation.onScheduleActivity(settings, newMedia.data(), newMedia.size()));
  // Fail both the new metadata write and the attempt to re-assert the previous
  // metadata. The implementation must not keep a configured schedule whose
  // persistence state is no longer trustworthy.
  TestPreferencesStore::failNextPut("idot-sched/s0", 2);
  f.automation.commitScheduleUpload();

  assert(std::string(f.automation.lastErrorText()) == "preferences");
  assert(f.automation.configuredScheduleCount() == 0);
  assert(!WLED_FS.exists("/idot_s0.bin"));
  assert(TestPreferencesStore::bytesLength("idot-sched", "s0") == 0);
}

static void testMissingMediaAndCorruptMetadataRecovery() {
  resetState();
  const std::vector<uint8_t> media{1, 3, 3, 7};
  {
    Fixture f;
    f.automation.begin();
    commitOne(f, media, 50);
  }
  WLED_FS.remove("/idot_s0.bin");
  {
    Fixture rebooted;
    rebooted.automation.begin();
    assert(rebooted.automation.configuredScheduleCount() == 0);
    assert(TestPreferencesStore::bytesLength("idot-sched", "s0") == 0);
  }

  resetState();
  TestPreferencesStore::setRaw("idot-sched", "s0", {1, 2, 3});
  TestPreferencesStore::setRaw("idot-alarm", "a0", {4, 5});
  Fixture corrupt;
  corrupt.automation.begin();
  assert(corrupt.automation.configuredScheduleCount() == 0);
  assert(corrupt.automation.configuredAlarmCount() == 0);
  assert(TestPreferencesStore::bytesLength("idot-sched", "s0") == 0);
  assert(TestPreferencesStore::bytesLength("idot-alarm", "a0") == 0);
}

static void testAlarmPersistence() {
  resetState();
  {
    Fixture f;
    f.automation.begin();
    IDotMatrixAlarmSettings alarm{};
    alarm.slot = 0;
    alarm.flags = 0x03;
    alarm.hour = 7;
    alarm.minute = 30;
    alarm.durationSeconds = 15;
    alarm.packetLength = 9;
    assert(f.automation.onAlarm(alarm, nullptr, 0));
    assert(f.automation.configuredAlarmCount() == 1);
  }
  Fixture rebooted;
  rebooted.automation.begin();
  assert(rebooted.automation.configuredAlarmCount() == 1);
  assert(rebooted.automation.alarms_[0].hour == 7);
  assert(rebooted.automation.alarms_[0].minute == 30);
}

static void testAlarmNvsFailureRollsBackFileAndMetadata() {
  resetState();
  const std::vector<uint8_t> oldMedia{1, 2, 3, 4};
  const std::vector<uint8_t> newMedia{9, 8, 7, 6, 5};
  Fixture f;
  f.automation.begin();

  auto oldAlarm = fullAlarmSettings(0, oldMedia, 7, 30, 10);
  assert(f.automation.onAlarm(oldAlarm, oldMedia.data(), oldMedia.size()));
  const auto oldMeta = TestPreferencesStore::bytes("idot-alarm", "a0");

  auto newAlarm = fullAlarmSettings(0, newMedia, 8, 45, 11);
  TestPreferencesStore::failNextPut("idot-alarm/a0");
  assert(!f.automation.onAlarm(newAlarm, newMedia.data(), newMedia.size()));
  assert(std::string(f.automation.lastErrorText()) == "preferences");
  assert(f.automation.alarms_[0].hour == 7);
  assert(f.automation.alarms_[0].mediaSize == oldMedia.size());
  assert(WLED_FS.get("/idot_a0.bin") == oldMedia);
  assert(TestPreferencesStore::bytes("idot-alarm", "a0") == oldMeta);
  assert(!WLED_FS.exists("/idot_ab0.bin"));

  Fixture rebooted;
  rebooted.automation.begin();
  assert(rebooted.automation.configuredAlarmCount() == 1);
  assert(rebooted.automation.alarms_[0].hour == 7);
  assert(rebooted.automation.alarms_[0].mediaSize == oldMedia.size());
  assert(WLED_FS.get("/idot_a0.bin") == oldMedia);
}

static void testAlarmNvsRollbackFailureConvergesToEmpty() {
  resetState();
  const std::vector<uint8_t> oldMedia{3, 3, 3, 3};
  const std::vector<uint8_t> newMedia{4, 4, 4, 4, 4};
  Fixture f;
  f.automation.begin();
  auto oldAlarm = fullAlarmSettings(0, oldMedia, 6, 10, 20);
  assert(f.automation.onAlarm(oldAlarm, oldMedia.data(), oldMedia.size()));

  auto newAlarm = fullAlarmSettings(0, newMedia, 6, 20, 21);
  TestPreferencesStore::failNextPut("idot-alarm/a0", 2);
  assert(!f.automation.onAlarm(newAlarm, newMedia.data(), newMedia.size()));
  assert(std::string(f.automation.lastErrorText()) == "preferences");
  assert(f.automation.configuredAlarmCount() == 0);
  assert(TestPreferencesStore::bytesLength("idot-alarm", "a0") == 0);
  assert(!WLED_FS.exists("/idot_a0.bin"));
  assert(!WLED_FS.exists("/idot_ab0.bin"));
}

static void testAlarmBootRecoveryClearsUnrecoverableMedia() {
  resetState();
  const std::vector<uint8_t> media{1, 2, 3, 4};
  {
    Fixture f;
    f.automation.begin();
    auto alarm = fullAlarmSettings(0, media, 5, 15, 30);
    assert(f.automation.onAlarm(alarm, media.data(), media.size()));
  }
  WLED_FS.set("/idot_a0.bin", {9, 9, 9}); // wrong size, no backup
  Fixture rebooted;
  rebooted.automation.begin();
  assert(rebooted.automation.configuredAlarmCount() == 0);
  assert(TestPreferencesStore::bytesLength("idot-alarm", "a0") == 0);
  assert(!WLED_FS.exists("/idot_a0.bin"));
}

static void testOneShotAlarmNvsFailureConvergesToEmpty() {
  resetState();
  Fixture f;
  f.automation.begin();

  testMillis = 1000;
  testYear = 1970;
  IDotMatrixTimeSyncSettings sync{};
  sync.year = 2026; sync.month = 10; sync.day = 3;
  sync.hour = 6; sync.minute = 30; sync.second = 0;
  f.automation.onTimeSync(sync);

  IDotMatrixAlarmSettings alarm{};
  alarm.slot = 0;
  alarm.flags = 0x01; // enabled one-shot: no weekday bits
  alarm.hour = 6;
  alarm.minute = 30;
  alarm.durationSeconds = 10;
  alarm.packetLength = 12;
  assert(f.automation.onAlarm(alarm, nullptr, 0));
  assert(f.automation.configuredAlarmCount() == 1);

  // Consuming a one-shot writes the disabled metadata. If that NVS write
  // fails, rc.2 must converge to an empty slot rather than firing an alarm
  // that can resurrect after reboot with the old enabled metadata.
  TestPreferencesStore::failNextPut("idot-alarm/a0");
  f.automation.loop(1600);
  assert(!f.automation.alarmActive_);
  assert(f.automation.configuredAlarmCount() == 0);
  assert(std::string(f.automation.lastErrorText()) == "preferences");
  assert(TestPreferencesStore::bytesLength("idot-alarm", "a0") == 0);

  Fixture rebooted;
  rebooted.automation.begin();
  assert(rebooted.automation.configuredAlarmCount() == 0);
}

static void testScheduleQuietCommitWaitsForMultipart() {
  resetState();
  const std::vector<uint8_t> first{1, 2, 3, 4};
  const std::vector<uint8_t> second{5, 6, 7, 8, 9};
  Fixture f;
  f.automation.begin();
  NullProtocolEvents events;
  IDotMatrixProtocol protocol(events);
  f.automation.attachProtocol(&protocol);

  f.automation.onScheduleGlobal(0x01);
  auto s0 = scheduleSettings(0, first, 40);
  assert(f.automation.onScheduleActivity(s0, first.data(), first.size()));

  // The next Program object is still in the protocol multipart assembler well
  // beyond the 900 ms quiet commit delay. The first activity must not publish.
  protocol.programTransfer_.active = true;
  protocol.programTransfer_.lastRxMs = 100;
  testMillis = f.automation.scheduleLastRxMs_ + IDotMatrixAutomation::SCHEDULE_COMMIT_DELAY_MS + 100;
  f.automation.updateSchedule(testMillis);
  assert(f.automation.scheduleUploadOpen_);
  assert(f.automation.configuredScheduleCount() == 0);

  // Complete the second object; onScheduleActivity restarts the quiet period.
  protocol.programTransfer_.active = false;
  auto s1 = scheduleSettings(1, second, 41);
  assert(f.automation.onScheduleActivity(s1, second.data(), second.size()));
  const uint32_t secondCompleteAt = f.automation.scheduleLastRxMs_;
  f.automation.updateSchedule(secondCompleteAt + IDotMatrixAutomation::SCHEDULE_COMMIT_DELAY_MS - 1);
  assert(f.automation.configuredScheduleCount() == 0);
  f.automation.updateSchedule(secondCompleteAt + IDotMatrixAutomation::SCHEDULE_COMMIT_DELAY_MS);
  assert(f.automation.configuredScheduleCount() == 2);
  assert(WLED_FS.get("/idot_s0.bin") == first);
  assert(WLED_FS.get("/idot_s1.bin") == second);
}

static void testAutomationRejectsInvalidClockFields() {
  resetState();
  Fixture f;
  f.automation.begin();

  IDotMatrixAlarmSettings alarm{};
  alarm.slot = 0;
  alarm.flags = 0x01;
  alarm.hour = 24;
  alarm.minute = 0;
  alarm.packetLength = 9;
  assert(!f.automation.onAlarm(alarm, nullptr, 0));
  assert(f.automation.configuredAlarmCount() == 0);

  const std::vector<uint8_t> media{1, 2, 3};
  auto schedule = scheduleSettings(0, media, 55);
  schedule.endMinute = 60;
  f.automation.onScheduleGlobal(0x01);
  assert(!f.automation.onScheduleActivity(schedule, media.data(), media.size()));
  assert(f.automation.configuredScheduleCount() == 0);
}

static void testTimeBehaviour() {
  resetState();
  Fixture f;
  f.automation.begin();

  testYear = 2026;
  testMonth = 9;
  testDay = 10;
  testHour = 14;
  testMinute = 25;
  testSecond = 33;
  IDotMatrixAutomation::DateTimeParts current{};
  assert(f.automation.currentDateTime(1234, current));
  assert(current.year == 2026 && current.month == 9 && current.day == 10);
  assert(current.hour == 14 && current.minute == 25 && current.second == 33);

  testYear = 1970; // invalidate WLED localTime and exercise app-time operation
  testMillis = 1000;
  IDotMatrixTimeSyncSettings sync{};
  sync.year = 2026;
  sync.month = 12;
  sync.day = 31;
  sync.hour = 23;
  sync.minute = 59;
  sync.second = 50;
  f.automation.onTimeSync(sync);
  assert(f.automation.currentDateTime(16000, current));
  assert(current.year == 2027 && current.month == 1 && current.day == 1);
  assert(current.hour == 0 && current.minute == 0 && current.second == 5);

  assert(IDotMatrixAutomation::weekdayBit(2026, 9, 7) == 0x02u); // Monday
  assert(IDotMatrixAutomation::weekdayBit(2026, 9, 6) == 0x80u); // Sunday

  IDotMatrixAutomation::ScheduleActivity normal{};
  normal.startHour = 8;
  normal.endHour = 10;
  assert(IDotMatrixAutomation::scheduleTimeInside(normal, 8u * 60u));
  assert(IDotMatrixAutomation::scheduleTimeInside(normal, 9u * 60u + 59u));
  assert(!IDotMatrixAutomation::scheduleTimeInside(normal, 10u * 60u));

  IDotMatrixAutomation::ScheduleActivity overnight{};
  overnight.startHour = 22;
  overnight.endHour = 6;
  assert(IDotMatrixAutomation::scheduleTimeInside(overnight, 23u * 60u));
  assert(IDotMatrixAutomation::scheduleTimeInside(overnight, 5u * 60u));
  assert(!IDotMatrixAutomation::scheduleTimeInside(overnight, 12u * 60u));
}


static void testAppTimeSyncOverridesValidWledClockForAlarm() {
  resetState();
  Fixture f;
  f.automation.begin();

  // Model the S3/NTP regression: WLED exposes a valid localTime, but the
  // iDotMatrix app has just supplied a different local clock. Compatibility
  // requires Alarm/Program to follow the app-synchronized time.
  testYear = 2026;
  testMonth = 9;
  testDay = 16;
  testHour = 0;
  testMinute = 51;
  testSecond = 0;

  testMillis = 1000;
  IDotMatrixTimeSyncSettings sync{};
  sync.year = 2026;
  sync.month = 9;
  sync.day = 16;
  sync.hour = 2;
  sync.minute = 51;
  sync.second = 0;
  f.automation.onTimeSync(sync);

  IDotMatrixAlarmSettings alarm{};
  alarm.slot = 0;
  alarm.flags = 0x01;
  alarm.hour = 2;
  alarm.minute = 51;
  alarm.durationSeconds = 10;
  alarm.buzzer = 1;
  alarm.packetLength = 12;
  assert(f.automation.onAlarm(alarm, nullptr, 0));

  f.automation.loop(1600);
  assert(f.automation.alarmActive_);
  assert(f.automation.activeAlarmSlot_ == 0);
  assert(f.automation.alarmSoundRequested());
  assert((f.automation.alarms_[0].flags & 0x01u) == 0); // one-shot consumed
}

static void testSilentAlarmRemainsSilent() {
  resetState();
  Fixture f;
  f.automation.begin();

  testMillis = 1000;
  testYear = 1970;
  IDotMatrixTimeSyncSettings sync{};
  sync.year = 2026; sync.month = 9; sync.day = 28;
  sync.hour = 9; sync.minute = 15; sync.second = 0;
  f.automation.onTimeSync(sync);

  IDotMatrixAlarmSettings alarm{};
  alarm.slot = 0;
  alarm.flags = 0x01;
  alarm.hour = 9;
  alarm.minute = 15;
  alarm.durationSeconds = 10;
  alarm.buzzer = 0;
  alarm.packetLength = 12;
  assert(f.automation.onAlarm(alarm, nullptr, 0));

  f.automation.loop(1600);
  assert(f.automation.alarmActive_);
  assert(!f.automation.alarmSoundRequested());
}

static void testScheduleSoundStartsOnActivation() {
  resetState();
  Fixture f;
  f.automation.begin();

  testMillis = 1000;
  testYear = 1970;
  IDotMatrixTimeSyncSettings sync{};
  sync.year = 2026; sync.month = 9; sync.day = 28;
  sync.hour = 9; sync.minute = 0; sync.second = 0;
  f.automation.onTimeSync(sync);

  const std::vector<uint8_t> media{1, 2, 3, 4};
  f.automation.onScheduleGlobal(0x03); // enabled + sound
  const auto settings = scheduleSettings(0, media, 88);
  assert(f.automation.onScheduleActivity(settings, media.data(), media.size()));
  f.automation.commitScheduleUpload();

  f.automation.loop(1600);
  assert(f.automation.scheduleActiveIndex_ == 0);
  assert(f.automation.takeScheduleSoundRequest());
  assert(!f.automation.takeScheduleSoundRequest());
}


static void testAlarmSoundUsesPostMediaTimestamp() {
  resetState();
  Fixture f;
  f.automation.begin();

  testMillis = 1000;
  testYear = 1970;
  IDotMatrixTimeSyncSettings sync{};
  sync.year = 2026; sync.month = 9; sync.day = 28;
  sync.hour = 10; sync.minute = 30; sync.second = 0;
  f.automation.onTimeSync(sync);

  const std::vector<uint8_t> media{1, 2, 3, 4, 5, 6};
  IDotMatrixAlarmSettings alarm{};
  alarm.slot = 0;
  alarm.flags = 0x01;
  alarm.hour = 10;
  alarm.minute = 30;
  alarm.durationSeconds = 10;
  alarm.contentType = 0x02; // RAW
  alarm.buzzer = 1;
  alarm.mediaSize = static_cast<uint32_t>(media.size());
  alarm.mediaCRC = crc32(media.data(), media.size());
  alarm.packetLength = IDotMatrixAlarmSettings::FULL_HEADER_SIZE;
  alarm.fullHeader = true;
  assert(f.automation.onAlarm(alarm, media.data(), media.size()));

  f.adapter.mediaLoadDelayMs = 250;
  testMillis = 1600;
  f.automation.loop(1600);

  assert(f.automation.alarmActive_);
  assert(f.automation.alarmSoundRequested());
  assert(f.automation.alarmEndsAt_ == 11850);
}

static void testScheduleSoundUsesPostMediaTimestamp() {
  resetState();
  Fixture f;
  f.automation.begin();

  testMillis = 1000;
  testYear = 1970;
  IDotMatrixTimeSyncSettings sync{};
  sync.year = 2026; sync.month = 9; sync.day = 28;
  sync.hour = 9; sync.minute = 0; sync.second = 0;
  f.automation.onTimeSync(sync);

  const std::vector<uint8_t> media{1, 2, 3, 4};
  f.automation.onScheduleGlobal(0x03); // enabled + sound
  const auto settings = scheduleSettings(0, media, 89);
  assert(f.automation.onScheduleActivity(settings, media.data(), media.size()));
  f.automation.commitScheduleUpload();

  f.adapter.mediaLoadDelayMs = 250;
  testMillis = 1600;
  f.automation.loop(1600);

  assert(f.automation.scheduleActiveIndex_ == 0);
  assert(testMillis == 1850);
  assert(f.automation.takeScheduleSoundRequest());
}

static void testDeviceResetClearsPersistentAutomationButKeepsTimeSync() {
  resetState();
  Fixture f;
  f.automation.begin();

  IDotMatrixTimeSyncSettings sync{};
  sync.year = 2026; sync.month = 9; sync.day = 12;
  sync.hour = 16; sync.minute = 15; sync.second = 0;
  testMillis = 1000;
  testYear = 1970;
  f.automation.onTimeSync(sync);
  assert(f.automation.timeValid());

  IDotMatrixAlarmSettings alarm{};
  alarm.slot = 0; alarm.flags = 0x01; alarm.hour = 7; alarm.minute = 30;
  alarm.packetLength = 9;
  assert(f.automation.onAlarm(alarm, nullptr, 0));

  const std::vector<uint8_t> media{4, 5, 6, 7};
  commitOne(f, media, 77);
  assert(f.automation.configuredAlarmCount() == 1);
  assert(f.automation.configuredScheduleCount() == 1);
  assert(TestPreferencesStore::bytesLength("idot-alarm", "a0") != 0);
  assert(TestPreferencesStore::bytesLength("idot-sched", "s0") != 0);
  assert(WLED_FS.exists("/idot_s0.bin"));

  f.automation.resetPersistent();
  assert(f.automation.configuredAlarmCount() == 0);
  assert(f.automation.configuredScheduleCount() == 0);
  assert(!f.automation.scheduleEnabled());
  assert(f.automation.timeValid());
  assert(TestPreferencesStore::bytesLength("idot-alarm", "a0") == 0);
  assert(TestPreferencesStore::bytesLength("idot-sched", "s0") == 0);
  assert(TestPreferencesStore::bytesLength("idot-sched", "flags") == 0);
  assert(!WLED_FS.exists("/idot_s0.bin"));

  Fixture rebooted;
  rebooted.automation.begin();
  assert(rebooted.automation.configuredAlarmCount() == 0);
  assert(rebooted.automation.configuredScheduleCount() == 0);
  assert(!rebooted.automation.scheduleEnabled());
}

static void testExplicitScheduleClear() {
  resetState();
  const std::vector<uint8_t> media{8, 8, 8};
  Fixture f;
  f.automation.begin();
  commitOne(f, media, 60);
  assert(f.automation.configuredScheduleCount() == 1);
  f.automation.clearScheduleMeta(0);
  assert(f.automation.configuredScheduleCount() == 0);
  assert(!WLED_FS.exists("/idot_s0.bin"));
  assert(TestPreferencesStore::bytesLength("idot-sched", "s0") == 0);
}

int main() {
  testPersistenceAndSuccessfulReplacement();
  testTemporaryWriteFailure();
  testRenameFailurePreservesPreviousSchedule();
  testRollbackFailureConvergesToEmptyState();
  testNvsFailureRollsBackMedia();
  testNvsRollbackMetadataFailureConvergesToEmptyState();
  testMissingMediaAndCorruptMetadataRecovery();
  testAlarmPersistence();
  testAlarmNvsFailureRollsBackFileAndMetadata();
  testAlarmNvsRollbackFailureConvergesToEmpty();
  testOneShotAlarmNvsFailureConvergesToEmpty();
  testAlarmBootRecoveryClearsUnrecoverableMedia();
  testScheduleQuietCommitWaitsForMultipart();
  testAutomationRejectsInvalidClockFields();
  testTimeBehaviour();
  testAppTimeSyncOverridesValidWledClockForAlarm();
  testSilentAlarmRemainsSilent();
  testScheduleSoundStartsOnActivation();
  testAlarmSoundUsesPostMediaTimestamp();
  testScheduleSoundUsesPostMediaTimestamp();
  testDeviceResetClearsPersistentAutomationButKeepsTimeSync();
  testExplicitScheduleClear();
  return 0;
}
