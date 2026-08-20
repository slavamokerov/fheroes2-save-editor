// gettext-style translation provider (.mo and .po files).
// Ported from fheroes2 src/engine/translations.cpp (GPL-2.0), trimmed to what
// this editor needs. The .mo container layout is described in
// https://www.gnu.org/software/gettext/manual/html_node/MO-Files.html

#include "gettextmo.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <unordered_map>
#include <vector>

#include "codepages.h"

namespace fh2 {

namespace {

enum class Locale { EN, BE, BG, CS, DA, DE, EL, EO, ES, FR, HU, IT, LT, NB, NL, PL, PT, RO, RU, SK, SV, TR, UK, VI };

Locale localeFromCode( const std::string & lang )
{
    static const std::unordered_map<std::string, Locale> map = {
        { "be", Locale::BE }, { "bg", Locale::BG }, { "cs", Locale::CS }, { "da", Locale::DA }, { "de", Locale::DE },
        { "el", Locale::EL }, { "eo", Locale::EO }, { "es", Locale::ES }, { "fr", Locale::FR }, { "hu", Locale::HU },
        { "it", Locale::IT }, { "lt", Locale::LT }, { "nb", Locale::NB }, { "nl", Locale::NL }, { "pl", Locale::PL },
        { "pt", Locale::PT }, { "ro", Locale::RO }, { "ru", Locale::RU }, { "sk", Locale::SK }, { "sv", Locale::SV },
        { "tr", Locale::TR }, { "uk", Locale::UK }, { "vi", Locale::VI },
    };
    const auto it = map.find( lang );
    return it == map.end() ? Locale::EN : it->second;
}

// Plural form index (port of Translation::ngettext locale rules from fheroes2).
size_t pluralIndex( Locale locale, size_t n )
{
    switch ( locale ) {
    case Locale::BG:
    case Locale::DA:
    case Locale::DE:
    case Locale::EO:
    case Locale::ES:
    case Locale::HU:
    case Locale::IT:
    case Locale::NB:
    case Locale::NL:
    case Locale::SV:
    case Locale::TR:
        return ( n != 1 ) ? 1 : 0;
    case Locale::EL:
    case Locale::FR:
    case Locale::PT:
        return ( n > 1 ) ? 1 : 0;
    case Locale::RO:
        return ( n == 1 ? 0 : ( n == 0 || ( n != 1 && n % 100 >= 1 && n % 100 <= 19 ) ? 1 : 2 ) );
    case Locale::CS:
    case Locale::SK:
        return ( n == 1 ? 0 : ( n >= 2 && n <= 4 ? 1 : 2 ) );
    case Locale::RU:
        return ( n % 10 == 1 && n % 100 != 11 ? 0 : n % 10 >= 2 && n % 10 <= 4 && ( n % 100 < 10 || n % 100 >= 20 ) ? 1 : 2 );
    case Locale::LT:
        return ( n % 10 == 1 && n % 100 != 11 ? 0 : n % 10 >= 2 && ( n % 100 < 10 || n % 100 >= 20 ) ? 1 : 2 );
    case Locale::PL:
        return ( n == 1 ? 0 : n % 10 >= 2 && n % 10 <= 4 && ( n % 100 < 10 || n % 100 >= 20 ) ? 1 : 2 );
    case Locale::BE:
    case Locale::UK:
        return ( n % 10 == 1 && n % 100 != 11 ? 0 : n % 10 >= 2 && n % 10 <= 4 && ( n % 100 < 12 || n % 100 > 14 ) ? 1 : 2 );
    default:
        return ( n != 1 ) ? 1 : 0;
    }
}

uint32_t readBe32( const uint8_t * p )
{
    return ( static_cast<uint32_t>( p[0] ) << 24 ) | ( static_cast<uint32_t>( p[1] ) << 16 ) | ( static_cast<uint32_t>( p[2] ) << 8 )
           | static_cast<uint32_t>( p[3] );
}

uint32_t readLe32( const uint8_t * p )
{
    return static_cast<uint32_t>( p[0] ) | ( static_cast<uint32_t>( p[1] ) << 8 ) | ( static_cast<uint32_t>( p[2] ) << 16 )
           | ( static_cast<uint32_t>( p[3] ) << 24 );
}

// Splits a string by '\0'; the plural forms in an .mo entry are NUL-separated.
std::vector<std::string> splitNul( const char * data, size_t len )
{
    std::vector<std::string> parts;
    const char * begin = data;
    for ( size_t i = 0; i < len; ++i ) {
        if ( data[i] == '\0' ) {
            parts.emplace_back( begin, data + i );
            begin = data + i + 1;
        }
    }
    if ( begin < data + len )
        parts.emplace_back( begin, data + len );
    return parts;
}

class TranslationDomain
{
public:
    void reset( const std::string & lang )
    {
        _locale = localeFromCode( lang );
        _map.clear();
        _codepage = nullptr;
    }

