#!/usr/bin/env bash
# Build and run the editor with a single command: ./scripts/run.sh
set -e
cd "$(dirname "$0")"

# Toolchain check (macOS only, first run)
if [[ "$(uname -s)" == "Darwin" ]]; then
    command -v brew >/dev/null 2>&1 || { echo "Homebrew is required: https://brew.sh"; exit 1; }
    command -v cmake >/dev/null 2>&1 || brew install cmake
    QT_PREFIX="$(brew --prefix qt 2>/dev/null || echo /usr/local/opt/qt)"
    if [[ ! -d "$QT_PREFIX" ]]; then
        echo "Installing Qt6 (one-time, ~1-2 minutes)..."
        brew install qt
        QT_PREFIX="$(brew --prefix qt)"
    fi
fi

cmake -S . -B build -DCMAKE_PREFIX_PATH="${QT_DIR:-${QT_PREFIX:-/usr/local/opt/qt}}" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j "$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

if [[ "$(uname -s)" == "Darwin" ]]; then
    open build/fheroes2-save-editor.app
else
    ./build/fheroes2-save-editor
fi
