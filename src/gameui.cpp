#include "gameui.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>

#include "assets.h"
#include "buttonfont.h"
#include "constants.h"
#include "textutil.h"

namespace fh2 {

namespace {

constexpr int BORDER_SIZE = 16; // borderWidthPx in fheroes2

QPixmap alphaScaled( const QPixmap & pm, QSize size )
{
    if ( pm.isNull() || size.isEmpty() )
        return {};
    return pm.scaled( size, Qt::IgnoreAspectRatio, Qt::FastTransformation );
}

// Darkening by transform tables 2..5 (like fheroes2 ApplyTransform).
QColor darkenByTransform( QColor c, int transformId )
{
    const double k = 1.15 - 0.17 * transformId; // t=2: 0.81, t=3: 0.64, t=4: 0.47, t=5: 0.30
    return QColor( static_cast<int>( c.red() * k ), static_cast<int>( c.green() * k ), static_cast<int>( c.blue() * k ), c.alpha() );
}

} // namespace

QImage grabBackdrop( QWidget * overlay )
{
    QWidget * parent = overlay ? overlay->parentWidget() : nullptr;
    if ( parent == nullptr || parent->size().isEmpty() )
        return {};

    const bool wasVisible = overlay->isVisible();
    if ( wasVisible )
        overlay->hide();
    const QPixmap shot = parent->grab();
    if ( wasVisible )
        overlay->show();

    return shot.toImage().convertToFormat( QImage::Format_RGBA8888 );
}

void addSilhouetteShadow( QImage & out, const QPoint & outPos, const QImage & in, const QPoint & shadowOffset, const int transformId )
{
    if ( out.isNull() || in.isNull() || shadowOffset.x() > 0 || shadowOffset.y() < 0 )
        return;

    for ( int y = 0; y < in.height(); ++y ) {
        for ( int x = 0; x < in.width(); ++x ) {
            if ( in.pixelColor( x, y ).alpha() == 0 )
                continue;
            const int outX = outPos.x() + shadowOffset.x() + x;
            const int outY = outPos.y() + shadowOffset.y() + y;
            if ( outX < 0 || outY < 0 || outX >= out.width() || outY >= out.height() )
                continue;
            const QColor c = out.pixelColor( outX, outY );
            if ( c.alpha() == 0 )
                continue;
            out.setPixelColor( outX, outY, darkenByTransform( c, transformId ) );
        }
    }
}

void drawFogBackground( QPainter & p, const Assets * assets, const QSize & size, const QPoint & origin )
{
    if ( size.isEmpty() )
        return;

    const auto floorDiv = []( int a, int b ) { return a >= 0 ? a / b : -( ( -a + b - 1 ) / b ); };
    const auto mod = []( int a, int b ) { return ( a % b + b ) % b; };

    if ( assets ) {
        QPixmap tiles[FOG_TILE_VARIANTS];
        bool haveTiles = true;
        for ( int i = 0; i < FOG_TILE_VARIANTS; ++i ) {
            tiles[i] = assets->tilPixmap( TIL_CLOF32, i );
            if ( tiles[i].isNull() )
                haveTiles = false;
        }

        if ( haveTiles ) {
            const int ts = FOG_TILE_SIZE;
            const int startX = -mod( origin.x(), ts );
            const int startY = -mod( origin.y(), ts );
            for ( int y = startY; y < size.height(); y += ts ) {
                const int ty = floorDiv( origin.y() + y, ts );
                for ( int x = startX; x < size.width(); x += ts ) {
                    const int tx = floorDiv( origin.x() + x, ts );
                    p.drawPixmap( x, y, tiles[mod( tx + ty, FOG_TILE_VARIANTS )] );
                }
            }
            return;
        }

        // Fallback: stone background if the TIL is unavailable (old behavior).
        const QPixmap stone = assets->icnPixmap( ICN_STONEBAK, 0 );
        if ( !stone.isNull() && stone.width() > 0 && stone.height() > 0 ) {
            const int startX = -mod( origin.x(), stone.width() );
            const int startY = -mod( origin.y(), stone.height() );
            for ( int y = startY; y < size.height(); y += stone.height() ) {
                for ( int x = startX; x < size.width(); x += stone.width() )
                    p.drawPixmap( x, y, stone );
            }
            return;
        }
    }

    p.fillRect( QRect( QPoint( 0, 0 ), size ), QColor( "#7a6a5a" ) );
}

void addGradientShadow( QImage & out, const QPoint & outPos, const QImage & in, const QPoint & shadowOffset )
{
    // Port of fheroes2::addGradientShadow (src/engine/image.cpp).
    if ( in.isNull() || out.isNull() || ( shadowOffset.x() == 0 && shadowOffset.y() == 0 ) || outPos.x() < 0 || outPos.y() < 0 )
        return;
    const int outWidth = out.width();
    int inWidth = in.width();
    if ( outWidth < ( inWidth + outPos.x() + std::max( shadowOffset.x(), 0 ) ) )
        inWidth = outWidth - ( outPos.x() + std::max( shadowOffset.x(), 0 ) );

    int inHeight = in.height();
    if ( out.height() < ( inHeight + outPos.y() + std::max( shadowOffset.y(), 0 ) ) )
        inHeight = out.height() - ( outPos.y() + std::max( shadowOffset.y(), 0 ) );

    if ( inWidth <= 1 || inHeight <= 1 )
        return;

    const int shadowOffsetX = std::min( shadowOffset.x(), 0 );
    const int shadowOffsetY = std::min( shadowOffset.y(), 0 );
    const int startOffsetX = outPos.x() + shadowOffsetX;
    const int startOffsetY = outPos.y() + shadowOffsetY;

    const int absOffsetX = std::abs( shadowOffset.x() );
    const int absOffsetY = std::abs( shadowOffset.y() );

    // Shadow line (like in the engine).
    std::vector<QPoint> shadowLine;
    shadowLine.reserve( std::max( absOffsetX, absOffsetY ) + 1 );
    if ( shadowOffset.x() == 0 ) {
        for ( int y = shadowOffsetY; y <= absOffsetY + shadowOffsetY; ++y )
            shadowLine.emplace_back( 0, y );
    }
    else {
        const double slopeFactor = static_cast<double>( shadowOffset.y() ) / shadowOffset.x();
        if ( absOffsetX >= absOffsetY ) {
            for ( int x = shadowOffsetX; x <= absOffsetX + shadowOffsetX; ++x )
                shadowLine.emplace_back( x, static_cast<int>( std::round( x * slopeFactor ) ) );
        }
        else {
            for ( int y = shadowOffsetY; y <= absOffsetY + shadowOffsetY; ++y )
                shadowLine.emplace_back( static_cast<int>( std::round( y / slopeFactor ) ), y );
        }
    }

    const int maxX = inWidth + absOffsetX;
    const int maxY = inHeight + absOffsetY;

    const auto isTransparent = [&]( int offsetX, int offsetY ) {
        if ( offsetX < 0 || offsetY < 0 || offsetX >= inWidth || offsetY >= inHeight )
            return true;
        return in.pixelColor( offsetX, offsetY ).alpha() == 0;
    };

    for ( int y = 0; y < maxY; ++y ) {
        const int offsetY = y + shadowOffsetY;
        for ( int x = 0; x < maxX; ++x ) {
            const int offsetX = x + shadowOffsetX;

            if ( !isTransparent( offsetX, offsetY ) )
                continue;

            int transformTableId = 6;
            for ( const QPoint & shadowLineOffset : shadowLine ) {
                const int lineX = offsetX - shadowLineOffset.x();
                const int lineY = offsetY - shadowLineOffset.y();
                if ( !isTransparent( lineX, lineY ) ) {
                    --transformTableId;
                    if ( transformTableId == 2 )
                        break;
                }
            }

            if ( transformTableId == 6 )
                continue;

            const int outX = startOffsetX + x;
            const int outY = startOffsetY + y;
            if ( outX < 0 || outY < 0 || outX >= out.width() || outY >= out.height() )
                continue;
            const QColor c = out.pixelColor( outX, outY );
            if ( c.alpha() == 0 )
                continue;
            out.setPixelColor( outX, outY, darkenByTransform( c, transformTableId ) );
        }
    }
}

// Port of fheroes2::addGradientShadowForArea (src/engine/image.cpp): gradient
// background darkening to the left and below a rectangular area (dialog window
// shadow). Applied to the background snapshot (grabBackdrop), so we darken RGB,
// like the engine.
void addGradientShadowForArea( QImage & out, const QPoint & outPos, const int areaWidth, const int areaHeight, const int shadowOffset )
{
    if ( out.isNull() || outPos.x() < 0 || outPos.y() < 0 || shadowOffset < 1 )
        return;

    const auto applyTransform = [&]( int x, int y, int w, int h, int transformId ) {
        for ( int yy = y; yy < y + h; ++yy ) {
            for ( int xx = x; xx < x + w; ++xx ) {
                if ( xx < 0 || yy < 0 || xx >= out.width() || yy >= out.height() )
                    continue;
                const QColor c = out.pixelColor( xx, yy );
                if ( c.alpha() == 0 )
                    continue;
                out.setPixelColor( xx, yy, darkenByTransform( c, transformId ) );
            }
        }
    };

    // Shadow to the left of the area.
    int offsetY = outPos.y() + shadowOffset;
    applyTransform( outPos.x() - shadowOffset, offsetY, shadowOffset, 1, 5 );
    ++offsetY;
    applyTransform( outPos.x() - shadowOffset, offsetY, 1, areaHeight, 5 );
    applyTransform( outPos.x() - shadowOffset + 1, offsetY, shadowOffset - 1, 1, 4 );
    ++offsetY;
    applyTransform( outPos.x() - shadowOffset + 1, offsetY, 1, areaHeight - 4, 4 );
    applyTransform( outPos.x() - shadowOffset + 2, offsetY, shadowOffset - 2, 1, 3 );
    ++offsetY;
    applyTransform( outPos.x() - shadowOffset + 2, offsetY, 1, areaHeight - 6, 3 );
    applyTransform( outPos.x() - shadowOffset + 3, offsetY, shadowOffset - 3, areaHeight - shadowOffset - 3, 2 );

    // Shadow below the area.
    offsetY = outPos.y() + areaHeight;
    const int shadowBottomEdge = outPos.y() + areaHeight + shadowOffset;
    applyTransform( outPos.x() - shadowOffset + 3, offsetY, areaWidth - 6, shadowOffset - 3, 2 );
    applyTransform( outPos.x() - shadowOffset + 2, shadowBottomEdge - 3, areaWidth - 4, 1, 3 );
    applyTransform( outPos.x() - shadowOffset + areaWidth - 3, offsetY, 1, shadowOffset - 3, 3 );
    applyTransform( outPos.x() - shadowOffset + 1, shadowBottomEdge - 2, areaWidth - 2, 1, 4 );
    applyTransform( outPos.x() - shadowOffset + areaWidth - 2, offsetY, 1, shadowOffset - 2, 4 );
    applyTransform( outPos.x() - shadowOffset, shadowBottomEdge - 1, areaWidth, 1, 5 );
    applyTransform( outPos.x() - shadowOffset + areaWidth - 1, offsetY, 1, shadowOffset, 5 );
}

GameButton::GameButton( const QPixmap & released, const QPixmap & pressed, QWidget * parent )
    : QWidget( parent )
    , _released( released )
    , _pressed( pressed.isNull() ? released : pressed )
{
    setFixedSize( _released.size() );
}

GameButton::GameButton( const QPixmap & released, QWidget * parent )
    : GameButton( released, released, parent )
{}

void GameButton::drawShadow( QImage & out ) const
{
    if ( _down )
        addGradientShadow( out, pos(), _pressed.toImage(), QPoint( -5, 5 ) );
    else
        addGradientShadow( out, pos(), _released.toImage(), QPoint( -5, 5 ) );
}

void GameButton::paintEvent( QPaintEvent * )
{
    QPainter p( this );
    QPixmap pm = _down ? _pressed : _released;
    if ( !_enabled ) {
        QPixmap dim = pm;
        QImage img = dim.toImage();
        for ( int y = 0; y < img.height(); ++y ) {
            for ( int x = 0; x < img.width(); ++x ) {
                const QColor c = img.pixelColor( x, y );
                if ( c.alpha() != 0 )
                    img.setPixelColor( x, y, QColor( c.red() * 2 / 3, c.green() * 2 / 3, c.blue() * 2 / 3, c.alpha() ) );
            }
        }
        pm = QPixmap::fromImage( img );
    }
    p.drawPixmap( 0, 0, pm );
}

void GameButton::mousePressEvent( QMouseEvent * event )
{
    if ( !_enabled || event->button() != Qt::LeftButton )
        return;
    _down = true;
    update();
}

void GameButton::mouseReleaseEvent( QMouseEvent * event )
{
    if ( !_enabled || event->button() != Qt::LeftButton )
        return;
    const bool wasDown = _down;
    _down = false;
    update();
    if ( wasDown && rect().contains( event->position().toPoint() ) )
        Q_EMIT clicked();
}

namespace {

// Indexed buffer — analog of fheroes2::Image: a plane of KB.PAL palette indices +
// a transform plane (1 — transparent, 0 — color from image, 2..9 — darkening or
// lightening of the already drawn background). Buttons are assembled exactly like
// in the engine, otherwise the button font transform effects have nothing to
// apply to.
struct IndexedImage {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> image;
    std::vector<uint8_t> transform;

