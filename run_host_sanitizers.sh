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

$CXX $FLAGS -DIDOT_GIF_SOURCE_CACHE_HOST_TEST -DIDOT_GIF_PSRAM_STAGE_MAX=1024 -DIDOT_GIF_PSRAM_SOURCE_CACHE_MAX_BYTES=32 -DIDOT_GIF_PSRAM_SOURCE_CACHE_ENTRY_MAX=32 -DIDOT_GIF_PSRAM_SOURCE_CACHE_MAX_ENTRIES=3 -Itests/media_stub IDotMatrixGifSourceStage.cpp tests/test_gif_source_cache.cpp -o "$TMP/idotmatrix_gif_source_cache_san"
"$TMP/idotmatrix_gif_source_cache_san"

$CXX $FLAGS -Itests/media_stub IDotMatrixRenderer.cpp IDotMatrixCompactGif.cpp IDotMatrixGifSourceStage.cpp IDotMatrixMedia.cpp tests/test_media.cpp -lz -o "$TMP/idotmatrix_media_san"
"$TMP/idotmatrix_media_san"

$CXX $FLAGS -DIDOT_GIF_BITS=12 -DIDOT_GIF_MAX_DIM=64 -Itests/media_stub IDotMatrixRenderer.cpp IDotMatrixCompactGif.cpp IDotMatrixGifSourceStage.cpp IDotMatrixMedia.cpp tests/test_media.cpp -lz -o "$TMP/idotmatrix_media12_san"
"$TMP/idotmatrix_media12_san"

$CXX $FLAGS -DIDOT_PRESET_HOST_TEST -Itests/media_stub IDotMatrixProtocol.cpp IDotMatrixPreset.cpp tests/test_preset.cpp -o "$TMP/idotmatrix_preset_san"
"$TMP/idotmatrix_preset_san"

$CXX $FLAGS -DIDOT_CAROUSEL_HOST_TEST -Itests/media_stub IDotMatrixProtocol.cpp IDotMatrixCarousel.cpp tests/test_carousel.cpp -o "$TMP/idotmatrix_carousel_san"
"$TMP/idotmatrix_carousel_san"

$CXX $FLAGS -Itests/wled_stub IDotMatrixRenderer.cpp IDotMatrixWLEDAdapter.cpp tests/test_wled_adapter.cpp -o "$TMP/idotmatrix_adapter_san"
"$TMP/idotmatrix_adapter_san"

$CXX $FLAGS IDotMatrixRenderer.cpp IDotMatrixCompactGif.cpp tests/test_compact_gif.cpp -o "$TMP/idotmatrix_compact_gif_san"
"$TMP/idotmatrix_compact_gif_san"

$CXX $FLAGS -DIDOT_GIF_BITS=11 -DIDOT_GIF_MAX_DIM=32 -Itests/media_stub IDotMatrixRenderer.cpp IDotMatrixCompactGif.cpp IDotMatrixGifSourceStage.cpp IDotMatrixMedia.cpp tests/test_media.cpp -lz -o "$TMP/idotmatrix_media11_san"
"$TMP/idotmatrix_media11_san"

$CXX $FLAGS -I. tests/test_buzzer_bridge_absent.cpp -o "$TMP/idotmatrix_buzzer_bridge_absent_san"
"$TMP/idotmatrix_buzzer_bridge_absent_san"
$CXX $FLAGS -I. tests/test_buzzer_bridge_present.cpp -o "$TMP/idotmatrix_buzzer_bridge_present_san"
"$TMP/idotmatrix_buzzer_bridge_present_san"

echo "ASan/UBSan host tests passed."
