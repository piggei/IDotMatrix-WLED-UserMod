#!/usr/bin/env python3
"""Stable-release package and critical-section regression checks for 0.9.4."""

from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def check_versioning() -> None:
    library = json.loads((ROOT / "library.json").read_text(encoding="utf-8"))
    assert library["version"] == "0.9.4"
    usermod = (ROOT / "usermod_idotmatrix.cpp").read_text(encoding="utf-8")
    assert 'IDOTMATRIX_RELEASE = "0.9.4"' in usermod
    assert 'IDOTMATRIX_BUILD = "0.9.4"' in usermod
    assert "IDOTMATRIX_APP_RELEASE_MINOR = 0x09" in usermod
    assert "IDotMatrixAudioSource" in usermod
    adapter = (ROOT / "IDotMatrixWLEDAdapter.cpp").read_text(encoding="utf-8")
    assert '"iDotMatrix@;;;2"' in adapter
    assert '"iDotMatrix Display@;;;2"' not in adapter
    assert "if (currentPlaylist >= 0) applyPreset(0, CALL_MODE_DIRECT_CHANGE);" in adapter
    preset = (ROOT / "IDotMatrixPreset.cpp").read_text(encoding="utf-8")
    preset_h = (ROOT / "IDotMatrixPreset.h").read_text(encoding="utf-8")
    assert "adapter_.beginTransferIndicator(totalLength, 250u, 0, 0);" in preset
    assert "adapter_.updateTransferIndicator(rxWritten_, rxExpected_);" in preset
    assert "finishUploadIndicator(millis(), false);" in preset
    assert "UPLOAD_IDLE_TIMEOUT_MS = 5000u" in preset_h


def check_release_surface() -> None:
    required = [
        "overrides/esp32c3-16x16.ini",
        "overrides/esp32c3-16x16-audio.ini",
        "overrides/esp32c3-16x16-audio-ota.ini",
        "overrides/matrixportal-s3-hub75.ini",
        "overrides/waveshare-s3-hub75.ini",
        "partitions/WLED_ESP32_4MB_IDOT_NO_OTA.csv",
        "partitions/WLED_ESP32_4MB_IDOT_OTA.csv",
        "RELEASE_NOTES_0.9.4.md",
        "RELEASE_QUALIFICATION_0.9.4.md",
        "RELEASE_NOTES_0.9.4-rc.2.md",
        "RELEASE_NOTES_0.9.4-dev.16.md",
        "RELEASE_NOTES_0.9.4-dev.15.md",
        "RELEASE_NOTES_0.9.4-dev.14.md",
        "RELEASE_NOTES_0.9.4-dev.13.md",
        "RELEASE_NOTES_0.9.4-dev.12.md",
        "RELEASE_NOTES_0.9.4-dev.11.md",
        "RELEASE_NOTES_0.9.4-dev.10.md",
        "RELEASE_NOTES_0.9.4-dev.9.md",
        "RELEASE_NOTES_0.9.4-dev.8.md",
        "RELEASE_NOTES_0.9.4-dev.7.md",
        "RELEASE_NOTES_0.9.4-dev.6.md",
        "RELEASE_NOTES_0.9.3.md",
        "RELEASE_NOTES_0.9.2.md",
        "RELEASE_NOTES_0.9.1.md",
        "RELEASE_NOTES_0.9.0.md",
        "RELEASE_NOTES_0.8.2.md",
        "IDotMatrixAudioSource.h",
        "IDotMatrixAudioSource.cpp",
        "IDotMatrixGifSourceStage.h",
        "IDotMatrixGifSourceStage.cpp",
        "IDotMatrixCarousel.h",
        "IDotMatrixCarousel.cpp",
        "IDotMatrixPreset.h",
        "IDotMatrixPreset.cpp",
        "tests/test_audio_source.cpp",
        "tests/test_ble_framing.cpp",
        "tests/test_gif_source_cache.cpp",
        "IDotMatrixBLEFraming.h",
        "run_host_tests.sh",
        "run_host_sanitizers.sh",
    ]
    for name in required:
        assert (ROOT / name).is_file(), f"missing release file: {name}"
    # Override/partition support files belong in dedicated directories from 0.9.1 onward.
    assert not list(ROOT.glob("platformio_override.ini*"))
    assert not list(ROOT.glob("WLED_ESP32_*_IDOT_*.csv"))
    assert not list(ROOT.glob("RELEASE_NOTES_0.8.2-rc.*.md"))
    assert not list(ROOT.glob("RELEASE_NOTES_0.9.0-dev.*.md"))
    assert not list(ROOT.glob("RELEASE_NOTES_0.9.0-rc.*.md"))
    assert not list(ROOT.glob("RELEASE_NOTES_0.9.1-rc.*.md"))
    assert not list(ROOT.glob("RELEASE_NOTES_0.9.2-dev.*.md"))
    assert list(ROOT.glob("RELEASE_NOTES_0.9.4-rc.2.md"))
    assert list(ROOT.glob("RELEASE_NOTES_0.9.3.md"))
    assert not list(ROOT.glob("RELEASE_NOTES_0.10.0-dev.*.md"))

