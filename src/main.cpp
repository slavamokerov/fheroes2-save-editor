#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#ifdef _MSC_VER
#include <process.h>
#else
#include <unistd.h>
#endif
#include <vector>

#include <QApplication>
#include <QDir>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include "constants.h"
#include "gettextmo.h"
#include "mainwindow.h"
#include "savefile.h"
#include "translations.h"

namespace {

// CP1251 -> UTF-8 (used to match hero names in CLI mode).
std::string cp1251ToUtf8( const std::string & s )
{
    static const uint32_t table[128] = {
        0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021, 0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
        0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F, 0x00A0,
        0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7, 0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407, 0x00B0,
        0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7, 0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457, 0x00BF,
        0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
        0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
        0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
        0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
    };
    std::string out;
    for ( unsigned char c : s ) {
        if ( c < 0x80 ) {
            out.push_back( static_cast<char>( c ) );
            continue;
        }
        uint32_t code = table[c - 0x80];
        if ( code < 0x800 ) {
            out.push_back( static_cast<char>( 0xC0 | ( code >> 6 ) ) );
            out.push_back( static_cast<char>( 0x80 | ( code & 0x3F ) ) );
        }
        else {
            out.push_back( static_cast<char>( 0xE0 | ( code >> 12 ) ) );
            out.push_back( static_cast<char>( 0x80 | ( ( code >> 6 ) & 0x3F ) ) );
            out.push_back( static_cast<char>( 0x80 | ( code & 0x3F ) ) );
        }
    }
    return out;
}

bool makeBackup( const std::string & path )
{
    const std::string backup = path + ".bak";
    QFile f( QString::fromStdString( path ) );
    if ( QFileInfo::exists( QString::fromStdString( backup ) ) )
        return true;
    return f.copy( QString::fromStdString( backup ) );
}

// fheroes2-save-editor --add <file.sav> <hero_name> <monster_id> <count>
// Writes a troop to the hero (slot with the same monster -> empty slot -> first slot).
// Replaces the removed patch_save.py script.
int cliAdd( int argc, char * argv[] )
{
    if ( argc < 4 ) {
        fprintf( stderr, "Usage: fheroes2-save-editor --add <file.sav> <hero_name> <monster_id> <count>\n" );
        return 2;
    }
    const std::string path = argv[2];
    const std::string heroName = argv[3];
    const int monsterId = std::atoi( argv[4] );
    const int count = argc > 5 ? std::atoi( argv[5] ) : 1;

    try {
        fh2::SaveFile sv = fh2::SaveFile::load( path );

        const std::string utf8Name = cp1251ToUtf8( heroName );
        fh2::HeroRecord * target = nullptr;
        for ( fh2::HeroRecord & h : sv.heroes() ) {
            if ( cp1251ToUtf8( h.name ) == heroName ) {
                target = &h;
                break;
            }
            // The name may not be localized in the save — fall back to the default hero names list.
            auto it = fh2::heroDefaultNames().find( h.heroId );
            if ( it != fh2::heroDefaultNames().end() && it->second == heroName ) {
                target = &h;
                break;
            }
        }
        if ( target == nullptr ) {
            fprintf( stderr, "Hero \"%s\" not found\n", heroName.c_str() );
            return 3;
        }

        int slot = -1;
        for ( int i = 0; i < 5; ++i ) {
            if ( target->slots[i].monsterId == monsterId ) {
                slot = i;
                break;
            }
        }
        if ( slot < 0 ) {
            for ( int i = 0; i < 5; ++i ) {
                if ( target->slots[i].monsterId == 0 && target->slots[i].count == 0 ) {
                    slot = i;
                    break;
                }
            }
        }
        if ( slot < 0 )
            slot = 0;

        sv.setSlot( *target, slot, monsterId, count );
        if ( !makeBackup( path ) )
            fprintf( stderr, "Warning: could not create a backup %s.bak\n", path.c_str() );
        sv.save();
        printf( "OK: hero \"%s\", slot %d: troop %d (%s) x%d\n",
                heroName.c_str(), slot + 1, monsterId, fh2::monsterName( monsterId ).c_str(), count );
        return 0;
    }
    catch ( const fh2::SaveError & e ) {
        fprintf( stderr, "Error: %s\n", e.what() );
        return 1;
    }
}

// fheroes2 UI language — from the fheroes2.cfg config file ("lang" parameter),
// as in the engine (System::GetConfigDirectory("fheroes2") + Settings::configFileName).
// Port of TinyConfig::Load (engine/tinyconfig.cpp): "key = value" lines,
// '#' — comment, keys are case- and space-insensitive.
QString fheroes2ConfigLanguage()
{
    QStringList paths;
    // Current path (fheroes2 1.1.x writes here; QStandardPaths::GenericConfigLocation):
    // macOS ~/Library/Preferences/fheroes2/fheroes2.cfg, Windows %APPDATA%/fheroes2/,
    // Linux ~/.config/fheroes2/.
    paths << QStandardPaths::writableLocation( QStandardPaths::GenericConfigLocation ) + QStringLiteral( "/fheroes2/fheroes2.cfg" );
    // Legacy: SDL_GetPrefPath("fheroes2") -> ~/Library/Application Support/fheroes2/.
    paths << QDir::home().filePath( QStringLiteral( "Library/Application Support/fheroes2/fheroes2.cfg" ) );

    for ( const QString & path : paths ) {
        QFile f( path );
        if ( !f.open( QIODevice::ReadOnly | QIODevice::Text ) )
            continue;
        while ( !f.atEnd() ) {
            QString line = QString::fromUtf8( f.readLine() ).trimmed();
            if ( line.isEmpty() || line.startsWith( QLatin1Char( '#' ) ) )
                continue;
            const int pos = line.indexOf( QLatin1Char( '=' ) );
            if ( pos < 0 )
                continue;
            const QString key = line.left( pos ).trimmed().toLower();
            const QString value = line.mid( pos + 1 ).trimmed().toLower();
            if ( key == QLatin1String( "lang" ) )
                return value;
        }
    }
    return {}; // no config or no language set — English
}

// Game translations: the compiled <lang>.mo files shipped with fheroes2.
// Editor translations: the <lang>.po files shipped with this app.
// Neither is critical — untranslated strings fall back to English.
void initTranslations( const QString & langCode )
{
    const QString lang = langCode.isEmpty() ? QStringLiteral( "en" ) : langCode.toLower();
    fh2::initTranslationDomains( lang.toStdString() );

    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList poCandidates = {
        appDir + "/../Resources/translations/", // macOS .app bundle
        appDir + "/../files/lang/",             // source tree next to the build dir
        appDir + "/files/lang/",                // next to the binary (Windows)
    };
    for ( const QString & dir : poCandidates ) {
        if ( fh2::loadEditorTranslation( ( dir + lang + ".po" ).toStdString() ) )
            break;
    }

    const QStringList moCandidates = {
        QStringLiteral( "/usr/local/opt/fheroes2/fheroes2.app/Contents/Resources/translations/" ),
        QStringLiteral( "/opt/homebrew/opt/fheroes2/fheroes2.app/Contents/Resources/translations/" ),
        QStringLiteral( "/Applications/fheroes2.app/Contents/Resources/translations/" ),
        QStringLiteral( "/usr/share/locale/" ) + lang + QStringLiteral( "/LC_MESSAGES/fheroes2.mo" ),
        QDir::home().filePath( QStringLiteral( ".local/share/fheroes2/files/lang/" ) ),
        QStandardPaths::writableLocation( QStandardPaths::GenericConfigLocation ) + QStringLiteral( "/fheroes2/files/lang/" ),
    };
    for ( const QString & path : moCandidates ) {
        const QString file = path.endsWith( QLatin1String( ".mo" ) ) ? path : path + lang + QStringLiteral( ".mo" );
        if ( QFileInfo::exists( file ) && fh2::loadGameTranslation( file.toStdString() ) )
            break;
    }
}

} // namespace

