#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
OUT_DIR="$SCRIPT_DIR/frontend/wasm"
mkdir -p "$OUT_DIR"

echo "Building WebAssembly (Emscripten) -> $OUT_DIR"

em++ \
  "$SCRIPT_DIR/backend/mst.cpp" \
  -O3 \
  -s MODULARIZE=1 \
  -s EXPORT_ES6=1 \
  -s EXPORT_NAME=createMSTModule \
  -s ENVIRONMENT=web \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s WASM=1 \
  --bind \
  -o "$OUT_DIR/graph_mst.js"

echo "Done. Outputs: graph_mst.js, graph_mst.wasm"


