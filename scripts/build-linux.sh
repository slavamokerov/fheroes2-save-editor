#!/usr/bin/env bash
# Build (and run) the editor on Linux: ./scripts/build-linux.sh [--build-only]
#
# Why this script deliberately does NOT install dependencies:
# every distro installs packages differently (apt / dnf / pacman / zypper),
# and an apt-only installer would break everywhere else — the same trap
# fheroes2's own install_sdl2_dev.sh falls into. Install the build
# dependencies with your package manager first; the exact commands are
# listed in README.md under "Building from source → Prerequisites".
set -e
cd "$(dirname "$0")/.."

command -v cmake >/dev/null 2>&1 || { echo "cmake not found. Install the dependencies listed in README.md (Building from source → Prerequisites)."; exit 1; }
command -v g++ >/dev/null 2>&1 || { echo "No C++ compiler found. Install the dependencies listed in README.md (Building from source → Prerequisites)."; exit 1; }

CMAKE_ARGS=( -S . -B build -DCMAKE_BUILD_TYPE=Release )
[[ -n "${QT_DIR:-}" ]] && CMAKE_ARGS+=( -DCMAKE_PREFIX_PATH="$QT_DIR" )

cmake "${CMAKE_ARGS[@]}"
cmake --build build -j "$(nproc)"

[[ "${1:-}" == "--build-only" ]] && exit 0
./build/fheroes2-save-editor