def check_markdown_links() -> None:
    for path in ROOT.glob("*.md"):
        text = path.read_text(encoding="utf-8")
        for match in re.finditer(r"\[[^\]]+\]\(([^)]+)\)", text):
            target = match.group(1).strip()
            if target.startswith(("http://", "https://", "mailto:", "#")):
                continue
            target = target.split("#", 1)[0]
            if not target:
                continue
            assert (path.parent / target).exists(), f"{path.name}: broken local link {target}"


def check_documentation_contract() -> None:
    architecture = (ROOT / "ARCHITECTURE.md").read_text(encoding="utf-8")
    protocol = (ROOT / "PROTOCOL.md").read_text(encoding="utf-8")
    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    assert "release-candidate qualification" not in protocol.lower()
    assert "current stable release: 0.9.4" in readme.lower()
    profiles = (ROOT / "BUILD_PROFILES.md").read_text(encoding="utf-8")
    library = json.loads((ROOT / "library.json").read_text(encoding="utf-8"))
    testing = (ROOT / "TESTING.md").read_text(encoding="utf-8")
    usermod = (ROOT / "usermod_idotmatrix.cpp").read_text(encoding="utf-8")
    adapter = (ROOT / "IDotMatrixWLEDAdapter.cpp").read_text(encoding="utf-8")

    assert "4112 bytes of permanent" in architecture
    assert "8192" in architecture
    assert "supported 16x16 profiles compile `IDOT_GIF_LZW12`" in architecture
    assert "Most supplied overrides intentionally **replace** the base environment's" in architecture
    assert "esp32c3-16x16-audio-ota.ini" in architecture
    assert "matrixportal-s3-hub75.ini" in architecture
    assert "`${common.default_usermods}`" in architecture
    assert "four 517-byte queue slots" in architecture
    assert "/idot_cache.new" in architecture

    assert "4112 bytes of permanent inline storage" in protocol
    assert "8192-byte logical-packet maximum" in protocol
    assert "16654 payload bytes" in protocol
    assert "application time synchronization is also retained and used as an offline fallback" in protocol
    assert "not a general-purpose PNG" in protocol
    assert "Graffiti full-raster multipart transport" in protocol
    assert "marker: `0x00` first, `0x02` continuation" in protocol
    assert "05 00 00 00 02" in protocol and "05 00 00 00 01" in protocol
    assert "4096 + 4096 + 4096 = 12288 = 64 * 64 * 3" in protocol
    assert "No CRC field is present" in protocol
    assert "two independent fragmentation layers" in protocol
    assert "compact inline PNG" in protocol
    assert "Preset / Default active, pending, cache and backup files" in protocol
    assert "17.0.0-devV5" in readme
    assert "0.17.0-devV5" not in readme
    assert "0.9.4" in testing
    assert "30-second `HH:MM` / 5-second `DD/MM` alternation" in protocol
    assert protocol.count("30-second `HH:MM` / 5-second `DD/MM` alternation") == 1
    assert "styles **0** and **3** use the extra" in protocol
    assert "the date day field stays fixed while the `/` separator and both month digits are" in protocol

    assert readme.startswith("# WLED iDotMatrix Usermod — 0.9.4\n")
    assert "Release: 0.9.4 / build: 0.9.4" in readme
    assert "current stable release: 0.9.4" in readme.lower()
    assert "Graffiti full-raster multipart" in readme
    assert "overrides/esp32c3-16x16-audio-ota.ini" in readme
    assert "partitions/" in readme
    assert "BLE compatibility/security" in readme
    assert "unauthenticated" in readme
    assert "per-slot filesystem frame cache" in readme
    assert "`idotmatrix` wled effect" in readme.lower()
    assert "0.9.2-dev.11" not in readme
    assert "promotion to final 0.9.2 remains blocked" not in readme.lower()

    release_notes = (ROOT / "RELEASE_NOTES_0.9.4.md").read_text(encoding="utf-8")
    assert "MatrixPortal ESP32-S3" in release_notes
    assert "256 KiB" in release_notes and "1 MiB" in release_notes
    assert "persistent source cache" in release_notes.lower()
    assert "gifSourceCache" in release_notes and "gifPrefetch" in release_notes
    assert "0.9.3" in release_notes and "Stable behavioral baseline" in release_notes
    stable_093_notes = (ROOT / "RELEASE_NOTES_0.9.3.md").read_text(encoding="utf-8")
    assert "WLED Buzzer Usermod" in stable_093_notes
    assert "IDotMatrixBuzzerBridge.h" in stable_093_notes
    assert "Buzzer -> Enable" in stable_093_notes
    assert "IDotMatrixBuzzer" in stable_093_notes
    stable_notes = (ROOT / "RELEASE_NOTES_0.9.2.md").read_text(encoding="utf-8")
    assert "Clock presentation persistence" in stable_notes
    assert "NVS" in stable_notes and "showDate=0" in stable_notes

    assert "host" in testing.lower()
    assert "Graffiti full-raster validation" in testing
    assert "three consecutive WLED OTA updates: PASS" in testing
    assert "hardware-validated" in testing
    assert "complex photographic images" in testing
    history = (ROOT / "HISTORY.md").read_text(encoding="utf-8")
    assert history.startswith("## 0.9.4 - 2026-10-03\n")
    assert "4096-byte RGB chunks" in history
    assert "## 0.9.0\n" in history
    assert "## 0.9.0-rc.5" in history and "live TEXT" in history
    assert "audioreactive" in architecture.lower()
    assert "device assets" in protocol.lower()
    assert "compatibility/security note" in protocol.lower()
    assert "complete ATT write" in protocol
    assert "timesign" in protocol.lower()
    assert "imageindex" in protocol.lower()
    assert "overrides/esp32c3-16x16-audio.ini" in profiles
    assert "overrides/esp32c3-16x16-audio-ota.ini" in profiles
    assert "partitions/WLED_ESP32_4MB_IDOT_OTA.csv" in profiles
    assert "NimBLE-Arduino" not in library.get("dependencies", {})
    assert "h2zero/NimBLE-Arduino" not in library.get("dependencies", {})
    assert "CFG_BUZZER_ENABLED" in usermod
    assert "IDotMatrixBuzzerBridge::installed()" in usermod
    assert "buzzerType" not in usermod and "buzzerPassiveTrigger" not in usermod
    assert "clockPreferencesCallback_" in adapter
    assert "CLOCK_PREFS_NAMESPACE" in usermod

