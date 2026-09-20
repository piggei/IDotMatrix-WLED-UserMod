Import("env")

from pathlib import Path

# AnimatedGIF 1.4.7 keeps its LZW dictionary and pixel stack inside the
# AnimatedGIF object.  On a classic ESP32 the full 12-bit object is too large
# to keep in internal DRAM together with WLED + Wi-Fi + NimBLE.  iDotMatrix
# therefore builds the decoder to match the largest GIF profile required by
# the target firmware:
#
#   default               -> 10-bit / 16x16 (stable low-RAM baseline)
#   -D IDOT_GIF_LZW11     -> 11-bit / 32x32
#   -D IDOT_GIF_LZW12     -> 12-bit / 64x64 (runtime backend: PSRAM direct or compact cache)
#
# The script is idempotent and can upgrade pristine AnimatedGIF 1.4.7 as well
# as all earlier iDotMatrix patches left in .pio/libdeps.


def has_define(name):
    for item in env.get("CPPDEFINES", []):
        key = item[0] if isinstance(item, (tuple, list)) and item else item
        if str(key) == name:
            return True
    flags = env.get("BUILD_FLAGS", "")
    if isinstance(flags, (list, tuple)):
        flags = " ".join(str(x) for x in flags)
    flags = str(flags)
    return ("-D " + name) in flags or ("-D" + name) in flags


if has_define("IDOT_GIF_LZW11") and has_define("IDOT_GIF_LZW12"):
    raise RuntimeError("iDotMatrix: choose only one of IDOT_GIF_LZW11 / IDOT_GIF_LZW12")

bits = 12 if has_define("IDOT_GIF_LZW12") else 11 if has_define("IDOT_GIF_LZW11") else 10
max_width = 64 if bits == 12 else 32 if bits == 11 else 16

# Keep the validated 10-bit profile byte-for-byte equivalent to the stable low-RAM baseline.
# For a 32x32 frame, at most 1024 output pixels can be produced. GIF LZW
# starts with at most 258 reserved/initial codes and adds at most one
# dictionary entry per decoded code after the first. Therefore a 32x32
# frame can never require the full 2048-entry 11-bit dictionary. 1282
# entries cover the worst case with a small guard while retaining 11-bit
# code-width handling. The reverse pixel stack likewise needs at most one
# byte per output pixel (1024 bytes). This recovers about 4 KiB versus generic 11-bit storage.
if bits == 11:
    dictionary_entries = 1282
    file_buf = 1024
    marker = "IDOT_LZW11C"
elif bits == 12:
    # SAFE 64x64 profile. A 12-bit GIF stream may legally reference any of
    # 4096 dictionary codes. Earlier truncated-dictionary experiments reduced the
    # physical dictionary; complex GIFs could then index beyond the arrays
    # and corrupt RAM. Keep the complete dictionary and save memory only in
    # buffers whose size does not change the legal LZW code space.
    dictionary_entries = 4096
    file_buf = 1024
    marker = "IDOT_LZW12SAFE"
else:
    dictionary_entries = 1 << bits
    file_buf = dictionary_entries
    marker = "IDOT_LZW%d" % bits

pixel_last = dictionary_entries
pixel_bytes = dictionary_entries * 2

# Make the selected decoder profile visible to the usermod C++ code.
env.Append(CPPDEFINES=[("IDOT_GIF_BITS", bits), ("IDOT_GIF_MAX_DIM", max_width), ("IDOT_GIF_DICT_ENTRIES", dictionary_entries)])


def normalize(text, desired, alternatives, label):
    if desired in text:
        return text
    for old in alternatives:
        if old in text:
            return text.replace(old, desired)
    raise RuntimeError("iDotMatrix AnimatedGIF patch: unsupported 1.4.7 source (%s)" % label)


root = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV")
candidates = list(root.glob("*/src/AnimatedGIF.h"))
if not candidates:
    raise RuntimeError("iDotMatrix AnimatedGIF patch: AnimatedGIF.h not found under %s" % root)

header = candidates[0]
gif_inl = header.parent / "gif.inl"
if not gif_inl.exists():
    raise RuntimeError("iDotMatrix AnimatedGIF patch: gif.inl not found next to %s" % header)