int main( int argc, char * argv[] )
{
    // Headless CLI mode: --add <file> <hero_name> <monster_id> <count>
    if ( argc > 1 && std::strcmp( argv[1], "--add" ) == 0 )
        return cliAdd( argc, argv );

    QApplication app( argc, argv );
    QApplication::setApplicationName( "fheroes2 Save Editor" );
    QApplication::setOrganizationName( "fh2editor" );

    // UI language — the same as selected in fheroes2 (fheroes2.cfg, "lang").
    // No config — English. FH2_UI_LANG overrides the config (dev/testing/screenshots).
    // All texts come from the gettext files of the game and this editor:
    // game strings from fheroes2's <lang>.mo, editor strings from <lang>.po.
    const QByteArray uiLang = qgetenv( "FH2_UI_LANG" );
    initTranslations( uiLang.isEmpty() ? fheroes2ConfigLanguage() : QString::fromLatin1( uiLang ) );

    fh2::MainWindow window;
    window.show();
    // Open a save from the command line: fheroes2-save-editor <file.sav>
    if ( argc > 1 )
        window.openPath( QString::fromLocal8Bit( argv[1] ) );

    // Debug snapshot: FH2_DEBUG_SHOT=<file.png> — save the main window to PNG and quit.
    // FH2_DEBUG_DIALOG=<book|monster|artifact|hero|spell|skill|count|message|numpad|
    // armyinfo|spellinfo|skillinfo|artifactinfo|primcount|expcount|monstercount> —
    // open a dialog and snapshot it (dialogs are modal via a nested QEventLoop,
    // so after the snapshot we exit through _exit).
    const QByteArray shotPath = qgetenv( "FH2_DEBUG_SHOT" );
    const QByteArray shotDialog = qgetenv( "FH2_DEBUG_DIALOG" );
    if ( !shotPath.isEmpty() && argc > 1 ) {
        if ( shotDialog.isEmpty() ) {
            QTimer::singleShot( 2500, &window, [&window, shotPath]() {
                const QPixmap pm = window.grab();
                pm.save( QString::fromLocal8Bit( shotPath ) );
                QApplication::quit();
            } );
        }
        else {
            QTimer::singleShot( 2000, &window, [&window, shotDialog]() { window.openDebugDialog( QString::fromLatin1( shotDialog ) ); } );
            QTimer::singleShot( 2800, &window, [&window, shotPath]() {
                const QPixmap pm = window.grab();
                pm.save( QString::fromLocal8Bit( shotPath ) );
                ::_exit( 0 );
            } );
        }
    }

    return app.exec();
}
