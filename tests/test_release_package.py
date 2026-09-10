#!/usr/bin/env python3
"""Release-package and critical-section regression checks for 0.8.1."""

from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def check_versioning() -> None:
    library = json.loads((ROOT / "library.json").read_text(encoding="utf-8"))
    assert library["version"] == "0.8.1"
    usermod = (ROOT / "usermod_idotmatrix.cpp").read_text(encoding="utf-8")
    assert 'IDOTMATRIX_RELEASE = "0.8.1"' in usermod
    assert 'IDOTMATRIX_BUILD = "0.8.1-audit-fix1"' in usermod
    assert "0.8.1-dev" not in usermod


def check_release_surface() -> None:
    required = [
        "platformio_override.ini.c3",
        "RELEASE_NOTES_0.8.1.md",
        "AUDIT_REMEDIATION_0.8.1.md",
        "TEST_REPORT_0.8.1-audit-fix1.md",
        "run_host_tests.sh",
        "run_host_sanitizers.sh",
    ]
    for name in required:
        assert (ROOT / name).is_file(), f"missing release file: {name}"
    assert not list(ROOT.glob("platformio_override.ini.c3-dev*"))
    assert not list(ROOT.glob("RELEASE_NOTES_0.8.1-dev.*"))


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

    assert "4112 bytes of permanent inline storage" in protocol
    assert "8192-byte logical-packet maximum" in protocol
    assert "application time synchronization is also retained and used as an offline fallback" in protocol
    assert "not a general-purpose PNG" in protocol
    assert "LZW10/default" not in protocol

    assert "release **0.8.1**" in readme.lower()
    assert "0.8.1-audit-fix1" in readme
    assert "final public source release" in readme.lower()
    release_notes = (ROOT / "RELEASE_NOTES_0.8.1.md").read_text(encoding="utf-8")
    test_report = (ROOT / "TEST_REPORT_0.8.1-audit-fix1.md").read_text(encoding="utf-8")
    assert "audit-remediation candidate" not in release_notes.lower()
    assert "hardware pass" in test_report.lower()
    assert "Do not inherit `${env:<base>.custom_usermods}`" in profiles
    assert "NimBLE-Arduino" not in library.get("dependencies", {})
    assert "h2zero/NimBLE-Arduino" not in library.get("dependencies", {})


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
    check_no_heap_free_inside_queue_spinlock()
    check_repository_cleanliness()
    print("Release package checks passed.")


if __name__ == "__main__":
    main()
