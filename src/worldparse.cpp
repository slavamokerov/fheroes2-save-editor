#include "worldparse.h"


namespace fh2 {

namespace {

// Bounds-checked big-endian reader over the decompressed stream.
class Reader
{
public:
    Reader( const std::vector<uint8_t> & data )
        : _d( data )
    {}

    size_t pos() const
    {
        return _p;
    }

    void need( size_t n ) const
    {
        if ( _p + n > _d.size() )
            throw WorldParseError( "unexpected end of the stream at " + std::to_string( _p ) );
    }

    uint8_t u8()
    {
        need( 1 );
        return _d[_p++];
    }

    uint16_t u16()
    {
        need( 2 );
        const uint16_t v = static_cast<uint16_t>( ( _d[_p] << 8 ) | _d[_p + 1] );
        _p += 2;
        return v;
    }

    int16_t i16()
    {
        return static_cast<int16_t>( u16() );
    }

    uint32_t u32()
    {
        need( 4 );
        const uint32_t v = ( static_cast<uint32_t>( _d[_p] ) << 24 ) | ( static_cast<uint32_t>( _d[_p + 1] ) << 16 ) | ( static_cast<uint32_t>( _d[_p + 2] ) << 8 ) | _d[_p + 3];
        _p += 4;
        return v;
    }

    int32_t i32()
    {
        return static_cast<int32_t>( u32() );
    }

    std::string str( size_t maxLen = 1024 )
    {
        const uint32_t len = u32();
        if ( len > maxLen )
            throw WorldParseError( "string is too long (" + std::to_string( len ) + ") at " + std::to_string( _p ) );
        need( len );
        std::string s( reinterpret_cast<const char *>( _d.data() + _p ), len );
        _p += len;
        return s;
    }

