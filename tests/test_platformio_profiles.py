#!/usr/bin/env python3
"""Static regression checks for the shipped WLED v16.0.1 build profiles."""

from __future__ import annotations

import configparser
import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

PLATFORM = "espressif32@~6.13.0"
USERMOD = "wled-usermod-idotmatrix = symlink://../wled-usermod-idotmatrix"
NIMBLE = "h2zero/NimBLE-Arduino@1.4.3"
GIF = "bitbank2/AnimatedGIF@1.4.7"

NORMAL_TARGETS = {
    "esp32dev_idotmatrix": ("env:esp32dev", "esp32dev", "4MB"),
    "esp32dev_8M_idotmatrix": ("env:esp32dev_8M", "esp32dev_8M", "8MB"),
    "esp32dev_16M_idotmatrix": ("env:esp32dev_16M", "esp32dev_16M", "16MB"),
    "esp32_wrover_idotmatrix": ("env:esp32_wrover", "esp32_wrover", "4MB"),
    "esp32s3dev_8MB_opi_idotmatrix": ("env:esp32s3dev_8MB_opi", "esp32s3dev_8MB_opi", "8MB"),
    "esp32s3dev_8MB_qspi_idotmatrix": ("env:esp32s3dev_8MB_qspi", "esp32s3dev_8MB_qspi", "8MB"),
    "esp32s3dev_16MB_opi_idotmatrix": ("env:esp32s3dev_16MB_opi", "esp32s3dev_16MB_opi", "16MB"),
}

PROFILE_SUFFIX = {
    "platformio_override.ini.example": "16x16",
    "platformio_override.ini.32x32": "32x32",
    "platformio_override.ini.64x64": "64x64",
}

HUB_TARGETS = {
    "esp32dev_hub75_idotmatrix": ("env:esp32dev_hub75", "4MB"),
    "esp32dev_hub75_forum_pinout_idotmatrix": ("env:esp32dev_hub75_forum_pinout", "4MB"),
    "esp32s3dev_4MB_qspi_hub75_idotmatrix": ("env:esp32s3dev_4MB_qspi_hub75", "4MB"),
    "adafruit_matrixportal_esp32s3_idotmatrix": ("env:adafruit_matrixportal_esp32s3", "8MB"),
    "esp32s3dev_16MB_opi_hub75_idotmatrix": ("env:esp32s3dev_16MB_opi_hub75", "16MB"),
    "waveshare_esp32s3_32MB_hub75_idotmatrix": ("env:waveshare_esp32s3_32MB_hub75", "32MB"),
}

PARTITIONS = {
    "4MB": ("WLED_ESP32_4MB_IDOT_NO_OTA.csv", 4 * 1024 * 1024, 0x2F0000, 0x300000, 0x0F0000),
    "8MB": ("WLED_ESP32_8MB_IDOT_NO_OTA.csv", 8 * 1024 * 1024, 0x400000, 0x410000, 0x3E0000),
    "16MB": ("WLED_ESP32_16MB_IDOT_NO_OTA.csv", 16 * 1024 * 1024, 0x600000, 0x610000, 0x9E0000),
    "32MB": ("WLED_ESP32_32MB_IDOT_NO_OTA.csv", 32 * 1024 * 1024, 0x600000, 0x610000, 0x19E0000),
}


def read_ini(name: str) -> configparser.RawConfigParser:
    parser = configparser.RawConfigParser(
        interpolation=None,
        strict=True,
        inline_comment_prefixes=(";", "#"),
        empty_lines_in_values=True,
    )
    path = ROOT / name
    with path.open("r", encoding="utf-8") as handle:
        parser.read_file(handle)
    return parser


def value(parser: configparser.RawConfigParser, section: str, key: str) -> str:
    assert parser.has_section(section), f"missing [{section}]"
    assert parser.has_option(section, key), f"missing {key} in [{section}]"
    return parser.get(section, key, raw=True).strip()


def partition_path(flash: str) -> str:
    return f"../wled-usermod-idotmatrix/{PARTITIONS[flash][0]}"


def check_normal_profile(name: str, profile_define: str | None) -> None:
    parser = read_ini(name)
    suffix = PROFILE_SUFFIX[name]
    sections = {section for section in parser.sections() if section.startswith("env:")}
    expected = {f"env:{stem}_{suffix}" for stem in NORMAL_TARGETS}
    assert sections == expected, f"{name}: unexpected environment set: {sections ^ expected}"

    for stem, (extends, base, flash) in NORMAL_TARGETS.items():
        section = f"env:{stem}_{suffix}"
        assert value(parser, section, "extends") == extends
        assert value(parser, section, "platform") == PLATFORM
        assert value(parser, section, "platform_packages") == ""
        assert value(parser, section, "board_build.partitions") == partition_path(flash)

        flags = value(parser, section, "build_flags")
        assert f"${{env:{base}.build_flags}}" in flags
        assert "-D WLED_DISABLE_OTA" in flags
        assert ("IDOT_GIF_LZW11" in flags) == (profile_define == "IDOT_GIF_LZW11")
        assert ("IDOT_GIF_LZW12" in flags) == (profile_define == "IDOT_GIF_LZW12")
        assert not ("IDOT_GIF_LZW11" in flags and "IDOT_GIF_LZW12" in flags)

        deps = value(parser, section, "lib_deps")
        assert f"${{env:{base}.lib_deps}}" in deps
        assert NIMBLE in deps
        assert GIF in deps

        usermods = value(parser, section, "custom_usermods")
        assert f"${{env:{base}.custom_usermods}}" in usermods
        assert USERMOD in usermods


