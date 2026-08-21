// WebAssembly API for the web editor (GitHub Pages).
//
// Embind wrapper around the Qt-free core (savefile.cpp, constants.cpp,
// gettextmo.cpp). No Qt, no filesystem: the save comes in as bytes
// (Uint8Array), the edited save goes out as bytes.
//
// Comments are in English (project convention).

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "../src/constants.h"
#include "../src/savefile.h"

namespace {

std::unique_ptr<fh2::SaveFile> g_save;

// --- JSON helpers ---

std::string jsonString( const std::string & s )
{
    std::string out = "\"";
    for ( unsigned char c : s ) {
        switch ( c ) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if ( c < 0x20 )
                out += '?';
            else
                out += static_cast<char>( c );
            break;
        }
    }
    out += '"';
    return out;
}

std::string jsonInt( int64_t v )
{
    return std::to_string( v );
}

// --- cp1251 <-> UTF-8 (names in the save are cp1251) ---

// Windows-1251 upper half (0x80..0xFF) -> Unicode. Same table as in main.cpp.
const uint32_t CP1251_TO_UNICODE[128] = {
    0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021, 0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
    0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F, 0x00A0,
    0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7, 0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407, 0x00B0,
    0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7, 0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457, 0x00BF,
    0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
    0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
    0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
};

std::map<uint32_t, uint8_t> unicodeToCp1251Map()
{
    std::map<uint32_t, uint8_t> m;
    for ( int i = 0; i < 128; ++i )
        m[CP1251_TO_UNICODE[i]] = static_cast<uint8_t>( 0x80 + i );
    return m;
}