    void resize( int w, int h )
    {
        width = w;
        height = h;
        image.assign( static_cast<size_t>( w ) * h, 0 );
        transform.assign( static_cast<size_t>( w ) * h, 1 );
    }

    void setPixel( int x, int y, uint8_t colorId )
    {
        if ( x < 0 || y < 0 || x >= width || y >= height )
            return;
        const size_t offset = static_cast<size_t>( y ) * width + x;
        image[offset] = colorId;
        transform[offset] = 0;
    }
};

// Port of fheroes2::Copy: copies both the color and the transform (no blending).
void copyIcnPart( const IcnSprite & src, int inX, int inY, IndexedImage & out, int outX, int outY, int width, int height )
{
    if ( src.idx.isNull() || src.tf.isNull() )
        return;
    for ( int y = 0; y < height; ++y ) {
        const int sy = inY + y;
        const int dy = outY + y;
        if ( sy < 0 || sy >= src.idx.height() || dy < 0 || dy >= out.height )
            continue;
        for ( int x = 0; x < width; ++x ) {
            const int sx = inX + x;
            const int dx = outX + x;
            if ( sx < 0 || sx >= src.idx.width() || dx < 0 || dx >= out.width )
                continue;
            const size_t offset = static_cast<size_t>( dy ) * out.width + dx;
            out.image[offset] = static_cast<uint8_t>( src.idx.pixelIndex( sx, sy ) );
            out.transform[offset] = static_cast<uint8_t>( src.tf.pixelIndex( sx, sy ) );
        }
    }
}

// Port of resizeButton (ui_button.cpp:38) for the two cases we need: the button
// is wider than the original (we stretch the middle by thirds) and the button is
// no larger than the original (we glue it from the four corners).
IndexedImage resizeButton( const IcnSprite & original, int buttonWidth, int buttonHeight )
{
    IndexedImage out;
    out.resize( buttonWidth, buttonHeight );
    if ( original.idx.isNull() )
        return out;

    const int originalWidth = original.idx.width();
    const int originalHeight = original.idx.height();

    if ( buttonWidth == originalWidth && buttonHeight == originalHeight ) {
        copyIcnPart( original, 0, 0, out, 0, 0, originalWidth, originalHeight );
        return out;
    }

    if ( buttonWidth > originalWidth && buttonHeight == originalHeight ) {
        const int middleWidth = originalWidth / 3;
        const int overallMiddleWidth = buttonWidth - middleWidth * 2;
        const int middleWidthCount = overallMiddleWidth / middleWidth;
        const int middleWidthLeftOver = overallMiddleWidth - middleWidthCount * middleWidth;

        copyIcnPart( original, 0, 0, out, 0, 0, middleWidth, originalHeight );

        int offsetX = middleWidth;
        for ( int i = 0; i < middleWidthCount; ++i ) {
            copyIcnPart( original, middleWidth, 0, out, offsetX, 0, middleWidth, originalHeight );
            offsetX += middleWidth;
        }
        if ( middleWidthLeftOver > 0 ) {
            copyIcnPart( original, middleWidth, 0, out, offsetX, 0, middleWidthLeftOver, originalHeight );
            offsetX += middleWidthLeftOver;
        }
        copyIcnPart( original, originalWidth - middleWidth, 0, out, offsetX, 0, middleWidth, originalHeight );
        return out;
    }

    if ( buttonWidth <= originalWidth && buttonHeight <= originalHeight ) {
        const int secondHalfHeight = buttonHeight - buttonHeight / 2;
        const int secondHalfWidth = buttonWidth - buttonWidth / 2;

        copyIcnPart( original, 0, 0, out, 0, 0, buttonWidth / 2, buttonHeight / 2 );
        copyIcnPart( original, 0, originalHeight - secondHalfHeight, out, 0, buttonHeight - secondHalfHeight, secondHalfWidth, secondHalfHeight );
        copyIcnPart( original, originalWidth - secondHalfWidth, 0, out, buttonWidth - secondHalfWidth, 0, secondHalfWidth, secondHalfHeight );
        copyIcnPart( original, originalWidth - secondHalfWidth, originalHeight - secondHalfHeight, out, buttonWidth - secondHalfWidth,
                     buttonHeight - secondHalfHeight, secondHalfWidth, secondHalfHeight );
        return out;
    }

    // The remaining cases never occur in the editor: just copy what is there.
    copyIcnPart( original, 0, 0, out, 0, 0, std::min( buttonWidth, originalWidth ), std::min( buttonHeight, originalHeight ) );
    return out;
}

// Port of makeTransparentBackground (ui_button.cpp:718): the background (stone
// from the tile CENTER) is inserted ONLY into the pressed button and only where
// it is transparent, the released one is not.
void makeTransparentBackground( const IndexedImage & released, IndexedImage & pressed, const IcnSprite & background )
{
    if ( background.idx.isNull() )
        return;
    const int offsetX = ( background.idx.width() - pressed.width ) / 2;
    const int offsetY = ( background.idx.height() - pressed.height ) / 2;

    for ( int y = 0; y < pressed.height; ++y ) {
        for ( int x = 0; x < pressed.width; ++x ) {
            const size_t offset = static_cast<size_t>( y ) * pressed.width + x;
            if ( pressed.transform[offset] != 1 || released.transform[offset] != 0 )
                continue;
            const int sx = std::clamp( offsetX + x, 0, background.idx.width() - 1 );
            const int sy = std::clamp( offsetY + y, 0, background.idx.height() - 1 );
            pressed.image[offset] = static_cast<uint8_t>( background.idx.pixelIndex( sx, sy ) );
            pressed.transform[offset] = 0;
        }
    }
}

// Port of addButtonShine (ui_button.cpp:209): shine along the edges of the
// released button, colors KB.PAL 10 / 37 / 39.
void addButtonShine( IndexedImage & released )
{
    const int w = released.width;
    const int h = released.height;
    constexpr uint8_t firstColor = 10;
    constexpr uint8_t secondColor = 37;
    constexpr uint8_t lastColor = 39;

    const auto put = [&released]( int x, int y, uint8_t c ) { released.setPixel( x, y, c ); };
    const auto line = [&released]( int x1, int y1, int x2, int y2, uint8_t c ) {
        const int dx = std::abs( x2 - x1 );
        const int dy = std::abs( y2 - y1 );
        const int steps = std::max( dx, dy );
        for ( int i = 0; i <= steps; ++i ) {
            const int x = steps == 0 ? x1 : x1 + ( x2 - x1 ) * i / steps;
            const int y = steps == 0 ? y1 : y1 + ( y2 - y1 ) * i / steps;
            released.setPixel( x, y, c );
        }
    };

    put( 11, 4, firstColor );
    put( 13, 4, firstColor );
    put( 9, 6, firstColor );
    put( 10, 5, secondColor );
    put( 12, 5, secondColor );
    put( 8, 7, lastColor );
    put( 15, 4, lastColor );
    put( w - 9, 4, firstColor );
    put( w - 7, 4, firstColor );
    line( w - 10, 5, w - 11, 6, secondColor );
    put( w - 8, 5, secondColor );
    if ( w > 50 ) {
        line( 11, 4, 9, 6, firstColor );
        put( 8, 7, lastColor );
        put( 13, 4, secondColor );
        line( 12, 5, 9, 8, lastColor );
        line( 8, 9, 7, 10, firstColor );
        put( 6, 11, lastColor );
        line( 15, 4, 13, 6, firstColor );
        put( 12, 7, lastColor );
        put( w - 10, 2, firstColor );
        put( w - 8, 3, secondColor );
        put( w - 11, 3, secondColor );
        put( w - 12, 4, firstColor );
        line( w - 13, 5, w - 14, 6, lastColor );
        line( w - 10, 5, w - 12, 7, firstColor );
        line( w - 13, 8, w - 14, 9, secondColor );
        line( w - 8, 5, w - 10, 7, firstColor );
        put( w - 11, 8, lastColor );
        put( 11, h - 6, lastColor );
        line( 12, h - 7, 13, h - 8, firstColor );
        put( 14, h - 9, lastColor );
        if ( w > 96 || h > 25 ) {
            put( 10, h - 5, lastColor );
            put( 30, h - 5, secondColor );
            put( 31, h - 6, lastColor );
            line( 32, h - 5, 34, h - 7, firstColor );
            put( 35, h - 8, secondColor );
            put( 36, h - 9, lastColor );
            put( 34, h - 5, secondColor );
            put( 35, h - 6, lastColor );
        }
    }
}

QImage indexedToImage( const IndexedImage & in, const Assets & assets )
{
    QImage out( in.width, in.height, QImage::Format_RGBA8888 );
    out.fill( Qt::transparent );
    for ( int y = 0; y < in.height; ++y ) {
        for ( int x = 0; x < in.width; ++x ) {
            const size_t offset = static_cast<size_t>( y ) * in.width + x;
            if ( in.transform[offset] != 0 )
                continue; // 1 — transparent; other values do not remain on a finished button
            const QColor c = assets.paletteColor( in.image[offset] );
            out.setPixelColor( x, y, c );
        }
    }
    return out;
}

// Palette swap tables that turn GOOD-theme graphics into the EVIL (dark)
// theme. Port of PAL::GetPalette from the engine's pal.cpp.
const uint8_t kGoodToEvilButtonPalette[256] = {
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
    32,  33,  34,  35,  36,  14,  15,  16,  17,  17,  18,  19,  22,  21,  22,  23,  24,  25,  26,  26,  27,  28,  29,  30,  31,  32,  58,  59,  60,  35,  35,  63,
    64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
    96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
    31,  129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 31,  214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
};

// Applies a palette swap table to an indexed sprite (in place).
void applyPaletteRemap( IndexedImage & img, const uint8_t table[256] )
{
    for ( uint8_t & index : img.image )
        index = table[index];
}

// PAL::GetPalette(PaletteType::DARKENING) from the engine's pal.cpp: maps a
// color to its darker variant. Used for the pressed toolbar buttons.
const uint8_t kDarkeningPalette[256] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  16,  17,  18,  19,  20,  21,
    22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  36,
    36,  36,  36,  36,  36,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,
    54,  55,  56,  57,  58,  59,  60,  61,  62,  62,  62,  62,  62,  62,  62,  68,
    69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,
    84,  84,  84,  84,  84,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101,
    102, 103, 104, 105, 106, 107, 107, 107, 107, 107, 107, 107, 114, 115, 116, 117,
    118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 130, 130, 130,
    130, 130, 130, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148,
    149, 150, 151, 151, 151, 151, 151, 151, 158, 159, 160, 161, 162, 163, 164, 165,
    166, 167, 168, 169, 170, 171, 172, 173, 174, 174, 174, 174, 174, 174, 174, 180,
    181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196,
    197, 197, 197, 197, 197, 197, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211,
    212, 213, 213, 213, 213, 213, 214, 215, 216, 217, 218, 219, 220, 221, 225, 226,
    227, 228, 229, 230, 230, 230, 230,  73,  75,  77,  79,  81,  76,  78,  74,  76,
    78,  80, 244, 245, 245, 245,  73,  75,  77,  81, 250, 251, 252, 253,   0,   0,
};

// Builds an IndexedImage (palette indexes + transform) from a decoded ICN sprite.
IndexedImage indexedFromIcn( const IcnSprite & sprite )
{
    IndexedImage out;
    out.width = sprite.idx.width();
    out.height = sprite.idx.height();
    out.image.resize( static_cast<size_t>( out.width ) * out.height );
    out.transform.resize( static_cast<size_t>( out.width ) * out.height );
    for ( int y = 0; y < out.height; ++y ) {
        for ( int x = 0; x < out.width; ++x ) {
            const size_t offset = static_cast<size_t>( y ) * out.width + x;
            out.image[offset] = static_cast<uint8_t>( sprite.idx.pixelIndex( x, y ) );
            out.transform[offset] = static_cast<uint8_t>( sprite.tf.pixelIndex( x, y ) );
        }
    }
    return out;
}

} // namespace

