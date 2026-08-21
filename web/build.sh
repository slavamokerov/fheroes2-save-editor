#!/usr/bin/env bash
# Builds the WASM core for the web editor (GitHub Pages).
# Requires the Emscripten SDK (emcc/em++) on PATH. Output: web/deploy/.
set -euo pipefail

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
OUT="$ROOT/web/deploy"

mkdir -p "$OUT"

em++ -O2 \
  -std=c++17 \
  -I"$ROOT/src" \
  --no-entry \
  -s WASM=1 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=createFh2Module \
  -s USE_ZLIB=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s ENVIRONMENT=web \
  -s EXPORTED_RUNTIME_METHODS=[] \
  -s FILESYSTEM=0 \
  -s DISABLE_EXCEPTION_CATCHING=0 \
  --bind \
  -o "$OUT/fh2core.js" \
  "$ROOT/src/savefile.cpp" \
  "$ROOT/src/constants.cpp" \
  "$ROOT/src/gettextmo.cpp" \
  "$ROOT/web/wasm_api.cpp"

# Static site files (index.html, style.css, app.js, icon) live next to the
# wasm output in web/deploy/.
cp "$ROOT/web/index.html" "$ROOT/web/style.css" "$ROOT/web/app.js" "$OUT/"
cp "$ROOT/web/robots.txt" "$ROOT/web/sitemap.xml" "$OUT/"
cp "$ROOT/web/og-image.png" "$OUT/og-image.png"
cp "$ROOT/packaging/icon.svg" "$OUT/icon.svg"
cp -R "$ROOT/web/fonts" "$OUT/fonts"
mkdir -p "$OUT/screenshots"
cp "$ROOT/screenshots/demo.gif" "$ROOT/screenshots/spellbook.png" "$ROOT/screenshots/count-ghosts.png" \
   "$ROOT/screenshots/monster.png" "$ROOT/screenshots/artifact.png" \
   "$ROOT/screenshots/skillinfo.png" "$ROOT/screenshots/armyinfo.png" "$OUT/screenshots/"

echo "Built. Deploy folder: $OUT"
ls -lh "$OUT"
