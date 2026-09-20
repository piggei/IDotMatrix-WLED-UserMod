#!/usr/bin/env python3
"""Release-candidate and critical-section regression checks for 0.9.1-rc.1."""

from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def check_versioning() -> None:
    library = json.loads((ROOT / "library.json").read_text(encoding="utf-8"))
    assert library["version"] == "0.9.1"
    usermod = (ROOT / "usermod_idotmatrix.cpp").read_text(encoding="utf-8")
    assert 'IDOTMATRIX_RELEASE = "0.9.1"' in usermod
    assert 'IDOTMATRIX_BUILD = "0.9.1-rc.1"' in usermod
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
        "partitions/WLED_ESP32_4MB_IDOT_NO_OTA.csv",
        "partitions/WLED_ESP32_4MB_IDOT_OTA.csv",
        "RELEASE_NOTES_0.9.1-rc.1.md",
        "RELEASE_NOTES_0.9.1-dev.1.md",
        "RELEASE_NOTES_0.9.0.md",
        "RELEASE_NOTES_0.8.2.md",
        "IDotMatrixAudioSource.h",
        "IDotMatrixAudioSource.cpp",
        "IDotMatrixCarousel.h",
        "IDotMatrixCarousel.cpp",
        "IDotMatrixPreset.h",
        "IDotMatrixPreset.cpp",
        "tests/test_audio_source.cpp",
        "tests/test_ble_framing.cpp",
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
    profiles = (ROOT / "BUILD_PROFILES.md").read_text(encoding="utf-8")
    library = json.loads((ROOT / "library.json").read_text(encoding="utf-8"))
    testing = (ROOT / "TESTING.md").read_text(encoding="utf-8")

    assert "4112 bytes of permanent" in architecture
    assert "8192" in architecture
    assert "supported 16x16 profiles compile `IDOT_GIF_LZW12`" in architecture
    assert "do not inherit `${env:<base>.custom_usermods}`" in architecture
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

    assert readme.startswith("# WLED iDotMatrix Usermod — 0.9.1 development\n")
    assert "Release: 0.9.1 / build: 0.9.1-rc.1" in readme
    assert "stable 0.9.0" in readme.lower()
    assert "Graffiti full-raster multipart" in readme
    assert "overrides/esp32c3-16x16-audio-ota.ini" in readme
    assert "partitions/" in readme
    assert "BLE compatibility/security" in readme
    assert "unauthenticated" in readme
    assert "per-slot frame cache" in readme
    assert "`idotmatrix` wled effect" in readme.lower()

    release_notes = (ROOT / "RELEASE_NOTES_0.9.1-rc.1.md").read_text(encoding="utf-8")
    assert "Graffiti" in release_notes
    assert "0x00" in release_notes and "0x02" in release_notes
    assert "OTA" in release_notes
    assert "overrides/" in release_notes and "partitions/" in release_notes

    assert "host" in testing.lower()
    assert "Graffiti full-raster validation" in testing
    assert "three consecutive WLED OTA updates: PASS" in testing
    assert "hardware-validated" in testing
    assert "complex photographic images" in testing
    history = (ROOT / "HISTORY.md").read_text(encoding="utf-8")
    assert history.startswith("## 0.9.1-rc.1")
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
    assert (ROOT / "RELEASE_NOTES_0.9.1-rc.1.md").is_file()

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
    assert "RELEASE_NOTES_0.9.1-rc.1.md" in readme
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


def main() -> None:
    check_versioning()
    check_release_surface()
    check_markdown_links()
    check_documentation_contract()
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
    check_repository_cleanliness()
    print("Release package checks passed.")


if __name__ == "__main__":
    main()