GameButton * makeGameTextButton( const Assets & assets, const GameFont & font, const QString & text, QWidget * parent, int fixedWidth, bool evilTheme )
{
    (void)font; // the button font is a custom one (buttonfont.h), not FONT.ICN

    // Port of getTextAdaptedSprite (ui_button.cpp:840) for ICN::EMPTY_GOOD_BUTTON:
    // textAreaBorders {8,2}, minimumTextArea {86,15}, maximumTextArea {200,50},
    // backgroundBorders {10,8}, releasedOffset {6,4}, pressedOffset {5,5}.
    // Text — cp1251: the button font contains ASCII and Cyrillic.
    const std::string ascii = encodeCp1251( text );
    const int textWidth = buttonTextWidth( ascii );
    const int textHeight = buttonFontHeight();

    const int textAreaWidth = fixedWidth > 0 ? fixedWidth - 10 : std::clamp( textWidth + 8, 86, 200 );
    const int textAreaHeight = std::clamp( textHeight + 2, 15, 50 );

    const int w = textAreaWidth + 10;
    const int h = textAreaHeight + 8;

    const IcnSprite & frameRel = assets.icnSprite( "SYSTEM.ICN", 11 );
    const IcnSprite & framePress = assets.icnSprite( "SYSTEM.ICN", 12 );
    const IcnSprite & stone = assets.icnSprite( ICN_STONEBAK, 0 );

    IndexedImage released = resizeButton( frameRel, w, h );
    IndexedImage pressed;

    addButtonShine( released );

    // Dark interface theme: the released sprite gets the GOOD_TO_EVIL_BUTTON
    // palette swap; the pressed state is the same sprite darkened (the
    // engine's DARKENING palette) with the text sunk by one pixel — no extra
    // frame/outline. The pressed SYSTEM[12] sprite is not used here.
    if ( evilTheme ) {
        applyPaletteRemap( released, kGoodToEvilButtonPalette );
        pressed = released;
        applyPaletteRemap( pressed, kDarkeningPalette );
    }
    else {
        pressed = resizeButton( framePress, w, h );
        makeTransparentBackground( released, pressed, stone );
    }

    // Button font letters are shifted 1px left by their shadow, so the engine
    // adds +1 to the pen position (ui_button.cpp:891).
    const int textX = ( textAreaWidth - textWidth ) / 2;
    const int textY = ( textAreaHeight - textHeight ) / 2;
    drawButtonText( released.image.data(), released.width, released.height, 6 + 1 + textX, 4 + textY, ascii, false );
    drawButtonText( pressed.image.data(), pressed.width, pressed.height, 5 + 1 + textX, 5 + textY, ascii, true );

    return new GameButton( QPixmap::fromImage( indexedToImage( released, assets ) ), QPixmap::fromImage( indexedToImage( pressed, assets ) ), parent );
}