void appendUtf8( std::string & out, uint32_t code )
{
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

std::string cp1251ToUtf8( const std::string & s )
{
    std::string out;
    for ( unsigned char c : s ) {
        if ( c < 0x80 )
            out.push_back( static_cast<char>( c ) );
        else
            appendUtf8( out, CP1251_TO_UNICODE[c - 0x80] );
    }
    return out;
}

// UTF-8 -> cp1251. Throws on characters outside cp1251 (byte values < 0x80
// always pass, matching cp1251ToUtf8's behavior for 0x80..0xFF round-trips).
std::string utf8ToCp1251( const std::string & s )
{
    static const std::map<uint32_t, uint8_t> rev = unicodeToCp1251Map();
    std::string out;
    for ( size_t i = 0; i < s.size(); ) {
        const unsigned char c = s[i];
        if ( c < 0x80 ) {
            out.push_back( static_cast<char>( c ) );
            ++i;
            continue;
        }
        uint32_t code = 0;
        int len = 0;
        if ( ( c & 0xE0 ) == 0xC0 && i + 1 < s.size() ) {
            code = ( static_cast<uint32_t>( c & 0x1F ) << 6 ) | ( s[i + 1] & 0x3F );
            len = 2;
        }
        else if ( ( c & 0xF0 ) == 0xE0 && i + 2 < s.size() ) {
            code = ( static_cast<uint32_t>( c & 0x0F ) << 12 ) | ( static_cast<uint32_t>( s[i + 1] & 0x3F ) << 6 ) | ( s[i + 2] & 0x3F );
            len = 3;
        }
        else {
            throw std::runtime_error( "Invalid UTF-8 in name" );
        }
        const auto it = rev.find( code );
        if ( it == rev.end() )
            throw std::runtime_error( "Character not supported (name must use the save's codepage)" );
        out.push_back( static_cast<char>( it->second ) );
        i += len;
    }
    return out;
}

// --- typed array <-> bytes ---

std::vector<uint8_t> valToBytes( const emscripten::val & v )
{
    const unsigned len = v["byteLength"].as<unsigned>();
    std::vector<uint8_t> out( len );
    emscripten::val view = emscripten::val( emscripten::typed_memory_view( len, out.data() ) );
    view.call<void>( "set", v );
    return out;
}

std::vector<int> valToIntVector( const emscripten::val & v )
{
    const unsigned len = v["length"].as<unsigned>();
    std::vector<int> out;
    out.reserve( len );
    for ( unsigned i = 0; i < len; ++i )
        out.push_back( v[i].as<int>() );
    return out;
}

// std::vector<uint8_t> has no embind binding — return a real Uint8Array instead.
emscripten::val bytesToVal( const std::vector<uint8_t> & bytes )
{
    emscripten::val arr = emscripten::val::global( "Uint8Array" ).new_( bytes.size() );
    emscripten::val view = emscripten::val( emscripten::typed_memory_view( bytes.size(), bytes.data() ) );
    arr.call<void>( "set", view );
    return arr;
}

// --- error wrapper: setters return "" on success, an error message otherwise ---

template <typename F>
std::string call( F && f )
{
    try {
        f();
        return {};
    }
    catch ( const std::exception & e ) {
        return e.what();
    }
}

fh2::SaveFile & sv()
{
    if ( !g_save )
        throw std::runtime_error( "No save file is open" );
    return *g_save;
}

fh2::HeroRecord & hero( int index )
{
    if ( index < 0 || index >= static_cast<int>( sv().heroes().size() ) )
        throw std::runtime_error( "Invalid hero index" );
    return sv().heroes()[static_cast<size_t>( index )];
}

// --- API ---

std::string openSave( const std::string & name, const emscripten::val & data )
{
    try {
        g_save = std::make_unique<fh2::SaveFile>( fh2::SaveFile::loadFromBytes( name, valToBytes( data ) ) );
        return {};
    }
    catch ( const std::exception & e ) {
        g_save.reset();
        return e.what();
    }
}

void closeSave()
{
    g_save.reset();
}

std::string mapInfoJson()
{
    const fh2::MapInfo & m = sv().mapInfo();
    std::string out = "{";
    out += "\"name\":" + jsonString( cp1251ToUtf8( m.name ) ) + ",";
    out += "\"filename\":" + jsonString( m.filename ) + ",";
    out += "\"width\":" + jsonInt( m.width ) + ",";
    out += "\"height\":" + jsonInt( m.height ) + ",";
    out += "\"difficulty\":" + jsonInt( m.difficulty ) + ",";
    out += "\"worldMonth\":" + jsonInt( m.worldMonth ) + ",";
    out += "\"worldWeek\":" + jsonInt( m.worldWeek ) + ",";
    out += "\"worldDay\":" + jsonInt( m.worldDay ) + ",";
    out += "\"formatVersion\":" + jsonInt( sv().formatVersion() );
    out += "}";
    return out;
}

int heroCount()
{
    return static_cast<int>( sv().heroes().size() );
}

std::string heroName( int index )
{
    return cp1251ToUtf8( hero( index ).name );
}

std::string heroDataJson( int index )
{
    const fh2::HeroRecord & h = hero( index );
    std::string out = "{";
    out += "\"name\":" + jsonString( cp1251ToUtf8( h.name ) ) + ",";
    out += "\"nameLen\":" + jsonInt( h.name.size() ) + ",";
    out += "\"color\":" + jsonInt( h.color ) + ",";
    out += "\"race\":" + jsonInt( h.race ) + ",";
    out += "\"raceName\":" + jsonString( fh2::raceName( h.race ) ) + ",";
    out += "\"heroId\":" + jsonInt( h.heroId ) + ",";
    out += "\"portrait\":" + jsonInt( h.portrait ) + ",";
    out += "\"heroBaseParsed\":" + std::string( h.heroBaseParsed ? "true" : "false" ) + ",";
    out += "\"experience\":" + jsonInt( h.experience ) + ",";
    out += "\"level\":" + jsonInt( fh2::heroLevel( h.experience ) ) + ",";
    out += "\"primary\":{\"attack\":" + jsonInt( h.primary.attack ) + ",\"defense\":" + jsonInt( h.primary.defense )
           + ",\"power\":" + jsonInt( h.primary.power ) + ",\"knowledge\":" + jsonInt( h.primary.knowledge ) + "},";
    out += "\"spellPoints\":" + jsonInt( h.spellPoints ) + ",";
    out += "\"army\":[";
    for ( int i = 0; i < 5; ++i ) {
        if ( i )
            out += ",";
        out += "{\"monsterId\":" + jsonInt( h.slots[i].monsterId ) + ",\"monsterName\":"
               + jsonString( fh2::monsterName( h.slots[i].monsterId ) ) + ",\"count\":" + jsonInt( h.slots[i].count ) + "}";
    }
    out += "],";
    out += "\"skills\":[";
    for ( size_t i = 0; i < h.skills.size(); ++i ) {
        if ( i )
            out += ",";
        out += "{\"id\":" + jsonInt( h.skills[i].first ) + ",\"name\":" + jsonString( fh2::skillName( h.skills[i].first ) )
               + ",\"level\":" + jsonInt( h.skills[i].second ) + ",\"levelName\":"
               + jsonString( fh2::skillLevelName( h.skills[i].second ) ) + "}";
    }
    out += "],";
    out += "\"artifactCount\":" + jsonInt( h.artifactCount ) + ",";
    out += "\"artifacts\":[";
    for ( int i = 0; i < 14; ++i ) {
        if ( i )
            out += ",";
        const int id = i < h.artifactCount ? h.artifacts[i].id : 0;
        out += "{\"id\":" + jsonInt( id ) + ",\"name\":" + jsonString( id > 0 ? fh2::artifactName( id ) : std::string() ) + "}";
    }
    out += "],";
    out += "\"spells\":[";
    for ( size_t i = 0; i < h.spells.size(); ++i ) {
        if ( i )
            out += ",";
        out += jsonInt( h.spells[i] );
    }
    out += "]";
    out += "}";
    return out;
}

std::string setSlot( int index, int slot, int monsterId, int count )
{
    return call( [&]() { sv().setSlot( hero( index ), slot, monsterId, count ); } );
}

std::string setPrimarySkill( int index, int skillIndex, int value )
{
    return call( [&]() { sv().setPrimarySkill( hero( index ), skillIndex, value ); } );
}

std::string setSpellPoints( int index, int value )
{
    return call( [&]() { sv().setSpellPoints( hero( index ), value ); } );
}

std::string setExperience( int index, double value )
{
    return call( [&]() { sv().setExperience( hero( index ), static_cast<uint32_t>( value ) ); } );
}

std::string setArtifact( int index, int slot, int id )
{
    return call( [&]() {
        fh2::HeroRecord & h = hero( index );
        if ( slot >= h.artifactCount ) {
            // Visual slots beyond the bag: append (0 is a no-op there).
            if ( id == 0 )
                return;
            sv().setArtifact( h, h.artifactCount, id );
        }
        else {
            sv().setArtifact( h, slot, id );
        }
    } );
}

std::string setSecondarySkill( int index, int skillIndex, int skillId, int level )
{
    return call( [&]() {
        fh2::HeroRecord & h = hero( index );
        if ( skillIndex < static_cast<int>( h.skills.size() ) ) {
            if ( level == 0 )
                sv().removeSecondarySkill( h, skillIndex );
            else
                sv().setSecondarySkill( h, skillIndex, skillId, level );
        }
        else if ( level != 0 ) {
            sv().setSecondarySkill( h, skillIndex, skillId, level );
        }
    } );
}

std::string setSpells( int index, const emscripten::val & spellIds )
{
    return call( [&]() { sv().setSpells( hero( index ), valToIntVector( spellIds ) ); } );
}

std::string setRace( int index, int race )
{
    return call( [&]() { sv().setRace( hero( index ), race ); } );
}

std::string setPortrait( int index, int portrait )
{
    return call( [&]() { sv().setPortrait( hero( index ), portrait ); } );
}

std::string setName( int index, const std::string & utf8Name )
{
    return call( [&]() { sv().setName( hero( index ), utf8ToCp1251( utf8Name ) ); } );
}

// Byte length of the UTF-8 name encoded as cp1251, or -1 if it contains
// characters outside cp1251 (used by the UI name-length hint).
int nameCp1251Len( const std::string & utf8Name )
{
    try {
        return static_cast<int>( utf8ToCp1251( utf8Name ).size() );
    }
    catch ( ... ) {
        return -1;
    }
}

emscripten::val saveBytes()
{
    return bytesToVal( sv().saveToBytes() );
}

// --- reference tables (names come from constants.cpp, one source of truth) ---

std::string monsterListJson()
{
    std::string out = "[";
    bool first = true;
    for ( int id : fh2::editableMonsters() ) {
        if ( !first )
            out += ",";
        first = false;
        out += "{\"id\":" + jsonInt( id ) + ",\"name\":" + jsonString( fh2::monsterName( id ) ) + ",\"hp\":"
               + jsonInt( fh2::monsterHp( id ) ) + "}";
    }
    out += "]";
    return out;
}

std::string skillListJson()
{
    std::string out = "[";
    for ( int id = 1; id <= 14; ++id ) {
        if ( id > 1 )
            out += ",";
        out += "{\"id\":" + jsonInt( id ) + ",\"name\":" + jsonString( fh2::skillName( id ) ) + "}";
    }
    out += "]";
    return out;
}

std::string artifactListJson()
{
    std::string out = "[";
    for ( int id = 1; id <= 82; ++id ) {
        if ( id > 1 )
            out += ",";
        out += "{\"id\":" + jsonInt( id ) + ",\"name\":" + jsonString( fh2::artifactName( id ) ) + "}";
    }
    out += "]";
    return out;
}

std::string spellListJson()
{
    std::string out = "[";
    for ( int id = 1; id <= 65; ++id ) {
        if ( id > 1 )
            out += ",";
        out += "{\"id\":" + jsonInt( id ) + ",\"name\":" + jsonString( fh2::spellName( id ) ) + ",\"level\":"
               + jsonInt( fh2::spellLevel( id ) ) + "}";
    }
    out += "]";
    return out;
}

std::string primarySkillNamesJson()
{
    std::string out = "[";
    for ( int i = 0; i < 4; ++i ) {
        if ( i )
            out += ",";
        out += jsonString( fh2::primarySkillName( i ) );
    }
    out += "]";
    return out;
}

std::string skillLevelNamesJson()
{
    std::string out = "[";
    for ( int i = 1; i <= 3; ++i ) {
        if ( i > 1 )
            out += ",";
        out += jsonString( fh2::skillLevelName( i ) );
    }
    out += "]";
    return out;
}

std::string colorName( int mask )
{
    const auto & colors = fh2::playerColors();
    const auto it = colors.find( mask );
    return it == colors.end() ? std::string( "?" ) : it->second.name;
}

} // namespace

