#pragma once

#include <algorithm>
#include <memory>

#include <QEventLoop>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QTimer>
#include <QVector>
#include <QWidget>

#include "gamefont.h"

namespace fh2 {

class Assets;
class GameFont;
class GameButton;

// Generated button with game-style text: SYSTEM[11..12] frame (3-slice),
// STONEBAK stone from the tile center, shine, dark letters of the button font
// (port of resizeButton + makeTransparentBackground + addButtonShine +
// getTextAdaptedSprite). fixedWidth <= 0 — width fits the text (min 96).
// evilTheme=true produces the dark interface theme: the sprites are
// palette-swapped like the engine's convertToEvilButtonBackground, and the
// pressed fill uses the STONEBAK_EVIL palette.
GameButton * makeGameTextButton( const Assets & assets, const GameFont & font, const QString & text, QWidget * parent, int fixedWidth = 0, bool evilTheme = false );

// Dark-theme (palette-swapped) version of an indexed sprite as a QPixmap
// (like the engine's EDITBTNS_EVIL — a GOOD_TO_EVIL_BUTTON palette swap).
QPixmap evilThemePixmap( const Assets & assets, const std::string & icnName, int index, double scale = 1.0 );

// Frame of a standard game window (SURDRBKG/WINLOSE) for an active area of
// the given size (port of StandardWindow::render). background=false —
// the active area stays transparent (renderBackground=false in the engine,
// that is how the hero window opens in the map editor).
QPixmap makeStandardFrame( const Assets * assets, const QSize & activeSize, bool background = true );

// Gradient shadow — exact port of fheroes2::addGradientShadow (src/engine/image.cpp).
// Draws the sprite's shadow on an opaque background (like drawButtonShadow in the recruit dialog).
void addGradientShadow( QImage & out, const QPoint & outPos, const QImage & in, const QPoint & shadowOffset );

// Port of fheroes2::addGradientShadowForArea: gradient shadow of a dialog window
// (background darkening to the left and below the rectangular area).
void addGradientShadowForArea( QImage & out, const QPoint & outPos, int areaWidth, int areaHeight, int shadowOffset );

// Background snapshot under the modal overlay (analog of fheroes2::ImageRestorer).
// The overlay draws it at the start of every paintEvent: otherwise the
// semi-transparent darkening accumulates across repaints (black bands around
// the window).
QImage grabBackdrop( QWidget * overlay );

// Port of fheroes2::makeShadow + addShadow (image.cpp:2665, 2949): shadow along
// the SILHOUETTE of the sprite's opaque pixels with offset (-x, +y) and a single
// darkening strength transformId (for the spell book — {-16,16} and transform 3).
void addSilhouetteShadow( QImage & out, const QPoint & outPos, const QImage & in, const QPoint & shadowOffset, int transformId );

// Application background — the «star sky» of an unexplored map: 32×32 CLOF32.TIL
// tiles, variant (tx + ty) % 4, like drawFog for a fully fogged tile
// (maps_tiles_render.cpp:709). origin — the area's position in top-level window
// coordinates: the tile grid is shared by the main window and the hero panel,
// so there is no seam at their border. If the TIL is unavailable — STONEBAK
// stone, then a solid color.
void drawFogBackground( QPainter & p, const Assets * assets, const QSize & size, const QPoint & origin );

// Sprite button made of game resources (released/pressed).
class GameButton : public QWidget
{
    Q_OBJECT
public:
    GameButton( const QPixmap & released, const QPixmap & pressed, QWidget * parent = nullptr );
    GameButton( const QPixmap & released, QWidget * parent = nullptr ); // pressed = released

    void setEnabled( bool enabled ) { _enabled = enabled; update(); }
    bool isEnabled() const { return _enabled; }

    // Button shadow on the parent's background (the parent draws it, like drawButtonShadow).
    void drawShadow( QImage & out ) const;

Q_SIGNALS:
    void clicked();

protected:
    void paintEvent( QPaintEvent * event ) override;
    void mousePressEvent( QMouseEvent * event ) override;
    void mouseReleaseEvent( QMouseEvent * event ) override;

private:
    QPixmap _released;
    QPixmap _pressed;
    bool _down = false;
    bool _enabled = true;
};

// Game-style window: an overlay over the whole editor window (darkening + frame
// made of SURDRBKG/WINLOSE), without the macOS system frame. Modality — QEventLoop.
// Child widgets are placed inside activeArea().
class GameWindow : public QWidget
{
    Q_OBJECT
public:
    GameWindow( const Assets * assets, const QSize & activeSize, QWidget * parent = nullptr );

