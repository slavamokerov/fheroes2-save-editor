<!-- Thank you for contributing! Keep the checklist below in mind and check off
     the items that apply. CI builds all desktop platforms and the web editor. -->

## Description

What does this change do, and why? If it fixes an issue, link it with `Fixes #N`.

## Checklist

- [ ] Target branch is `main`.
- [ ] If the change touches the save format / hero screen, edits go through
      `SaveFile` and the decompressed stream size does **not** change (all edits
      happen at the same offsets, per `FH2_SAVE_FORMAT.md`).
- [ ] Code comments are in English.
- [ ] If the web version is affected, the FAQ in `web/index.html` (including the
      JSON-LD block) stays in sync with the README FAQ.
- [ ] Tests / CI are green: `ctest --test-dir build` and the GitHub Actions
      checks.

## Testing

- Platforms checked: (e.g. macOS arm64, Windows, Linux, web)

## Screenshots

<!-- Drag & drop screenshots if the change affects the UI. -->