EMSCRIPTEN_BINDINGS( fh2editor )
{
    emscripten::function( "openSave", &openSave );
    emscripten::function( "closeSave", &closeSave );
    emscripten::function( "mapInfoJson", &mapInfoJson );
    emscripten::function( "heroCount", &heroCount );
    emscripten::function( "heroName", &heroName );
    emscripten::function( "heroDataJson", &heroDataJson );
    emscripten::function( "setSlot", &setSlot );
    emscripten::function( "setPrimarySkill", &setPrimarySkill );
    emscripten::function( "setSpellPoints", &setSpellPoints );
    emscripten::function( "setExperience", &setExperience );
    emscripten::function( "setArtifact", &setArtifact );
    emscripten::function( "setSecondarySkill", &setSecondarySkill );
    emscripten::function( "setSpells", &setSpells );
    emscripten::function( "setRace", &setRace );
    emscripten::function( "setPortrait", &setPortrait );
    emscripten::function( "setName", &setName );
    emscripten::function( "nameCp1251Len", &nameCp1251Len );
    emscripten::function( "saveBytes", &saveBytes );
    emscripten::function( "monsterListJson", &monsterListJson );
    emscripten::function( "skillListJson", &skillListJson );
    emscripten::function( "artifactListJson", &artifactListJson );
    emscripten::function( "spellListJson", &spellListJson );
    emscripten::function( "primarySkillNamesJson", &primarySkillNamesJson );
    emscripten::function( "skillLevelNamesJson", &skillLevelNamesJson );
    emscripten::function( "colorName", &colorName );
    emscripten::function( "raceName", +[]( int race ) { return fh2::raceName( race ); } );
}
