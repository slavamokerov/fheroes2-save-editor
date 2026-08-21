#include "savefile.h"

#include <algorithm>
#include <cstring>
#include <fstream>

#include <zlib.h>

#include "constants.h"

namespace fh2 {

namespace {

uint32_t readBe32( const uint8_t * p )
{
    return ( static_cast<uint32_t>( p[0] ) << 24 ) | ( static_cast<uint32_t>( p[1] ) << 16 ) | ( static_cast<uint32_t>( p[2] ) << 8 ) | static_cast<uint32_t>( p[3] );
}

uint16_t readBe16( const uint8_t * p )
{
    return static_cast<uint16_t>( ( static_cast<uint16_t>( p[0] ) << 8 ) | static_cast<uint16_t>( p[1] ) );
}

int32_t readBe32s( const uint8_t * p )
{
    uint32_t v = readBe32( p );
    return static_cast<int32_t>( v );
}

void writeBe32( std::vector<uint8_t> & out, uint32_t v )
{
    out.push_back( static_cast<uint8_t>( v >> 24 ) );
    out.push_back( static_cast<uint8_t>( v >> 16 ) );
    out.push_back( static_cast<uint8_t>( v >> 8 ) );
    out.push_back( static_cast<uint8_t>( v ) );
}

void writeBe16( std::vector<uint8_t> & out, uint16_t v )
{
    out.push_back( static_cast<uint8_t>( v >> 8 ) );
    out.push_back( static_cast<uint8_t>( v ) );
}

void putBe32( std::vector<uint8_t> & buf, size_t pos, uint32_t v )
{
    buf[pos] = static_cast<uint8_t>( v >> 24 );
    buf[pos + 1] = static_cast<uint8_t>( v >> 16 );
    buf[pos + 2] = static_cast<uint8_t>( v >> 8 );
    buf[pos + 3] = static_cast<uint8_t>( v );
}

std::vector<uint8_t> readFileBytes( const std::string & path )
{
    std::ifstream f( path, std::ios::binary );
    if ( !f )
        throw SaveError( "Could not open file: " + path );
    return std::vector<uint8_t>( std::istreambuf_iterator<char>( f ), std::istreambuf_iterator<char>() );
}

// string: u32 length + bytes
size_t readString( const std::vector<uint8_t> & buf, size_t pos, size_t maxLen, std::string & out )
{
    if ( pos + 4 > buf.size() )
        throw SaveError( "Invalid file (string out of bounds)" );
    uint32_t n = readBe32( buf.data() + pos );
    if ( n > maxLen || pos + 4 + n > buf.size() )
        throw SaveError( "Invalid file (string length)" );
    out.assign( reinterpret_cast<const char *>( buf.data() + pos + 4 ), n );
    return pos + 4 + n;
}

// Non-throwing variant of readString: returns false instead of throwing.
// The hero scan calls it for every byte of the stream, so exceptions are not
// acceptable on this hot path.
bool tryReadString( const std::vector<uint8_t> & buf, size_t pos, size_t maxLen, std::string & out, size_t & endPos )
{
    if ( pos + 4 > buf.size() )
        return false;
    const uint32_t n = readBe32( buf.data() + pos );
    if ( n > maxLen || pos + 4 + n > buf.size() )
        return false;
    out.assign( reinterpret_cast<const char *>( buf.data() + pos + 4 ), n );
    endPos = pos + 4 + n;
    return true;
}

MapInfo parseMapInfo( const std::vector<uint8_t> & buf, size_t pos, int formatVersion, size_t * endPos = nullptr )
{
    MapInfo info;
    size_t p = pos;
    p = readString( buf, p, 1 << 20, info.filename );
    p = readString( buf, p, 1 << 20, info.name );
    p = readString( buf, p, 1 << 20, info.description );
    if ( p + 6 > buf.size() )
        throw SaveError( "Invalid FileInfo" );
    const uint8_t * b = buf.data() + p;
    info.width = readBe16( b );
    info.height = readBe16( b + 2 );
    info.difficulty = b[4];
    info.kingdomMax = b[5];
    p += 6;
    if ( p + static_cast<size_t>( info.kingdomMax ) * 2 > buf.size() )
        throw SaveError( "Invalid FileInfo (races)" );
    for ( int i = 0; i < info.kingdomMax; ++i ) {
        info.races.push_back( buf[p] );
        info.unions.push_back( buf[p + 1] );
        p += 2;
    }
    if ( p + 4 > buf.size() )
        throw SaveError( "Invalid FileInfo (colors)" );
    info.kingdomColors = buf[p];
    info.colorsAvailableForHumans = buf[p + 1];
    info.colorsAvailableForComp = buf[p + 2];
    info.colorsOfRandomRaces = buf[p + 3];
    p += 4;
    if ( p + 3 > buf.size() )
        throw SaveError( "Invalid FileInfo (victory)" );
    info.victoryConditionType = buf[p];
    info.compAlsoWins = buf[p + 1] != 0;
    info.allowNormalVictory = buf[p + 2] != 0;
    p += 3;
    if ( p + 4 > buf.size() )
        throw SaveError( "Invalid FileInfo" );
    info.victoryConditionParams[0] = readBe16( buf.data() + p );
    info.victoryConditionParams[1] = readBe16( buf.data() + p + 2 );
    p += 4;
    if ( p + 5 > buf.size() )
        throw SaveError( "Invalid FileInfo (loss)" );
    info.lossConditionType = buf[p];
    p += 1;
    info.lossConditionParams[0] = readBe16( buf.data() + p );
    info.lossConditionParams[1] = readBe16( buf.data() + p + 2 );
    p += 4;
    if ( p + 9 > buf.size() )
        throw SaveError( "Invalid FileInfo (time)" );
    b = buf.data() + p;
    info.timestamp = readBe32( b );
    info.startWithHeroInFirstCastle = b[4] != 0;
    info.gameVersion = readBe32s( b + 5 );
    p += 9;
    if ( p + 12 > buf.size() )
        throw SaveError( "Invalid FileInfo (date)" );
    b = buf.data() + p;
    info.worldDay = readBe32( b );
    info.worldWeek = readBe32( b + 4 );
    info.worldMonth = readBe32( b + 8 );
    p += 12;
    if ( p + 1 > buf.size() )
        throw SaveError( "Invalid FileInfo (language)" );
    info.mainLanguage = buf[p];
    p += 1;
    if ( formatVersion >= 10033 )
        p = readString( buf, p, 100000, info.creatorNotes );
    if ( endPos )
        *endPos = p;
    return info;
}

// Returns (block offset, rawSize, zipSize, decompressed data)
bool findZlibBlock( const std::vector<uint8_t> & data, size_t & zpos, size_t & zipSize, std::vector<uint8_t> & raw )
{
    for ( size_t p = 0; p + 13 < data.size(); ++p ) {
        const uint8_t * b = data.data() + p;
        uint32_t rawSize = readBe32( b );
        uint32_t zs = readBe32( b + 4 );
        uint16_t ver = readBe16( b + 8 );
        uint16_t unused = readBe16( b + 10 );
        if ( ver != 0 || unused != 0 || zs == 0 || p + 12 + zs > data.size() )
            continue;
        // A zlib stream starts with 0x78 and a level byte. Without this check
        // every plausible-looking header triggers a decompression attempt,
        // which makes opening a save take several hundred milliseconds.
        if ( b[12] != 0x78 || ( b[13] != 0x01 && b[13] != 0x5E && b[13] != 0x9C && b[13] != 0xDA ) )
            continue;
        if ( rawSize == 0 || rawSize > ( 64u << 20 ) )
            continue;
        uLongf dstLen = rawSize;
        raw.resize( rawSize );
        int ret = uncompress( raw.data(), &dstLen, data.data() + p + 12, zs );
        if ( ret != Z_OK || dstLen != rawSize )
            continue;
        if ( raw.size() >= 2 && raw[raw.size() - 2] == 0xFF && raw[raw.size() - 1] == 0x03 ) {
            zpos = p;
            zipSize = zs;
            return true;
        }
    }
    return false;
}

// Parses the HeroBase stored BEFORE the hero's name (§6.1): primary skills,
// center/modes, mana/movement points, spell book, artifacts.
// Backward scan from the start of the name with strict validation.
// Actual order (verified on 73 records of real 10032/10034 saves):
//   attack, defense, knowledge, power (i32×4), center (i16×2), modes (u32),
//   spellPoints (u32), movePoints (u32), spellBook (u32 + i32×n),
//   bagArtifacts (u32 count + (i32 id, i32 ext)×count), then the name.
void tryParseHeroBase( const std::vector<uint8_t> & buf, size_t namePos, HeroRecord & hero )
{
    // bagArtifacts: count 0..14, size 4 + 8×count, adjacent to the name.
    // Scan FROM 14 DOWN: with 0 the 4 bytes BEFORE the name are checked — that
    // is the ext of the last artifact (usually 0), and a 0→14 scan used to
    // wrongly return "0 artifacts" for every hero. The full validation below
    // rules out false matches in a descending scan.
    for ( int bagCount = 14; bagCount >= 0; --bagCount ) {
        const size_t bagSize = 4 + static_cast<size_t>( bagCount ) * 8;
        if ( namePos < bagSize )
            continue;
        const size_t artifactsStart = namePos - bagSize;
        if ( readBe32( buf.data() + artifactsStart ) != static_cast<uint32_t>( bagCount ) )
            continue;
        ArtifactSlot arts[14];
        bool ok = true;
        for ( int i = 0; i < bagCount && ok; ++i ) {
            const int id = readBe32s( buf.data() + artifactsStart + 4 + static_cast<size_t>( i ) * 8 );
            const int ext = readBe32s( buf.data() + artifactsStart + 4 + static_cast<size_t>( i ) * 8 + 4 );
            if ( id < 0 || id > 126 || ext < 0 || ext > 63 )
                ok = false;
            else
                arts[i] = { id, ext };
        }
        if ( !ok )
            continue;

        // spellBook: u32 count + count×i32, adjacent to the bag.
        size_t spellBookStart = 0;
        std::vector<int> spellIds;
        for ( int n = 0; n <= 64; ++n ) {
            const size_t q = artifactsStart - 4 - static_cast<size_t>( n ) * 4;
            if ( q < 32 )
                break;
            if ( readBe32( buf.data() + q ) != static_cast<uint32_t>( n ) )
                continue;
            bool spellsOk = true;
            std::vector<int> ids;
            ids.reserve( static_cast<size_t>( n ) );
            for ( int i = 0; i < n && spellsOk; ++i ) {
                const int id = readBe32s( buf.data() + q + 4 + static_cast<size_t>( i ) * 4 );
                if ( id < 1 || id > 65 )
                    spellsOk = false;
                else
                    ids.push_back( id );
            }
            if ( spellsOk ) {
                spellBookStart = q;
                spellIds = std::move( ids );
                break;
            }
        }
        if ( spellBookStart == 0 )
            continue;

        // Fixed tail: movePoints, spellPoints, modes, center (i16×2), power, knowledge, defense, attack.
        const size_t primaryOffset = spellBookStart - 32;
        const int attack = readBe32s( buf.data() + primaryOffset );
        const int defense = readBe32s( buf.data() + primaryOffset + 4 );
        const int knowledge = readBe32s( buf.data() + primaryOffset + 8 );
        const int power = readBe32s( buf.data() + primaryOffset + 12 );
        const int centerX = static_cast<int16_t>( readBe16( buf.data() + primaryOffset + 16 ) );
        const int centerY = static_cast<int16_t>( readBe16( buf.data() + primaryOffset + 18 ) );
        const uint32_t spellPoints = readBe32( buf.data() + primaryOffset + 24 );
        const uint32_t movePoints = readBe32( buf.data() + primaryOffset + 28 );
        if ( attack < 0 || attack > 99 || defense < 0 || defense > 99 || power < 0 || power > 99 || knowledge < 0 || knowledge > 99 )
            continue;
        if ( centerX < -10000 || centerX > 10000 || centerY < -10000 || centerY > 10000 )
            continue;
        if ( spellPoints > 9999 || movePoints > 999999 )
            continue;

        hero.heroBaseParsed = true;
        hero.primary = { attack, defense, power, knowledge };
        hero.spellPoints = static_cast<int>( spellPoints );
        hero.artifactCount = bagCount;
        for ( int i = 0; i < 14; ++i )
            hero.artifacts[i] = i < bagCount ? arts[i] : ArtifactSlot{};
        hero.spells = std::move( spellIds );
        hero.primaryOffset = primaryOffset;
        hero.spellPointsOffset = primaryOffset + 24;
        hero.artifactsOffset = artifactsStart + 4;
        hero.nameOffset = namePos;
        hero.experienceOffset = namePos + 4 + hero.name.size() + 1;
        hero.spellBookOffset = spellBookStart;
        hero.spellBookSize = 4 + static_cast<size_t>( hero.spells.size() ) * 4;
        // army: slots after armySize; after the slots spread+color, then heroId, portrait, race.
        hero.raceOffset = hero.armyOffset + 40 + 2 + 4 + 4;
        return;
    }
}

bool tryParseHero( const std::vector<uint8_t> & buf, size_t pos, HeroRecord & hero )
{
    std::string name;
    size_t p = 0;
    if ( !tryReadString( buf, pos, 64, name, p ) )
        return false;
    if ( name.empty() || name.find( '\0' ) != std::string::npos )
        return false;
    if ( p + 1 + 4 + 4 > buf.size() )
        return false;
    int color = buf[p];
    p += 1;
    if ( color > 63 )
        return false;
    uint32_t experience = readBe32( buf.data() + p );
    p += 4;
    uint32_t skillCount = readBe32( buf.data() + p );
    p += 4;
    if ( skillCount > 8 )
        return false;
    std::vector<std::pair<int, int>> skills;
    for ( uint32_t i = 0; i < skillCount; ++i ) {
        if ( p + 8 > buf.size() )
            return false;
        int s = readBe32s( buf.data() + p );
        int lvl = readBe32s( buf.data() + p + 4 );
        p += 8;
        if ( s < 0 || s > 14 || lvl < 0 || lvl > 3 )
            return false;
        skills.emplace_back( s, lvl );
    }
    if ( p + 4 > buf.size() )
        return false;
    uint32_t armySize = readBe32( buf.data() + p );
    p += 4;
    if ( armySize != 5 )
        return false;
    size_t armyOffset = p;
    Troop slots[5];
    for ( int i = 0; i < 5; ++i ) {
        if ( p + 8 > buf.size() )
            return false;
        int32_t mid = readBe32s( buf.data() + p );
        uint32_t cnt = readBe32( buf.data() + p + 4 );
        p += 8;
        if ( mid < 0 || mid > 72 || cnt > 999999 )
            return false;
        if ( mid == 0 && cnt != 0 )
            return false;
        slots[i] = { mid, cnt };
    }
    if ( p + 2 > buf.size() )
        return false;
    if ( buf[p] > 1 )
        return false;
    p += 1; // spread
    if ( buf[p] > 63 )
        return false;
    p += 1; // army color
    if ( p + 12 > buf.size() )
        return false;
    int heroId = readBe32s( buf.data() + p );
    int portrait = readBe32s( buf.data() + p + 4 );
    int race = readBe32s( buf.data() + p + 8 );
    if ( heroId < 0 || heroId >= HEROES_COUNT || portrait < 0 || portrait >= HEROES_COUNT )
        return false;
    if ( race < 0 || race > 0xFF )
        return false;

    hero.pos = pos;
    hero.name = name;
    hero.color = color;
    hero.experience = experience;
    hero.skills = std::move( skills );
    for ( int i = 0; i < 5; ++i )
        hero.slots[i] = slots[i];
    hero.armyOffset = armyOffset;
    hero.heroId = heroId;
    hero.portrait = portrait;
    hero.race = race;
    tryParseHeroBase( buf, pos, hero );
    return true;
}

bool parsePlayer( const std::vector<uint8_t> & buf, size_t & p, PlayerInfo & player )
{
    try {
        if ( p + 4 + 4 + 1 + 4 + 1 > buf.size() )
            return false;
        p += 4; // modes
        int control = readBe32s( buf.data() + p );
        p += 4;
        int color = buf[p];
        p += 1;
        int race = readBe32s( buf.data() + p );
        p += 4;
        int friends = buf[p];
        p += 1;
        std::string name;
        p = readString( buf, p, 256, name );
        if ( p + 8 + 4 + 1 > buf.size() )
            return false;
        int focus0 = readBe32s( buf.data() + p );
        int focus1 = readBe32s( buf.data() + p + 4 );
        p += 8;
        int personality = readBe32s( buf.data() + p );
        p += 4;
        int handicap = buf[p];
        p += 1;
        if ( color != 0 && color != 1 && color != 2 && color != 4 && color != 8 && color != 16 && color != 32 )
            return false;
        if ( control != 0 && control != 1 && control != 4 && control != 5 )
            return false;
        player.color = color;
        player.control = control;
        player.race = race;
        player.friends = friends;
        player.name = name;
        player.focus[0] = focus0;
        player.focus[1] = focus1;
        player.personality = personality;
        player.handicap = handicap;
        return true;
    }
    catch ( const SaveError & ) {
        return false;
    }
}

// Searches for Settings.players using the anchor "filename + map name".
// On success sets playersAnchor to the offset of the anchor in the stream.
bool findPlayers( const std::vector<uint8_t> & raw, const MapInfo & headerMapInfo, int headerGameType, int formatVersion,
                  std::vector<PlayerInfo> & players, int & colorsMask, int & current, size_t * playersAnchor = nullptr )
{
    if ( headerMapInfo.filename.empty() || headerMapInfo.name.empty() )
        return false;
    std::vector<uint8_t> anchor;
    writeBe32( anchor, static_cast<uint32_t>( headerMapInfo.filename.size() ) );
    anchor.insert( anchor.end(), headerMapInfo.filename.begin(), headerMapInfo.filename.end() );
    writeBe32( anchor, static_cast<uint32_t>( headerMapInfo.name.size() ) );
    anchor.insert( anchor.end(), headerMapInfo.name.begin(), headerMapInfo.name.end() );

    for ( size_t off = 0; off + anchor.size() + 40 < raw.size(); ++off ) {
        // Cheap pre-filter: the u32 before the anchor must be the filename length.
        if ( readBe32( raw.data() + off ) != static_cast<uint32_t>( headerMapInfo.filename.size() ) )
            continue;
        if ( std::memcmp( raw.data() + off, anchor.data(), anchor.size() ) != 0 )
            continue;
        try {
            size_t p = off;
            MapInfo info = parseMapInfo( raw, off, formatVersion, &p );
            if ( info.width != headerMapInfo.width || info.height != headerMapInfo.height )
                continue;
            if ( p + 8 > raw.size() )
                continue;
            p += 4; // difficulty
            int gameType = readBe32s( raw.data() + p );
            p += 4;
            if ( gameType != headerGameType )
                continue;
            if ( p + 2 > raw.size() )
                continue;
            colorsMask = raw[p];
            current = raw[p + 1];
            p += 2;
            int count = 0;
            for ( int v = colorsMask; v; v &= v - 1 )
                ++count;
            if ( count < 1 || count > 6 )
                continue;
            std::vector<PlayerInfo> parsed;
            bool ok = true;
            for ( int i = 0; i < count; ++i ) {
                PlayerInfo pl;
                if ( !parsePlayer( raw, p, pl ) ) {
                    ok = false;
                    break;
                }
                parsed.push_back( pl );
            }
            if ( ok ) {
                players = std::move( parsed );
                if ( playersAnchor != nullptr )
                    *playersAnchor = off;
                return true;
            }
        }
        catch ( const SaveError & ) {
            continue;
        }
    }
    return false;
}

} // namespace

SaveFile SaveFile::load( const std::string & path )
{
    return loadFromBytes( path, readFileBytes( path ) );
}

SaveFile SaveFile::loadFromBytes( const std::string & name, const std::vector<uint8_t> & data )
{
    SaveFile sv;
    sv._path = name;
    sv._data = data;
    if ( sv._data.size() < 12 || sv._data[0] != 0xFF || sv._data[1] != 0x03 )
        throw SaveError( "Does not look like an fheroes2 save file" );

    size_t pos = 2;
    std::string versionString;
    pos = readString( sv._data, pos, 64, versionString );
    if ( pos + 4 > sv._data.size() )
        throw SaveError( "Invalid header" );
    sv._formatVersion = readBe16( sv._data.data() + pos );
    sv._requirements = readBe16( sv._data.data() + pos + 2 );
    pos += 4;
    if ( sv._formatVersion < MIN_SUPPORTED_VERSION || sv._formatVersion > MAX_SUPPORTED_VERSION )
        throw SaveError( "Unsupported format version: " + std::to_string( sv._formatVersion ) );
    sv._mapInfo = parseMapInfo( sv._data, pos, sv._formatVersion, &pos );
    if ( pos + 4 > sv._data.size() )
        throw SaveError( "Invalid header (gameType)" );
    sv._gameType = readBe32s( sv._data.data() + pos );

    size_t zipSize = 0;
    if ( !findZlibBlock( sv._data, sv._zpos, zipSize, sv._raw ) )
        throw SaveError( "zlib data block not found" );

    // The Settings section (players) comes after the World section that holds
    // the heroes — finding the players anchor first lets us bound the hero
    // scan and skip the tail of the stream.
    size_t playersAnchor = 0;
    findPlayers( sv._raw, sv._mapInfo, sv._gameType, sv._formatVersion, sv._players, sv._playersColorsMask, sv._playersCurrentColor, &playersAnchor );

    const size_t scanEnd = playersAnchor > 0 ? playersAnchor : sv._raw.size();
    for ( size_t off = 0; off + 8 < scanEnd; ++off ) {
        // Cheap pre-filter: tryParseHero expects the u32 name length AT the
        // candidate offset and the name bytes right after it. Requiring a
        // plausible length and a non-zero first name byte rejects almost
        // every offset without entering the parser.
        const uint32_t nameLen = readBe32( sv._raw.data() + off );
        if ( nameLen < 1 || nameLen > 63 )
            continue;
        if ( sv._raw[off + 4] == 0 )
            continue;
        HeroRecord hero;
        if ( tryParseHero( sv._raw, off, hero ) )
            sv._heroes.push_back( std::move( hero ) );
    }

    return sv;
}

std::vector<int> SaveFile::humanColors() const
{
    std::vector<int> out;
    for ( const auto & p : _players )
        if ( p.isHuman() && p.color != 0 )
            out.push_back( p.color );
    return out;
}

std::vector<HeroRecord *> SaveFile::heroesByColor( int color )
{
    std::vector<HeroRecord *> out;
    for ( auto & h : _heroes )
        if ( h.color == color )
            out.push_back( &h );
    return out;
}

std::vector<const HeroRecord *> SaveFile::heroesByColor( int color ) const
{
    std::vector<const HeroRecord *> out;
    for ( const auto & h : _heroes )
        if ( h.color == color )
            out.push_back( &h );
    return out;
}

void SaveFile::setSlot( HeroRecord & hero, int slotIndex, int monsterId, int count )
{
    if ( slotIndex < 0 || slotIndex > 4 )
        throw SaveError( "Invalid army slot index" );
    if ( monsterId < 0 || monsterId > 72 )
        throw SaveError( "Invalid monster ID" );
    if ( count < 0 || count > 999999 )
        throw SaveError( "Invalid troop count" );
    size_t off = hero.armyOffset + static_cast<size_t>( slotIndex ) * 8;
    putBe32( _raw, off, static_cast<uint32_t>( monsterId ) );
    putBe32( _raw, off + 4, static_cast<uint32_t>( count ) );
    hero.slots[slotIndex] = { monsterId, static_cast<uint32_t>( count ) };
    _dirty = true;
}

int SaveFile::addBlackDragons( HeroRecord & hero, int count )
{
    for ( int i = 0; i < 5; ++i ) {
        if ( hero.slots[i].monsterId == MONSTER_BLACK_DRAGON && hero.slots[i].count > 0 ) {
            setSlot( hero, i, MONSTER_BLACK_DRAGON, static_cast<int>( std::min<uint32_t>( 999999, hero.slots[i].count + count ) ) );
            return i;
        }
    }
    for ( int i = 0; i < 5; ++i ) {
        if ( hero.slots[i].monsterId == 0 && hero.slots[i].count == 0 ) {
            setSlot( hero, i, MONSTER_BLACK_DRAGON, count );
            return i;
        }
    }
    int weakest = -1;
    double weakestStrength = 0;
    for ( int i = 0; i < 5; ++i ) {
        double strength = static_cast<double>( hero.slots[i].count ) * monsterHp( hero.slots[i].monsterId );
        if ( weakest < 0 || strength < weakestStrength ) {
            weakest = i;
            weakestStrength = strength;
        }
    }
    if ( weakest < 0 )
        throw SaveError( "All army slots are empty" );
    setSlot( hero, weakest, MONSTER_BLACK_DRAGON, count );
    return weakest;
}

void SaveFile::setPrimarySkill( HeroRecord & hero, int index, int value )
{
    if ( !hero.heroBaseParsed )
        throw SaveError( "Primary skills unavailable: hero record not recognized" );
    if ( index < 0 || index > 3 || value < 0 || value > 99 )
        throw SaveError( "Invalid skill value" );
    putBe32( _raw, hero.primarySkillOffset( index ), static_cast<uint32_t>( value ) );
    switch ( index ) {
    case 0: hero.primary.attack = value; break;
    case 1: hero.primary.defense = value; break;
    case 2: hero.primary.power = value; break;
    case 3: hero.primary.knowledge = value; break;
    default: break;
    }
    _dirty = true;
}

void SaveFile::setSpellPoints( HeroRecord & hero, int value )
{
    if ( !hero.heroBaseParsed )
        throw SaveError( "Spell points unavailable: hero record not recognized" );
    if ( value < 0 || value > 9999 )
        throw SaveError( "Invalid spell points value" );
    putBe32( _raw, hero.spellPointsOffset, static_cast<uint32_t>( value ) );
    hero.spellPoints = value;
    _dirty = true;
}

void SaveFile::setExperience( HeroRecord & hero, uint32_t value )
{
    if ( hero.experienceOffset == 0 )
        throw SaveError( "Experience unavailable: hero record not recognized" );
    putBe32( _raw, hero.experienceOffset, value );
    hero.experience = value;
    _dirty = true;
}

void SaveFile::shiftHeroOffsets( HeroRecord & hero, size_t from, ptrdiff_t delta, unsigned skipMask )
{
    if ( delta == 0 )
        return;
    const auto shift = [&]( size_t & off ) {
        if ( off >= from )
            off = static_cast<size_t>( static_cast<ptrdiff_t>( off ) + delta );
    };
    shift( hero.pos );
    shift( hero.armyOffset );
    shift( hero.nameOffset );
    shift( hero.experienceOffset );
    shift( hero.primaryOffset );
    shift( hero.spellPointsOffset );
    shift( hero.raceOffset );
    if ( ( skipMask & 1 ) == 0 )
        shift( hero.artifactsOffset );
    if ( ( skipMask & 2 ) == 0 )
        shift( hero.spellBookOffset );
}

void SaveFile::resizeRegion( size_t start, size_t oldLen, const std::vector<uint8_t> & newData, HeroRecord * owner, unsigned skipOwned )
{
    if ( start + oldLen > _raw.size() )
        throw SaveError( "Invalid save stream region" );
    const ptrdiff_t delta = static_cast<ptrdiff_t>( newData.size() ) - static_cast<ptrdiff_t>( oldLen );
    std::vector<uint8_t> rebuilt;
    rebuilt.reserve( _raw.size() + std::max<ptrdiff_t>( 0, delta ) );
    rebuilt.insert( rebuilt.end(), _raw.begin(), _raw.begin() + static_cast<ptrdiff_t>( start ) );
    rebuilt.insert( rebuilt.end(), newData.begin(), newData.end() );
    rebuilt.insert( rebuilt.end(), _raw.begin() + static_cast<ptrdiff_t>( start + oldLen ), _raw.end() );
    _raw = std::move( rebuilt );

    if ( delta != 0 ) {
        const size_t from = start + oldLen;
        for ( HeroRecord & h : _heroes )
            shiftHeroOffsets( h, from, delta, &h == owner ? skipOwned : 0 );
    }
    _dirty = true;
}

void SaveFile::setSpells( HeroRecord & hero, const std::vector<int> & spellIds )
{
    if ( !hero.heroBaseParsed )
        throw SaveError( "Spells unavailable: hero record not recognized" );
    if ( spellIds.size() > SPELL_COUNT )
        throw SaveError( "Too many spells" );
    std::set<int> seen;
    for ( int id : spellIds ) {
        if ( id < 1 || id > SPELL_COUNT || !seen.insert( id ).second )
            throw SaveError( "Invalid or duplicate spell ID" );
    }

    std::vector<uint8_t> data;
    writeBe32( data, static_cast<uint32_t>( spellIds.size() ) );
    for ( int id : spellIds )
        writeBe32( data, static_cast<uint32_t>( id ) );

    resizeRegion( hero.spellBookOffset, hero.spellBookSize, data, &hero, 2 );
    hero.spells = spellIds;
    hero.spellBookSize = data.size();
}

void SaveFile::setArtifact( HeroRecord & hero, int slot, int id )
{
    if ( !hero.heroBaseParsed )
        throw SaveError( "Artifacts unavailable: hero record not recognized" );
    if ( id < 0 || id > 82 )
        throw SaveError( "Invalid artifact" );
    if ( slot < 0 || slot > 13 )
        throw SaveError( "Invalid artifact slot" );

    // In-place replacement; id == 0 removes the artifact from the bag (rebuild
    // without "holes": empty cells between artifacts are removed too).
    if ( slot < hero.artifactCount ) {
        if ( id != 0 ) {
            size_t off = hero.artifactsOffset + static_cast<size_t>( slot ) * 8;
            putBe32( _raw, off, static_cast<uint32_t>( id ) );
            putBe32( _raw, off + 4, 0 );
            hero.artifacts[slot] = { id, 0 };
            _dirty = true;
            return;
        }

        const size_t bagStart = hero.artifactsOffset - 4;
        const size_t oldLen = 4 + static_cast<size_t>( hero.artifactCount ) * 8;
        std::vector<uint8_t> data;
        int remaining = 0;
        for ( int i = 0; i < hero.artifactCount; ++i ) {
            if ( i == slot || hero.artifacts[i].id == 0 )
                continue;
            writeBe32( data, static_cast<uint32_t>( hero.artifacts[i].id ) );
            writeBe32( data, 0 );
            ++remaining;
        }
        std::vector<uint8_t> rebuilt;
        writeBe32( rebuilt, static_cast<uint32_t>( remaining ) );
        rebuilt.insert( rebuilt.end(), data.begin(), data.end() );
        resizeRegion( bagStart, oldLen, rebuilt, &hero, 1 );
        int out = 0;
        for ( int i = 0; i < hero.artifactCount; ++i ) {
            if ( i == slot || hero.artifacts[i].id == 0 )
                continue;
            hero.artifacts[out++] = hero.artifacts[i];
        }
        hero.artifactCount = out;
        for ( int i = out; i < 14; ++i )
            hero.artifacts[i] = ArtifactSlot{};
        return;
    }

    // Append to the end of the bag (in the stream the slots are consecutive, count < 14).
    if ( id == 0 )
        return; // an empty slot beyond the bag is already empty
    if ( slot != hero.artifactCount )
        throw SaveError( "Cannot append an artifact out of order" );
    if ( hero.artifactCount >= 14 )
        throw SaveError( "Artifact bag is full" );

    // New bag block: count+1 + (id,ext)×count + (id,ext).
    const size_t bagStart = hero.artifactsOffset - 4;
    const size_t oldLen = 4 + static_cast<size_t>( hero.artifactCount ) * 8;
    std::vector<uint8_t> data;
    writeBe32( data, static_cast<uint32_t>( hero.artifactCount + 1 ) );
    for ( int i = 0; i < hero.artifactCount; ++i ) {
        writeBe32( data, static_cast<uint32_t>( hero.artifacts[i].id ) );
        writeBe32( data, 0 );
    }
    writeBe32( data, static_cast<uint32_t>( id ) );
    writeBe32( data, 0 );

    resizeRegion( bagStart, oldLen, data, &hero, 1 );
    hero.artifacts[hero.artifactCount] = { id, 0 };
    ++hero.artifactCount;
}

void SaveFile::moveArtifact( HeroRecord & hero, int fromSlot, int toSlot )
{
    if ( !hero.heroBaseParsed )
        throw SaveError( "Artifacts unavailable: hero record not recognized" );
    if ( fromSlot < 0 || fromSlot >= hero.artifactCount || hero.artifacts[fromSlot].id == 0 )
        throw SaveError( "Invalid source artifact slot" );
    if ( toSlot < 0 || toSlot > hero.artifactCount || toSlot > 13 )
        throw SaveError( "Invalid destination slot" );
    if ( toSlot == fromSlot )
        return;
    if ( toSlot < hero.artifactCount && hero.artifacts[toSlot].id != 0 )
        throw SaveError( "Destination slot is occupied" );

    const int id = hero.artifacts[fromSlot].id;

    if ( toSlot < hero.artifactCount ) {
        // Empty cell inside the bag: values are changed at the same offsets.
        const size_t offFrom = hero.artifactsOffset + static_cast<size_t>( fromSlot ) * 8;
        const size_t offTo = hero.artifactsOffset + static_cast<size_t>( toSlot ) * 8;
        putBe32( _raw, offFrom, 0 );
        putBe32( _raw, offFrom + 4, 0 );
        putBe32( _raw, offTo, static_cast<uint32_t>( id ) );
        putBe32( _raw, offTo + 4, 0 );
        hero.artifacts[fromSlot] = ArtifactSlot{};
        hero.artifacts[toSlot] = { id, 0 };
        _dirty = true;
        return;
    }

    // The slot right after the bag: a hole remains at the old position, and
    // the artifact is appended to the end of the block (count+1).
    if ( hero.artifactCount >= 14 )
        throw SaveError( "Artifact bag is full" );
    const size_t bagStart = hero.artifactsOffset - 4;
    const size_t oldLen = 4 + static_cast<size_t>( hero.artifactCount ) * 8;
    std::vector<uint8_t> data;
    writeBe32( data, static_cast<uint32_t>( hero.artifactCount + 1 ) );
    for ( int i = 0; i < hero.artifactCount; ++i ) {
        if ( i == fromSlot ) {
            writeBe32( data, 0 );
            writeBe32( data, 0 );
            continue;
        }
        writeBe32( data, static_cast<uint32_t>( hero.artifacts[i].id ) );
        writeBe32( data, 0 );
    }
    writeBe32( data, static_cast<uint32_t>( id ) );
    writeBe32( data, 0 );

    resizeRegion( bagStart, oldLen, data, &hero, 1 );
    hero.artifacts[fromSlot] = ArtifactSlot{};
    hero.artifacts[hero.artifactCount] = { id, 0 };
    ++hero.artifactCount;
}

void SaveFile::swapArtifacts( HeroRecord & hero, int slotA, int slotB )
{
    if ( !hero.heroBaseParsed )
        throw SaveError( "Artifacts unavailable: hero record not recognized" );
    if ( slotA < 0 || slotA >= hero.artifactCount || slotB < 0 || slotB >= hero.artifactCount )
        throw SaveError( "Invalid artifact slot" );

    const ArtifactSlot a = hero.artifacts[slotA];
    const ArtifactSlot b = hero.artifacts[slotB];
    const size_t offA = hero.artifactsOffset + static_cast<size_t>( slotA ) * 8;
    const size_t offB = hero.artifactsOffset + static_cast<size_t>( slotB ) * 8;
    putBe32( _raw, offA, static_cast<uint32_t>( b.id ) );
    putBe32( _raw, offA + 4, 0 );
    putBe32( _raw, offB, static_cast<uint32_t>( a.id ) );
    putBe32( _raw, offB + 4, 0 );
    hero.artifacts[slotA] = b;
    hero.artifacts[slotB] = a;
    _dirty = true;
}

void SaveFile::setSecondarySkill( HeroRecord & hero, int skillIndex, int skillId, int level )
{
    if ( skillId < 1 || skillId > 14 || level < 1 || level > 3 )
        throw SaveError( "Invalid secondary skill" );
    if ( skillIndex < 0 || skillIndex > static_cast<int>( hero.skills.size() ) || skillIndex >= 8 )
        throw SaveError( "Invalid skill index" );

    // Replace an existing skill.
    if ( skillIndex < static_cast<int>( hero.skills.size() ) ) {
        size_t off = hero.nameOffset + 4 + hero.name.size() + 1 + 4 + 4 + static_cast<size_t>( skillIndex ) * 8;
        putBe32( _raw, off, static_cast<uint32_t>( skillId ) );
        putBe32( _raw, off + 4, static_cast<uint32_t>( level ) );
        hero.skills[skillIndex] = { skillId, level };
        _dirty = true;
        return;
    }

    // Append a new skill to the end of the list (count < 8).
    if ( hero.skills.size() >= 8 )
        throw SaveError( "The hero already has 8 secondary skills" );
    for ( const auto & s : hero.skills ) {
        if ( s.first == skillId )
            throw SaveError( "The hero already has this skill" );
    }

    // Skills block: nameOffset + 4 (len) + name + 1 (color) + 4 (experience) →
    // u32 skillCount + (skillId, level) pairs.
    const size_t skillsStart = hero.nameOffset + 4 + hero.name.size() + 1 + 4;
    const size_t oldLen = 4 + hero.skills.size() * 8;
    std::vector<uint8_t> data;
    writeBe32( data, static_cast<uint32_t>( hero.skills.size() + 1 ) );
    for ( const auto & s : hero.skills ) {
        writeBe32( data, static_cast<uint32_t>( s.first ) );
        writeBe32( data, static_cast<uint32_t>( s.second ) );
    }
    writeBe32( data, static_cast<uint32_t>( skillId ) );
    writeBe32( data, static_cast<uint32_t>( level ) );

    resizeRegion( skillsStart, oldLen, data, &hero );
    hero.skills.emplace_back( skillId, level );
}

void SaveFile::removeSecondarySkill( HeroRecord & hero, int skillIndex )
{
    if ( skillIndex < 0 || skillIndex >= static_cast<int>( hero.skills.size() ) )
        throw SaveError( "Invalid skill index" );

    // Rebuild the skills block: count−1 + pairs without the removed slot.
    // Block: nameOffset + 4 (len) + name + 1 (color) + 4 (experience) →
    // u32 skillCount + (skillId, level) pairs.
    const size_t skillsStart = hero.nameOffset + 4 + hero.name.size() + 1 + 4;
    const size_t oldLen = 4 + hero.skills.size() * 8;
    std::vector<uint8_t> data;
    writeBe32( data, static_cast<uint32_t>( hero.skills.size() - 1 ) );
    for ( int i = 0; i < static_cast<int>( hero.skills.size() ); ++i ) {
        if ( i == skillIndex )
            continue;
        writeBe32( data, static_cast<uint32_t>( hero.skills[i].first ) );
        writeBe32( data, static_cast<uint32_t>( hero.skills[i].second ) );
    }

    resizeRegion( skillsStart, oldLen, data, &hero );
    hero.skills.erase( hero.skills.begin() + skillIndex );
}

void SaveFile::setRace( HeroRecord & hero, int race )
{
    if ( race < 1 || race > 0xFF )
        throw SaveError( "Invalid race" );
    putBe32( _raw, hero.raceOffset, static_cast<uint32_t>( race ) );
    hero.race = race;
    _dirty = true;
}

void SaveFile::setPortrait( HeroRecord & hero, int portrait )
{
    if ( portrait < 0 || portrait >= HEROES_COUNT )
        throw SaveError( "Invalid portrait" );
    putBe32( _raw, hero.raceOffset - 4, static_cast<uint32_t>( portrait ) );
    hero.portrait = portrait;
    _dirty = true;
}

void SaveFile::setName( HeroRecord & hero, const std::string & cp1251Name )
{
    if ( cp1251Name.size() != hero.name.size() )
        throw SaveError( "The name can only be changed to the same length (otherwise the whole stream shifts)" );
    std::copy( cp1251Name.begin(), cp1251Name.end(), _raw.begin() + hero.nameOffset + 4 );
    hero.name = cp1251Name;
    _dirty = true;
}

void SaveFile::save( const std::string & outPath ) const
{
    const std::vector<uint8_t> out = saveToBytes();

    std::ofstream f( outPath, std::ios::binary | std::ios::trunc );
    if ( !f )
        throw SaveError( "Could not write file: " + outPath );
    f.write( reinterpret_cast<const char *>( out.data() ), static_cast<std::streamsize>( out.size() ) );
}

std::vector<uint8_t> SaveFile::saveToBytes() const
{
    uLongf zipBound = compressBound( static_cast<uLong>( _raw.size() ) );
    std::vector<uint8_t> zipped( zipBound );
    uLongf zippedLen = zipBound;
    if ( compress( zipped.data(), &zippedLen, _raw.data(), static_cast<uLong>( _raw.size() ) ) != Z_OK )
        throw SaveError( "Data compression error" );
    zipped.resize( zippedLen );

    std::vector<uint8_t> out;
    out.reserve( _zpos + 12 + zipped.size() );
    out.insert( out.end(), _data.begin(), _data.begin() + _zpos );
    writeBe32( out, static_cast<uint32_t>( _raw.size() ) );
    writeBe32( out, static_cast<uint32_t>( zipped.size() ) );
    writeBe16( out, 0 );
    writeBe16( out, 0 );
    out.insert( out.end(), zipped.begin(), zipped.end() );
    return out;
}

} // namespace fh2
