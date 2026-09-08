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
- Don't commit local or personal files; they are developer-local (`.gitignore`).

## Reporting issues

Before opening an issue, check the [FAQ](README.md#faq) and the save-format
[documentation](FH2_SAVE_FORMAT.md). When you do, pick the matching template from
the [issues page](https://github.com/slavamokerov/fheroes2-save-editor/issues):

- [Bug report](https://github.com/slavamokerov/fheroes2-save-editor/issues/new?template=bug_report.md) —
  a crash or a wrong value. Include the editor version (desktop or web), OS, the
  fheroes2 version and save format, and attach the `.sav` (or a `.bak`) and
  screenshots if you can.
- [Feature request](https://github.com/slavamokerov/fheroes2-save-editor/issues/new?template=feature_request.md) —
  the problem, the proposed solution and the alternatives you considered.
- [Question](https://github.com/slavamokerov/fheroes2-save-editor/issues/new?template=question.md) —
  how to do something, plus what you've already tried.

## Pull requests

- Target the `main` branch.
- Keep the web FAQ (in `web/index.html`, including the JSON-LD block) in sync
  with the README FAQ.
- CI (GitHub Actions) builds all three desktop platforms and the web editor —
  make sure it's green.
- The [pull request template](https://github.com/slavamokerov/fheroes2-save-editor/blob/main/.github/PULL_REQUEST_TEMPLATE.md)
  checklist is prefilled — tick the boxes that apply.

If something is unclear, open an
[issue](https://github.com/slavamokerov/fheroes2-save-editor/issues).

## Want to contribute?

Check the issues labeled
[`help wanted`](https://github.com/slavamokerov/fheroes2-save-editor/issues?q=is%3Aissue+is%3Aopen+label%3A%22help+wanted%22)
(features open for implementation) and
[`good first issue`](https://github.com/slavamokerov/fheroes2-save-editor/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22).
The [README roadmap](README.md#roadmap) lists the bigger picture.