# Values known from pristine 1.4.7 and all previous iDotMatrix experiments.
width_alts = [
    "#define MAX_WIDTH 320",
    "#define MAX_WIDTH 16 // IDOT_16X16",
    "#define MAX_WIDTH 16 // IDOT_LZW10",
    "#define MAX_WIDTH 32 // IDOT_LZW11",
    "#define MAX_WIDTH 64 // IDOT_MAX_64",
    "#define MAX_WIDTH 64 // IDOT_LZW11",
    "#define MAX_WIDTH 64 // IDOT_LZW12",
    "#define MAX_WIDTH 64 // IDOT_LZW12C",
    "#define MAX_WIDTH 64 // IDOT_LZW12C2560",
    "#define MAX_WIDTH 64 // IDOT_LZW12C2304",
    "#define MAX_WIDTH 64 // IDOT_LZW12SAFE",
]
file_alts = [
    "#define FILE_BUF_SIZE 4096",
    "#define FILE_BUF_SIZE 1024 // IDOT_16X16",
    "#define FILE_BUF_SIZE 1024 // IDOT_LZW10",
    "#define FILE_BUF_SIZE 2048 // IDOT_LZW11",
    "#define FILE_BUF_SIZE 1024 // IDOT_LZW11C",
    "#define FILE_BUF_SIZE 4096 // IDOT_LZW12",
    "#define FILE_BUF_SIZE 1024 // IDOT_LZW12C",
    "#define FILE_BUF_SIZE 2048 // IDOT_LZW12C2560",
    "#define FILE_BUF_SIZE 1024 // IDOT_LZW12C2304",
    "#define FILE_BUF_SIZE 1024 // IDOT_LZW12SAFE",
    "#define FILE_BUF_SIZE 1024 // IDOT_MAX_64",
]
pixel_last_alts = [
    "#define PIXEL_LAST 4096",
    "#define PIXEL_LAST 1024 // IDOT_16X16",
    "#define PIXEL_LAST 1024 // IDOT_LZW10",
    "#define PIXEL_LAST 2048 // IDOT_LZW11",
    "#define PIXEL_LAST 1282 // IDOT_LZW11C",
    "#define PIXEL_LAST 4096 // IDOT_LZW12",
    "#define PIXEL_LAST 4096 // IDOT_LZW12C",
    "#define PIXEL_LAST 2560 // IDOT_LZW12C2560",
    "#define PIXEL_LAST 2304 // IDOT_LZW12C2304",
    "#define PIXEL_LAST 4096 // IDOT_LZW12SAFE",
    "#define PIXEL_LAST 4096 // IDOT_MAX_64",
]
maxcode_alts = [
    "#define MAXMAXCODE 4096",
    "#define MAXMAXCODE 1024 // IDOT_16X16",
    "#define MAXMAXCODE 1024 // IDOT_LZW10",
    "#define MAXMAXCODE 2048 // IDOT_LZW11",
    "#define MAXMAXCODE 1282 // IDOT_LZW11C",
    "#define MAXMAXCODE 4096 // IDOT_LZW12",
    "#define MAXMAXCODE 4096 // IDOT_LZW12C",
    "#define MAXMAXCODE 2560 // IDOT_LZW12C2560",
    "#define MAXMAXCODE 2304 // IDOT_LZW12C2304",
    "#define MAXMAXCODE 4096 // IDOT_LZW12SAFE",
    "#define MAXMAXCODE 4096 // IDOT_MAX_64",
]
table_alts = [
    "unsigned short usGIFTable[4096];",
    "unsigned short usGIFTable[1024]; // IDOT_16X16",
    "unsigned short usGIFTable[1024]; // IDOT_LZW10",
    "unsigned short usGIFTable[2048]; // IDOT_LZW11",
    "unsigned short usGIFTable[1282]; // IDOT_LZW11C",
    "unsigned short usGIFTable[4096]; // IDOT_LZW12",
    "unsigned short usGIFTable[4096]; // IDOT_LZW12C",
    "unsigned short usGIFTable[2560]; // IDOT_LZW12C2560",
    "unsigned short usGIFTable[2304]; // IDOT_LZW12C2304",
    "unsigned short usGIFTable[4096]; // IDOT_LZW12SAFE",
    "unsigned short usGIFTable[4096]; // IDOT_MAX_64",
]
pixels_alts = [
    "unsigned char ucGIFPixels[8192];",
    "unsigned char ucGIFPixels[2048]; // IDOT_16X16",
    "unsigned char ucGIFPixels[2048]; // IDOT_LZW10",
    "unsigned char ucGIFPixels[4096]; // IDOT_LZW11",
    "unsigned char ucGIFPixels[2564]; // IDOT_LZW11C",
    "unsigned char ucGIFPixels[8192]; // IDOT_LZW12",
    "unsigned char ucGIFPixels[8192]; // IDOT_LZW12C",
    "unsigned char ucGIFPixels[5120]; // IDOT_LZW12C2560",
    "unsigned char ucGIFPixels[4608]; // IDOT_LZW12C2304",
    "unsigned char ucGIFPixels[8192]; // IDOT_LZW12SAFE",
    "unsigned char ucGIFPixels[8192]; // IDOT_MAX_64",
]

