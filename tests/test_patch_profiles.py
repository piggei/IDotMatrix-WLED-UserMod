#!/usr/bin/env python3
"""Host regression test for the PlatformIO AnimatedGIF profile patch."""
from pathlib import Path
import tempfile

SCRIPT = Path(__file__).resolve().parents[1] / "patch_animatedgif_profiles.py"

class MockEnv(dict):
    def subst(self, value):
        if value == "$PROJECT_LIBDEPS_DIR": return self["ROOT"]
        if value == "$PIOENV": return "testenv"
        return value
    def Append(self, **kwargs):
        for key, values in kwargs.items():
            self.setdefault(key, []).extend(values)

def run_patch(header_text, inl_text, flags):
    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / "testenv" / "AnimatedGIF" / "src"
        src.mkdir(parents=True)
        header = src / "AnimatedGIF.h"
        inl = src / "gif.inl"
        header.write_text(header_text, encoding="utf-8")
        inl.write_text(inl_text, encoding="utf-8")
        env = MockEnv(ROOT=tmp, CPPDEFINES=[], BUILD_FLAGS=flags)
        namespace = {"env": env, "Import": lambda _name: None}
        exec(compile(SCRIPT.read_text(encoding="utf-8"), str(SCRIPT), "exec"), namespace)
        return header.read_text(encoding="utf-8"), inl.read_text(encoding="utf-8")

base_header = """#define MAX_WIDTH 320
#define FILE_BUF_SIZE 4096
#define PIXEL_LAST 4096
#define MAXMAXCODE 4096
unsigned short usGIFTable[4096];
unsigned char ucGIFPixels[8192];
"""
base_inl = """memset(&pGIF->usGIFTable[cc], 0x17, (4096 - cc)*sizeof(short));
if (avail == (1 << codesize) && codesize < 12) codesize++;
"""

header, inl = run_patch(base_header, base_inl, "-D IDOT_GIF_LZW11")
assert "#define MAX_WIDTH 32 // IDOT_LZW11C" in header
assert "#define FILE_BUF_SIZE 1024 // IDOT_LZW11C" in header
assert "#define MAXMAXCODE 1282 // IDOT_LZW11C" in header
assert "usGIFTable[1282]" in header
assert "ucGIFPixels[2564]" in header
assert "(1282 - cc)*sizeof(short) /* IDOT_LZW11C */" in inl
assert "codesize < 11 /* IDOT_LZW11C */" in inl

# Applying the patch again to an already-patched dependency must be a no-op.
header2, inl2 = run_patch(header, inl, "-D IDOT_GIF_LZW11")
assert header2 == header
assert inl2 == inl

print("AnimatedGIF profile patch tests passed.")

header12, inl12 = run_patch(base_header, base_inl, "-D IDOT_GIF_LZW12")
assert "#define MAX_WIDTH 64 // IDOT_LZW12SAFE" in header12
assert "#define FILE_BUF_SIZE 1024 // IDOT_LZW12SAFE" in header12
assert "#define MAXMAXCODE 4096 // IDOT_LZW12SAFE" in header12
assert "usGIFTable[4096]" in header12
assert "ucGIFPixels[8192]" in header12
assert "(4096 - cc)*sizeof(short) /* IDOT_LZW12SAFE */" in inl12
assert "codesize < 12 /* IDOT_LZW12SAFE */" in inl12
header12b, inl12b = run_patch(header12, inl12, "-D IDOT_GIF_LZW12")
assert header12b == header12
assert inl12b == inl12

# Upgrading an already compact 11-bit dependency to 12-bit must work without
# deleting .pio/libdeps. This is the normal transition from the validated
# 32x32 build to the 64x64 build.
header_up, inl_up = run_patch(header, inl, "-D IDOT_GIF_LZW12")
assert "#define MAX_WIDTH 64 // IDOT_LZW12SAFE" in header_up
assert "#define FILE_BUF_SIZE 1024 // IDOT_LZW12SAFE" in header_up
assert "#define MAXMAXCODE 4096 // IDOT_LZW12SAFE" in header_up
assert "usGIFTable[4096]" in header_up
assert "ucGIFPixels[8192]" in header_up
assert "(4096 - cc)*sizeof(short) /* IDOT_LZW12SAFE */" in inl_up
assert "codesize < 12 /* IDOT_LZW12SAFE */" in inl_up