def check_final_documentation_hygiene() -> None:
    current = [
        "README.md", "PROTOCOL.md", "ARCHITECTURE.md", "BUILD_PROFILES.md",
        "TESTING.md", "TODO.md", "RELEASE_NOTES_0.9.4.md", "RELEASE_NOTES_0.9.4-rc.2.md", "RELEASE_NOTES_0.9.4-dev.16.md", "RELEASE_NOTES_0.9.4-dev.15.md", "RELEASE_NOTES_0.9.4-dev.14.md", "RELEASE_NOTES_0.9.4-dev.13.md", "RELEASE_NOTES_0.9.4-dev.12.md", "RELEASE_NOTES_0.9.4-dev.11.md", "RELEASE_NOTES_0.9.4-dev.10.md", "RELEASE_NOTES_0.9.4-dev.9.md", "RELEASE_NOTES_0.9.4-dev.8.md", "RELEASE_NOTES_0.9.4-dev.7.md", "RELEASE_NOTES_0.9.4-dev.6.md", "RELEASE_NOTES_0.9.4-dev.5.md", "RELEASE_NOTES_0.9.3.md",
        "RELEASE_NOTES_0.9.2.md", "RELEASE_NOTES_0.9.1.md",
    ]
    for name in current:
        text = (ROOT / name).read_text(encoding="utf-8")
        assert "0.9.1-rc." not in text, f"{name}: stale RC marker"
        assert "0.9.1-dev." not in text, f"{name}: stale previous-development marker"
    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    assert "current stable release: 0.9.4" in readme.lower()
    protocol = (ROOT / "PROTOCOL.md").read_text(encoding="utf-8")
    assert "`iDotMatrix\nDisplay`" not in protocol
    assert "device-level rotation, energy-saving, and reset commands" not in protocol