QPixmap evilThemePixmap( const Assets & assets, const std::string & icnName, int index, double scale )
{
    IndexedImage img = indexedFromIcn( assets.icnSprite( icnName, index ) );
    if ( img.width <= 0 || img.height <= 0 )
        return {};
    applyPaletteRemap( img, kGoodToEvilButtonPalette );
    QPixmap pm = QPixmap::fromImage( indexedToImage( img, assets ) );
    if ( scale != 1.0 && !pm.isNull() )
        pm = pm.scaled( std::max( 1, qRound( pm.width() * scale ) ), std::max( 1, qRound( pm.height() * scale ) ),
                        Qt::IgnoreAspectRatio, Qt::FastTransformation );
    return pm;
}

// --- GameWindow ---

GameWindow::GameWindow( const Assets * assets, const QSize & activeSize, QWidget * parent )
    : QWidget( nullptr )
    , _assets( assets )
{
    // The overlay lives on top of the editor's main window (not a separate macOS window).
    QWidget * top = parent ? parent->window() : nullptr;
    if ( top ) {
        setParent( top, Qt::Widget );
        top->installEventFilter( this );
        setGeometry( top->rect() ); // adopt the editor window's size right away
    }
    setFocusPolicy( Qt::StrongFocus );

    buildFrame( activeSize );
    _activeSize = activeSize;
    _frameSize = QSize( activeSize.width() + BORDER_SIZE * 2, activeSize.height() + BORDER_SIZE * 2 );
}

