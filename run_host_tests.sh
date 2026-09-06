#!/bin/sh
set -eu
CXX="${CXX:-g++}"
FLAGS="-std=c++11 -Wall -Wextra -Werror -pedantic"
TMP="${TMPDIR:-/tmp}"

$CXX $FLAGS -DEXPECTED_SCREEN_MAX_DIM=16 -DEXPECTED_DECODER_MAX_DIM=16 tests/test_build_profile.cpp -o "$TMP/idotmatrix_profile16_test"
"$TMP/idotmatrix_profile16_test"
$CXX $FLAGS -DIDOT_GIF_MAX_DIM=64 -DIDOT_SCREEN_MAX_DIM=16 -DEXPECTED_SCREEN_MAX_DIM=16 -DEXPECTED_DECODER_MAX_DIM=64 tests/test_build_profile.cpp -o "$TMP/idotmatrix_profile16_lzw12_test"
"$TMP/idotmatrix_profile16_lzw12_test"
$CXX $FLAGS -DIDOT_GIF_MAX_DIM=32 -DEXPECTED_SCREEN_MAX_DIM=32 -DEXPECTED_DECODER_MAX_DIM=32 tests/test_build_profile.cpp -o "$TMP/idotmatrix_profile32_test"
"$TMP/idotmatrix_profile32_test"
$CXX $FLAGS -DIDOT_GIF_MAX_DIM=64 -DEXPECTED_SCREEN_MAX_DIM=64 -DEXPECTED_DECODER_MAX_DIM=64 tests/test_build_profile.cpp -o "$TMP/idotmatrix_profile64_test"
"$TMP/idotmatrix_profile64_test"

$CXX $FLAGS IDotMatrixProtocol.cpp tests/test_protocol.cpp -o "$TMP/idotmatrix_protocol_test"
"$TMP/idotmatrix_protocol_test"

$CXX $FLAGS IDotMatrixRenderer.cpp tests/test_renderer.cpp -o "$TMP/idotmatrix_renderer_test"
"$TMP/idotmatrix_renderer_test"

$CXX $FLAGS IDotMatrixBuzzer.cpp tests/test_buzzer.cpp -o "$TMP/idotmatrix_buzzer_test"
"$TMP/idotmatrix_buzzer_test"

$CXX $FLAGS -Itests/automation_stub -fsyntax-only IDotMatrixAutomation.cpp

$CXX $FLAGS -Itests/wled_stub IDotMatrixRenderer.cpp IDotMatrixWLEDAdapter.cpp tests/test_wled_adapter.cpp -o "$TMP/idotmatrix_adapter_test"
"$TMP/idotmatrix_adapter_test"

$CXX $FLAGS IDotMatrixBulkTransfer.cpp tests/test_bulk_transfer.cpp -o "$TMP/idotmatrix_bulk_test"
"$TMP/idotmatrix_bulk_test"

$CXX $FLAGS IDotMatrixFA02Assembler.cpp tests/test_fa02_assembler.cpp -o "$TMP/idotmatrix_fa02_test"
"$TMP/idotmatrix_fa02_test"

$CXX $FLAGS IDotMatrixRenderer.cpp IDotMatrixCompactGif.cpp tests/test_compact_gif.cpp -o "$TMP/idotmatrix_compact_gif_test"
"$TMP/idotmatrix_compact_gif_test"

$CXX $FLAGS -Itests/media_stub IDotMatrixRenderer.cpp IDotMatrixCompactGif.cpp IDotMatrixMedia.cpp tests/test_media.cpp -lz -o "$TMP/idotmatrix_media_test"
"$TMP/idotmatrix_media_test"

$CXX $FLAGS -DIDOT_GIF_BITS=11 -DIDOT_GIF_MAX_DIM=32 -Itests/media_stub IDotMatrixRenderer.cpp IDotMatrixCompactGif.cpp IDotMatrixMedia.cpp tests/test_media.cpp -lz -o "$TMP/idotmatrix_media11_test"
"$TMP/idotmatrix_media11_test"

$CXX $FLAGS -DIDOT_GIF_BITS=12 -DIDOT_GIF_MAX_DIM=64 -Itests/media_stub IDotMatrixRenderer.cpp IDotMatrixCompactGif.cpp IDotMatrixMedia.cpp tests/test_media.cpp -lz -o "$TMP/idotmatrix_media12_test"
"$TMP/idotmatrix_media12_test"

python3 tests/test_patch_profiles.py
python3 tests/test_platformio_profiles.py

echo "All iDotMatrix host tests passed."
