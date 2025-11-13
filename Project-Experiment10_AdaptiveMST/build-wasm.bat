@echo off
setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0
set OUT_DIR=%SCRIPT_DIR%frontend\wasm
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

echo Building WebAssembly (Emscripten) -> %OUT_DIR%

em++ ^
  "%SCRIPT_DIR%backend\mst.cpp" ^
  -O3 ^
  -s MODULARIZE=1 ^
  -s EXPORT_ES6=1 ^
  -s EXPORT_NAME=createMSTModule ^
  -s ENVIRONMENT=web ^
  -s ALLOW_MEMORY_GROWTH=1 ^
  -s WASM=1 ^
  --bind ^
  -o "%OUT_DIR%\graph_mst.js"

echo Done. Outputs: graph_mst.js, graph_mst.wasm

endlocal