QPoint GameWindow::framePosFor( const QSize & size ) const
{
    // Like StandardWindow (ui_window.cpp:107): the ACTIVE area is centered, the
    // frame is the active area expanded by borderSize on each side; the shadow
    // (16px to the left and below) does not take part in the centering.
    return QPoint( ( size.width() - _frameSize.width() ) / 2, ( size.height() - _frameSize.height() ) / 2 );
}

QRect GameWindow::activeArea() const
{
    return QRect( framePosFor( size() ) + QPoint( BORDER_SIZE, BORDER_SIZE ), _activeSize );
}

void GameWindow::showEvent( QShowEvent * )
{
    if ( parentWidget() )
        setGeometry( parentWidget()->rect() );
    layoutWidgets();
    setFocus();
}

bool GameWindow::eventFilter( QObject * watched, QEvent * event )
{
    if ( watched == parentWidget() && event->type() == QEvent::Resize && isVisible() ) {
        setGeometry( parentWidget()->rect() );
        // The active area has moved — redo the whole layout
        // (the inheritors' cached rectangles too).
        layoutWidgets();
        _backdrop = grabBackdrop( this );
        update();
    }
    return QWidget::eventFilter( watched, event );
}

QImage GameWindow::makeBuffer() const
{
    QImage buf( size(), QImage::Format_RGBA8888 );
    if ( _backdrop.size() == size() ) {
        buf = _backdrop;
        // Background darkening under the modal window (in the game modality is
        // shown only by the window shadow, but our overlay is over the editor).
        QPainter dim( &buf );
        dim.fillRect( buf.rect(), QColor( 0, 0, 0, 120 ) );
        dim.end();
    }
    else {
        buf.fill( QColor( 0, 0, 0, 120 ) );
    }
    return buf;
}