def check_waveshare_dev7_contract() -> None:
    usermod = (ROOT / "usermod_idotmatrix.cpp").read_text(encoding="utf-8")
    override = (ROOT / "overrides/waveshare-s3-hub75.ini").read_text(encoding="utf-8")
    legacy = (ROOT / "overrides/hub75-legacy.ini").read_text(encoding="utf-8")
    profiles = (ROOT / "BUILD_PROFILES.md").read_text(encoding="utf-8")
    readme = (ROOT / "README.md").read_text(encoding="utf-8")

    assert "IDOT_WAVESHARE_S3_RGB_MATRIX" in usermod
    assert "target=Waveshare-ESP32-S3-RGB-Matrix" in usermod
    assert "wledRelease=17.0.0-devV5" in usermod
    assert "wledBase=devV5" in usermod
    assert "WAVESHARE_S3_PINOUT" in usermod

    assert "extends = env:waveshare_esp32s3_32MB_hub75" in override
    assert "[env:waveshare]" in override
    assert "${env:waveshare_esp32s3_32MB_hub75.custom_usermods}" not in override
    assert "SHTC3_v2 = git+https://github.com/lost-hope/SHTC3_v2.git#1f6e3fc" in override
    assert "/SHTC3_v2/commit/" not in override
    assert "IDOT_SCREEN_MAX_DIM=64" in override
    assert "IDOT_DEFAULT_SCREEN_TYPE=0x04" in override
    assert "IDOT_WAVESHARE_AUDIO_DIAG" not in override
    assert "-D SR_DEBUG" not in override
    assert "audioDiag=i2c:" not in usermod
    assert "[IDM AUDIO DIAG]" not in usermod
    assert "WLED_DISABLE_OTA" not in override
    assert "board_build.partitions" not in override
    assert "waveshare_esp32s3_32MB_hub75_idotmatrix" not in legacy

    assert "waveshare-s3-hub75.ini" in profiles
    assert "environment = waveshare" in profiles
    assert "WLED 17.0.0-devV5" in readme
    assert "16 MB PSRAM" in readme
    assert "psram=total:" in usermod
    assert "minFree:" in usermod and "peakUsed:" in usermod and "minLargest:" in usermod
    assert "gifCache=state:" in usermod
    media_h = (ROOT / "IDotMatrixMedia.h").read_text(encoding="utf-8")
    assert "gifCacheStateText" in media_h
    assert "gifCacheBytes" in media_h
    assert "gifCacheFrameBytes" in media_h
    assert "IDotMatrixGifSourceStage" in media_h
    assert "gifPsramStageActive" in media_h
    assert "gifPsramStageAttempts" in media_h
    media = (ROOT / "IDotMatrixMedia.cpp").read_text(encoding="utf-8")
    stage_h = (ROOT / "IDotMatrixGifSourceStage.h").read_text(encoding="utf-8")
    stage_cpp = (ROOT / "IDotMatrixGifSourceStage.cpp").read_text(encoding="utf-8")
    assert "IDOT_GIF_PSRAM_STAGE_MAX" in stage_h
    assert "IDOT_GIF_PSRAM_STAGE_RESERVE" in stage_h
    assert "IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_BYTES" in stage_h
    assert "IDOT_GIF_PSRAM_SOURCE_CACHE_ENTRY_MAX" in stage_h
    assert "IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_ENTRIES" in stage_h
    assert "MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT" in stage_cpp
    assert "remaining < 4096u ? remaining : 4096u" in stage_cpp
    assert "yield();" in stage_cpp
    assert "findLruEntry" in stage_cpp and "cacheEvictions_" in stage_cpp
    assert "bool IDotMatrixGifSourceStage::prefetch" in stage_cpp
    assert "prefetchAttempts_" in stage_h and "prefetchSuccesses_" in stage_h
    assert "gifSourceStage_.stage(gifPlayPath_, gifSourceCacheEligible_)" in media
    assert "gifSourceStage_.release();" in media
    assert "gifSourceStage_.data() + file->iPos" in media
    assert "invalidateStoredGifSource" in media_h
    assert "clearStoredGifSourceCache" in media_h
    assert "stageGifToPsram" not in media
    assert "gifStageBuffer_" not in media_h
    assert "gifStage=state:" in usermod
    assert "gifSourceCache=state:" in usermod
    assert "gifPrefetch=attempts:" in usermod
    matrixportal_override = (ROOT / "overrides/matrixportal-s3-hub75.ini").read_text(encoding="utf-8")
    assert "IDOT_GIF_PSRAM_STAGE_MAX=262144" in matrixportal_override
    assert "IDOT_GIF_PSRAM_STAGE_RESERVE=1048576" in matrixportal_override
    assert "IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_BYTES=393216" in matrixportal_override
    assert "IDOT_GIF_PSRAM_SOURCE_CACHE_ENTRY_MAX=262144" in matrixportal_override
    assert "IDOT_GIF_PSRAM_SOURCE_CACHE_MAX_ENTRIES=8" in matrixportal_override
    assert "IDOT_GIF_CAROUSEL_PREFETCH_ENABLED=1" in matrixportal_override
    carousel = (ROOT / "IDotMatrixCarousel.cpp").read_text(encoding="utf-8")
    assert "NEXT_GIF_PREFETCH_DELAY_MS = 250u" in (ROOT / "IDotMatrixCarousel.h").read_text(encoding="utf-8")
    assert "adapter_.prefetchStoredGifSource" in carousel


