#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
usermod = (ROOT / "usermod_idotmatrix.cpp").read_text(encoding="utf-8")
automation_h = (ROOT / "IDotMatrixAutomation.h").read_text(encoding="utf-8")
automation_cpp = (ROOT / "IDotMatrixAutomation.cpp").read_text(encoding="utf-8")

# Internal hardware/timing backend is intentionally gone.
for forbidden in (
    "buzzer-pin", "buzzerType", "buzzerActiveHigh",
    "buzzerPassiveTrigger", "ledcWriteTone", "esp_timer_start_periodic",
    "/idotmatrix/buzzer-test", "PinManager::isPinOk",
):
    assert forbidden not in usermod, forbidden

assert not (ROOT / "IDotMatrixBuzzer.h").exists()
assert not (ROOT / "IDotMatrixBuzzer.cpp").exists()

# Optional external service contract. iDotMatrix must not include the external
# C++ service header: doing so lets PlatformIO LDF auto-discover and compile the
# sibling Buzzer repository even when it is not selected in custom_usermods.
bridge = (ROOT / "IDotMatrixBuzzerBridge.h").read_text(encoding="utf-8")
assert '__has_include("WLEDBuzzerService.h")' not in usermod
assert '#include "WLEDBuzzerService.h"' not in usermod
assert "WLEDBuzzerService::instance()" not in usermod
for required in (
    'wledBuzzerServiceReady',
    'wledBuzzerServicePlaying',
    'wledBuzzerServicePlay',
    'wledBuzzerServiceStop',
    'wledBuzzerServiceCurrentSoundId',
    '__attribute__((weak))',
):
    assert required in bridge, required
for required in (
    'BUZZER_SOUND_ALARM = "triple_beep"',
    'BUZZER_SOUND_PROGRAM = "notification"',
    'BUZZER_SOUND_COUNTDOWN = "triple_beep"',
    'BUZZER_SOUND_CONNECT = "connect"',
    'BUZZER_SOUND_DISCONNECT = "disconnect"',
    'CFG_BUZZER_ENABLED',
    "addInfo('iDotMatrix:buzzerEnabled'",
    'WLED Buzzer Usermod is not installed in this build.',
    'Requires the WLED Buzzer Usermod.',
):
    assert required in usermod, required
assert 'playBuzzerSound(BUZZER_SOUND_ALARM, true)' in usermod
assert 'disconnectionBeepPending_' in usermod

# Automation retains only protocol-level sound intent.
assert "IDotMatrixBuzzer" not in automation_h
assert "IDotMatrixBuzzer" not in automation_cpp
assert "alarmSoundRequested() const" in automation_h
assert "takeScheduleSoundRequest()" in automation_h
assert "alarms_[activeAlarmSlot_].buzzer != 0" in automation_cpp
assert "scheduleSoundPending_ = true" in automation_cpp

print("External buzzer service integration contract checks passed.")