h = header.read_text(encoding="utf-8")
for desired, alternatives, label in [
    ("#define MAX_WIDTH %d // %s" % (max_width, marker), width_alts, "MAX_WIDTH"),
    ("#define FILE_BUF_SIZE %d // %s" % (file_buf, marker), file_alts, "FILE_BUF_SIZE"),
    ("#define PIXEL_LAST %d // %s" % (pixel_last, marker), pixel_last_alts, "PIXEL_LAST"),
    ("#define MAXMAXCODE %d // %s" % (dictionary_entries, marker), maxcode_alts, "MAXMAXCODE"),
    ("unsigned short usGIFTable[%d]; // %s" % (dictionary_entries, marker), table_alts, "usGIFTable"),
    ("unsigned char ucGIFPixels[%d]; // %s" % (pixel_bytes, marker), pixels_alts, "ucGIFPixels"),
]:
    h = normalize(h, desired, alternatives, label)
header.write_text(h, encoding="utf-8")

g = gif_inl.read_text(encoding="utf-8")
memset_alts = [
    "(4096 - cc)*sizeof(short)",
    "(1024 - cc)*sizeof(short) /* IDOT_16X16 */",
    "(1024 - cc)*sizeof(short) /* IDOT_LZW10 */",
    "(2048 - cc)*sizeof(short) /* IDOT_LZW11 */",
    "(1282 - cc)*sizeof(short) /* IDOT_LZW11C */",
    "(4096 - cc)*sizeof(short) /* IDOT_LZW12 */",
    "(4096 - cc)*sizeof(short) /* IDOT_LZW12C */",
    "(2560 - cc)*sizeof(short) /* IDOT_LZW12C2560 */",
    "(2304 - cc)*sizeof(short) /* IDOT_LZW12C2304 */",
    "(4096 - cc)*sizeof(short) /* IDOT_MAX_64 */",
]
code_alts = [
    "codesize < 12",
    "codesize < 10 /* IDOT_16X16 */",
    "codesize < 10 /* IDOT_LZW10 */",
    "codesize < 11 /* IDOT_LZW11 */",
    "codesize < 11 /* IDOT_LZW11C */",
    "codesize < 12 /* IDOT_LZW12 */",
    "codesize < 12 /* IDOT_LZW12C */",
    "codesize < 12 /* IDOT_LZW12C2560 */",
    "codesize < 12 /* IDOT_LZW12C2304 */",
    "codesize < 12 /* IDOT_MAX_64 */",
]
g = normalize(g, "(%d - cc)*sizeof(short) /* %s */" % (dictionary_entries, marker), memset_alts, "dictionary memset")
g = normalize(g, "codesize < %d /* %s */" % (bits, marker), code_alts, "maximum code size")
gif_inl.write_text(g, encoding="utf-8")

print("[iDotMatrix] AnimatedGIF 1.4.7 profile: %d-bit LZW / max %dx%d / dict=%d / filebuf=%d" % (bits, max_width, max_width, dictionary_entries, file_buf))