def check_idot_display_fallback() -> None:
    usermod = (ROOT / "usermod_idotmatrix.cpp").read_text(encoding="utf-8")
    adapter = (ROOT / "IDotMatrixWLEDAdapter.cpp").read_text(encoding="utf-8")
    assert "if (carousel_.hasAssets()) carousel_.enter();" in usermod
    assert "else adapter_.restoreClockFallback();" in usermod
    assert "adapter_.pollDisplayEffectSelection();" in usermod
    assert "displayEffectCallbackLeaseActive" in adapter
    assert "Switching the public WLED segment mode to Static" in adapter
    assert "displayEffectActivationRequested_ = true" in adapter
    assert "strip.addEffect(\n    255," in adapter
    assert "for (uint8_t i = 0; i < count; ++i)" in adapter
    assert "strip.getSegment(i).mode == displayEffectId_" in adapter
    assert "bootPresetReplay" not in usermod
    assert "if (carousel_.hasAssets()) carousel_.enter();" in usermod
    assert "else adapter_.restoreClockFallback();" in usermod
    carousel = (ROOT / "IDotMatrixCarousel.cpp").read_text(encoding="utf-8")
    assert "adapter_.beginCarouselPlayback();" in carousel
    assert "nextSwitchAt_ = manifest_.slots[slot].type == TYPE_GIF ? 0" in carousel
    assert "captureAndSuspendContentForGifStaging();" in adapter
    assert "GIF_PREV_TEXT" in adapter



def check_device_reset_contract() -> None:
    protocol_h = (ROOT / "IDotMatrixProtocol.h").read_text(encoding="utf-8")
    protocol_cpp = (ROOT / "IDotMatrixProtocol.cpp").read_text(encoding="utf-8")
    carousel_h = (ROOT / "IDotMatrixCarousel.h").read_text(encoding="utf-8")
    automation_h = (ROOT / "IDotMatrixAutomation.h").read_text(encoding="utf-8")
    automation_cpp = (ROOT / "IDotMatrixAutomation.cpp").read_text(encoding="utf-8")
    protocol_doc = (ROOT / "PROTOCOL.md").read_text(encoding="utf-8")
    assert "command == 0x03 && subcommand == 0x80" in protocol_cpp
    assert "onDeviceReset()" in protocol_h
    assert "onCarouselReset()" in protocol_h
    assert "onAutomationReset()" in protocol_h
    assert "resetPersistent" in carousel_h
    assert "resetPersistent" in automation_h
    assert 'schedulePrefs_->remove("flags")' in automation_cpp
    assert "Device reset (`03 80`)" in protocol_doc
    assert "not an ESP32/WLED reboot" in protocol_doc
    assert (ROOT / "RELEASE_NOTES_0.9.2.md").is_file()

def check_no_heap_free_inside_queue_spinlock() -> None:
    source = (ROOT / "IDotMatrixBLEServer.cpp").read_text(encoding="utf-8")
    blocks = re.findall(
        r"portENTER_CRITICAL\(&queueMux_\);(.*?)portEXIT_CRITICAL\(&queueMux_\);",
        source,
        flags=re.S,
    )
    assert blocks
    for block in blocks:
        assert "free(" not in block
        assert "malloc(" not in block
        assert "faAssembler_.reset();" not in block



def check_fa02_single_owner_contract() -> None:
    source = (ROOT / "IDotMatrixBLEServer.cpp").read_text(encoding="utf-8")
    callback = source.split("void IDotMatrixBLEServer::enqueueFromCallback", 1)[1].split("bool IDotMatrixBLEServer::dequeue", 1)[0]
    assert "faAssembler_" not in callback
    assert "bulkTransfer_" not in callback
    assert "processAudioStream" not in callback
    assert "packet.length = static_cast<uint16_t>(value.length())" in callback
    framing = (ROOT / "IDotMatrixBLEFraming.h").read_text(encoding="utf-8")
    assert "command == 0x03 && subcommand == 0x80" in framing
    assert "command == 0x02 && subcommand == 0x01" in framing
    assert "command == 0x0A && subcommand == 0x01" in framing
    assert "subcommand == 0x01 || subcommand == 0x02" in framing

def check_rc4_media_contract() -> None:
    media = (ROOT / "IDotMatrixMedia.cpp").read_text(encoding="utf-8")
    carousel = (ROOT / "IDotMatrixCarousel.cpp").read_text(encoding="utf-8")
    adapter_h = (ROOT / "IDotMatrixWLEDAdapter.h").read_text(encoding="utf-8")
    assert "GIF_CACHE_NEW" in media
    assert "commitStagedCachedReplacement" in media
    assert "rollbackStagedCachedReplacement" in media
    assert "playStoredGif(path, cache)" in carousel
    assert "failedMask_" in carousel
    assert "cachePath(rxSlot_" in carousel
    # RC4 regression: Carousel owns the framebuffer before the first cold GIF
    # becomes visible, and GIF dwell starts only after asynchronous staging.
    assert "beginCarouselPlayback" in adapter_h
    assert "adapter_.beginCarouselPlayback();" in carousel
    assert "manifest_.slots[slot].type == TYPE_GIF ? 0" in carousel
    assert "if (adapter_.isGifPending()) return;" in carousel
    assert "if (adapter_.isGifActive())" in carousel
    # RC5 regression: close open Carousel GIF/cache media before unlinking the
    # Carousel bank on reset/reconfigure. The release helper must be called
    # before clearFiles() in both paths.
    assert "releaseCarouselMediaForStorageMutation" in adapter_h
    reset_body = carousel.split("void IDotMatrixCarousel::resetPersistent()", 1)[1].split("void IDotMatrixCarousel::configure", 1)[0]
    configure_body = carousel.split("void IDotMatrixCarousel::configure", 1)[1].split("void IDotMatrixCarousel::enter", 1)[0]
    assert reset_body.index("releaseCarouselMediaForStorageMutation") < reset_body.index("clearFiles()")
    assert configure_body.index("releaseCarouselMediaForStorageMutation") < configure_body.index("clearFiles()")