    // Modal show: 1 — accepted, 0 — rejected.
    virtual int exec();
    virtual void accept();
    virtual void reject();

    QRect activeArea() const;

protected:
    void paintEvent( QPaintEvent * event ) override;
    void showEvent( QShowEvent * event ) override;
    bool eventFilter( QObject * watched, QEvent * event ) override;

    // Layout of child widgets and cached rectangles relative to
    // activeArea(). Called on show and when the editor window is resized
    // (the active area's coordinates change then).
    virtual void layoutWidgets() {}

    // Background snapshot + darkening: the base of the overlay's drawing buffer.
    QImage makeBuffer() const;

private:
    void buildFrame( const QSize & activeSize );
    QPoint framePosFor( const QSize & size ) const;

    const Assets * _assets = nullptr;
    QPixmap _background; // active area + frame (whole)
    QImage _backdrop;    // background snapshot under the overlay (like ImageRestorer)
    QSize _frameSize;
    QSize _activeSize;
    QEventLoop * _loop = nullptr;
};

// Vertical scrollbar from SCROLL.ICN — port of fheroes2::Scrollbar
// (ui_scrollbar.cpp) together with the track background from ADVBORD
// (StandardWindow::renderScrollbarBackground, ui_window.cpp:400).
class GameScrollBar : public QWidget
{
    Q_OBJECT
public:
    explicit GameScrollBar( const Assets * assets, QWidget * parent = nullptr );

    // itemCount — total items, visibleCount — how many are visible (maxItems).
    void setRange( int itemCount, int visibleCount );
    int value() const { return _value; }
    int itemCount() const { return _itemCount; }
    int visibleCount() const { return _visibleCount; }
    int maxValue() const { return std::max( 0, _itemCount - _visibleCount ); }
    void setValue( int value );
    void scrollBy( int delta );

Q_SIGNALS:
    void valueChanged( int value );

protected:
    void paintEvent( QPaintEvent * event ) override;
    void mousePressEvent( QMouseEvent * event ) override;
    void mouseMoveEvent( QMouseEvent * event ) override;
    void mouseReleaseEvent( QMouseEvent * event ) override;
    void wheelEvent( QWheelEvent * event ) override;
    void resizeEvent( QResizeEvent * event ) override;

private:
    enum Zone { ZONE_NONE, ZONE_UP, ZONE_DOWN, ZONE_CHANNEL };
    Zone hitZone( const QPoint & pos ) const;
    // Slider movement channel: {3, 19, 10, H-38} — like setScrollBarArea
    // (dialog_selectitems.cpp:939).
    QRect sliderChannel() const;
    QRect sliderRect() const;
    const QPixmap & sliderPixmap() const; // cache; port of generateScrollbarSlider
    void moveToPos( int y );               // port of Scrollbar::moveToPos
    void startHold( int delta );           // arrow auto-repeat 500 → 100 ms

    QPixmap _up[2];
    QPixmap _down[2];
    QPixmap _slider;      // SCROLL[4], 8×17 — the original slider
    QPixmap _trackTop;    // ADVBORD (536,176,16,19)
    QPixmap _trackMiddle; // ADVBORD (536,196,16,88)
    QPixmap _trackBottom; // ADVBORD (536,285,16,19)
    bool _upDown = false;
    bool _downDown = false;
    bool _draggingSlider = false;
    int _value = 0;
    int _itemCount = 0;
    int _visibleCount = 0;
    int _wheelDelta = 0; // accumulator of wheel «notches» (Qt sends fractional steps)
    std::unique_ptr<QTimer> _holdTimer;
    int _holdDelta = 0;
    bool _holdInitial = true;

    mutable QPixmap _sliderCache;
    mutable int _cacheItems = -1;
    mutable int _cacheVisible = -1;
    mutable int _cacheLength = -1;
};

} // namespace fh2