    void skip( size_t n )
    {
        need( n );
        _p += n;
    }

private:
    const std::vector<uint8_t> & _d;
    size_t _p = 0;
};

ObjectPart readObjectPart( Reader & r )
{
    ObjectPart p;
    p.layerType = r.u8();
    p.uid = r.u32();
    p.icnType = r.u8();
    p.icnIndex = r.u8();
    return p;
}

void readHeroBase( Reader & r, int32_t * primary, uint32_t & spellPoints, uint32_t & movePoints, std::vector<int32_t> & spells, int16_t * centerX = nullptr,
                   int16_t * centerY = nullptr, std::vector<std::pair<int32_t, int32_t>> * artifacts = nullptr )
{
    primary[0] = r.i32(); // attack
    primary[1] = r.i32(); // defense
    primary[2] = r.i32(); // knowledge
    primary[3] = r.i32(); // power
    const int16_t cx = r.i16();
    const int16_t cy = r.i16();
    if ( centerX )
        *centerX = cx;
    if ( centerY )
        *centerY = cy;
    r.skip( 4 );          // modes u32
    spellPoints = r.u32();
    movePoints = r.u32();
    const uint32_t spellCount = r.u32();
    if ( spellCount > 100 )
        throw WorldParseError( "implausible spell book size " + std::to_string( spellCount ) );
    spells.reserve( spellCount );
    for ( uint32_t i = 0; i < spellCount; ++i )
        spells.push_back( r.i32() );
    const uint32_t artifactCount = r.u32();
    if ( artifactCount > 14 )
        throw WorldParseError( "implausible artifact bag size " + std::to_string( artifactCount ) );
    if ( artifacts ) {
        artifacts->reserve( artifactCount );
        for ( uint32_t i = 0; i < artifactCount; ++i ) {
            const int32_t id = r.i32();
            const int32_t ext = r.i32();
            artifacts->emplace_back( id, ext );
        }
    }
    else {
        for ( uint32_t i = 0; i < artifactCount; ++i ) {
            r.skip( 4 ); // id
            r.skip( 4 ); // ext
        }
    }
}

// Army: u32 size (=5) + 5×(i32 monsterId, u32 count) + u8 spread + u8 color.
void readArmy( Reader & r, int32_t * monsterId, uint32_t * count )
{
    const uint32_t size = r.u32();
    if ( size > 100 )
        throw WorldParseError( "implausible army size " + std::to_string( size ) );
    for ( uint32_t i = 0; i < size; ++i ) {
        const int32_t id = r.i32();
        const uint32_t cnt = r.u32();
        if ( i < 5 ) {
            monsterId[i] = id;
            count[i] = cnt;
        }
    }
    r.skip( 1 ); // spread
    r.skip( 1 ); // color
}

} // namespace

WorldData parseWorld( const std::vector<uint8_t> & raw, int formatVersion )
{
    Reader r( raw );
    WorldData w;

    w.width = static_cast<int>( r.u32() );
    w.height = static_cast<int>( r.u32() );
    if ( w.width < 1 || w.width > 1000 || w.height < 1 || w.height > 1000 )
        throw WorldParseError( "implausible map size " + std::to_string( w.width ) + "x" + std::to_string( w.height ) );

    // vec_tiles
    const uint32_t tileCount = r.u32();
    if ( tileCount != static_cast<uint32_t>( w.width ) * w.height )
        throw WorldParseError( "tile count " + std::to_string( tileCount ) + " != " + std::to_string( w.width * w.height ) );
    w.tiles.reserve( tileCount );
    for ( uint32_t i = 0; i < tileCount; ++i ) {
        WorldTile t;
        t.index = r.i32();
        t.terrainImageIndex = r.u16();
        t.terrainFlags = r.u8();
        t.passability = r.u16();
        t.mainPart = readObjectPart( r );
        t.mainObjectType = r.u16();
        t.fogColors = r.u8();
        const uint32_t metaCount = r.u32();
        if ( metaCount > 8 )
            throw WorldParseError( "implausible tile metadata size " + std::to_string( metaCount ) );
        for ( uint32_t m = 0; m < metaCount; ++m ) {
            const uint32_t v = r.u32();
            if ( m < 3 )
                t.metadata[m] = v;
        }
        t.occupantHeroId = r.u8();
        t.markedAsRoad = r.u8();
        uint32_t partCount = r.u32();
        if ( partCount > 64 )
            throw WorldParseError( "implausible ground part count " + std::to_string( partCount ) );
        t.groundParts.reserve( partCount );
        for ( uint32_t p = 0; p < partCount; ++p )
            t.groundParts.push_back( readObjectPart( r ) );
        partCount = r.u32();
        if ( partCount > 64 )
            throw WorldParseError( "implausible top part count " + std::to_string( partCount ) );
        t.topParts.reserve( partCount );
        for ( uint32_t p = 0; p < partCount; ++p )
            t.topParts.push_back( readObjectPart( r ) );
        t.boatOwnerColor = r.u8();
        w.tiles.push_back( t );
    }

    // AllHeroes
    const uint32_t heroCount = r.u32();
    if ( heroCount > 100 )
        throw WorldParseError( "implausible hero count " + std::to_string( heroCount ) );
    w.heroes.reserve( heroCount );
    for ( uint32_t i = 0; i < heroCount; ++i ) {
        WorldHero h;
        readHeroBase( r, h.primary, h.spellPoints, h.movePoints, h.spells, &h.centerX, &h.centerY, &h.artifacts );
        h.name = r.str( 64 );
        h.color = r.u8();
        h.experience = r.i32();
        const uint32_t skillCount = r.u32();
        if ( skillCount > 8 )
            throw WorldParseError( "implausible secondary skill count " + std::to_string( skillCount ) );
        r.skip( skillCount * 8 );
        readArmy( r, h.monsterId, h.monsterCount );
        h.id = r.i32();
        h.portrait = r.i32();
        h.race = r.u32();
        r.skip( 2 ); // _objectTypeUnderHero
        h.route.hide = ( r.u8() != 0 );
        const uint32_t routeCount = r.u32();
        if ( routeCount > 100000 )
            throw WorldParseError( "implausible route length " + std::to_string( routeCount ) );
        h.route.steps.reserve( routeCount );
        for ( uint32_t s = 0; s < routeCount; ++s ) {
            RouteStep st;
            st.from = r.i32();
            st.direction = r.i32();
            st.penalty = r.u32();
            h.route.steps.push_back( st );
        }
        h.direction = r.i32();
        h.spriteIndex = r.i32();
        r.skip( 8 );  // _patrolCenter
        r.skip( 4 );  // _patrolDistance
        const uint32_t visitedCount = r.u32();
        if ( visitedCount > 100000 )
            throw WorldParseError( "implausible visited objects count " + std::to_string( visitedCount ) );
        r.skip( visitedCount * 6 ); // i32 index + u16 objectType
        r.skip( 4 );                // _lastGroundRegion
        w.heroes.push_back( std::move( h ) );
    }

    // vec_castles
    const uint32_t castleCount = r.u32();
    if ( castleCount > 200 )
        throw WorldParseError( "implausible castle count " + std::to_string( castleCount ) );
    w.castles.reserve( castleCount );
    for ( uint32_t i = 0; i < castleCount; ++i ) {
        WorldCastle c;
        c.x = r.i16();
        c.y = r.i16();
        c.modes = r.u32();
        c.race = r.u32();
        c.constructedBuildings = r.u32();
        c.disabledBuildings = r.u32();
        uint32_t captainSp = 0, captainMp = 0;
        readHeroBase( r, c.captainPrimary, captainSp, captainMp, c.captainSpells );
        c.captainSpellPoints = captainSp;
        c.captainMovePoints = captainMp;
        c.color = r.u8();
        c.name = r.str( 64 );
        // mageGuild: general + library
        const uint32_t gCount = r.u32();
        if ( gCount > 100 )
            throw WorldParseError( "implausible mage guild size" );
        c.mageGuildGeneral.reserve( gCount );
        for ( uint32_t s = 0; s < gCount; ++s )
            c.mageGuildGeneral.push_back( r.i32() );
        const uint32_t lCount = r.u32();
        if ( lCount > 100 )
            throw WorldParseError( "implausible library size" );
        c.mageGuildLibrary.reserve( lCount );
        for ( uint32_t s = 0; s < lCount; ++s )
            c.mageGuildLibrary.push_back( r.i32() );
        const uint32_t dwellingCount = r.u32();
        if ( dwellingCount > 32 )
            throw WorldParseError( "implausible dwelling size" );
        c.dwelling.reserve( dwellingCount );
        for ( uint32_t d = 0; d < dwellingCount; ++d )
            c.dwelling.push_back( r.u32() );
        readArmy( r, c.garrisonMonsterId, c.garrisonCount );
        w.castles.push_back( std::move( c ) );
    }

    // vec_kingdoms
    const uint32_t kingdomCount = r.u32();
    if ( kingdomCount > 16 )
        throw WorldParseError( "implausible kingdom count " + std::to_string( kingdomCount ) );
    w.kingdoms.reserve( kingdomCount );
    for ( uint32_t i = 0; i < kingdomCount; ++i ) {
        WorldKingdom k;
        k.modes = r.u32();
        k.color = r.u8();
        k.resourcesOffset = r.pos();
        for ( int res = 0; res < 7; ++res )
            k.resources[res] = r.u32();
        k.lostTownDays = r.i32();
        const uint32_t kCastleCount = r.u32();
        if ( kCastleCount > 200 )
            throw WorldParseError( "implausible kingdom castle list" );
        k.castleIds.reserve( kCastleCount );
        for ( uint32_t c = 0; c < kCastleCount; ++c )
            k.castleIds.push_back( r.i32() );
        const uint32_t kHeroCount = r.u32();
        if ( kHeroCount > 100 )
            throw WorldParseError( "implausible kingdom hero list" );
        k.heroIds.reserve( kHeroCount );
        for ( uint32_t h = 0; h < kHeroCount; ++h )
            k.heroIds.push_back( r.i32() );
        // recruits: pair<Recruit, Recruit>
        k.recruitIds[0] = r.i32();
        k.recruitDays[0] = r.u32();
        k.recruitIds[1] = r.i32();
        k.recruitDays[1] = r.u32();
        // visit_object
        const uint32_t visitCount = r.u32();
        if ( visitCount > 100000 )
            throw WorldParseError( "implausible visited objects count" );
        k.visited.reserve( visitCount );
        for ( uint32_t v = 0; v < visitCount; ++v ) {
            const int32_t idx = r.i32();
            const uint16_t objType = r.u16();
            k.visited.emplace_back( idx, objType );
        }
        // puzzle
        k.puzzleBits = r.str( 64 );
        for ( int z = 0; z < 4; ++z ) {
            const uint8_t size = r.u8();
            k.puzzleZones[z].reserve( size );
            for ( uint8_t b = 0; b < size; ++b )
                k.puzzleZones[z].push_back( r.u8() );
        }
        k.visitedTents = r.i32();
        k.topCastle = r.i32();
        k.topHero = r.i32();
        if ( formatVersion >= 10034 ) {
            const uint32_t uVisionCount = r.u32();
            if ( uVisionCount > 100000 )
                throw WorldParseError( "implausible monstersUnderVision size" );
            k.monstersUnderVision.reserve( uVisionCount );
            for ( uint32_t m = 0; m < uVisionCount; ++m )
                k.monstersUnderVision.push_back( r.i32() );
        }
        w.kingdoms.push_back( std::move( k ) );
    }

    // _customRumors
    const uint32_t rumorCount = r.u32();
    if ( rumorCount > 100 )
        throw WorldParseError( "implausible rumor count " + std::to_string( rumorCount ) );
    w.rumors.reserve( rumorCount );
    for ( uint32_t i = 0; i < rumorCount; ++i )
        w.rumors.push_back( r.str( 4096 ) );

    // vec_eventsday
    const uint32_t eventCount = r.u32();
    if ( eventCount > 10000 )
        throw WorldParseError( "implausible event count " + std::to_string( eventCount ) );
    w.events.reserve( eventCount );
    for ( uint32_t i = 0; i < eventCount; ++i ) {
        WorldEvent e;
        for ( int res = 0; res < 7; ++res )
            e.resources[res] = r.u32();
        e.forAI = r.u8();
        e.day = r.u32();
        e.period = r.u32();
        e.colors = r.u8();
        e.message = r.str( 4096 );
        e.title = r.str( 256 );
        w.events.push_back( std::move( e ) );
    }

    // map_captureobj: map<i32, CapturedObject>
    w.offsetAfterEvents = r.pos();
    const uint32_t capturedCount = r.u32();
    w.capturedCount = capturedCount;
    if ( capturedCount > 100000 )
        throw WorldParseError( "implausible captured objects count " + std::to_string( capturedCount ) );
    for ( uint32_t i = 0; i < capturedCount; ++i ) {
        const int32_t idx = r.i32();
        if ( idx < 0 || idx >= w.width * w.height )
            throw WorldParseError( "captured object index out of range at " + std::to_string( r.pos() ) );
        r.skip( 2 ); // ObjectColor: object type u16
        r.skip( 1 ); // ObjectColor: color u8
        r.skip( 8 ); // Troop: i32 + u32
    }

    // _ultimateArtifact: UltimateArtifact = Artifact (i32 id + i32 ext)
    // + i32 tile index + bool isFound + Point offset (i32 x, i32 y).
    w.ultimateArtifactId = r.i32();
    r.skip( 4 ); // ext
    w.ultimateArtifactIndex = r.i32();
    r.skip( 1 ); // isFound
    r.skip( 8 ); // offset
    w.dayOffset = r.pos();
    w.day = r.u32();
    w.weekOffset = r.pos();
    w.week = r.u32();
    w.monthOffset = r.pos();
    w.month = r.u32();
    w.winHeroId = r.i32();
    w.lossHeroId = r.i32();
    w.offsetAfterCaptureObj = r.pos();

    return w;
}

} // namespace fh2