def check_rc7_carousel_update_hold_contract() -> None:
    carousel = (ROOT / "IDotMatrixCarousel.cpp").read_text(encoding="utf-8")
    adapter = (ROOT / "IDotMatrixWLEDAdapter.cpp").read_text(encoding="utf-8")
    adapter_h = (ROOT / "IDotMatrixWLEDAdapter.h").read_text(encoding="utf-8")
    assert "beginCarouselUpdateHold" in adapter_h
    assert "carouselUpdateHold_" in adapter
    assert "carouselUpdateHold_" in adapter.split("bool IDotMatrixWLEDAdapter::hasLogicalContent() const", 1)[1].split("}", 1)[0]
    configure_body = carousel.split("void IDotMatrixCarousel::configure", 1)[1].split("void IDotMatrixCarousel::enter", 1)[0]
    assert configure_body.index("releaseCarouselMediaForStorageMutation") < configure_body.index("startUpdateHold") < configure_body.index("clearFiles()")
    enter_body = carousel.split("void IDotMatrixCarousel::enter", 1)[1].split("void IDotMatrixCarousel::suspend", 1)[0]
    assert enter_body.index("endUpdateHold") < enter_body.index("beginCarouselPlayback")
    assert "UPDATE_HOLD_TIMEOUT_MS = 8000u" in (ROOT / "IDotMatrixCarousel.h").read_text(encoding="utf-8")



def check_rc2_consolidation_contract() -> None:
    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    profiles = (ROOT / "BUILD_PROFILES.md").read_text(encoding="utf-8")
    protocol = (ROOT / "PROTOCOL.md").read_text(encoding="utf-8")
    preset = (ROOT / "IDotMatrixPreset.cpp").read_text(encoding="utf-8")
    carousel = (ROOT / "IDotMatrixCarousel.cpp").read_text(encoding="utf-8")
    override = (ROOT / "overrides/matrixportal-s3-hub75.ini").read_text(encoding="utf-8")
    sha = "06ae26db67107cb3f6a3d107a92340035991a063"

    assert sha in readme and sha in profiles and sha in override
    assert "adafruit_matrixportal_esp32s3_idotmatrix_64x64" in readme
    assert "RELEASE_NOTES_0.9.2.md" in readme
    assert "release=0.8.2\nbuild=0.8.2" not in readme
    assert "0.9.0-dev.3" not in override
    assert "iDotMatrix Display" not in protocol
    assert "activateTransactional" in preset
    assert "backupPath" in preset and ".bak" in preset
    assert "Preset/Default is intentionally volatile" in preset
    assert "lastManifestSaveOk_ = saveManifest()" in carousel
    assert (ROOT / "tests/test_preset.cpp").is_file()
    assert (ROOT / "tests/test_carousel.cpp").is_file()
    assert not (ROOT / "RELEASE_NOTES_0.9.0-rc.1.md").exists()


def check_rc4_large_text_contract() -> None:
    bulk_h = (ROOT / "IDotMatrixBulkTransfer.h").read_text(encoding="utf-8")
    bulk_cpp = (ROOT / "IDotMatrixBulkTransfer.cpp").read_text(encoding="utf-8")
    carousel = (ROOT / "IDotMatrixCarousel.cpp").read_text(encoding="utf-8")
    preset = (ROOT / "IDotMatrixPreset.cpp").read_text(encoding="utf-8")
    notes = (ROOT / "RELEASE_NOTES_0.9.0.md").read_text(encoding="utf-8")
    assert "MAX_TEXT_PAYLOAD = 16654" in bulk_h
    assert "uint8_t* textPayload_ = nullptr" in bulk_h
    assert "psramFound()" in bulk_cpp
    assert "allocateTextBuffer(expectedSize_)" in bulk_cpp
    assert "static uint8_t textBuffer[4096]" not in carousel
    assert "static uint8_t textBuffer[4096]" not in preset
    assert "MAX_TEXT_PAYLOAD" in carousel and "allocateTextScratch" in carousel
    assert "MAX_TEXT_PAYLOAD" in preset and "allocateTextScratch" in preset
    assert "16654" in notes



