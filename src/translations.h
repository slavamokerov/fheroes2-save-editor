#pragma once

#include <QString>

namespace fh2 {

// Localized game text: looked up in the fheroes2 translation (.mo) of the
// current language; falls back to the msgid (English) when untranslated.
// The engine uses named placeholders like %{name}, %{monster} — substitute
// them with replaceName() after the call.
QString gameText( const char * msgid );

// Localized editor string: looked up in the editor's own .po file of the
// current language; falls back to the msgid (English).
QString editorText( const char * msgid );

// Replaces a %{...} placeholder with a name, lowercasing it when it is not at
// the start of the sentence (port of StringReplaceWithLowercase from fheroes2).
QString replaceName( QString text, const char * placeholder, const QString & name );

// Toolbar/dialog buttons. The engine labels (msgids OKAY/CANCEL/YES/NO/EXIT/
// DISMISS/MAX/MIN) come from the game translation; OPEN SAVE... and
// GAME DATA... are the editor's own strings.
enum class UiButton {
    Okay,
    Cancel,
    Yes,
    No,
    Exit,
    Dismiss,
    Max,
    Min,
    OpenSave,
    GameData,
};

// Button label in the current language (fallback — English).
QString uiButtonText( UiButton key );

} // namespace fh2