int GameWindow::exec()
{
    _backdrop = grabBackdrop( this );
    show();
    raise();
    setFocus();

    QEventLoop loop;
    _loop = &loop;
    const int result = loop.exec();
    _loop = nullptr;
    hide();
    return result;
}

void GameWindow::accept()
{
    if ( _loop )
        _loop->exit( 1 );
}

void GameWindow::reject()
{
    if ( _loop )
        _loop->exit( 0 );
}

QPixmap makeStandardFrame( const Assets * assets, const QSize & activeSize, bool drawBackground )
{
    QPixmap framePm( activeSize.width() + BORDER_SIZE * 2, activeSize.height() + BORDER_SIZE * 2 );
    framePm.fill( Qt::transparent );
    QPainter p( &framePm );

    const QPixmap stone = assets ? assets->icnPixmap( ICN_STONEBAK, 0 ) : QPixmap();
    const QPixmap vertical = assets ? assets->icnPixmap( ICN_WINLOSE, 0 ) : QPixmap();
    const QPixmap horizontal = assets ? assets->icnPixmap( ICN_SURDRBKG, 0 ) : QPixmap();

    const int w = activeSize.width();
    const int h = activeSize.height();
    const int bw = BORDER_SIZE;

    // Background — a piece of stone (only when the window has a background).
    if ( drawBackground && !stone.isNull() ) {
        p.drawPixmap( bw, bw, stone, 0, 0, w, h );
    }

    // Frame (a simplified port of StandardWindow::render). SURDRBKG and WINLOSE
    // have a 16px shadow on the left and at the bottom — the bottom/right edges
    // of the sprites do not fit the frame (otherwise the bottom band is black).
    if ( !vertical.isNull() && !horizontal.isNull() ) {
        const int vw = vertical.width();
        const int vh = vertical.height();
        const int hw = horizontal.width();
        const int hh = horizontal.height();
        const int vhUsable = vh - bw; // without the bottom shadow
        const int hwUsable = hw - bw; // without the left shadow

        // 16×16 corners from WINLOSE corners (it has 16px shadows on the left-bottom).
        p.drawPixmap( 0, 0, vertical, 0, 0, bw, bw );
        p.drawPixmap( bw + w, 0, vertical, vw - bw, 0, bw, bw );
        p.drawPixmap( 0, bw + h, vertical, 0, vh - bw, bw, bw );
        p.drawPixmap( bw + w, bw + h, vertical, vw - bw, vh - bw, bw, bw );

        // Top and bottom bands from the usable area of SURDRBKG (no black shadow).
        if ( w > 0 ) {
            const QPixmap top = horizontal.copy( bw, 0, hwUsable - bw, bw ).scaled( w, bw, Qt::IgnoreAspectRatio, Qt::FastTransformation );
            const QPixmap bottom = horizontal.copy( bw, hh - bw * 2, hwUsable - bw, bw ).scaled( w, bw, Qt::IgnoreAspectRatio, Qt::FastTransformation );
            p.drawPixmap( bw, 0, top );
            p.drawPixmap( bw, bw + h, bottom );
        }

        // Left and right bands from the usable area of WINLOSE.
        if ( h > 0 ) {
            const QPixmap left = vertical.copy( 0, bw, bw, vhUsable - bw * 2 ).scaled( bw, h, Qt::IgnoreAspectRatio, Qt::FastTransformation );
            const QPixmap right = vertical.copy( vw - bw, bw, bw, vhUsable - bw * 2 ).scaled( bw, h, Qt::IgnoreAspectRatio, Qt::FastTransformation );
            p.drawPixmap( 0, bw, left );
            p.drawPixmap( bw + w, bw, right );
        }
    }
    p.end();
    return framePm;
}

void GameWindow::buildFrame( const QSize & activeSize )
{
    _background = makeStandardFrame( _assets, activeSize );
}

void GameWindow::paintEvent( QPaintEvent * )
{
    // Overlay: background snapshot + editor window darkening, dialog window on top.
    QImage buf = makeBuffer();
    QPainter p( &buf );

    const QPoint framePos = framePosFor( size() );

    // Window shadow on the left and below, like in the game (StandardWindow::render →
    // addGradientShadowForArea with borderSize = 16).
    addGradientShadowForArea( buf, framePos, _frameSize.width(), _frameSize.height(), BORDER_SIZE );

    // No button shadows: the opaque button slabs would cast visible dark
    // rectangles (in the engine they are invisible on the matching stone
    // background).
    p.drawImage( framePos, _background.toImage() );
    p.end();

    QPainter view( this );
    view.drawImage( 0, 0, buf );
}