def check_rc5_text_ownership_contract() -> None:
    protocol_h = (ROOT / "IDotMatrixProtocol.h").read_text(encoding="utf-8")
    protocol_cpp = (ROOT / "IDotMatrixProtocol.cpp").read_text(encoding="utf-8")
    carousel = (ROOT / "IDotMatrixCarousel.cpp").read_text(encoding="utf-8")
    preset = (ROOT / "IDotMatrixPreset.cpp").read_text(encoding="utf-8")
    assert "bool takeDisplayOwnership = true" in protocol_h
    assert "if (takeDisplayOwnership)" in protocol_cpp
    assert "carouselEvents_->onCarouselSuspend()" in protocol_cpp
    assert "presetEvents_->onPresetSuspend()" in protocol_cpp
    assert "processTextPayload(textBuffer, bytes, false)" in carousel
    assert "processTextPayload(textBuffer, bytes, false)" in preset

def check_graffiti_multipart_contract() -> None:
    protocol_h = (ROOT / "IDotMatrixProtocol.h").read_text(encoding="utf-8")
    protocol_cpp = (ROOT / "IDotMatrixProtocol.cpp").read_text(encoding="utf-8")
    ble = (ROOT / "IDotMatrixBLEServer.cpp").read_text(encoding="utf-8")
    adapter = (ROOT / "IDotMatrixWLEDAdapter.cpp").read_text(encoding="utf-8")
    test = (ROOT / "tests/test_protocol.cpp").read_text(encoding="utf-8")
    assert "processGraffitiRaster" in protocol_h and "GraffitiRasterTransfer" in protocol_h
    assert "GRAFFITI_RASTER_TIMEOUT_MS = 5000u" in protocol_h
    assert "data[4] != 0x00 && data[4] != 0x02" in protocol_cpp
    assert "const uint8_t response[] = {0x05, 0x00, 0x00, 0x00, status};" in protocol_cpp
    assert "processInlinePng(data, length, reply)" in ble
    assert "processGraffitiRaster(data, length, reply)" in ble
    assert ble.index("processInlinePng(data, length, reply)") < ble.index("processGraffitiRaster(data, length, reply)") < ble.index("bulkTransfer_.processPacket")
    assert "renderer_.beginRawImage(byteLength)" in adapter
    assert "renderer_.writeRawImage(offset, data, length)" in adapter
    assert "diySessionActive_ = true" in adapter
    assert "4096 + 4096 + 4096 RGB payload bytes" in test
    assert "graffitiContinueAck" in test and "graffitiCompleteAck" in test


def check_repository_cleanliness() -> None:
    forbidden_dirs = {".pio", "__pycache__", ".pytest_cache"}
    forbidden_suffixes = {".o", ".obj", ".elf", ".pyc", ".swp", ".tmp", ".log", ".orig", ".bak"}
    forbidden_names = {".DS_Store", "Thumbs.db"}
    for path in ROOT.rglob("*"):
        rel = path.relative_to(ROOT)
        assert not any(part in forbidden_dirs for part in rel.parts), f"forbidden directory artifact: {rel}"
        if path.is_file():
            assert path.name not in forbidden_names, f"forbidden OS artifact: {rel}"
            assert path.suffix.lower() not in forbidden_suffixes, f"forbidden build/temp artifact: {rel}"



