#!/bin/sh
set -eu
CXX="${CXX:-g++}"
FLAGS="-std=c++11 -Wall -Wextra -Werror -pedantic -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined"
TMP="${TMPDIR:-/tmp}"
export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1:halt_on_error=1}"
export UBSAN_OPTIONS="${UBSAN_OPTIONS:-halt_on_error=1:print_stacktrace=1}"

$CXX $FLAGS IDotMatrixProtocol.cpp tests/test_protocol.cpp -o "$TMP/idotmatrix_protocol_san"
"$TMP/idotmatrix_protocol_san"

$CXX $FLAGS IDotMatrixAudioSource.cpp tests/test_audio_source.cpp -o "$TMP/idotmatrix_audio_source_san"
"$TMP/idotmatrix_audio_source_san"

$CXX $FLAGS IDotMatrixRenderer.cpp tests/test_renderer.cpp -o "$TMP/idotmatrix_renderer_san"
"$TMP/idotmatrix_renderer_san"

$CXX $FLAGS IDotMatrixBulkTransfer.cpp tests/test_bulk_transfer.cpp -o "$TMP/idotmatrix_bulk_san"
"$TMP/idotmatrix_bulk_san"

$CXX $FLAGS IDotMatrixFA02Assembler.cpp tests/test_fa02_assembler.cpp -o "$TMP/idotmatrix_fa02_san"
"$TMP/idotmatrix_fa02_san"

$CXX $FLAGS tests/test_ble_framing.cpp -o "$TMP/idotmatrix_ble_framing_san"
"$TMP/idotmatrix_ble_framing_san"

$CXX $FLAGS -DIDOT_AUTOMATION_HOST_TEST -Itests/automation_stub IDotMatrixProtocol.cpp IDotMatrixAutomation.cpp tests/test_automation.cpp -o "$TMP/idotmatrix_automation_san"
"$TMP/idotmatrix_automation_san"

$CXX $FLAGS -Itests/media_stub IDotMatrixRenderer.cpp IDotMatrixCompactGif.cpp IDotMatrixMedia.cpp tests/test_media.cpp -lz -o "$TMP/idotmatrix_media_san"
"$TMP/idotmatrix_media_san"

$CXX $FLAGS -DIDOT_GIF_BITS=12 -DIDOT_GIF_MAX_DIM=64 -Itests/media_stub IDotMatrixRenderer.cpp IDotMatrixCompactGif.cpp IDotMatrixMedia.cpp tests/test_media.cpp -lz -o "$TMP/idotmatrix_media12_san"
"$TMP/idotmatrix_media12_san"

$CXX $FLAGS -DIDOT_PRESET_HOST_TEST -Itests/media_stub IDotMatrixProtocol.cpp IDotMatrixPreset.cpp tests/test_preset.cpp -o "$TMP/idotmatrix_preset_san"
"$TMP/idotmatrix_preset_san"

$CXX $FLAGS -DIDOT_CAROUSEL_HOST_TEST -Itests/media_stub IDotMatrixProtocol.cpp IDotMatrixCarousel.cpp tests/test_carousel.cpp -o "$TMP/idotmatrix_carousel_san"
"$TMP/idotmatrix_carousel_san"

echo "ASan/UBSan host tests passed."