// --- GameScrollBar (port of fheroes2::Scrollbar) ---

namespace {

// Engine scrollbar geometry (dialog_selectitems.cpp:934-939, ui_window.cpp:403):
constexpr int SCROLLBAR_WIDTH = 16;
constexpr int TRACK_CAP_HEIGHT = 19;  // «caps» of the track background at the top and bottom
constexpr int TRACK_MIDDLE_HEIGHT = 88;
constexpr int ARROW_SIZE = 14;        // SCROLL[0..3]
constexpr int MIN_SLIDER_LENGTH = 15; // minimumSliderLength, ui_scrollbar.cpp:28
constexpr int SLIDER_PART_HEIGHT = 8; // startSliderArea/middleSliderArea: {0,0,w,8}/{0,7,w,8}

} // namespace

GameScrollBar::GameScrollBar( const Assets * assets, QWidget * parent )
    : QWidget( parent )
{
    _up[0] = assets->icnPixmap( ICN_SCROLL, 0 );
    _up[1] = assets->icnPixmap( ICN_SCROLL, 1 );
    _down[0] = assets->icnPixmap( ICN_SCROLL, 2 );
    _down[1] = assets->icnPixmap( ICN_SCROLL, 3 );
    _slider = assets->icnPixmap( ICN_SCROLL, 4 );

    // Track background — cuts from ADVBORD, like renderScrollbarBackground in the game:
    // top (536,176,16,19), middle (536,196,16,88), bottom (536,285,16,19).
    const QPixmap adv = assets->icnPixmap( ICN_ADVBORD, 0 );
    _trackTop = adv.copy( 536, 176, SCROLLBAR_WIDTH, TRACK_CAP_HEIGHT );
    _trackMiddle = adv.copy( 536, 196, SCROLLBAR_WIDTH, TRACK_MIDDLE_HEIGHT );
    _trackBottom = adv.copy( 536, 285, SCROLLBAR_WIDTH, TRACK_CAP_HEIGHT );
    setFixedWidth( SCROLLBAR_WIDTH );

    // Arrow auto-repeat: like TimedEventValidator(500, 100) in the engine (ui_tool.h:228).
    _holdTimer = std::make_unique<QTimer>( this );
    connect( _holdTimer.get(), &QTimer::timeout, this, [this]() {
        scrollBy( _holdDelta );
        _holdInitial = false;
        _holdTimer->start( 100 );
    } );
}

void GameScrollBar::setRange( int itemCount, int visibleCount )
{
    _itemCount = std::max( 0, itemCount );
    _visibleCount = std::max( 1, visibleCount );
    _value = std::clamp( _value, 0, maxValue() );
    update();
}

void GameScrollBar::setValue( int value )
{
    const int newValue = std::clamp( value, 0, maxValue() );
    if ( newValue == _value )
        return;
    _value = newValue;
    update();
}

void GameScrollBar::scrollBy( int delta )
{
    const int newValue = std::clamp( _value + delta, 0, maxValue() );
    if ( newValue != _value ) {
        _value = newValue;
        update();
        Q_EMIT valueChanged( _value );
    }
}

void GameScrollBar::resizeEvent( QResizeEvent * )
{
    _cacheLength = -1; // the channel changed — rebuild the slider
}

QRect GameScrollBar::sliderChannel() const
{
    // setScrollBarArea({x+3, y+19, 10, h − 2*19}) (dialog_selectitems.cpp:939).
    return QRect( 3, TRACK_CAP_HEIGHT, 10, height() - 2 * TRACK_CAP_HEIGHT );
}

const QPixmap & GameScrollBar::sliderPixmap() const
{
    const int areaLength = sliderChannel().height();
    if ( _cacheItems == _itemCount && _cacheVisible == _visibleCount && _cacheLength == areaLength )
        return _sliderCache;

    _cacheItems = _itemCount;
    _cacheVisible = _visibleCount;
    _cacheLength = areaLength;

    // Port of fheroes2::generateScrollbarSlider (ui_scrollbar.cpp:120) for a
    // vertical slider with startSliderArea={0,0,w,8}, middleSliderArea={0,7,w,8}.
    const int originalLength = _slider.height();
    const int perView = std::max( 1, _visibleCount );
    const int total = std::max( 1, _itemCount );
    if ( _slider.isNull() || areaLength < originalLength || areaLength * perView == originalLength * total ) {
        _sliderCache = _slider;
        return _sliderCache;
    }

    const int sliderWidth = _slider.width();
    const int middleLength = areaLength * perView / std::max( perView, total ) - originalLength;
    const int length = std::max( MIN_SLIDER_LENGTH, std::max( originalLength + middleLength, SLIDER_PART_HEIGHT * 2 ) );

    QPixmap out( sliderWidth, length );
    out.fill( Qt::transparent );
    QPainter p( &out );

    // Upper part of the slider.
    p.drawPixmap( 0, 0, _slider, 0, 0, sliderWidth, SLIDER_PART_HEIGHT );

    if ( middleLength < 0 ) {
        // The slider is shortened: the remainder is copied from the END of the original sprite.
        const int copyHeight = length - SLIDER_PART_HEIGHT;
        p.drawPixmap( 0, SLIDER_PART_HEIGHT, _slider, 0, originalLength - copyHeight, sliderWidth, copyHeight );
        p.end();
        _sliderCache = out;
        return _sliderCache;
    }

    // Middle: {0,7,w,8} tiles + the remainder.
    int offset = SLIDER_PART_HEIGHT;
    const int middleChunkCount = middleLength / SLIDER_PART_HEIGHT;
    for ( int i = 0; i < middleChunkCount; ++i ) {
        p.drawPixmap( 0, offset, _slider, 0, SLIDER_PART_HEIGHT - 1, sliderWidth, SLIDER_PART_HEIGHT );
        offset += SLIDER_PART_HEIGHT;
    }
    const int leftover = middleLength - middleChunkCount * SLIDER_PART_HEIGHT;
    if ( leftover > 0 ) {
        p.drawPixmap( 0, offset, _slider, 0, SLIDER_PART_HEIGHT - 1, sliderWidth, leftover );
        offset += leftover;
    }

    // Lower part: originalLength − 8 rows (for SCROLL[4] that is 9 rows).
    p.drawPixmap( 0, offset, _slider, 0, SLIDER_PART_HEIGHT, sliderWidth, originalLength - SLIDER_PART_HEIGHT );
    p.end();

    _sliderCache = out;
    return _sliderCache;
}