def check_external_buzzer_integration() -> None:
    usermod = (ROOT / "usermod_idotmatrix.cpp").read_text(encoding="utf-8")
    automation_h = (ROOT / "IDotMatrixAutomation.h").read_text(encoding="utf-8")
    automation_cpp = (ROOT / "IDotMatrixAutomation.cpp").read_text(encoding="utf-8")

    # The old hardware/pattern backend is completely removed from this repository.
    assert not (ROOT / "IDotMatrixBuzzer.h").exists()
    assert not (ROOT / "IDotMatrixBuzzer.cpp").exists()
    for forbidden in (
        "buzzer-pin", "buzzerType", "buzzerActiveHigh", "buzzerPassiveTrigger",
        "ledcWriteTone", "BUZZER_SERVICE_PERIOD_US", "esp_timer_start_periodic",
        "/idotmatrix/buzzer-test", "PinManager::isPinOk",
    ):
        assert forbidden not in usermod, forbidden

    # Optional consumer contract for the standalone WLED Buzzer Usermod.
    bridge = (ROOT / "IDotMatrixBuzzerBridge.h").read_text(encoding="utf-8")
    assert '__has_include("WLEDBuzzerService.h")' not in usermod
    assert '#include "WLEDBuzzerService.h"' not in usermod
    assert "WLEDBuzzerService::instance()" not in usermod
    for symbol in (
        "wledBuzzerServiceReady", "wledBuzzerServicePlaying",
        "wledBuzzerServicePlay", "wledBuzzerServiceStop",
        "wledBuzzerServiceCurrentSoundId",
    ):
        assert symbol in bridge
    assert '__attribute__((weak))' in bridge
    assert 'BUZZER_SOUND_ALARM = "triple_beep"' in usermod
    assert 'BUZZER_SOUND_PROGRAM = "notification"' in usermod
    assert 'BUZZER_SOUND_COUNTDOWN = "triple_beep"' in usermod
    assert 'BUZZER_SOUND_CONNECT = "connect"' in usermod
    assert 'BUZZER_SOUND_DISCONNECT = "disconnect"' in usermod
    assert "playBuzzerSound(BUZZER_SOUND_ALARM, true)" in usermod
    assert "disconnectionBeepPending_" in usermod
    assert "serviceExternalBuzzerEvents();" in usermod

    # Automation now publishes only logical sound intent.
    assert "IDotMatrixBuzzer" not in automation_h
    assert "IDotMatrixBuzzer" not in automation_cpp
    assert "alarmSoundRequested() const" in automation_h
    assert "takeScheduleSoundRequest()" in automation_h

    # Settings UI contains only Enable plus the requested dependency note.
    assert 'Buzzer</div>' in usermod
    assert "addInfo('iDotMatrix:buzzerEnabled'" in usermod
    assert "Requires the WLED Buzzer Usermod." in usermod
    assert "e.disabled=true" in usermod
    assert "WLED Buzzer Usermod is not installed in this build." in usermod
    assert ">Test buzzer</button>" not in usermod

    # Visible settings labels must replace WLED's generated key text, never be
    # added as a second prefix via addInfo().
    assert "rl=(n,t)=>" in usermod
    assert "rl('iDotMatrix:enabled','Enabled:')" in usermod
    assert "rl('iDotMatrix:screenType','Screen Type:')" in usermod
    assert "rl('iDotMatrix:deviceName','Device Name: IDM-')" in usermod
    assert "ScreenType:" not in usermod
    cfg_block = usermod.split('void addToConfig(JsonObject& root) override', 1)[1].split('bool readFromConfig', 1)[0]
    assert cfg_block.index('CFG_SCREEN_TYPE') < cfg_block.index('CFG_RESCALE') < cfg_block.index('CFG_DEVICE_NAME')
    assert "IDM-DeviceName:" not in usermod
    assert "DeviceName: IDM-" not in usermod
    assert "Device Name: IDM-" in usermod
    assert "rl('iDotMatrix:rescale','Low-memory canvas downscale:')" in usermod
    assert "Stores a larger logical profile at the physical matrix size to reduce RAM. Output scaling itself is automatic." in usermod
    assert "rl('iDotMatrix:audioSource','Audio Source:')" in usermod
    assert "rl('iDotMatrix:buzzerEnabled','Enable')" in usermod
    build_profile = (ROOT / "IDotMatrixBuildProfile.h").read_text(encoding="utf-8")
    assert "IDOT_LOW_MEMORY_RESCALE" in build_profile
    assert "supportsRescale() { return IDOT_LOW_MEMORY_RESCALE != 0; }" in build_profile
    for profile_name in ("waveshare-s3-hub75.ini", "matrixportal-s3-hub75.ini"):
        profile = (ROOT / "overrides" / profile_name).read_text(encoding="utf-8")
        assert "-D IDOT_LOW_MEMORY_RESCALE=0" in profile
    assert "idotmatrix-prefix" not in usermod
    assert not re.search(r"addInfo\('iDotMatrix:[^']+',1,'[^']*','[^']+'\)", usermod)


def check_append_config_data_budget() -> None:
    import re
    usermod = (ROOT / "usermod_idotmatrix.cpp").read_text(encoding="utf-8")
    block = usermod.split("void appendConfigData() override", 1)[1].split("\n  }\n};", 1)[0]
    literals = re.findall(r'oappend\(F\("((?:\\.|[^"\\])*)"\)\);', block)
    total = sum(len(bytes(x, "utf-8").decode("unicode_escape").encode("utf-8")) for x in literals)
    assert total < 2800, f"appendConfigData script budget exceeded: {total} bytes"


def main() -> None:
    check_versioning()
    check_release_surface()
    check_markdown_links()
    check_documentation_contract()
    check_waveshare_dev7_contract()
    check_idot_display_fallback()
    check_device_reset_contract()
    check_no_heap_free_inside_queue_spinlock()
    check_fa02_single_owner_contract()
    check_rc4_media_contract()
    check_rc7_carousel_update_hold_contract()
    check_rc2_consolidation_contract()
    check_rc4_large_text_contract()
    check_rc5_text_ownership_contract()
    check_graffiti_multipart_contract()
    check_external_buzzer_integration()
    check_append_config_data_budget()
    check_repository_cleanliness()
    check_final_documentation_hygiene()
    print("Release package checks passed.")


if __name__ == "__main__":
    main()
