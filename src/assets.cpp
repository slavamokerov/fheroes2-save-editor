#include "assets.h"

#include <cmath>

#include <QFileInfo>
#include <QStandardPaths>

#include "constants.h"

namespace fh2 {

namespace {

Qt::TransformationMode scaleMode( double scale )
{
    // Integer factor — nearest-neighbor (sharp pixel art),
    // fractional — smooth (otherwise edges look torn).
    return std::abs( scale - std::round( scale ) ) < 0.01 ? Qt::FastTransformation : Qt::SmoothTransformation;
}

} // namespace

Assets Assets::load( const std::string & dataDir )
{
    Assets assets;
    const QString dir = QString::fromStdString( dataDir );

    assets._agg = std::make_unique<AggContainer>();
    assets._aggX = std::make_unique<AggContainer>();
    if ( assets._agg->open( ( dir + "/HEROES2.AGG" ).toStdString() ) ) {
        assets._valid = true;
        assets._aggX->open( ( dir + "/HEROES2X.AGG" ).toStdString() );

        // KB.PAL palette: 768 bytes, 6 bits per channel → 8-bit (<< 2), as in
        // the engine's setGamePalette + getNormalizedRGBGamePalette. Plus copies
        // of cyclic colors: water 231..233,235 → 246..249; lava 214..217 → 250..253.
        std::vector<uint8_t> kbPal = assets._agg->read( "KB.PAL" );
        if ( kbPal.empty() )
            kbPal = assets._aggX->read( "KB.PAL" );
        if ( kbPal.size() == 768 ) {
            for ( size_t i = 0; i < 256; ++i ) {
                const uint8_t r = static_cast<uint8_t>( kbPal[i * 3] << 2 );
                const uint8_t g = static_cast<uint8_t>( kbPal[i * 3 + 1] << 2 );
                const uint8_t b = static_cast<uint8_t>( kbPal[i * 3 + 2] << 2 );
                assets._palette[i] = qRgb( r, g, b );
            }
            for ( int i = 0; i < 3; ++i )
                assets._palette[246 + i] = assets._palette[231 + i];
            assets._palette[249] = assets._palette[235];
            for ( int i = 0; i < 4; ++i )
                assets._palette[250 + i] = assets._palette[214 + i];
        }
    }
    return assets;
}

std::string Assets::defaultDataDir()
{
    // Standard fheroes2 paths (macOS/Linux/Windows)
    QString home = QStandardPaths::writableLocation( QStandardPaths::HomeLocation );
    QStringList candidates;
    candidates << home + "/Library/Application Support/fheroes2/data";
    candidates << home + "/.local/share/fheroes2/data";
    candidates << home + "/.fheroes2/files/data";
    candidates << home + "/AppData/Local/fheroes2/data";
    candidates << home + "/AppData/Roaming/fheroes2/data";
    for ( const QString & c : candidates ) {
        if ( QFileInfo::exists( c + "/HEROES2.AGG" ) )
            return c.toStdString();
    }
    return {};
}

QColor Assets::paletteColor( int index ) const
{
    if ( index < 0 || index >= 256 )
        return QColor( 0, 0, 0 );
    return QColor( _palette[index] );
}

const IcnSprite & Assets::icnSprite( const std::string & name, int index ) const
{
    const std::string key = name + "#" + std::to_string( index );
    auto it = _cache.find( key );
    if ( it != _cache.end() )
        return it->second;
    IcnSprite sprite = loadIcn( name, index );
    _cache[key] = sprite;
    return _cache.at( key );
}

IcnSprite Assets::loadIcn( const std::string & name, int index ) const
{
    std::vector<uint8_t> body = _agg->read( name );
    if ( index >= 0 && body.size() >= 2 ) {
        // Some files in HEROES2X.AGG carry more frames than their HEROES2.AGG
        // counterparts (MINIPORT.ICN: 60 vs 71, PORTMEDI.ICN: 61 vs 72 — the
        // PoL hero portraits). Take the X version when the AGG copy is too short.
        const size_t frameCount = static_cast<uint16_t>( body[0] ) | ( static_cast<uint16_t>( body[1] ) << 8 );
        if ( static_cast<size_t>( index ) >= frameCount )
            body.clear();
    }
    if ( body.empty() && _aggX )
        body = _aggX->read( name );
    if ( body.size() < 6 )
        return {};
    return decodeIcnSprite( body, index, _palette );
}

QPixmap Assets::icnPixmap( const std::string & name, int index, double scale ) const
{
    const IcnSprite & sprite = icnSprite( name, index );
    QPixmap pm = QPixmap::fromImage( sprite.image );
    if ( scale != 1.0 && !pm.isNull() )
        pm = pm.scaled( std::max( 1, qRound( pm.width() * scale ) ), std::max( 1, qRound( pm.height() * scale ) ),
                        Qt::IgnoreAspectRatio, scaleMode( scale ) );
    return pm;
}

int Assets::icnFrameCount( const std::string & name ) const
{
    std::vector<uint8_t> body = _agg ? _agg->read( name ) : std::vector<uint8_t>();
    if ( body.size() >= 2 && _aggX ) {
        // Prefer the HEROES2X.AGG copy when it carries more frames (PoL files).
        const std::vector<uint8_t> xbody = _aggX->read( name );
        if ( xbody.size() >= 2 ) {
            const int aggCount = static_cast<uint16_t>( body[0] ) | ( static_cast<uint16_t>( body[1] ) << 8 );
            const int xCount = static_cast<uint16_t>( xbody[0] ) | ( static_cast<uint16_t>( xbody[1] ) << 8 );
            if ( xCount > aggCount )
                body = xbody;
        }
    }
    if ( body.size() < 2 )
        return 0;
    return static_cast<int>( static_cast<uint16_t>( body[0] ) | ( static_cast<uint16_t>( body[1] ) << 8 ) );
}

const IcnSprite & Assets::tilSprite( const std::string & name, int index ) const
{
    const std::string key = name + "#" + std::to_string( index );
    auto it = _cache.find( key );
    if ( it != _cache.end() )
        return it->second;
    _cache[key] = loadTil( name, index );
    return _cache.at( key );
}

IcnSprite Assets::loadTil( const std::string & name, int index ) const
{
    std::vector<uint8_t> body = _agg ? _agg->read( name ) : std::vector<uint8_t>();
    if ( body.empty() && _aggX )
        body = _aggX->read( name );
    if ( body.size() < 6 )
        return {};
    return decodeTilSprite( body, index, _palette );
}

QPixmap Assets::tilPixmap( const std::string & name, int index ) const
{
    return QPixmap::fromImage( tilSprite( name, index ).image );
}

const QImage & Assets::monsterImage( int monsterId ) const
{
    if ( monsterId < 1 || monsterId > 66 )
        return _emptySprite.image;
    char buf[32];
    snprintf( buf, sizeof( buf ), "MONH%04d.ICN", monsterId - 1 );
    return icnSprite( buf ).image;
}

const QImage & Assets::portraitImage( int heroId ) const
{
    if ( heroId < 1 || heroId > 71 )
        return _emptySprite.image;
    char buf[32];
    snprintf( buf, sizeof( buf ), "PORT%04d.ICN", heroId - 1 );
    return icnSprite( buf ).image;
}

QPixmap Assets::monsterPixmap( int monsterId, double scale ) const
{
    QPixmap pm = QPixmap::fromImage( monsterImage( monsterId ) );
    if ( scale != 1.0 && !pm.isNull() )
        pm = pm.scaled( std::max( 1, qRound( pm.width() * scale ) ), std::max( 1, qRound( pm.height() * scale ) ),
                        Qt::IgnoreAspectRatio, scaleMode( scale ) );
    return pm;
}

QPixmap Assets::portraitPixmap( int heroId, double scale ) const
{
    QPixmap pm = QPixmap::fromImage( portraitImage( heroId ) );
    if ( scale != 1.0 && !pm.isNull() )
        pm = pm.scaled( std::max( 1, qRound( pm.width() * scale ) ), std::max( 1, qRound( pm.height() * scale ) ),
                        Qt::IgnoreAspectRatio, scaleMode( scale ) );
    return pm;
}

} // namespace fh2
