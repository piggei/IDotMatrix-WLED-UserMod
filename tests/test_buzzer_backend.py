#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
usermod = (root / "usermod_idotmatrix.cpp").read_text(encoding="utf-8")
buzzer = (root / "IDotMatrixBuzzer.cpp").read_text(encoding="utf-8")
automation = (root / "IDotMatrixAutomation.cpp").read_text(encoding="utf-8")
adapter = (root / "IDotMatrixWLEDAdapter.cpp").read_text(encoding="utf-8")

for token in [
    'CFG_BUZZER_TYPE', 'CFG_BUZZER_PASSIVE_TRIGGER',
    'BUZZER_TYPE_ACTIVE', 'BUZZER_TYPE_PASSIVE',
    'BUZZER_TRIGGER_HIGH', 'BUZZER_TRIGGER_LOW',
    'BUZZER_PASSIVE_FREQUENCY_HZ = 2000u',
    'ledcAttach(', 'ledcWriteTone(', 'ledcWrite(', 'ledcDetach(',
    'ledcSetup(', 'ledcAttachPin(', 'ledcDetachPin(',
    "addOption(dd,'Active',0)", "addOption(dd,'Passive',1)",
    "addOption(dd,'High',0)", "addOption(dd,'Low',1)",
    'bleConnectedNow && !bleConnectedLast_', 'startConnectionBeep(millis())',
    '!automation_.alarmActive() && !buzzer_.isPlaying()',
    'BUZZER_SERVICE_PERIOD_US = 2000u', 'esp_timer_start_periodic(',
    'buzzerTiming=',
]:
    assert token in usermod, token

for token in [
    'void IDotMatrixBuzzer::startCountdownAlert(uint32_t now)',
    'void IDotMatrixBuzzer::startConnectionBeep(uint32_t now)',
    'startPatternLocked(now, 1);',
]:
    assert token in buzzer, token

for token in [
    'alarms_[activeAlarmSlot_].buzzer != 0',
    'buzzer_.startTrill(now)',
    'buzzer_.startScheduleAlert(alertStartNow)',
    'const uint32_t alarmStartNow = millis()',
    'const uint32_t alertStartNow = millis()',
]:
    assert token in automation, token

for token in [
    'countdownBuzzerPending_ = true',
    'bool IDotMatrixWLEDAdapter::takeCountdownBuzzerRequest()',
]:
    assert token in adapter, token

assert 'takeCountdownBuzzerRequest()' in usermod
assert 'startCountdownAlert(millis())' in usermod
assert 'passiveIdleLevel()' in usermod
assert 'passiveIdleDuty()' in usermod
assert 'low-level trigger' in usermod
print('Buzzer backend/runtime routing regression checks passed.')
