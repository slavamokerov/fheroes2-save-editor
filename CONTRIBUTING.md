# Contributing

Thanks for your interest in the project! This page covers how to build, test
and contribute to the fheroes2 Save Editor.

## Code of Conduct

Everyone participating in the project is expected to follow the
[Code of Conduct](CODE_OF_CONDUCT.md).

## Building

See [Building from source](README.md#building-from-source) in the README:
CMake ≥ 3.21, Qt6 Widgets, a C++17 compiler (zlib is fetched automatically).

## Running the tests

The core round-trip tests (`tests/core_test.cpp`) run against real fheroes2
save files and are deliberately **not committed**: the reference suite lives
with the maintainer, because it points at personal saves. If you add your own
`tests/core_test.cpp` (the folder is in `.gitignore`), the build wires it in
automatically:

```bash
FH2_SAVE_DIR="$HOME/Library/Application Support/fheroes2/files/save" ctest --test-dir build
```

Tests only write to temporary files and never modify the original saves.

## How the code is organized

- `src/savefile.cpp` — the Qt-free core: parses and edits saves, never changes
  the stream size (all edits happen at the same offsets).
- `src/aggicn.cpp` — own loader of the game resources (AGG/ICN/KB.PAL/TIL).
- `src/constants.cpp`, `src/gamedata.cpp` — ID tables and game data.
- `web/` — the WebAssembly build of the same core.

The save format is documented in
[`FH2_SAVE_FORMAT.md`](FH2_SAVE_FORMAT.md) — read it before touching the core.

## Conventions

- Code comments are in English.
- Save edits go through `SaveFile` and must not change the decompressed
  stream size (see the invariants in the README and the format doc).
- UI is a 1:1 port of fheroes2's map editor hero screen; game look and feel
  beats convenience.
- Don't commit personal files: `tests/`, `SESSION_NOTES.md`, `FH2_SAVE_FORMAT_DEV.md`,
  `AGENTS.md` are developer-local (`.gitignore`).

## Pull requests

- Target the `main` branch.
- Keep the web FAQ (in `web/index.html`, including the JSON-LD block) in sync
  with the README FAQ.
- CI (GitHub Actions) builds all three desktop platforms and the web editor —
  make sure it's green.

If something is unclear, open an
[issue](https://github.com/slavamokerov/fheroes2-save-editor/issues).
