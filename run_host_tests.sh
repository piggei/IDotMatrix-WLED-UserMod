#!/bin/sh
set -eu
CXX="${CXX:-g++}"
FLAGS="-std=c++11 -Wall -Wextra -Werror -pedantic"
TMP="${TMPDIR:-/tmp}"

$CXX $FLAGS IDotMatrixProtocol.cpp tests/test_protocol.cpp -o "$TMP/idotmatrix_protocol_test"
"$TMP/idotmatrix_protocol_test"

$CXX $FLAGS IDotMatrixRenderer.cpp tests/test_renderer.cpp -o "$TMP/idotmatrix_renderer_test"
"$TMP/idotmatrix_renderer_test"

$CXX $FLAGS -Itests/wled_stub IDotMatrixRenderer.cpp IDotMatrixWLEDAdapter.cpp tests/test_wled_adapter.cpp -o "$TMP/idotmatrix_adapter_test"
"$TMP/idotmatrix_adapter_test"

$CXX $FLAGS IDotMatrixBulkTransfer.cpp tests/test_bulk_transfer.cpp -o "$TMP/idotmatrix_bulk_test"
"$TMP/idotmatrix_bulk_test"

$CXX $FLAGS IDotMatrixFA02Assembler.cpp tests/test_fa02_assembler.cpp -o "$TMP/idotmatrix_fa02_test"
"$TMP/idotmatrix_fa02_test"

$CXX $FLAGS -Itests/media_stub IDotMatrixRenderer.cpp IDotMatrixMedia.cpp tests/test_media.cpp -lz -o "$TMP/idotmatrix_media_test"
"$TMP/idotmatrix_media_test"

echo "All iDotMatrix host tests passed."
