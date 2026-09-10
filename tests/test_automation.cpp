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
  IDotMatrixBuzzer buzzer;
  IDotMatrixAutomation automation;

  Fixture() : automation(renderer, adapter, media, buzzer) {}
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

  testYear = 1970; // invalidate WLED localTime and exercise app fallback
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
  testTimeBehaviour();
  testExplicitScheduleClear();
  return 0;
}
