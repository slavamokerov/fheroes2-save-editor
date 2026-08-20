#include "gamefont.h"

#include <algorithm>

#include "assets.h"
#include "constants.h"
#include "textutil.h"

namespace fh2 {

namespace {

// yellowTextTable — port from fheroes2 src/engine/pal.cpp (color index replacement).
const uint8_t yellowTextTable[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 114, 115, 115, 116, 117, 117,
    118, 119, 119, 120, 121, 121, 122, 123, 123, 124, 125, 125, 126, 127, 127, 128,
    129, 129, 130, 130, 130, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// grayTextTable — port from fheroes2 src/engine/pal.cpp (color index replacement).
const uint8_t grayTextTable[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
    36, 36, 36, 36, 36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// --- glyph operations (analogs of fheroes2 Sprite operations) ---

using Glyph = GameFont::Glyph;

void gResize( Glyph & g, int w, int h )
{
    g.img = QImage( w, h, QImage::Format_RGBA8888 );
    g.img.fill( Qt::transparent );
    g.idx = QImage( w, h, QImage::Format_Indexed8 );
    for ( int i = 0; i < 256; ++i )
        g.idx.setColor( i, QColor( i, i, i ).rgb() );
    g.idx.fill( 0xFF );
    g.empty = false;
}

// Copy(src, sx, sy, dst, dx, dy, w, h) — copies a rectangle (with shadows).
void gCopy( const Glyph & src, int sx, int sy, Glyph & dst, int dx, int dy, int w, int h )
{
    // Clipping to boundaries (like Copy in fheroes2).
    if ( sx < 0 ) {
        w += sx;
        dx -= sx;
        sx = 0;
    }
    if ( sy < 0 ) {
        h += sy;
        dy -= sy;
        sy = 0;
    }
    w = std::min( w, src.img.width() - sx );
    h = std::min( h, src.img.height() - sy );
    w = std::min( w, dst.img.width() - dx );
    h = std::min( h, dst.img.height() - dy );
    if ( w <= 0 || h <= 0 )
        return;
    QPainter p( &dst.img );
    p.setCompositionMode( QPainter::CompositionMode_SourceOver );
    p.drawImage( dx, dy, src.img, sx, sy, w, h );
    p.end();
    if ( !src.idx.isNull() ) {
        for ( int yy = 0; yy < h; ++yy ) {
            for ( int xx = 0; xx < w; ++xx )
                dst.idx.setPixel( dx + xx, dy + yy, src.idx.pixelIndex( sx + xx, sy + yy ) );
        }
    }
}

// FillTransform(...,1) — make the region transparent.
void gErase( Glyph & g, int x, int y, int w, int h )
{
    w = std::min( w, g.img.width() - x );
    h = std::min( h, g.img.height() - y );
    if ( w <= 0 || h <= 0 )
        return;
    QPainter p( &g.img );
    p.setCompositionMode( QPainter::CompositionMode_Source );
    p.fillRect( x, y, w, h, Qt::transparent );
    p.end();
    if ( !g.idx.isNull() ) {
        for ( int yy = 0; yy < h; ++yy ) {
            for ( int xx = 0; xx < w; ++xx )
                g.idx.setPixel( x + xx, y + yy, 0xFF );
        }
    }
}

// Fill(dst, x, y, w, h, color of pixel (sx, sy)) — like Fill with image()[idx] in fheroes2.
void gFillColor( Glyph & g, int x, int y, int w, int h, int sx, int sy )
{
    w = std::min( w, g.img.width() - x );
    h = std::min( h, g.img.height() - y );
    if ( w <= 0 || h <= 0 )
        return;
    const QColor c = g.img.pixelColor( sx, sy );
    QPainter p( &g.img );
    p.fillRect( x, y, w, h, c );
    p.end();
    if ( !g.idx.isNull() ) {
        const uint idx = g.idx.pixelIndex( sx, sy );
        for ( int yy = 0; yy < h; ++yy ) {
            for ( int xx = 0; xx < w; ++xx )
                g.idx.setPixel( x + xx, y + yy, idx );
        }
    }
}

// updateShadow({-1,2}, 2, false) / ({-1,1}, 2, true) — shadow inside the glyph:
// semi-transparent black where transparent, if the source (x+1, y−offset)
// is opaque. In the engine the shadow is transform layer 2: when blitting, the background
// is darkened to ~0.81 (see ApplyTransform), the equivalent is black with alpha ~19%.
constexpr int SHADOW_ALPHA = 48; // 255 × (1 − 0.81)

void gShadow( Glyph & g, int shadowY, bool connectCorners )
{
    const int w = g.img.width();
    const int h = g.img.height();
    const QColor shadow( 0, 0, 0, SHADOW_ALPHA );
    for ( int y = 0; y < h; ++y ) {
        for ( int x = 0; x < w; ++x ) {
            if ( g.img.pixelColor( x, y ).alpha() != 0 )
                continue;
            const int sx = x + 1;
            const int sy = y - shadowY;
            const bool direct = sx >= 0 && sx < w && sy >= 0 && sy < h && g.img.pixelColor( sx, sy ).alpha() != 0;
            // Corners like in fheroes2 updateShadow (cornerOffsetX = −1, cornerOffsetY = +width):
            // both neighbors of the source — to the left (x, sy) and below (sx, sy+1) — are opaque.
            const bool corner = connectCorners && x < w && sy >= 0 && sy < h && g.img.pixelColor( x, sy ).alpha() != 0
                                && sx < w && sy + 1 < h && sy + 1 >= 0 && g.img.pixelColor( sx, sy + 1 ).alpha() != 0;
            if ( direct || corner ) {
                g.img.setPixelColor( x, y, shadow );
                if ( !g.idx.isNull() )
                    g.idx.setPixel( x, y, 0 );
            }
        }
    }
}

// Glyph recoloring: like CopyICNWithPalette in fheroes2 — replacement of color indices
// via a table (yellowTextTable/grayTextTable from pal.cpp) + the color from KB.PAL.
QImage tintedTable( const QImage & img, const QImage & idx, const Assets & assets, const uint8_t * table )
{
    QImage out = img;
    for ( int y = 0; y < out.height(); ++y ) {
        for ( int x = 0; x < out.width(); ++x ) {
            const QColor c = out.pixelColor( x, y );
            if ( c.alpha() == 0 )
                continue;
            const uint srcIdx = idx.pixelIndex( x, y );
            if ( srcIdx == 0xFF )
                continue;
            // Letter shadow (idx 0) — semi-transparent black, do not recolor.
            if ( srcIdx == 0 )
                continue;
            const QColor newColor = assets.paletteColor( table[srcIdx] );
            out.setPixelColor( x, y, newColor );
        }
    }
    return out;
}

QImage tinted( const QImage & img, const QImage & idx, const Assets & assets, GameFont::Color color )
{
    if ( color == GameFont::Color::WHITE || idx.isNull() )
        return img;
    const uint8_t * table = nullptr;
    switch ( color ) {
    case GameFont::Color::YELLOW:
        table = yellowTextTable;
        break;
    case GameFont::Color::GRAY:
        table = grayTextTable;
        break;
    default:
        return img;
    }
    return tintedTable( img, idx, assets, table );
}

} // namespace

GameFont::GameFont( const Assets * assets )
    : _assets( assets )
{
    if ( !assets || !assets->valid() )
        return;

    // ASCII: glyphs at index charcode-0x20.
    for ( int i = 0; i < 96; ++i ) {
        const IcnSprite & s = assets->icnSprite( ICN_FONT, i );
        if ( !s.isNull() ) {
            _normal[i] = { s.image, s.idx, s.offsetX, s.offsetY, false };
        }
        const IcnSprite & ss = assets->icnSprite( ICN_SMALFONT, i );
        if ( !ss.isNull() ) {
            _small[i] = { ss.image, ss.idx, ss.offsetX, ss.offsetY, false };
        }
    }

    // In the original assets, color index 50 in FONT.ICN is an error (3 pixels).
    // The engine replaces it with transform 2 (shadow): ReplaceColorIdByTransformId in fheroes2.
    for ( Glyph & glyph : _normal ) {
        if ( glyph.empty || glyph.idx.isNull() )
            continue;
        for ( int y = 0; y < glyph.img.height(); ++y ) {
            for ( int x = 0; x < glyph.img.width(); ++x ) {
                if ( glyph.idx.pixelIndex( x, y ) == 50 )
                    glyph.img.setPixelColor( x, y, QColor( 0, 0, 0, SHADOW_ALPHA ) );
            }
        }
    }

    // --- port of modifyBaseNormalFont / modifyBaseSmallFont (without French fixes) ---

    // $: position.
    _normal['$' - 0x20].y = 2;
    _small['$' - 0x20].y = -1;

    // %: remove the white line.
    gErase( _normal['%' - 0x20], 5, 0, 5, 1 );
    gErase( _normal['%' - 0x20], 6, 2, 2, 1 );
    gShadow( _normal['%' - 0x20], 2, false );
    gErase( _small['%' - 0x20], 3, 0, 4, 1 );
    gErase( _small['%' - 0x20], 4, 1, 2, 1 );
    gShadow( _small['%' - 0x20], 1, true );

    // &: position.
    _normal['&' - 0x20].y = 1;

    // '-': lower.
    ++_normal['-' - 0x20].y;
    gShadow( _normal['-' - 0x20], 2, false );

    // '\': build from '/'.
    gResize( _normal['\\' - 0x20], 8, 14 );
    gCopy( _normal['/' - 0x20], 0, 0, _normal['\\' - 0x20], 1, 0, 7, 12 );
    _normal['\\' - 0x20].x = _normal['/' - 0x20].x;
    _normal['\\' - 0x20].y = _normal['/' - 0x20].y;
    gShadow( _normal['\\' - 0x20], 2, false );

    gResize( _small['\\' - 0x20], 5, 9 );
    gCopy( _small['/' - 0x20], 4, 0, _small['\\' - 0x20], 1, 0, 1, 2 );
    gCopy( _small['/' - 0x20], 4, 0, _small['\\' - 0x20], 2, 2, 1, 2 );
    gCopy( _small['/' - 0x20], 4, 0, _small['\\' - 0x20], 3, 4, 1, 2 );
    gCopy( _small['/' - 0x20], 4, 0, _small['\\' - 0x20], 4, 6, 1, 2 );
    _small['\\' - 0x20].x = _small['/' - 0x20].x;
    _small['\\' - 0x20].y = _small['/' - 0x20].y;
    gShadow( _small['\\' - 0x20], 1, true );

    // Lowercase k (normal font).
    {
        Glyph & k = _normal['k' - 0x20];
        gErase( k, 4, 1, 5, 8 );
        gCopy( _normal['K' - 0x20], 6, 5, k, 4, 7, 3, 1 );
        gCopy( _normal['K' - 0x20], 6, 4, k, 4, 6, 4, 1 );
        gCopy( _normal['K' - 0x20], 7, 4, k, 6, 5, 3, 1 );
        gCopy( _normal['K' - 0x20], 7, 4, k, 7, 4, 2, 1 );
        gCopy( _normal['K' - 0x20], 6, 6, k, 4, 8, 4, 1 );
        gShadow( k, 2, false );
    }

    // Lowercase k (small font).
    {
        Glyph & k = _small['k' - 0x20];
        gResize( k, 6, 8 );
        gCopy( _small['l' - 0x20], 1, 0, k, 1, 0, 2, 7 );
        gCopy( _small['l' - 0x20], 1, 0, k, 1, 6, 1, 1 );
        gCopy( _small['X' - 0x20], 6, 0, k, 3, 2, 3, 3 );
        gCopy( _small['a' - 0x20], 2, _small['a' - 0x20].img.height() - 2, k, 5, 6, 2, 1 );
        gCopy( _small['a' - 0x20], 2, 0, k, 4, 5, 1, 1 );
        gShadow( k, 1, true );
    }

    // '|' (vertical bar, input cursor).
    {
        Glyph & bar = _normal['|' - 0x20];
        const int srcH = _normal['[' - 0x20].img.height();
        gResize( bar, 3, srcH + 3 );
        gCopy( _normal['[' - 0x20], 0, 0, bar, 0, 0, 3, srcH - 4 );
        gCopy( _normal['[' - 0x20], 0, srcH - 7, bar, 0, srcH - 4, 3, 7 );
        bar.x = _normal['[' - 0x20].x;
        bar.y = _normal['[' - 0x20].y;
    }

    // '*' (small font).
    {
        Glyph & star = _small['*' - 0x20];
        gResize( star, 6, 8 );
        star.x = 0;
        star.y = 0;
        QPainter p( &star.img );
        p.setPen( QColor( 0, 0, 0, 255 ) );
        p.drawLine( 3, 0, 3, 6 );
        p.drawLine( 1, 1, 5, 5 );
        p.drawLine( 5, 1, 1, 5 );
        p.end();
        gShadow( star, 1, true );
    }

    // --- CP1251 generation (port of generateCP1251Alphabet) ---
    // Indices are cp1251 charcode − 0x20 (see fheroes2: chars 0x20-0x7F,
    // 0xC0-0xDF, 0xA8, 0xE0-0xFF, 0xB8 at charcode-0x20 indices).

    // Normal font.
    {
        // ' (right single quotation mark, 0x92).
        _normal[114] = _normal[',' - 0x20];
        _normal[114].y -= 6;

        // Yo (0xA8=168): E with two dots above.
        {
            const Glyph & E = _normal['E' - 0x20];
            Glyph & g = _normal[136];
            gResize( g, E.img.width(), E.img.height() + 3 );
            gCopy( E, 0, 0, g, 0, 3, E.img.width(), E.img.height() );
            gCopy( g, 5, 5, g, 4, 0, 1, 1 );
            gCopy( g, 5, 5, g, 7, 0, 1, 1 );
            gCopy( g, 4, 5, g, 4, 1, 1, 1 );
            gCopy( g, 4, 5, g, 7, 1, 1, 1 );
            g.x = E.x;
            g.y = E.y - 3;
            gShadow( g, 2, false );
        }

        // NBSP (0xA0=160).
        gResize( _normal[128], 6, 1 );

        // U with breve (0xA1=161): Y with dots.
        {
            const Glyph & Y = _normal['Y' - 0x20];
            Glyph & g = _normal[129];
            gResize( g, Y.img.width(), Y.img.height() + 3 );
            gCopy( Y, 0, 0, g, 0, 3, Y.img.width(), Y.img.height() );
            gCopy( _normal[136], 3, 0, g, 7, 0, 2, 3 );
            g.x = Y.x;
            g.y = Y.y - 3;
            gShadow( g, 2, false );
        }

        // I (0xA2=162): y with a dot.
        {
            const Glyph & y = _normal['y' - 0x20];
            Glyph & g = _normal[130];
            gResize( g, y.img.width(), y.img.height() + 3 );
            gCopy( y, 0, 0, g, 0, 3, y.img.width(), y.img.height() );
            gCopy( y, 4, 1, g, 6, 0, 1, 1 );
            gCopy( y, 4, 0, g, 6, 1, 1, 1 );
            g.x = y.x;
            g.y = y.y - 3;
            gShadow( g, 2, false );
        }

        // Ukrainian IE (0xAA=170): C with a line.
        {
            Glyph & g = _normal[138];
            g = _normal['C' - 0x20];
            gCopy( g, 7, 0, g, 6, 5, 4, 2 );
            gShadow( g, 2, false );
        }

        // Ukrainian ie (0xBA=186): c with a line.
        {
            Glyph & g = _normal[154];
            g = _normal['c' - 0x20];
            gCopy( g, 4, 0, g, 3, 3, 3, 1 );
            gShadow( g, 2, false );
        }

        // Yi (0xAF=175): I with dots.
        {
            const Glyph & I = _normal['I' - 0x20];
            Glyph & g = _normal[143];
            gResize( g, I.img.width(), I.img.height() + 3 );
            gCopy( I, 0, 0, g, 0, 3, I.img.width(), I.img.height() );
            gCopy( _normal[136], 3, 0, g, 2, 0, 5, 3 );
            g.x = I.x;
            g.y = I.y - 3;
        }

        // yi (0xBF=191): i with dots.
        {
            Glyph & g = _normal[159];
            g = _normal['i' - 0x20];
            gErase( g, 2, 0, 1, 3 );
        }

        // J and j (0xA3=163, 0xBC=188).
        _normal[131] = _normal['J' - 0x20];
        _normal[156] = _normal['j' - 0x20];

        // S and s (0xBD=189, 0xBE=190).
        _normal[157] = _normal['S' - 0x20];
        _normal[158] = _normal['s' - 0x20];

        // I and i (0xB2=178, 0xB3=179).
        _normal[146] = _normal['I' - 0x20];
        _normal[147] = _normal['i' - 0x20];

        // A (0xC0=192).
        _normal[160] = _normal['A' - 0x20];

        // Be (0xC1=193): B without the lower loop + horizontal.
        {
            const Glyph & B = _normal['B' - 0x20];
            Glyph & g = _normal[161];
            g = B;
            gErase( g, 9, 4, 2, 1 );
            gCopy( _normal['F' - 0x20], 6, 0, g, 6, 0, 5, 4 );
            gCopy( g, 9, 5, g, 8, 4, 1, 1 );
            gShadow( g, 2, false );
        }

        // Ve (0xC2=194).
        _normal[162] = _normal['B' - 0x20];

        // Ghe (0xC3=195): F without the crossbar.
        {
            Glyph & g = _normal[163];
            g = _normal['F' - 0x20];
            gErase( g, 6, 4, 3, 4 );
        }

        // Ghe with upturn (0xA5=165): Ghe with a vertical tail.
        {
            const Glyph & G = _normal[163];
            Glyph & g = _normal[133];
            gResize( g, G.img.width(), G.img.height() + 1 );
            gCopy( G, 0, 0, g, 0, 1, G.img.width(), G.img.height() );
            gCopy( G, 9, 1, g, 9, 0, 2, 1 );
            gCopy( G, 9, 1, g, 10, 1, 1, 1 );
            gCopy( G, 10, 0, g, 10, 2, 1, 1 );
            gCopy( G, 8, 1, g, 9, 2, 1, 1 );
            g.x = G.x;
            g.y = G.y - 1;
        }

        // De (0xC4=196).
        _normal[164] = _normal['D' - 0x20];

        // Ye (0xC5=197).
        _normal[165] = _normal['E' - 0x20];

        // Zhe (0xC6=198): X with |.
        {
            const Glyph & X = _normal['X' - 0x20];
            Glyph & g = _normal[166];
            gResize( g, X.img.width() + 1, X.img.height() );
            gCopy( X, 1, 0, g, 1, 0, 8, 11 );
            gCopy( X, 9, 0, g, 10, 0, 6, 11 );
            gFillColor( g, 9, 1, 1, 9, 1, 0 );
            g.x = X.x;
            g.y = X.y;
            gShadow( g, 2, false );
        }

        // Ze (0xC7=199): digit 3 with a narrow top.
        {
            const Glyph & three = _normal['3' - 0x20];
            Glyph & g = _normal[167];
            gResize( g, three.img.width() + 1, three.img.height() );
            gCopy( three, 1, 0, g, 1, 0, 5, 3 );
            gCopy( three, 5, 0, g, 6, 0, 3, 4 );
            gCopy( three, 3, 5, g, 4, 4, 5, 4 );
            gCopy( three, 1, 8, g, 1, 8, 5, 3 );
            gCopy( three, 5, 8, g, 6, 8, 3, 3 );
            gErase( g, 2, 6, 5, 3 );
            g.x = three.x;
            g.y = three.y;
            gShadow( g, 2, false );
        }

        // I (0xC8=200): mirrored N.
        {
            const Glyph & N = _normal['N' - 0x20];
            Glyph & g = _normal[168];
            g = N;
            gErase( g, 6, 1, 5, 11 );
            gCopy( N, 6, 2, g, 6, 6, 1, 3 );
            gCopy( N, 7, 3, g, 7, 5, 1, 3 );
            gCopy( N, 8, 4, g, 8, 4, 1, 3 );
            gCopy( N, 8, 4, g, 9, 3, 1, 3 );
            gCopy( N, 8, 4, g, 10, 2, 1, 3 );
            gCopy( N, 8, 4, g, 11, 1, 1, 3 );
            gCopy( N, 11, 7, g, 11, 8, 1, 1 );
            gCopy( N, 13, 9, g, 11, 9, 1, 1 );
            gShadow( g, 2, false );
        }

        // Short I (0xC9=201): I with a dot.
        {
            const Glyph & I = _normal[168];
            Glyph & g = _normal[169];
            gResize( g, I.img.width(), I.img.height() + 3 );
            gCopy( I, 0, 0, g, 0, 3, I.img.width(), I.img.height() );
            g.x = I.x;
            g.y = I.y - 3;
            gCopy( g, 12, 4, g, 8, 0, 1, 1 );
            gCopy( g, 11, 10, g, 8, 1, 1, 1 );
            gShadow( g, 2, false );
        }

        // Ka (0xCA=202).
        _normal[170] = _normal['K' - 0x20];

        // Em (0xCC=204), En (0xCD=205), O (0xCE=206).
        _normal[172] = _normal['M' - 0x20];
        _normal[173] = _normal['H' - 0x20];
        _normal[174] = _normal['O' - 0x20];

        // Pe (0xCF=207): Ghe + right stem.
        {
            const Glyph & G = _normal[163];
            Glyph & g = _normal[175];
            g = G;
            gCopy( g, 4, 1, g, 8, 1, 2, 9 );
            gCopy( g, 4, 9, g, 8, 10, 2, 1 );
            gCopy( g, 6, 0, g, 10, 0, 1, 2 );
            gShadow( g, 2, false );
        }

        // El (0xCB=203): Pe without the right part.
        {
            const Glyph & P = _normal[175];
            Glyph & g = _normal[171];
            gResize( g, P.img.width() - 1, P.img.height() );
            gCopy( P, 0, 0, g, 0, 0, P.img.width() - 1, P.img.height() );
            gErase( g, 0, 0, 4, 6 );
            gErase( g, 4, 0, 3, 2 );
            gCopy( g, 4, 2, g, 5, 1, 2, 1 );
            gCopy( g, 1, 10, g, 5, 0, 2, 1 );
            g.x = P.x;
            g.y = P.y;
            gShadow( g, 2, false );
        }

        // Er (0xD0=208), Es (0xD1=209).
        _normal[176] = _normal['P' - 0x20];
        _normal[177] = _normal['C' - 0x20];

        // Te (0xD2=210): Pe with a widened crossbar.
        {
            const Glyph & P = _normal[175];
            Glyph & g = _normal[178];
            gResize( g, P.img.width() + 4, P.img.height() );
            gCopy( P, 0, 0, g, 0, 0, P.img.width(), P.img.height() );
            gCopy( g, 7, 0, g, 11, 0, 4, P.img.height() );
            g.x = P.x;
            g.y = P.y;
        }

        // U (0xD3=211).
        _normal[179] = _normal['Y' - 0x20];

        // Ef (0xD4=212): P with a mirrored half.
        {
            const Glyph & P = _normal['P' - 0x20];
            Glyph & g = _normal[180];
            gResize( g, P.img.width() + 1, P.img.height() );
            gCopy( P, 0, 0, g, 1, 0, P.img.width(), P.img.height() );
            // Horizontal flip: copy the row backwards (with color indices).
            for ( int y = 0; y < 6; ++y ) {
                for ( int x = 0; x < 5; ++x ) {
                    g.img.setPixelColor( 1 + x, y, P.img.pixelColor( 6 + 5 - 1 - x, y ) );
                    if ( !P.idx.isNull() )
                        g.idx.setPixel( 1 + x, y, P.idx.pixelIndex( 6 + 5 - 1 - x, y ) );
                }
            }
            g.x = P.x;
            g.y = P.y;
            gShadow( g, 2, false );
        }

        // Ha (0xD5=213).
        _normal[181] = _normal['X' - 0x20];

        // Tse (0xD6=214): U + tail.
        {
            const Glyph & U = _normal['U' - 0x20];
            Glyph & g = _normal[182];
            gResize( g, U.img.width() + 2, U.img.height() + 1 );
            gCopy( U, 0, 0, g, 0, 0, U.img.width(), U.img.height() );
            gCopy( g, 9, 1, g, 11, 9, 1, 1 );
            gCopy( g, 9, 1, g, 12, 8, 1, 1 );
            gCopy( g, 9, 1, g, 12, 10, 1, 2 );
            gCopy( g, 10, 1, g, 12, 9, 1, 1 );
            gCopy( g, 10, 1, g, 13, 8, 1, 4 );
            g.x = U.x;
            g.y = U.y;
            gShadow( g, 2, false );
        }

        // Sha (0xD8=216): U with two stems.
        {
            const Glyph & U = _normal['U' - 0x20];
            Glyph & g = _normal[184];
            gResize( g, U.img.width() + 2, U.img.height() );
            gCopy( U, 0, 0, g, 0, 0, 6, 11 );
            gCopy( U, 8, 0, g, 7, 0, 3, 11 );
            gCopy( U, 8, 0, g, 11, 0, 3, 11 );
            gCopy( _normal[172], 10, 0, g, 6, 5, 3, 5 );
            gCopy( _normal[172], 10, 0, g, 10, 5, 3, 5 );
            gErase( g, 7, 10, 1, 1 );
            gErase( g, 11, 10, 1, 1 );
            g.x = U.x;
            g.y = U.y;
            gShadow( g, 2, false );
        }

        // Che (0xD7=215): U with a narrow crossbar.
        {
            const Glyph & U = _normal['U' - 0x20];
            Glyph & g = _normal[183];
            g = U;
            gErase( g, 3, 6, 6, 7 );
            gCopy( _normal[184], 4, 5, g, 4, 3, 4, 6 );
            gCopy( g, 6, 5, g, 8, 3, 1, 3 );
            gCopy( g, 7, 4, g, 9, 2, 1, 2 );
            gCopy( g, 9, 8, g, 9, 9, 1, 1 );
            gShadow( g, 2, false );
        }

        // Shcha (0xD9=217): Sha + tail.
        {
            const Glyph & Sh = _normal[184];
            Glyph & g = _normal[185];
            gResize( g, Sh.img.width() + 2, Sh.img.height() + 1 );
            gCopy( Sh, 0, 0, g, 0, 0, Sh.img.width(), Sh.img.height() );
            gCopy( _normal[182], 11, 8, g, 14, 8, 3, 4 );
            g.x = Sh.x;
            g.y = Sh.y;
            gShadow( g, 2, false );
        }

        // Hard sign (0xDA=218): Be with a vertical.
        {
            const Glyph & B = _normal[161];
            Glyph & g = _normal[186];
            gResize( g, B.img.width() + 1, B.img.height() );
            gCopy( B, 0, 0, g, 1, 0, B.img.width(), B.img.height() );
            gCopy( B, 1, 0, g, 1, 0, 3, 4 );
            gErase( g, 7, 0, 5, 4 );
            g.x = B.x;
            g.y = B.y;
            gShadow( g, 2, false );
        }

        // Soft sign (0xDC=220): Be with a clean left part.
        {
            const Glyph & B = _normal[161];
            Glyph & g = _normal[188];
            g = B;
            gErase( g, 0, 0, 4, 6 );
            gErase( g, 6, 0, 5, 4 );
            gCopy( _normal['U' - 0x20], 8, 0, g, 3, 0, 3, 1 );
            gShadow( g, 2, false );
        }

        // Yeru (0xDB=219): Soft sign + I.
        {
            const Glyph & B = _normal[188];
            Glyph & g = _normal[187];
            gResize( g, B.img.width() + 3, B.img.height() );
            gCopy( B, 0, 0, g, 0, 0, B.img.width(), B.img.height() );
            gCopy( g, 3, 0, g, 11, 0, 3, 9 );
            gCopy( _normal[175], 8, 9, g, 12, 9, 2, 2 );
            g.x = B.x;
            g.y = B.y;
            gShadow( g, 2, false );
        }

        // E (0xDD=221): O with the left part cut off.
        {
            const Glyph & O = _normal['O' - 0x20];
            Glyph & g = _normal[189];
            gResize( g, O.img.width() - 3, O.img.height() );
            gCopy( O, 4, 0, g, 1, 0, 9, 11 );
            gErase( g, 0, 3, 3, 5 );
            gCopy( g, 3, 0, g, 4, 5, 5, 1 );
            g.x = O.x;
            g.y = O.y;
            gShadow( g, 2, false );
        }

        // Yu (0xDE=222): Be + O.
        {
            const Glyph & B = _normal[161];
            const Glyph & O = _normal['O' - 0x20];
            Glyph & g = _normal[190];
            gResize( g, O.img.width() + 1, O.img.height() );
            gCopy( B, 0, 0, g, 0, 0, 6, 13 );
            gCopy( O, 4, 1, g, 7, 1, 4, 8 );
            gCopy( O, 10, 1, g, 11, 1, 3, 8 );
            gCopy( O, 5, 0, g, 8, 0, 3, 1 );
            gCopy( O, 10, 0, g, 11, 0, 2, 1 );
            gCopy( O, 4, 9, g, 7, 9, 4, 2 );
            gCopy( O, 10, 9, g, 11, 9, 3, 2 );
            gCopy( g, 2, 0, g, 6, 5, 2, 1 );
            g.x = B.x;
            g.y = B.y;
            gShadow( g, 2, false );
        }

        // Ya (0xDF=223): A-bottom + El.
        {
            const Glyph & L = _normal[171];
            Glyph & g = _normal[191];
            gResize( g, L.img.width() - 1, L.img.height() );
            gCopy( _normal['A' - 0x20], 0, 5, g, 0, 5, 7, 6 );
            gCopy( _normal[180], 0, 0, g, 1, 0, 7, 6 );
            gCopy( L, 8, 0, g, 7, 0, 2, 11 );
            gCopy( g, 6, 5, g, 7, 5, 1, 1 );
            g.x = L.x;
            g.y = L.y;
            gShadow( g, 2, false );
        }

        // --- lowercase (offset 32) ---

        // yo (0xB8=184).
        {
            const Glyph & e = _normal['e' - 0x20];
            Glyph & g = _normal[152];
            gResize( g, e.img.width(), e.img.height() + 3 );
            gCopy( e, 0, 0, g, 0, 3, e.img.width(), e.img.height() );
            gCopy( _normal[136], 3, 0, g, 3, 0, 2, 4 );
            gCopy( _normal[136], 3, 0, g, 5, 0, 2, 4 );
            g.x = e.x;
            g.y = e.y - 3;
        }

        // a (0xE0=224).
        _normal[192] = _normal['a' - 0x20];

        // be (0xE1=225).
        {
            const Glyph & e = _normal['e' - 0x20];
            const Glyph & c = _normal['c' - 0x20];
            Glyph & g = _normal[193];
            gResize( g, e.img.width(), e.img.height() + 3 );
            gCopy( e, 1, 5, g, 1, 8, 8, 2 );
            gCopy( e, 1, 0, g, 1, 6, 8, 2 );
            gCopy( c, 1, 0, g, 1, 0, 8, 2 );
            gCopy( _normal['M' - 0x20], 7, 3, g, 1, 2, 3, 1 );
            gCopy( _normal['M' - 0x20], 7, 3, g, 2, 3, 3, 1 );
            gCopy( _normal['M' - 0x20], 7, 3, g, 3, 4, 3, 1 );
            gCopy( _normal['M' - 0x20], 8, 3, g, 6, 5, 2, 1 );
            gCopy( _normal['M' - 0x20], 7, 3, g, 4, 5, 2, 1 );
            g.x = e.x;
            g.y = e.y - 3;
            gShadow( g, 2, false );
        }

        // ghe (0xE3=227): r with a crossbar.
        {
            Glyph & g = _normal[195];
            g = _normal['r' - 0x20];
            gCopy( g, 1, 0, g, 3, 0, 2, 1 );
            gCopy( g, 4, 2, g, 4, 1, 1, 1 );
            gErase( g, 4, 2, 1, 1 );
            gShadow( g, 2, false );
        }

        // ghe with upturn (0xB4=180): ghe with a tail.
        {
            const Glyph & g2 = _normal[195];
            Glyph & g = _normal[148];
            gResize( g, g2.img.width(), g2.img.height() + 1 );
            gCopy( g2, 0, 0, g, 0, 1, g2.img.width(), g2.img.height() );
            gCopy( g2, 6, 1, g, 6, 0, 3, 1 );
            gErase( g, 7, 2, 2, 1 );
            gErase( g, 6, 4, 2, 1 );
            g.x = g2.x;
            g.y = g2.y - 1;
        }

        // de (0xE4=228): g without the lower loop.
        _normal[196] = _normal['g' - 0x20];

        // ye (0xE5=229).
        _normal[197] = _normal['e' - 0x20];

        // zhe (0xE6=230): x with |.
        {
            const Glyph & x = _normal['x' - 0x20];
            Glyph & g = _normal[198];
            gResize( g, x.img.width() + 2, x.img.height() );
            gCopy( x, 0, 0, g, 0, 0, 6, 7 );
            gCopy( x, 5, 0, g, 7, 0, 5, 7 );
            gFillColor( g, 6, 1, 1, 5, 3, 0 );
            g.x = x.x;
            g.y = x.y;
            gShadow( g, 2, false );
        }

        // ze (0xE7=231): 3 without the top part.
        {
            const Glyph & three = _normal['3' - 0x20];
            Glyph & g = _normal[199];
            gResize( g, three.img.width(), three.img.height() - 4 );
            gCopy( three, 0, 0, g, 0, 0, three.img.width(), 3 );
            gCopy( three, 0, 5, g, 0, 3, three.img.width(), 1 );
            gCopy( three, 0, 8, g, 0, 4, three.img.width(), 4 );
            gErase( g, 0, 2, 3, 3 );
            g.x = three.x;
            g.y = three.y + 4;
            gShadow( g, 2, false );
        }

        // ve (0xE2=226): ze + left stem.
        {
            const Glyph & z = _normal[199];
            Glyph & g = _normal[194];
            gResize( g, z.img.width() + 1, z.img.height() );
            gCopy( z, 0, 0, g, 1, 0, z.img.width(), z.img.height() );
            gCopy( _normal['m' - 0x20], 1, 0, g, 1, 0, 3, 7 );
            gCopy( g, 7, 1, g, 3, 0, 1, 1 );
            gCopy( g, 7, 1, g, 3, 6, 1, 1 );
            gCopy( g, 3, 4, g, 3, 5, 1, 1 );
            g.x = z.x;
            g.y = z.y;
            gShadow( g, 2, false );
        }

        // i (0xE8=232).
        _normal[200] = _normal['u' - 0x20];

        // short i (0xE9=233).
        {
            const Glyph & u = _normal[200];
            Glyph & g = _normal[201];
            gResize( g, u.img.width(), u.img.height() + 3 );
            gCopy( u, 0, 0, g, 0, 3, u.img.width(), u.img.height() );
            gCopy( g, 8, 3, g, 5, 0, 1, 1 );
            gCopy( g, 7, 3, g, 5, 1, 1, 1 );
            g.x = u.x;
            g.y = u.y - 3;
            gShadow( g, 2, false );
        }

        // ka (0xEA=234): shortened k.
        {
            const Glyph & k = _normal['k' - 0x20];
            Glyph & g = _normal[202];
            gResize( g, k.img.width() - 1, k.img.height() - 4 );
            gCopy( k, 0, 0, g, 0, 0, 4, 6 );
            gCopy( k, 4, 4, g, 4, 0, 5, 6 );
            gCopy( k, 0, 10, g, 0, 6, 4, 1 );
            gCopy( k, 7, 10, g, 6, 6, 3, 1 );
            g.x = k.x;
            g.y = k.y + 4;
            gShadow( g, 2, false );
        }

        // el (0xEB=235): n with a flat left leg.
        {
            Glyph & g = _normal[203];
            g = _normal['n' - 0x20];
            gCopy( g, 3, 0, g, 2, 1, 1, 1 );
            gErase( g, 0, 0, 2, 3 );
            gErase( g, 2, 0, 1, 1 );
            gShadow( g, 2, false );
        }

        // em (0xEC=236).
        {
            Glyph & g = _normal[204];
            g = _normal['m' - 0x20];
            gCopy( _normal['w' - 0x20], 9, 0, g, 3, 0, 4, 7 );
            gCopy( _normal['w' - 0x20], 9, 0, g, 9, 0, 4, 7 );
            gErase( g, 0, 0, 3, 6 );
            gShadow( g, 2, false );
        }

        // en (0xED=237): n with a long right leg.
        {
            Glyph & g = _normal[205];
            g = _normal['n' - 0x20];
            gErase( g, 4, 0, 3, 8 );
            gCopy( g, 4, 1, g, 4, 3, 1, 2 );
            gCopy( g, 4, 1, g, 5, 3, 1, 2 );
            gCopy( g, 4, 1, g, 6, 3, 1, 2 );
            gCopy( g, 4, 1, g, 7, 3, 1, 1 );
            gShadow( g, 2, false );
        }

        // o (0xEE=238).
        _normal[206] = _normal['o' - 0x20];

        // pe (0xEF=239).
        _normal[207] = _normal['n' - 0x20];

        // er (0xF0=240), es (0xF1=241).
        _normal[208] = _normal['p' - 0x20];
        _normal[209] = _normal['c' - 0x20];

        // te (0xF2=242).
        _normal[210] = _normal['m' - 0x20];

        // u (0xF3=243).
        _normal[211] = _normal['y' - 0x20];

        // ef (0xF4=244).
        {
            const Glyph & q = _normal['q' - 0x20];
            Glyph & g = _normal[212];
            gResize( g, q.img.width(), q.img.height() );
            gCopy( _normal['p' - 0x20], 1, 0, g, 3, 0, 4, 10 );
            gCopy( q, 0, 0, g, 0, 0, 5, 7 );
            gCopy( _normal['p' - 0x20], 6, 0, g, 7, 0, 4, 7 );
            g.x = q.x;
            g.y = q.y;
            gShadow( g, 2, false );
        }

        // ha (0xF5=245).
        _normal[213] = _normal['x' - 0x20];

        // tse (0xF6=246).
        {
            const Glyph & u = _normal['u' - 0x20];
            Glyph & g = _normal[214];
            gResize( g, u.img.width() + 2, u.img.height() + 1 );
            gCopy( u, 0, 0, g, 0, 0, u.img.width(), u.img.height() );
            gCopy( g, 7, 4, g, 9, 5, 1, 1 );
            gCopy( g, 7, 4, g, 10, 4, 1, 1 );
            gCopy( g, 8, 1, g, 11, 4, 1, 4 );
            gCopy( g, 8, 1, g, 10, 5, 1, 1 );
            gCopy( g, 9, 5, g, 10, 6, 1, 1 );
            gCopy( g, 9, 5, g, 10, 7, 1, 1 );
            g.x = u.x;
            g.y = u.y;
            gShadow( g, 2, false );
        }

        // che (0xF7=247): u with a crossbar.
        {
            Glyph & g = _normal[215];
            g = _normal['u' - 0x20];
            gCopy( g, 2, 5, g, 2, 3, 6, 2 );
            gCopy( g, 8, 0, g, 7, 4, 1, 1 );
            gCopy( g, 8, 0, g, 7, 5, 1, 1 );
            gCopy( g, 8, 0, g, 7, 6, 1, 1 );
            gErase( g, 1, 5, 6, 4 );
            gShadow( g, 2, false );
        }

        // sha (0xF8=248).
        {
            const Glyph & u = _normal['u' - 0x20];
            Glyph & g = _normal[216];
            gResize( g, u.img.width() + 3, u.img.height() );
            gCopy( u, 0, 0, g, 0, 0, 4, 7 );
            gCopy( u, 1, 0, g, 5, 0, 4, 7 );
            gCopy( u, 6, 0, g, 9, 0, 4, 7 );
            gCopy( g, 8, 5, g, 4, 5, 4, 2 );
            g.x = u.x;
            g.y = u.y;
            gShadow( g, 2, false );
        }

        // shcha (0xF9=249).
        {
            const Glyph & sh = _normal[216];
            Glyph & g = _normal[217];
            gResize( g, sh.img.width() + 2, sh.img.height() );
            gCopy( sh, 0, 0, g, 0, 0, 12, 7 );
            gCopy( _normal[214], 9, 4, g, 12, 4, 3, 4 );
            g.x = sh.x;
            g.y = sh.y;
            gShadow( g, 2, false );
        }

        // soft sign (0xFC=252).
        {
            Glyph & g = _normal[220];
            g = _normal[194];
            gErase( g, 4, 0, 5, 3 );
        }

        // hard sign (0xFA=250).
        {
            const Glyph & b = _normal[220];
            Glyph & g = _normal[218];
            gResize( g, b.img.width() + 1, b.img.height() );
            gCopy( b, 0, 0, g, 1, 0, b.img.width(), b.img.height() );
            gCopy( b, 1, 0, g, 1, 0, 1, 2 );
            g.x = b.x;
            g.y = b.y;
            gShadow( g, 2, false );
        }

        // yeru (0xFB=251).
        {
            const Glyph & b = _normal[220];
            Glyph & g = _normal[219];
            gResize( g, b.img.width() + 3, b.img.height() );
            gCopy( b, 0, 0, g, 0, 0, b.img.width(), b.img.height() );
            gCopy( b, 2, 0, g, 10, 0, 2, 7 );
            g.x = b.x;
            g.y = b.y;
            gShadow( g, 2, false );
        }

        // e (0xFD=253): o with an open left part.
        {
            Glyph & g = _normal[221];
            g = _normal['o' - 0x20];
            gErase( g, 0, 2, 3, 3 );
            gCopy( g, 8, 3, g, 7, 3, 1, 1 );
            gCopy( g, 8, 3, g, 6, 3, 1, 1 );
            gCopy( g, 8, 3, g, 5, 3, 1, 1 );
            gShadow( g, 2, false );
        }

        // yu (0xFE=254).
        {
            const Glyph & y = _normal[219];
            const Glyph & o = _normal['o' - 0x20];
            Glyph & g = _normal[222];
            gResize( g, o.img.width() + 1, o.img.height() );
            gCopy( y, 1, 0, g, 1, 0, 3, 7 );
            gCopy( o, 2, 1, g, 6, 1, 1, 5 );
            gCopy( o, 3, 0, g, 6, 0, 3, 2 );
            gCopy( o, 7, 0, g, 9, 0, 1, 2 );
            gCopy( o, 8, 2, g, 9, 2, 1, 3 );
            gCopy( o, 7, 5, g, 9, 5, 1, 2 );
            gCopy( o, 3, 6, g, 6, 6, 3, 1 );
            gCopy( g, 1, 0, g, 4, 3, 2, 1 );
            g.x = y.x;
            g.y = y.y;
            gShadow( g, 2, false );
        }

        // ya (0xFF=255): a with a left leg.
        {
            Glyph & g = _normal[223];
            g = _normal['a' - 0x20];
            gErase( g, 0, 2, 6, 3 );
            gCopy( _normal['e' - 0x20], 2, 5, g, 1, 2, 6, 2 );
            gCopy( g, 6, 4, g, 6, 3, 1, 1 );
            gShadow( g, 2, false );
        }

        // Љ (0x8A=138).
        {
            const Glyph & L = _normal[171];
            const Glyph & B = _normal[188];
            Glyph & g = _normal[106];
            gResize( g, L.img.width() + B.img.width() - 6, L.img.height() );
            gCopy( L, 0, 0, g, 0, 0, L.img.width(), L.img.height() );
            gCopy( B, 4, 0, g, L.img.width() - 2, 0, B.img.width() - 4, B.img.height() );
            g.x = L.x;
            g.y = L.y;
            gShadow( g, 2, false );
        }

        // Њ (0x8C=140).
        {
            const Glyph & N = _normal[173];
            const Glyph & B = _normal[188];
            Glyph & g = _normal[108];
            gResize( g, N.img.width() + B.img.width() - 7, N.img.height() );
            gCopy( N, 0, 0, g, 0, 0, N.img.width(), N.img.height() );
            gCopy( B, 4, 0, g, N.img.width() - 3, 0, B.img.width() - 4, B.img.height() );
            g.x = N.x;
            g.y = N.y;
            gShadow( g, 2, false );
        }

        // љ (0x9A=154).
        {
            const Glyph & l = _normal[203];
            const Glyph & b = _normal[220];
            Glyph & g = _normal[122];
            gResize( g, l.img.width() + b.img.width() - 5, l.img.height() );
            gCopy( l, 0, 0, g, 0, 0, l.img.width(), l.img.height() );
            gCopy( b, 2, 0, g, l.img.width() - 3, 0, b.img.width() - 2, b.img.height() );
            g.x = l.x;
            g.y = l.y;
            gShadow( g, 2, false );
        }

        // њ (0x9C=156).
        {
            const Glyph & n = _normal[205];
            const Glyph & b = _normal[220];
            Glyph & g = _normal[124];
            gResize( g, n.img.width() + b.img.width() - 5, n.img.height() );
            gCopy( n, 0, 0, g, 0, 0, n.img.width(), n.img.height() );
            gCopy( b, 2, 0, g, n.img.width() - 3, 0, b.img.width() - 2, b.img.height() );
            g.x = n.x;
            g.y = n.y;
            gShadow( g, 2, false );
        }
    }

    // Small font (port of the SMALFONT part of generateCP1251Alphabet).
    {
        // ' (0x92).
        _small[114] = _small[',' - 0x20];
        _small[114].y -= 4;

        // Yo (0xA8=168).
        {
            const Glyph & E = _small['E' - 0x20];
            Glyph & g = _small[136];
            gResize( g, E.img.width(), E.img.height() + 2 );
            gCopy( E, 0, 0, g, 0, 2, E.img.width(), E.img.height() );
            gCopy( E, 3, 0, g, 3, 0, 1, 1 );
            gCopy( E, 3, 0, g, 5, 0, 1, 1 );
            g.x = E.x;
            g.y = E.y - 2;
            gShadow( g, 1, true );
        }

        // NBSP.
        gResize( _small[128], 4, 1 );

        // Ў (0xA1=161).
        {
            const Glyph & Y = _small['Y' - 0x20];
            Glyph & g = _small[129];
            gResize( g, Y.img.width(), Y.img.height() + 2 );
            gCopy( Y, 0, 0, g, 0, 2, Y.img.width(), Y.img.height() );
            gCopy( Y, 2, 0, g, 5, 0, 2, 1 );
            g.x = Y.x;
            g.y = Y.y - 2;
            gShadow( g, 1, true );
        }

        // І (0xA2=162).
        {
            const Glyph & y = _small['y' - 0x20];
            Glyph & g = _small[130];
            gResize( g, y.img.width(), y.img.height() + 2 );
            gCopy( y, 0, 0, g, 0, 2, y.img.width(), y.img.height() );
            gCopy( y, 1, 0, g, 4, 0, 2, 1 );
            g.x = y.x;
            g.y = y.y - 2;
            gShadow( g, 1, true );
        }

        // Є (0xAA=170).
        {
            Glyph & g = _small[138];
            g = _small['C' - 0x20];
            gCopy( g, 3, 0, g, 2, 3, 3, 1 );
            gShadow( g, 1, true );
        }

        // є (0xBA=186).
        {
            Glyph & g = _small[154];
            g = _small['c' - 0x20];
            gCopy( g, 2, 0, g, 2, 2, 2, 1 );
            gShadow( g, 1, true );
        }

        // Ї (0xAF=175).
        {
            Glyph & g = _small[143];
            g = _small['I' - 0x20];
        }

        // ї (0xBF=191).
        {
            Glyph & g = _small[159];
            g = _small['i' - 0x20];
            gCopy( g, 1, 0, g, 0, 0, 2, 2 );
            gCopy( g, 1, 0, g, 2, 0, 2, 2 );
        }

        // J and j.
        _small[131] = _small['J' - 0x20];
        _small[156] = _small['j' - 0x20];

        // S and s.
        _small[157] = _small['S' - 0x20];
        _small[158] = _small['s' - 0x20];

        // I and i.
        _small[146] = _small['I' - 0x20];
        _small[147] = _small['i' - 0x20];

        // A.
        _small[160] = _small['A' - 0x20];

        // Be (0xC1=193).
        {
            const Glyph & B = _small['B' - 0x20];
            Glyph & g = _small[161];
            g = B;
            gErase( g, 5, 1, 2, 2 );
            gCopy( g, 5, 0, g, 6, 0, 1, 1 );
            gShadow( g, 1, true );
        }

        // Ve.
        _small[162] = _small['B' - 0x20];

        // Ghe (0xC3=195).
        {
            const Glyph & B = _small[161];
            Glyph & g = _small[163];
            gResize( g, B.img.width() + 1, B.img.height() );
            gCopy( B, 0, 0, g, 0, 0, 4, 8 );
            gCopy( B, 3, 0, g, 4, 0, 4, 1 );
            g.x = B.x;
            g.y = B.y;
            gShadow( g, 1, true );
        }

        // Ґ (0xA5=165).
        {
            const Glyph & G = _small[163];
            Glyph & g = _small[133];
            gResize( g, G.img.width() - 1, G.img.height() + 1 );
            gCopy( G, 0, 0, g, 0, 1, G.img.width() - 1, G.img.height() );
            gCopy( G, 2, 0, g, 6, 0, 1, 1 );
            gErase( g, 6, 2, 1, 1 );
            g.x = G.x;
            g.y = G.y - 1;
            gShadow( g, 1, true );
        }

        // De.
        _small[164] = _small['D' - 0x20];

        // Ye.
        _small[165] = _small['E' - 0x20];

        // Zhe (0xC6=198).
        {
            const Glyph & X = _small['X' - 0x20];
            Glyph & g = _small[166];
            gResize( g, X.img.width() + 1, X.img.height() );
            gCopy( X, 1, 0, g, 1, 0, 3, 7 );
            gCopy( X, 7, 0, g, 7, 0, 2, 7 );
            gCopy( X, 4, 2, g, 3, 2, 1, 3 );
            gCopy( X, 6, 2, g, 7, 2, 1, 3 );
            gCopy( _small['E' - 0x20], 4, 5, g, 4, 2, 3, 3 );
            gCopy( _small['E' - 0x20], 3, 0, g, 5, 0, 1, 7 );
            gCopy( X, 8, 0, g, 9, 0, 1, 7 );
            g.x = X.x;
            g.y = X.y;
            gShadow( g, 1, true );
        }

        // Ze (0xC7=199).
        {
            const Glyph & three = _small['3' - 0x20];
            Glyph & g = _small[167];
            gResize( g, three.img.width() + 2, three.img.height() );
            gCopy( three, 1, 0, g, 1, 0, 3, 2 );
            gCopy( three, 2, 0, g, 4, 0, 3, 2 );
            gCopy( three, 2, 2, g, 3, 2, 3, 3 );
            gCopy( three, 2, 5, g, 4, 5, 3, 2 );
            gCopy( three, 1, 5, g, 1, 5, 3, 2 );
            gErase( g, 2, 4, 3, 2 );
            gErase( g, 5, 5, 1, 1 );
            gErase( g, 4, 2, 1, 1 );
            g.x = three.x;
            g.y = three.y;
            gShadow( g, 1, true );
        }

        // I (0xC8=200): H with a diagonal.
        {
            const Glyph & H = _small['H' - 0x20];
            Glyph & g = _small[168];
            g = H;
            gErase( g, 4, 2, 3, 4 );
            gCopy( H, 3, 0, g, 4, 4, 1, 1 );
            gCopy( H, 3, 0, g, 5, 3, 1, 1 );
            gCopy( H, 3, 0, g, 6, 2, 1, 1 );
            gShadow( g, 1, true );
        }

        // Short I (0xC9=201).
        {
            const Glyph & I = _small[168];
            Glyph & g = _small[169];
            gResize( g, I.img.width(), I.img.height() + 2 );
            gCopy( I, 1, 0, g, 1, 2, 8, 7 );
            gCopy( I, 2, 0, g, 5, 0, 2, 1 );
            g.x = I.x;
            g.y = I.y - 2;
            gShadow( g, 1, true );
        }

        // Ka.
        _small[170] = _small['K' - 0x20];

        // El (0xCB=203).
        {
            const Glyph & B = _small['B' - 0x20];
            Glyph & g = _small[171];
            gResize( g, B.img.width(), B.img.height() );
            gCopy( B, 1, 0, g, 1, 0, 3, 7 );
            gCopy( B, 3, 0, g, 6, 0, 1, 7 );
            gCopy( B, 3, 0, g, 4, 0, 2, 1 );
            gErase( g, 1, 0, 2, 2 );
            gErase( g, 3, 0, 1, 1 );
            g.x = B.x;
            g.y = B.y;
            gShadow( g, 1, true );
        }

        // Em, En, O.
        _small[172] = _small['M' - 0x20];
        _small[173] = _small['H' - 0x20];
        _small[174] = _small['O' - 0x20];

        // Pe (0xCF=207).
        {
            Glyph & g = _small[175];
            g = _small[163];
            gCopy( g, 3, 0, g, 6, 0, 1, 7 );
            gShadow( g, 1, true );
        }

        // Er, Es.
        _small[176] = _small['P' - 0x20];
        _small[177] = _small['C' - 0x20];

        // Te (0xD2=210).
        {
            const Glyph & P = _small[175];
            Glyph & g = _small[178];
            gResize( g, P.img.width() + 2, P.img.height() );
            gCopy( P, 0, 0, g, 0, 0, P.img.width(), P.img.height() );
            gCopy( g, 5, 0, g, 8, 0, 2, 8 );
            g.x = P.x;
            g.y = P.y;
        }

        // U, Ha.
        _small[179] = _small['Y' - 0x20];
        _small[181] = _small['X' - 0x20];

        // Tse (0xD6=214).
        {
            const Glyph & U = _small['U' - 0x20];
            Glyph & g = _small[182];
            gResize( g, U.img.width(), U.img.height() + 1 );
            gCopy( U, 1, 0, g, 1, 0, 8, 7 );
            gCopy( g, 3, 0, g, 9, 5, 1, 3 );
            g.x = U.x;
            g.y = U.y;
            gShadow( g, 1, true );
        }

        // Che (0xD7=215).
        {
            Glyph & g = _small[183];
            g = _small['U' - 0x20];
            gCopy( g, 3, 5, g, 3, 2, 4, 2 );
            gErase( g, 2, 4, 5, 4 );
            gShadow( g, 1, true );
        }

        // Sha (0xD8=216).
        {
            const Glyph & U = _small['U' - 0x20];
            Glyph & g = _small[184];
            gResize( g, U.img.width(), U.img.height() );
            gCopy( U, 1, 0, g, 1, 0, 4, 7 );
            gCopy( U, 7, 1, g, 6, 1, 2, 6 );
            gCopy( _small['L' - 0x20], 3, 0, g, 9, 0, 1, 8 );
            gCopy( U, 7, 1, g, 5, 5, 1, 1 );
            gCopy( U, 7, 1, g, 8, 5, 1, 1 );
            gCopy( U, 7, 1, g, 6, 0, 1, 1 );
            g.x = U.x;
            g.y = U.y;
            gShadow( g, 1, true );
        }

        // Shcha (0xD9=217).
        {
            const Glyph & Sh = _small[184];
            Glyph & g = _small[185];
            gResize( g, Sh.img.width() + 2, Sh.img.height() + 1 );
            gCopy( Sh, 1, 0, g, 1, 0, 9, 7 );
            gCopy( Sh, 3, 0, g, 10, 6, 1, 1 );
            gCopy( Sh, 3, 0, g, 11, 5, 1, 3 );
            g.x = Sh.x;
            g.y = Sh.y;
            gShadow( g, 1, true );
        }

        // Soft sign (0xDC=220).
        {
            const Glyph & B = _small['B' - 0x20];
            Glyph & g = _small[188];
            gResize( g, B.img.width(), B.img.height() );
            gCopy( B, 2, 0, g, 1, 0, 2, 7 );
            gCopy( B, 4, 3, g, 3, 3, 1, 4 );
            gCopy( B, 4, 3, g, 4, 3, 3, 4 );
            gErase( g, 1, 0, 1, 1 );
            g.x = B.x;
            g.y = B.y;
            gShadow( g, 1, true );
        }

        // Yeru (0xDB=219).
        {
            const Glyph & B = _small[188];
            Glyph & g = _small[187];
            gResize( g, B.img.width() + 2, B.img.height() );
            gCopy( B, 1, 0, g, 1, 0, 6, 7 );
            gCopy( B, 2, 0, g, 8, 0, 1, 7 );
            g.x = B.x;
            g.y = B.y;
            gShadow( g, 1, true );
        }

        // Hard sign (0xDA=218).
        {
            const Glyph & B = _small[188];
            Glyph & g = _small[186];
            gResize( g, B.img.width() + 2, B.img.height() );
            gCopy( B, 1, 0, g, 3, 0, 6, 7 );
            gCopy( B, 2, 3, g, 1, 0, 3, 1 );
            gCopy( B, 2, 3, g, 1, 1, 1, 1 );
            g.x = B.x;
            g.y = B.y;
            gShadow( g, 1, true );
        }

        // E (0xDD=221).
        {
            const Glyph & O = _small['O' - 0x20];
            Glyph & g = _small[189];
            gResize( g, O.img.width() - 1, O.img.height() );
            gCopy( O, 2, 0, g, 1, 0, 6, 7 );
            gCopy( O, 3, 0, g, 4, 3, 2, 1 );
            g.x = O.x;
            g.y = O.y;
            gShadow( g, 1, true );
        }

        // Yu (0xDE=222).
        {
            const Glyph & L = _small['L' - 0x20];
            const Glyph & O = _small['O' - 0x20];
            Glyph & g = _small[190];
            gResize( g, O.img.width() + 1, O.img.height() );
            gCopy( L, 2, 0, g, 1, 0, 2, 7 );
            gCopy( O, 2, 0, g, 4, 0, 5, 2 );
            gCopy( O, 2, 5, g, 4, 5, 5, 2 );
            gCopy( g, 1, 0, g, 3, 3, 1, 1 );
            gCopy( g, 2, 0, g, 4, 2, 1, 3 );
            gCopy( g, 2, 0, g, 8, 2, 1, 3 );
            g.x = O.x;
            g.y = O.y;
            gShadow( g, 1, true );
        }

        // Ya (0xDF=223).
        {
            const Glyph & P = _small['P' - 0x20];
            Glyph & g = _small[191];
            gResize( g, P.img.width() - 1, P.img.height() );
            gCopy( P, 7, 1, g, 2, 1, 1, 2 );
            gCopy( P, 2, 0, g, 3, 0, 3, 1 );
            gCopy( P, 2, 0, g, 3, 3, 3, 1 );
            gCopy( _small['L' - 0x20], 3, 0, g, 6, 0, 1, 7 );
            gCopy( _small['A' - 0x20], 1, 6, g, 1, 6, 2, 1 );
            gCopy( _small['A' - 0x20], 1, 6, g, 3, 5, 1, 1 );
            gCopy( _small['A' - 0x20], 1, 6, g, 4, 4, 1, 1 );
            g.x = P.x;
            g.y = P.y;
            gShadow( g, 1, true );
        }

        // --- lowercase ---

        // yo (0xB8=184).
        {
            const Glyph & e = _small['e' - 0x20];
            Glyph & g = _small[152];
            gResize( g, e.img.width(), e.img.height() + 2 );
            gCopy( e, 0, 0, g, 0, 2, e.img.width(), e.img.height() );
            gCopy( e, 2, 0, g, 2, 0, 1, 1 );
            gCopy( e, 2, 0, g, 4, 0, 1, 1 );
            g.x = e.x;
            g.y = e.y - 2;
            gShadow( g, 1, true );
        }

        // a.
        _small[192] = _small['a' - 0x20];

        // be (0xE1=225).
        {
            const Glyph & B = _small['B' - 0x20];
            Glyph & g = _small[193];
            gResize( g, B.img.width(), B.img.height() );
            gCopy( B, 4, 3, g, 4, 3, 3, 4 );
            gCopy( _small['a' - 0x20], 1, 2, g, 2, 4, 2, 3 );
            gErase( g, 3, 5, 1, 1 );
            gCopy( g, 2, 5, g, 2, 1, 2, 2 );
            gCopy( _small['E' - 0x20], 2, 0, g, 2, 0, 5, 1 );
            g.x = B.x;
            g.y = B.y;
            gShadow( g, 1, true );
        }

        // ve (0xE2=226).
        {
            const Glyph & r = _small['r' - 0x20];
            Glyph & g = _small[194];
            gResize( g, r.img.width() - 1, r.img.height() );
            gCopy( r, 1, 0, g, 1, 0, 2, 5 );
            gCopy( r, 1, 0, g, 3, 0, 2, 1 );
            gCopy( r, 1, 0, g, 3, 2, 2, 1 );
            gCopy( r, 1, 0, g, 3, 4, 2, 1 );
            gCopy( r, 1, 0, g, 5, 1, 1, 1 );
            gCopy( r, 1, 0, g, 5, 3, 1, 1 );
            g.x = r.x;
            g.y = r.y;
            gShadow( g, 1, true );
        }

        // ghe (0xE3=227).
        {
            Glyph & g = _small[195];
            g = _small['r' - 0x20];
            gCopy( g, 3, 1, g, 3, 0, 1, 1 );
            gErase( g, 3, 1, 1, 1 );
            gShadow( g, 1, true );
        }

        // ґ (0xB4=180).
        {
            const Glyph & g2 = _small[195];
            Glyph & g = _small[148];
            gResize( g, g2.img.width() - 1, g2.img.height() + 1 );
            gCopy( g2, 0, 0, g, 0, 1, g2.img.width() - 1, g2.img.height() );
            gCopy( g2, 5, 0, g, 5, 0, 1, 1 );
            gErase( g, 5, 2, 1, 1 );
            g.x = g2.x;
            g.y = g2.y - 1;
            gShadow( g, 1, true );
        }

        // de.
        _small[196] = _small['g' - 0x20];

        // ye.
        _small[197] = _small['e' - 0x20];

        // zhe (0xE6=230).
        {
            const Glyph & x = _small['x' - 0x20];
            Glyph & g = _small[198];
            gResize( g, x.img.width() + 1, x.img.height() );
            gCopy( x, 0, 0, g, 0, 0, 4, 5 );
            gCopy( x, 4, 0, g, 5, 0, 4, 5 );
            gCopy( _small['u' - 0x20], 2, 0, g, 4, 0, 1, 4 );
            gCopy( _small['u' - 0x20], 2, 0, g, 4, 4, 1, 1 );
            g.x = x.x;
            g.y = x.y;
            gShadow( g, 1, true );
        }

        // i (0xE8=232).
        _small[200] = _small['u' - 0x20];

        // short i (0xE9=233).
        {
            const Glyph & u = _small[200];
            Glyph & g = _small[201];
            gResize( g, u.img.width(), u.img.height() + 2 );
            gCopy( u, 1, 0, g, 1, 2, 7, 5 );
            gCopy( u, 1, 0, g, 3, 0, 2, 1 );
            g.x = u.x;
            g.y = u.y - 2;
            gShadow( g, 1, true );
        }

        // ka (0xEA=234).
        {
            const Glyph & k = _small['k' - 0x20];
            Glyph & g = _small[202];
            gResize( g, k.img.width(), k.img.height() - 2 );
            gCopy( k, 1, 0, g, 1, 0, 2, 5 );
            gCopy( k, 3, 2, g, 3, 0, 3, 5 );
            g.x = k.x;
            g.y = k.y + 2;
            gShadow( g, 1, true );
        }

        // el (0xEB=235).
        {
            const Glyph & a = _small['a' - 0x20];
            Glyph & g = _small[203];
            gResize( g, a.img.width() - 2, a.img.height() );
            gCopy( _small[171], 2, 3, g, 1, 1, 2, 4 );
            gCopy( _small[171], 5, 0, g, 3, 0, 2, 5 );
            gErase( g, 1, 1, 1, 1 );
            g.x = a.x;
            g.y = a.y;
            gShadow( g, 1, true );
        }

        // em (0xEC=236).
        {
            const Glyph & l = _small[203];
            Glyph & g = _small[204];
            gResize( g, l.img.width() + 3, l.img.height() );
            gCopy( l, 4, 0, g, 4, 0, 1, 5 );
            gCopy( l, 1, 0, g, 1, 1, 3, 3 );
            gCopy( g, 2, 0, g, 5, 0, 3, 5 );
            gCopy( g, 4, 0, g, 1, 4, 1, 1 );
            g.x = l.x;
            g.y = l.y;
            gShadow( g, 1, true );
        }

        // en (0xED=237).
        {
            const Glyph & h = _small['h' - 0x20];
            Glyph & g = _small[205];
            gResize( g, h.img.width() - 2, h.img.height() - 2 );
            gCopy( h, 1, 0, g, 1, 0, 2, 5 );
            gCopy( h, 2, 0, g, 5, 0, 1, 5 );
            gCopy( h, 3, 2, g, 3, 2, 2, 1 );
            g.x = h.x;
            g.y = h.y + 2;
            gShadow( g, 1, true );
        }

        // o, pe.
        _small[206] = _small['o' - 0x20];
        _small[207] = _small['n' - 0x20];

        // er, es.
        _small[208] = _small['p' - 0x20];
        _small[209] = _small['c' - 0x20];

        // te (0xF2=242).
        {
            const Glyph & m = _small['m' - 0x20];
            Glyph & g = _small[210];
            gResize( g, m.img.width() - 4, m.img.height() );
            gCopy( m, 1, 0, g, 1, 0, 2, 5 );
            gCopy( m, 6, 1, g, 4, 1, 1, 4 );
            gCopy( m, 10, 1, g, 6, 1, 2, 4 );
            gCopy( m, 3, 0, g, 2, 0, 3, 1 );
            gCopy( m, 3, 0, g, 5, 0, 1, 1 );
            g.x = m.x;
            g.y = m.y;
            gShadow( g, 1, true );
        }

        // u.
        _small[211] = _small['y' - 0x20];

        // ef (0xF4=244).
        {
            const Glyph & q = _small['q' - 0x20];
            Glyph & g = _small[212];
            gResize( g, q.img.width() + 3, q.img.height() + 1 );
            gCopy( q, 1, 0, g, 1, 0, 5, 7 );
            gCopy( q, 2, 0, g, 6, 0, 4, 4 );
            gCopy( q, 2, 4, g, 6, 4, 3, 1 );
            gCopy( q, 2, 4, g, 5, 7, 1, 1 );
            g.x = q.x;
            g.y = q.y;
            gShadow( g, 1, true );
        }

        // Capital ef (0xD4=212): a bigger variant.
        {
            Glyph & g = _small[180];
            g = _small[212];
            gCopy( g, 5, 1, g, 5, 0, 1, 1 );
            gCopy( g, 5, 1, g, 4, 7, 1, 1 );
            g.x = _small['P' - 0x20].x;
            g.y = _small['P' - 0x20].y;
            gShadow( g, 1, true );
        }

        // ha.
        _small[213] = _small['x' - 0x20];

        // tse (0xF6=246).
        {
            const Glyph & u = _small['u' - 0x20];
            Glyph & g = _small[214];
            gResize( g, u.img.width() + 1, u.img.height() + 1 );
            gCopy( u, 0, 0, g, 0, 0, u.img.width(), u.img.height() );
            gCopy( g, 2, 0, g, 8, 3, 1, 3 );
            g.x = u.x;
            g.y = u.y;
            gShadow( g, 1, true );
        }

        // che (0xF7=247).
        {
            Glyph & g = _small[215];
            g = _small['u' - 0x20];
            gCopy( g, 2, 4, g, 2, 2, 4, 1 );
            gCopy( g, 1, 0, g, 6, 4, 1, 1 );
            gErase( g, 1, 3, 5, 3 );
            gShadow( g, 1, true );
        }

        // sha (0xF8=248).
        {
            const Glyph & u = _small['u' - 0x20];
            Glyph & g = _small[216];
            gResize( g, u.img.width() + 2, u.img.height() );
            gCopy( u, 1, 0, g, 1, 0, 3, 5 );
            gCopy( u, 6, 0, g, 5, 0, 2, 5 );
            gCopy( u, 6, 0, g, 8, 0, 2, 5 );
            gCopy( u, 1, 0, g, 4, 4, 1, 1 );
            gCopy( u, 1, 0, g, 7, 4, 1, 1 );
            g.x = u.x;
            g.y = u.y;
            gShadow( g, 1, true );
        }

        // shcha (0xF9=249).
        {
            const Glyph & sh = _small[216];
            Glyph & g = _small[217];
            gResize( g, sh.img.width() + 1, sh.img.height() );
            gCopy( sh, 1, 0, g, 1, 0, 9, 5 );
            gCopy( sh, 2, 0, g, 10, 3, 1, 3 );
            g.x = sh.x;
            g.y = sh.y;
            gShadow( g, 1, true );
        }

        // soft sign (0xFC=252).
        {
            Glyph & g = _small[220];
            g = _small[194];
            gCopy( g, 1, 0, g, 5, 4, 1, 1 );
            gErase( g, 0, 0, 2, 2 );
            gErase( g, 3, 0, 3, 2 );
        }

        // hard sign (0xFA=250).
        {
            const Glyph & b = _small[220];
            Glyph & g = _small[218];
            gResize( g, b.img.width() + 1, b.img.height() );
            gCopy( b, 1, 0, g, 2, 0, 5, 5 );
            gCopy( b, 2, 2, g, 1, 0, 2, 1 );
            gCopy( b, 2, 2, g, 1, 1, 1, 1 );
            g.x = b.x;
            g.y = b.y;
            gShadow( g, 1, true );
        }

        // yeru (0xFB=251).
        {
            const Glyph & b = _small[220];
            Glyph & g = _small[219];
            gResize( g, b.img.width() + 2, b.img.height() );
            gCopy( b, 1, 0, g, 1, 0, 5, 5 );
            gCopy( b, 2, 0, g, 7, 0, 1, 5 );
            g.x = b.x;
            g.y = b.y;
            gShadow( g, 1, true );
        }

        // e (0xFD=253).
        {
            const Glyph & o = _small['o' - 0x20];
            Glyph & g = _small[221];
            gResize( g, o.img.width() - 1, o.img.height() );
            gCopy( o, 2, 0, g, 1, 0, 4, 5 );
            gCopy( o, 2, 0, g, 2, 2, 2, 1 );
            g.x = o.x;
            g.y = o.y;
            gShadow( g, 1, true );
        }

        // ze (0xE7=231) — built from e.
        {
            Glyph & g = _small[199];
            g = _small[221];
            gErase( g, 0, 1, 3, 3 );
            gErase( g, 4, 2, 1, 1 );
            gErase( g, 1, 0, 1, 1 );
            gCopy( _small[221], 1, 0, g, 1, 1, 1, 1 );
            gShadow( g, 1, true );
        }

        // yu (0xFE=254).
        {
            const Glyph & o = _small['o' - 0x20];
            Glyph & g = _small[222];
            gResize( g, o.img.width() + 2, o.img.height() );
            gCopy( _small['r' - 0x20], 1, 0, g, 1, 0, 2, 5 );
            gCopy( o, 1, 0, g, 4, 0, 3, 5 );
            gCopy( o, 5, 1, g, 7, 1, 1, 3 );
            gCopy( o, 5, 1, g, 3, 2, 1, 1 );
            g.x = o.x;
            g.y = o.y;
            gShadow( g, 1, true );
        }

        // ya (0xFF=255).
        {
            const Glyph & a = _small['a' - 0x20];
            Glyph & g = _small[223];
            gResize( g, a.img.width() - 1, a.img.height() );
            gCopy( a, 1, 2, g, 2, 0, 3, 3 );
            gCopy( _small[203], 4, 0, g, 5, 0, 1, 5 );
            gCopy( _small['A' - 0x20], 1, 5, g, 1, 3, 3, 2 );
            g.x = a.x;
            g.y = a.y;
            gShadow( g, 1, true );
        }

        // Љ (0x8A=138).
        {
            const Glyph & L = _small[171];
            const Glyph & B = _small[188];
            Glyph & g = _small[106];
            gResize( g, L.img.width() + B.img.width() - 4, L.img.height() );
            gCopy( L, 0, 0, g, 0, 0, L.img.width(), L.img.height() );
            gCopy( B, 2, 0, g, L.img.width() - 2, 0, B.img.width() - 2, B.img.height() );
            g.x = L.x;
            g.y = L.y;
            gShadow( g, 1, true );
        }

        // Њ (0x8C=140).
        {
            const Glyph & N = _small[173];
            const Glyph & B = _small[188];
            Glyph & g = _small[108];
            gResize( g, N.img.width() + B.img.width() - 4, N.img.height() );
            gCopy( N, 0, 0, g, 0, 0, N.img.width(), N.img.height() );
            gCopy( B, 2, 0, g, N.img.width() - 2, 0, B.img.width() - 2, B.img.height() );
            g.x = N.x;
            g.y = N.y;
            gShadow( g, 1, true );
        }

        // љ (0x9A=154).
        {
            const Glyph & l = _small[203];
            const Glyph & b = _small[220];
            Glyph & g = _small[122];
            gResize( g, l.img.width() + b.img.width() - 3, l.img.height() );
            gCopy( l, 0, 0, g, 0, 0, l.img.width(), l.img.height() );
            gCopy( b, 2, 0, g, l.img.width() - 1, 0, b.img.width() - 2, b.img.height() );
            g.x = l.x;
            g.y = l.y;
            gShadow( g, 1, true );
        }

        // њ (0x9C=156).
        {
            const Glyph & n = _small[205];
            const Glyph & b = _small[220];
            Glyph & g = _small[124];
            gResize( g, n.img.width() + b.img.width() - 3, n.img.height() );
            gCopy( n, 0, 0, g, 0, 0, n.img.width(), n.img.height() );
            gCopy( b, 2, 0, g, n.img.width() - 1, 0, b.img.width() - 2, b.img.height() );
            g.x = n.x;
            g.y = n.y;
            gShadow( g, 1, true );
        }
    }

    _valid = true;
}

GameFont::Glyph GameFont::glyph( Size size, unsigned char code ) const
{
    if ( code < 0x20 )
        return {};
    const Glyph & g = size == Size::SMALL ? _small[code - 0x20] : _normal[code - 0x20];
    if ( g.empty ) {
        // For cp1251 codes above the ASCII range the array index is code - 0x20,
        // but they are stored as charcode-0x20 (see the fheroes2 rearrange).
        // Latin cp1251 letters (0xC0-0xFF) are already generated at those indices.
        return {};
    }
    return g;
}

int GameFont::textWidth( const QString & text, Size size ) const
{
    if ( !_valid || text.isEmpty() )
        return 0;
    const std::string s = encodeCp1251( transliterateLatin1( text ) );
    const int space = size == Size::SMALL ? 4 : 6;
    int width = 0;
    for ( unsigned char c : s ) {
        if ( c == ' ' ) {
            width += space;
            continue;
        }
        const Glyph & g = glyph( size, c );
        width += g.empty ? 0 : g.img.width() + g.x;
    }
    return width;
}

int GameFont::lineHeight( Size size ) const
{
    switch ( size ) {
    case Size::SMALL:
        return 11;
    default:
        return 17;
    }
}

void GameFont::drawGlyph( QPainter & p, int x, int y, const Glyph & g, Color color ) const
{
    if ( g.empty || g.img.isNull() )
        return;
    const QImage img = tinted( g.img, g.idx, *_assets, color );
    p.drawImage( x, y, img );
}

void GameFont::drawText( QPainter & p, int x, int y, const QString & text, Size size, Color color, int maxWidth ) const
{
    QString t = transliterateLatin1( text );
    if ( maxWidth > 0 && textWidth( t, size ) > maxWidth )
        t = elideText( t, size, maxWidth );
    if ( t.isEmpty() )
        return;
    const std::string s = encodeCp1251( t );
    const int space = size == Size::SMALL ? 4 : 6;
    int offsetX = x;
    for ( unsigned char c : s ) {
        if ( c == ' ' ) {
            offsetX += space;
            continue;
        }
        const Glyph & g = glyph( size, c );
        if ( g.empty )
            continue;
        const QImage img = tinted( g.img, g.idx, *_assets, color );
        p.drawImage( offsetX + g.x, y + g.y, img );
        offsetX += g.img.width() + g.x;
    }
}

// Splits text into lines: '\n' — a hard break, the rest is wrapped by
// words within maxWidth (like getMultiRowInfo in ui_text.cpp).
QStringList GameFont::wrapLines( const QString & text, Size size, int maxWidth ) const
{
    QStringList lines;
    const QStringList paragraphs = text.split( QLatin1Char( '\n' ) );
    for ( const QString & paragraph : paragraphs ) {
        if ( maxWidth <= 0 || textWidth( paragraph, size ) <= maxWidth ) {
            lines.push_back( paragraph );
            continue;
        }
        QString line;
        const QStringList words = paragraph.split( QLatin1Char( ' ' ) );
        for ( const QString & word : words ) {
            const QString candidate = line.isEmpty() ? word : line + QLatin1Char( ' ' ) + word;
            if ( !line.isEmpty() && textWidth( candidate, size ) > maxWidth ) {
                lines.push_back( line );
                line = word;
            }
            else {
                line = candidate;
            }
        }
        lines.push_back( line );
    }
    return lines;
}

int GameFont::textRows( const QString & text, Size size, int maxWidth ) const
{
    if ( !_valid || text.isEmpty() )
        return 0;
    return static_cast<int>( wrapLines( text, size, maxWidth ).size() );
}

int GameFont::drawTextWrapped( QPainter & p, int x, int y, int maxWidth, const QString & text, Size size, Color color ) const
{
    if ( !_valid || text.isEmpty() )
        return 0;
    const QStringList lines = wrapLines( text, size, maxWidth );
    const int step = lineHeight( size );
    int offsetY = y;
    for ( const QString & line : lines ) {
        // Lines of multiline text are centered by maxWidth (ui_text.cpp:486).
        const int lineWidth = textWidth( line, size );
        drawText( p, x + ( maxWidth - lineWidth ) / 2, offsetY, line, size, color );
        offsetY += step;
    }
    return static_cast<int>( lines.size() );
}

QString GameFont::elideText( const QString & text, Size size, int maxWidth ) const
{
    if ( textWidth( text, size ) <= maxWidth )
        return text;
    const QString dots = QStringLiteral( "..." );
    const int dotsW = textWidth( dots, size );
    QString out;
    for ( int i = 0; i < text.size(); ++i ) {
        const QString candidate = out + text.at( i );
        if ( textWidth( candidate, size ) + dotsW > maxWidth )
            break;
        out = candidate;
    }
    return out + dots;
}

} // namespace fh2
