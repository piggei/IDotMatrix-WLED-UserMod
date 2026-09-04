Import("env")

from pathlib import Path

# AnimatedGIF 1.4.7 is intentionally general purpose and embeds ~22 KB of
# decode state in each AnimatedGIF object. the stable iDotMatrix GIF target is 16x16, so a
# 1024-entry (10-bit) LZW dictionary is sufficient: a frame can output at most
# 256 pixels and therefore cannot grow the dictionary beyond 10 bits.
#
# Patch only PlatformIO's per-environment dependency copy. This leaves the
# global registry package and other PlatformIO environments untouched.

def replace_required(text, old, new, label):
    if new in text:
        return text, False
    if old not in text:
        raise RuntimeError("iDotMatrix AnimatedGIF patch: unsupported 1.4.7 source (%s)" % label)
    return text.replace(old, new), True

root = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV")
candidates = list(root.glob("*/src/AnimatedGIF.h"))
if not candidates:
    raise RuntimeError("iDotMatrix AnimatedGIF patch: AnimatedGIF.h not found under %s" % root)

header = candidates[0]
gif_inl = header.parent / "gif.inl"
if not gif_inl.exists():
    raise RuntimeError("iDotMatrix AnimatedGIF patch: gif.inl not found next to %s" % header)

h = header.read_text(encoding="utf-8")
n = False
changes = [
    ("#define MAX_WIDTH 320", "#define MAX_WIDTH 16 // IDOT_16X16", "MAX_WIDTH"),
    ("#define FILE_BUF_SIZE 4096", "#define FILE_BUF_SIZE 1024 // IDOT_16X16", "FILE_BUF_SIZE"),
    ("#define PIXEL_LAST 4096", "#define PIXEL_LAST 1024 // IDOT_16X16", "PIXEL_LAST"),
    ("#define MAXMAXCODE 4096", "#define MAXMAXCODE 1024 // IDOT_16X16", "MAXMAXCODE"),
    ("unsigned short usGIFTable[4096];", "unsigned short usGIFTable[1024]; // IDOT_16X16", "usGIFTable"),
    ("unsigned char ucGIFPixels[8192];", "unsigned char ucGIFPixels[2048]; // IDOT_16X16", "ucGIFPixels"),
]
for old, new, label in changes:
    h, c = replace_required(h, old, new, label)
    n = n or c
if n:
    header.write_text(h, encoding="utf-8")

g = gif_inl.read_text(encoding="utf-8")
n = False
for old, new, label in [
    ("(4096 - cc)*sizeof(short)", "(1024 - cc)*sizeof(short) /* IDOT_16X16 */", "dictionary memset"),
    ("codesize < 12", "codesize < 10 /* IDOT_16X16 */", "maximum code size"),
]:
    g, c = replace_required(g, old, new, label)
    n = n or c
if n:
    gif_inl.write_text(g, encoding="utf-8")

print("[iDotMatrix] AnimatedGIF 1.4.7 patched for 16x16 low-RAM decoder")
