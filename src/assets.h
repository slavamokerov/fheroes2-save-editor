#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <memory>

#include <QImage>
#include <QPixmap>

#include "aggicn.h"

namespace fh2 {

struct AssetsError : std::runtime_error {
    explicit AssetsError( const std::string & msg )
        : std::runtime_error( msg )
    {}
};

// Game resource loading: AGG/ICN/KB.PAL are read by our own loader
// (aggicn.h) — the project does not depend on fheroes2 code. Formats are
// described in FH2_SAVE_FORMAT.md, § "Resources".
class Assets
{
public:
    // dataDir — folder with HEROES2.AGG (+HEROES2X.AGG)
    static Assets load( const std::string & dataDir );

    QPixmap monsterPixmap( int monsterId, double scale = 1.0 ) const;
    QPixmap portraitPixmap( int heroId, double scale = 1.0 ) const;
    const QImage & monsterImage( int monsterId ) const;
    const QImage & portraitImage( int heroId ) const;

    // Arbitrary sprite from AGG: "HEROBKG.ICN", "HSBTNS.ICN", ...
    const IcnSprite & icnSprite( const std::string & name, int index = 0 ) const;
    QPixmap icnPixmap( const std::string & name, int index = 0, double scale = 1.0 ) const;

    // Number of frames in an ICN file (for monster animations); 0 — file missing.
    int icnFrameCount( const std::string & name ) const;

    // Tile from TIL: "CLOF32.TIL" (fog), "GROUND32.TIL", "STON.TIL".
    const IcnSprite & tilSprite( const std::string & name, int index = 0 ) const;
    QPixmap tilPixmap( const std::string & name, int index = 0 ) const;

    bool valid() const { return _valid; }

    // Palette color by index (KB.PAL, 6-bit × 4), for repainting.
    QColor paletteColor( int index ) const;

    static std::string defaultDataDir();

private:
    bool _valid = false;
    std::unique_ptr<AggContainer> _agg;   // HEROES2.AGG
    std::unique_ptr<AggContainer> _aggX;  // HEROES2X.AGG (expansion)
    std::array<QRgb, 256> _palette{};     // normalized (8-bit) KB.PAL palette
    mutable std::map<std::string, IcnSprite> _cache;
    mutable IcnSprite _emptySprite;

    IcnSprite loadIcn( const std::string & name, int index ) const;
    IcnSprite loadTil( const std::string & name, int index ) const;
};

} // namespace fh2