QRect GameScrollBar::sliderRect() const
{
    const QRect channel = sliderChannel();
    const QPixmap & slider = sliderPixmap();
    if ( channel.height() <= 0 || slider.isNull() )
        return {};

    const int sliderH = std::min( slider.height(), channel.height() );
    const int roiHeight = channel.height() - sliderH;
    // Scrollbar::moveToIndex (ui_scrollbar.cpp:80-90); when min == max the slider
    // is centered in the channel (and is still drawn).
    const int y = maxValue() == 0 ? channel.y() + roiHeight / 2 : channel.y() + _value * roiHeight / maxValue();
    return QRect( channel.x() + ( channel.width() - slider.width() ) / 2, y, slider.width(), sliderH );
}

void GameScrollBar::paintEvent( QPaintEvent * )
{
    QPainter p( this );
    const int h = height();

    // Track background (renderScrollbarBackground, ui_window.cpp:404-423).
    const int middleAndBottomPartsHeight = h - TRACK_CAP_HEIGHT;
    p.drawPixmap( 0, 0, _trackTop );
    const int middlePartCount = ( h - 2 * TRACK_CAP_HEIGHT + TRACK_MIDDLE_HEIGHT - 1 ) / TRACK_MIDDLE_HEIGHT;
    int offsetY = TRACK_CAP_HEIGHT;
    for ( int i = 0; i < middlePartCount; ++i ) {
        const int part = std::min( TRACK_MIDDLE_HEIGHT, middleAndBottomPartsHeight - offsetY );
        if ( part <= 0 )
            break;
        p.drawPixmap( 0, offsetY, _trackMiddle, 0, 0, SCROLLBAR_WIDTH, part );
        offsetY += TRACK_MIDDLE_HEIGHT;
    }
    p.drawPixmap( 0, middleAndBottomPartsHeight, _trackBottom );

    // Slider (the engine always draws it, even when the whole list is visible).
    const QRect sr = sliderRect();
    if ( !sr.isEmpty() ) {
        const QPixmap & slider = sliderPixmap();
        p.drawPixmap( QRect( sr.topLeft(), QSize( slider.width(), sr.height() ) ), slider, QRect( 0, 0, slider.width(), sr.height() ) );
    }

    // Arrow buttons: (1,1) and (1, h−15) (dialog_selectitems.cpp:937-938).
    p.drawPixmap( 1, 1, _up[_upDown ? 1 : 0] );
    p.drawPixmap( 1, h - ARROW_SIZE - 1, _down[_downDown ? 1 : 0] );
}

GameScrollBar::Zone GameScrollBar::hitZone( const QPoint & pos ) const
{
    if ( QRect( 1, 1, ARROW_SIZE, ARROW_SIZE ).contains( pos ) )
        return ZONE_UP;
    if ( QRect( 1, height() - ARROW_SIZE - 1, ARROW_SIZE, ARROW_SIZE ).contains( pos ) )
        return ZONE_DOWN;
    if ( sliderChannel().contains( QPoint( sliderChannel().x(), pos.y() ) ) )
        return ZONE_CHANNEL;
    return ZONE_NONE;
}

void GameScrollBar::moveToPos( int y )
{
    // Port of Scrollbar::moveToPos (ui_scrollbar.cpp:105): the slider is centered
    // under the cursor, the index is rounded to the nearest.
    if ( maxValue() == 0 )
        return;
    const QRect channel = sliderChannel();
    const QRect sr = sliderRect();
    const int length = channel.height() - sr.height();
    if ( length <= 0 )
        return;
    const int pos = std::clamp( y - sr.height() / 2 - channel.y(), 0, length );
    const int newValue = ( maxValue() * pos + length / 2 ) / length;
    if ( newValue != _value ) {
        _value = newValue;
        update();
        Q_EMIT valueChanged( _value );
    }
}

void GameScrollBar::startHold( int delta )
{
    _holdDelta = delta;
    _holdInitial = true;
    _holdTimer->start( 500 );
}

void GameScrollBar::mousePressEvent( QMouseEvent * event )
{
    if ( event->button() != Qt::LeftButton )
        return;

    const QPoint pos = event->position().toPoint();
    switch ( hitZone( pos ) ) {
    case ZONE_UP:
        _upDown = true;
        update();
        scrollBy( -1 );
        startHold( -1 );
        break;
    case ZONE_DOWN:
        _downDown = true;
        update();
        scrollBy( +1 );
        startHold( +1 );
        break;
    case ZONE_CHANNEL:
        // Like in the engine: pressing anywhere in the channel «grabs» the slider
        // and it follows the cursor while the button is held (interface_list.h:493).
        _draggingSlider = true;
        moveToPos( pos.y() );
        break;
    default:
        break;
    }
}

void GameScrollBar::mouseMoveEvent( QMouseEvent * event )
{
    if ( !_draggingSlider || ( event->buttons() & Qt::LeftButton ) == 0 )
        return;
    moveToPos( event->position().toPoint().y() );
}

void GameScrollBar::mouseReleaseEvent( QMouseEvent * )
{
    _upDown = false;
    _downDown = false;
    _draggingSlider = false;
    _holdTimer->stop();
    update();
}

void GameScrollBar::wheelEvent( QWheelEvent * event )
{
    // One item per wheel «notch» (interface_list.h:456-475). Qt sends many small
    // events from a trackpad, so we accumulate up to 120 units.
    _wheelDelta += event->angleDelta().y();
    while ( _wheelDelta >= 120 ) {
        _wheelDelta -= 120;
        scrollBy( -1 );
    }
    while ( _wheelDelta <= -120 ) {
        _wheelDelta += 120;
        scrollBy( +1 );
    }
}

} // namespace fh2
