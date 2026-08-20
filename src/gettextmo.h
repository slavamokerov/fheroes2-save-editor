#pragma once

// gettext-style translation provider (.mo and .po files), Qt-free.
// Ported from fheroes2 src/engine/translations.cpp (GPL-2.0).
//
// Two independent domains:
//   - game: fheroes2 engine translations (compiled .mo files shipped with the game)
//   - editor: this editor's own strings (plain .po files shipped with the app)
//
// Lookups never fail: if a string is not translated, the msgid itself is returned.

#include <string>

namespace fh2 {

// Resets both domains and sets the active language (2-letter code, e.g. "ru").
// Called once at startup, before loading translation files.
void initTranslationDomains( const std::string & lang );

// The active language code set by initTranslationDomains.
const std::string & translationLanguage();

// Loads a compiled gettext .mo file (game domain).
bool loadGameTranslation( const std::string & path );

// Tries <dataDir>/files/lang/<lang>.mo (the game's own data folder).
bool loadGameTranslationFromDataDir( const std::string & dataDir );

// Loads a plain-text .po file (editor domain, msgid/msgstr pairs only).
bool loadEditorTranslation( const std::string & path );

// Returns the translation of msgid in the game domain (or msgid itself).
std::string trGame( const std::string & msgid );

// Returns the translation of msgid in the editor domain (or msgid itself).
std::string trEditor( const std::string & msgid );

// Game translation first, editor translation as the fallback (or msgid itself).
std::string trGameOrEditor( const std::string & msgid );

// Picks the plural form of the game-domain translation (locale rules from fheroes2).
std::string trGamePlural( const std::string & msgid, size_t n );

// Same for the editor domain.
std::string trEditorPlural( const std::string & msgid, size_t n );

} // namespace fh2
