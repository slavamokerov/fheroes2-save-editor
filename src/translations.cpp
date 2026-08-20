#include "translations.h"

#include "buttonfont.h"
#include "gettextmo.h"
#include "textutil.h"

namespace fh2 {

QString gameText( const char * msgid )
{
    return QString::fromUtf8( trGame( msgid ).c_str() );
}

QString editorText( const char * msgid )
{
    return QString::fromUtf8( trEditor( msgid ).c_str() );
}

QString replaceName( QString text, const char * placeholder, const QString & name )
{
    const int pos = text.indexOf( QLatin1String( placeholder ) );
    if ( pos < 0 )
        return text;
    // The name keeps its capitalization only at the start of a sentence.
    const QString left = text.left( pos ).trimmed();
    const bool sentenceStart = left.isEmpty() || left.endsWith( QLatin1Char( '.' ) ) || left.endsWith( QLatin1Char( '?' ) )
                               || left.endsWith( QLatin1Char( '!' ) );
    text.replace( QLatin1String( placeholder ), sentenceStart ? name : name.toLower() );
    return text;
}

QString uiButtonText( UiButton key )
{
    static const char * const msgids[] = {
        "OKAY", "CANCEL", "YES", "NO", "EXIT", "DISMISS", "MAX", "MIN", "OPEN SAVE...", "GAME DATA...",
    };
    const int idx = static_cast<int>( key );
    if ( idx < 0 || idx >= static_cast<int>( sizeof( msgids ) / sizeof( msgids[0] ) ) )
        return {};
    const char * msgid = msgids[idx];

    // The button font only contains ASCII and CP1251: labels with characters
    // outside these sets are replaced with English ones (like
    // getSupportedText in fheroes2).
    const std::string translated = ( key == UiButton::OpenSave || key == UiButton::GameData ) ? trEditor( msgid ) : trGame( msgid );
    const QString qs = transliterateLatin1( QString::fromUtf8( translated.c_str() ) );
    const std::string cp = encodeCp1251( qs );
    if ( cp.empty() || !buttonFontSupports( cp ) )
        return QString::fromUtf8( msgid );
    return qs;
}

} // namespace fh2