def check_lite_profile() -> None:
    parser = read_ini("platformio_override.ini.64x64-lite")
    lite_names = {"esp32dev_idotmatrix", "esp32dev_8M_idotmatrix", "esp32dev_16M_idotmatrix"}
    lite_targets = {key: target for key, target in NORMAL_TARGETS.items() if key in lite_names}
    sections = {section for section in parser.sections() if section.startswith("env:")}
    expected = {f"env:{stem}_64x64_lite" for stem in lite_targets}
    assert sections == expected

    disabled = {
        "WLED_DISABLE_ALEXA",
        "WLED_DISABLE_HUESYNC",
        "WLED_DISABLE_MQTT",
        "WLED_DISABLE_INFRARED",
        "WLED_DISABLE_ADALIGHT",
        "WLED_DISABLE_ESPNOW",
        "WLED_DISABLE_LOXONE",
    }
    for stem, (extends, base, flash) in lite_targets.items():
        section = f"env:{stem}_64x64_lite"
        assert value(parser, section, "extends") == extends
        assert value(parser, section, "platform") == PLATFORM
        assert value(parser, section, "board_build.partitions") == partition_path(flash)
        flags = value(parser, section, "build_flags")
        assert "-D WLED_DISABLE_OTA" in flags
        assert "-D IDOT_GIF_LZW12" in flags
        for define in disabled:
            assert f"-D {define}" in flags
        usermods = value(parser, section, "custom_usermods")
        assert "custom_usermods}" not in usermods
        assert USERMOD in usermods


def check_hub75_profile() -> None:
    parser = read_ini("platformio_override.ini.hub75")
    sections = {section for section in parser.sections() if section.startswith("env:")}
    expected = {f"env:{name}" for name in HUB_TARGETS}
    assert sections == expected, f"HUB75: unexpected environment set: {sections ^ expected}"

    for env_name, (extends, flash) in HUB_TARGETS.items():
        section = f"env:{env_name}"
        upstream = extends.removeprefix("env:")
        assert value(parser, section, "extends") == extends
        assert value(parser, section, "platform") == PLATFORM
        assert value(parser, section, "platform_packages") == ""
        assert value(parser, section, "board_build.partitions") == partition_path(flash)
        flags = value(parser, section, "build_flags")
        assert f"${{env:{upstream}.build_flags}}" in flags
        assert "-D WLED_DISABLE_OTA" in flags
        assert "-D IDOT_GIF_LZW12" in flags
        assert "IDOT_GIF_LZW11" not in flags
        deps = value(parser, section, "lib_deps")
        assert f"${{env:{upstream}.lib_deps}}" in deps
        assert NIMBLE in deps and GIF in deps
        assert USERMOD in value(parser, section, "custom_usermods")



def check_profile_environment_isolation() -> None:
    """Every media profile gets its own PIOENV/build/libdeps namespace."""
    files = [
        "platformio_override.ini.example",
        "platformio_override.ini.32x32",
        "platformio_override.ini.64x64",
        "platformio_override.ini.64x64-lite",
        "platformio_override.ini.hub75",
    ]
    owners: dict[str, str] = {}
    for filename in files:
        parser = read_ini(filename)
        for section in parser.sections():
            if not section.startswith("env:"):
                continue
            env_name = section.removeprefix("env:")
            previous = owners.setdefault(env_name, filename)
            assert previous == filename, (
                f"PlatformIO environment {env_name!r} is shared by {previous} and {filename}; "
                "decoder-profile switches must never reuse the same .pio/build or .pio/libdeps namespace"
            )


def parse_int(text: str) -> int:
    return int(text.strip(), 0)


def check_partitions() -> None:
    for flash, (filename, flash_bytes, app_size, fs_offset, fs_size) in PARTITIONS.items():
        rows = []
        with (ROOT / filename).open("r", encoding="utf-8", newline="") as handle:
            for row in csv.reader(line for line in handle if not line.lstrip().startswith("#")):
                if not row or not any(cell.strip() for cell in row):
                    continue
                rows.append([cell.strip() for cell in row])
        assert [row[0] for row in rows] == ["nvs", "app0", "spiffs", "coredump"], filename
        assert all(row[0] != "otadata" for row in rows), filename
        assert rows[1][1:3] == ["app", "factory"], filename

        nvs, app, fs, core = rows
        nvs_off, nvs_size = parse_int(nvs[3]), parse_int(nvs[4])
        app_off, app_len = parse_int(app[3]), parse_int(app[4])
        fs_off, fs_len = parse_int(fs[3]), parse_int(fs[4])
        core_off, core_len = parse_int(core[3]), parse_int(core[4])

        assert nvs_off == 0x9000 and nvs_size == 0x5000, filename
        assert app_off == 0x10000 and app_len == app_size, filename
        assert app_off + app_len == fs_off == fs_offset, filename
        assert fs_len == fs_size, filename
        assert fs_off + fs_len == core_off, filename
        assert core_len == 0x10000, filename
        assert core_off + core_len == flash_bytes, filename


def main() -> None:
    check_normal_profile("platformio_override.ini.example", None)
    check_normal_profile("platformio_override.ini.32x32", "IDOT_GIF_LZW11")
    check_normal_profile("platformio_override.ini.64x64", "IDOT_GIF_LZW12")
    check_lite_profile()
    check_hub75_profile()
    check_profile_environment_isolation()
    check_partitions()
    print("PlatformIO profile tests passed.")


if __name__ == "__main__":
    main()
