#!/usr/bin/env python3
"""Development release and critical-section regression checks for 0.9.0-dev.2."""

from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def check_versioning() -> None:
    library = json.loads((ROOT / "library.json").read_text(encoding="utf-8"))
    assert library["version"] == "0.9.0-dev.2"
    usermod = (ROOT / "usermod_idotmatrix.cpp").read_text(encoding="utf-8")
    assert 'IDOTMATRIX_RELEASE = "0.9.0"' in usermod
    assert 'IDOTMATRIX_BUILD = "0.9.0-dev.2"' in usermod
    assert "IDotMatrixAudioSource" in usermod
    adapter = (ROOT / "IDotMatrixWLEDAdapter.cpp").read_text(encoding="utf-8")
    assert '"iDotMatrix@;;;2"' in adapter
    assert '"iDotMatrix Display@;;;2"' not in adapter
    assert "if (currentPlaylist >= 0) applyPreset(0, CALL_MODE_DIRECT_CHANGE);" in adapter


def check_release_surface() -> None:
    required = [
        "platformio_override.ini.c3",
        "platformio_override.ini.c3-audio",
        "platformio_override.ini.matrixportal-s3-hub75",
        "RELEASE_NOTES_0.9.0-dev.2.md",
        "TEST_REPORT_0.9.0-dev.2.md",
        "HARDWARE_TEST_CHECKLIST_0.9.0-dev.2.md",
        "RELEASE_NOTES_0.8.2.md",
        "TEST_REPORT_0.8.2.md",
        "HARDWARE_TEST_CHECKLIST_0.8.2.md",
        "RELEASE_NOTES_0.8.1.md",
        "AUDIT_REMEDIATION_0.8.1.md",
        "TEST_REPORT_0.8.1-audit-fix1.md",
        "IDotMatrixAudioSource.h",
        "IDotMatrixAudioSource.cpp",
        "IDotMatrixCarousel.h",
        "IDotMatrixCarousel.cpp",
        "tests/test_audio_source.cpp",
        "tests/test_ble_framing.cpp",
        "IDotMatrixBLEFraming.h",
        "run_host_tests.sh",
        "run_host_sanitizers.sh",
    ]
    for name in required:
        assert (ROOT / name).is_file(), f"missing development file: {name}"
    assert not list(ROOT.glob("platformio_override.ini.c3-dev*"))
    assert not list(ROOT.glob("RELEASE_NOTES_0.8.2-rc.*.md"))
    assert not list(ROOT.glob("TEST_REPORT_0.8.2-rc.*.md"))
    assert not list(ROOT.glob("HARDWARE_TEST_CHECKLIST_0.8.2-rc.*.md"))


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

    assert "4112 bytes of permanent" in architecture
    assert "8192" in architecture
    assert "supported 16x16 profiles compile `IDOT_GIF_LZW12`" in architecture
    assert "default: 10-bit/16x16" not in architecture
    assert "do not inherit `${env:<base>.custom_usermods}`" in architecture
    assert "four 517-byte queue slots" in architecture
    assert "/idot_cache.new" in architecture

    assert "4112 bytes of permanent inline storage" in protocol
    assert "8192-byte logical-packet maximum" in protocol
    assert "application time synchronization is also retained and used as an offline fallback" in protocol
    assert "not a general-purpose PNG" in protocol
    assert "LZW10/default" not in protocol

    assert "release 0.8.2 / build 0.8.2" in readme.lower()
    assert "phone / ble" in readme.lower()
    assert "wled audioreactive" in readme.lower()
    assert "platformio_override.ini.c3-audio" in readme
    assert "BLE compatibility/security" in readme
    assert "unauthenticated" in readme
    assert "per-slot frame cache" in readme
    assert "current stable release" in readme.lower()
    assert "`idotmatrix` wled effect" in readme.lower()
    assert "terminates the active wled playlist" in readme.lower()
    assert "queued" in readme.lower() and "preset" in readme.lower()
    release_notes = (ROOT / "RELEASE_NOTES_0.8.2.md").read_text(encoding="utf-8")
    test_report = (ROOT / "TEST_REPORT_0.8.2.md").read_text(encoding="utf-8")
    assert "carousel" in release_notes.lower()
    assert "reset" in release_notes.lower()
    assert "alarm" in release_notes.lower()
    assert "schedule" in release_notes.lower()
    assert "host" in test_report.lower()
    assert "audioreactive" in architecture.lower()
    assert "device assets" in protocol.lower()
    assert "compatibility/security note" in protocol.lower()
    assert "complete ATT write" in protocol
    assert "timesign" in protocol.lower()
    assert "imageindex" in protocol.lower()
    assert "platformio_override.ini.c3-audio" in profiles
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
    assert (ROOT / "RELEASE_NOTES_0.9.0-dev.2.md").is_file()
    assert (ROOT / "TEST_REPORT_0.9.0-dev.2.md").is_file()

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


def check_repository_cleanliness() -> None:
    forbidden_dirs = {".pio", "__pycache__", ".pytest_cache"}
    forbidden_suffixes = {".o", ".obj", ".elf", ".pyc", ".swp", ".tmp", ".log"}
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
    check_repository_cleanliness()
    print("Release package checks passed.")


if __name__ == "__main__":
    main()
