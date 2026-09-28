#!/usr/bin/env python3
from pathlib import Path
src = (Path(__file__).resolve().parents[1] / "usermod_idotmatrix.cpp").read_text(encoding="utf-8")
for token in [
    'CFG_BUZZER_TYPE', 'CFG_BUZZER_PASSIVE_TRIGGER',
    'BUZZER_TYPE_ACTIVE', 'BUZZER_TYPE_PASSIVE',
    'BUZZER_TRIGGER_HIGH', 'BUZZER_TRIGGER_LOW',
    'BUZZER_PASSIVE_FREQUENCY_HZ = 2000u',
    'ledcAttach(', 'ledcWriteTone(', 'ledcWrite(', 'ledcDetach(',
    'ledcSetup(', 'ledcAttachPin(', 'ledcDetachPin(',
    "addOption(dd,'Active',0)", "addOption(dd,'Passive',1)",
    "addOption(dd,'High',0)", "addOption(dd,'Low',1)",
]:
    assert token in src, token
assert 'passiveIdleLevel()' in src
assert 'passiveIdleDuty()' in src
assert 'low-level trigger' in src
print('Buzzer backend regression checks passed.')