    // msgstrs of legacy .mo files are stored in single-byte encodings
    // (CP1251 for Russian, CP1250 for Polish, ...). The editor works with
    // UTF-8 everywhere, so the bytes are decoded at load time.
    void setCodepage( const char * charset )
    {
        _codepage = codepages::tableForCharset( charset );
    }

    std::string decode( const char * data, size_t len ) const
    {
        std::string out;
        out.reserve( len * 2 );
        for ( size_t i = 0; i < len; ++i ) {
            const unsigned char c = static_cast<unsigned char>( data[i] );
            if ( c < 0x80 || _codepage == nullptr ) {
                out.push_back( static_cast<char>( c ) );
                continue;
            }
            const uint16_t code = _codepage->high[c - 0x80];
            if ( code < 0x80 ) {
                out.push_back( static_cast<char>( code ) );
            }
            else if ( code < 0x800 ) {
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

    // Context-prefixed msgids ("skill|Basic", "morale|Good", "difficulty|Easy")
    // fall back to the part after the separator when no translation is found
    // (stripContext in the engine's translations.cpp).
    static std::string stripContext( const std::string & msgid )
    {
        const size_t pos = msgid.find( '|' );
        return pos == std::string::npos ? msgid : msgid.substr( pos + 1 );
    }

    std::string get( const std::string & msgid, size_t plural ) const
    {
        const auto it = _map.find( msgid );
        if ( it == _map.end() )
            return stripContext( msgid );
        const std::vector<std::string> & forms = it->second;
        if ( plural >= forms.size() || forms[plural].empty() )
            return stripContext( msgid );
        return forms[plural];
    }

    size_t pluralForm( size_t n ) const
    {
        return pluralIndex( _locale, n );
    }

    void insert( std::string msgid, std::vector<std::string> forms )
    {
        _map.emplace( std::move( msgid ), std::move( forms ) );
    }

private:
    Locale _locale = Locale::EN;
    const codepages::CodepageTable * _codepage = nullptr;
    std::unordered_map<std::string, std::vector<std::string>> _map;
};

TranslationDomain gameDomain;
TranslationDomain editorDomain;
std::string activeLanguage = "en";

std::vector<uint8_t> readFileBytes( const std::string & path )
{
    std::ifstream f( path, std::ios::binary );
    if ( !f )
        return {};
    return std::vector<uint8_t>( std::istreambuf_iterator<char>( f ), std::istreambuf_iterator<char>() );
}

// Minimal .po parser: msgid/msgstr pairs, double-quoted lines (no msgctxt/plural
// support — the editor's own .po files only use simple pairs).
bool parsePo( const std::string & text, TranslationDomain & domain )
{
    std::string msgid;
    std::string msgstr;
    bool inMsgid = false;
    bool inMsgstr = false;
    bool insertedAny = false;

    auto unescape = []( const std::string & s ) {
        std::string out;
        for ( size_t i = 0; i < s.size(); ++i ) {
            if ( s[i] == '\\' && i + 1 < s.size() ) {
                switch ( s[i + 1] ) {
                case 'n': out.push_back( '\n' ); ++i; continue;
                case 't': out.push_back( '\t' ); ++i; continue;
                case '"': out.push_back( '"' ); ++i; continue;
                case '\\': out.push_back( '\\' ); ++i; continue;
                default: break;
                }
            }
            out.push_back( s[i] );
        }
        return out;
    };

    // A quoted .po line: strips the surrounding double quotes.
    const auto parseQuoted = []( const std::string & s ) {
        if ( s.size() >= 2 && s.front() == '"' && s.back() == '"' )
            return s.substr( 1, s.size() - 2 );
        return s;
    };

    auto commit = [&]() {
        if ( !msgid.empty() && !msgstr.empty() ) {
            domain.insert( unescape( parseQuoted( msgid ) ), { unescape( parseQuoted( msgstr ) ) } );
            insertedAny = true;
        }
        msgid.clear();
        msgstr.clear();
    };

    size_t pos = 0;
    while ( pos < text.size() ) {
        const size_t lineEnd = text.find( '\n', pos );
        const std::string line = text.substr( pos, lineEnd == std::string::npos ? std::string::npos : lineEnd - pos );
        pos = ( lineEnd == std::string::npos ) ? text.size() : lineEnd + 1;

        if ( line.rfind( "msgid ", 0 ) == 0 ) {
            commit();
            inMsgid = true;
            inMsgstr = false;
            msgid = line.substr( 6 );
        }
        else if ( line.rfind( "msgstr ", 0 ) == 0 ) {
            inMsgid = false;
            inMsgstr = true;
            msgstr = line.substr( 7 );
        }
        else if ( !line.empty() && line[0] == '"' && ( inMsgid || inMsgstr ) ) {
            ( inMsgid ? msgid : msgstr ) += line;
        }
        else {
            // Comments and metadata lines end the current entry's continuation.
            if ( line.empty() ) {
                commit();
                inMsgid = inMsgstr = false;
            }
        }
    }
    commit();
    return insertedAny;
}

} // namespace
void initTranslationDomains( const std::string & lang )
{
    activeLanguage = lang;
    gameDomain.reset( lang );
    editorDomain.reset( lang );
}

const std::string & translationLanguage()
{
    return activeLanguage;
}

bool loadGameTranslationFromDataDir( const std::string & dataDir )
{
    if ( dataDir.empty() )
        return false;
    return loadGameTranslation( dataDir + "/files/lang/" + activeLanguage + ".mo" );
}



bool loadGameTranslation( const std::string & path )
{
    const std::vector<uint8_t> data = readFileBytes( path );
    if ( data.size() < 28 )
        return false;

    bool bigEndian = false;
    const uint32_t magic = readLe32( data.data() );
    if ( magic == 0x950412deu )
        bigEndian = false;
    else if ( magic == 0xde120495u )
        bigEndian = true;
    else
        return false;
    const auto rd32 = [bigEndian]( const uint8_t * p ) { return bigEndian ? readBe32( p ) : readLe32( p ); };

    if ( rd32( data.data() + 4 ) != 0 )
        return false; // major revision

    const uint32_t count = rd32( data.data() + 8 );
    const uint32_t origTable = rd32( data.data() + 12 );
    const uint32_t transTable = rd32( data.data() + 16 );
    if ( count == 0 )
        return false;

    // The metadata entry (msgid "") declares the encoding of the msgstrs —
    // legacy .mo files are single-byte (CP1251/CP1250/...). Read it first.
    {
        const size_t transEntry = transTable;
        if ( transEntry + 8 <= data.size() ) {
            const uint32_t transLen = rd32( data.data() + transEntry );
            const uint32_t transOff = rd32( data.data() + transEntry + 4 );
            if ( transLen > 0 && transOff + transLen <= data.size() ) {
                const std::string header( reinterpret_cast<const char *>( data.data() + transOff ), transLen );
                const std::string marker = "charset=";
                const size_t pos = header.find( marker );
                if ( pos != std::string::npos ) {
                    size_t end = pos + marker.size();
                    while ( end < header.size() && header[end] != '\n' && header[end] != ';' && header[end] != '\r' )
                        ++end;
                    const std::string charset = header.substr( pos + marker.size(), end - pos - marker.size() );
                    if ( charset != "UTF-8" && charset != "utf-8" && charset != "ASCII" && charset != "ascii" )
                        gameDomain.setCodepage( charset.c_str() );
                }
            }
        }
    }

    for ( uint32_t i = 0; i < count; ++i ) {
        const size_t origEntry = origTable + i * 8;
        const size_t transEntry = transTable + i * 8;
        if ( origEntry + 8 > data.size() || transEntry + 8 > data.size() )
            return false;

        const uint32_t origLen = rd32( data.data() + origEntry );
        const uint32_t origOff = rd32( data.data() + origEntry + 4 );
        const uint32_t transLen = rd32( data.data() + transEntry );
        const uint32_t transOff = rd32( data.data() + transEntry + 4 );
        if ( origLen == 0 || transLen == 0 )
            continue;
        if ( origOff + origLen > data.size() || transOff + transLen > data.size() )
            return false;

        // Plural entries store the msgid as "singular\0plural" — only the
        // singular part is the lookup key (gettext convention); the plural
        // forms live in the msgstr.
        const std::string msgid( reinterpret_cast<const char *>( data.data() + origOff ), origLen );
        std::vector<std::string> forms = splitNul( reinterpret_cast<const char *>( data.data() + transOff ), transLen );
        if ( forms.empty() )
            continue;
        for ( std::string & form : forms )
            form = gameDomain.decode( form.data(), form.size() );
        gameDomain.insert( std::string( msgid.c_str() ), std::move( forms ) );
    }
    return true;
}

bool loadEditorTranslation( const std::string & path )
{
    const std::vector<uint8_t> data = readFileBytes( path );
    if ( data.empty() )
        return false;
    return parsePo( std::string( data.begin(), data.end() ), editorDomain );
}

std::string trGame( const std::string & msgid )
{
    return gameDomain.get( msgid, 0 );
}

std::string trEditor( const std::string & msgid )
{
    return editorDomain.get( msgid, 0 );
}

std::string trGameOrEditor( const std::string & msgid )
{
    const std::string t = gameDomain.get( msgid, 0 );
    if ( t != msgid )
        return t;
    return editorDomain.get( msgid, 0 );
}

std::string trGamePlural( const std::string & msgid, size_t n )
{
    return gameDomain.get( msgid, gameDomain.pluralForm( n ) );
}

std::string trEditorPlural( const std::string & msgid, size_t n )
{
    return editorDomain.get( msgid, editorDomain.pluralForm( n ) );
}

} // namespace fh2
