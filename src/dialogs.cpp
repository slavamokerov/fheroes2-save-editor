#include "dialogs.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QWheelEvent>

#include <algorithm>
#include <functional>
#include <set>

#include "assets.h"
#include "constants.h"
#include "gamefont.h"
#include "gamedata.h"
#include "gameui.h"
#include "textutil.h"
#include "translations.h"

namespace fh2 {

namespace {

constexpr int BOX_AREA_W = 260; // boxAreaWidthPx in fheroes2
constexpr int FRAME_OFFSET = 6;

// Spell icon (with mass-spell generation) — defined below.
QPixmap spellIcon( const Assets & assets, int spellId );

// Height of list windows: in the engine this is display.height() − 150
// (dialog_selectitems.cpp:1217 and others). The editor window plays the role
// of the «display» here.
int listDialogHeight( QWidget * parent )
{
    const QWidget * top = parent ? parent->window() : nullptr;
    const int h = top ? top->height() : 480;
    return std::clamp( h - 150, 240, 700 );
}

// Draws text centered across the width (like Text::draw(x, y, maxWidth) in the engine).
void drawCentered( QPainter & p, const GameFont & font, int x, int y, int width, const QString & text, GameFont::Size size,
                   GameFont::Color color )
{
    const int w = font.textWidth( text, size );
    font.drawText( p, x + ( width - w ) / 2, y, text, size, color );
}

// --- Base modal overlay (frameless) ---
class OverlayDialog : public QWidget
{
public:
    OverlayDialog( QWidget * parent )
        : QWidget( nullptr )
    {
        QWidget * top = parent ? parent->window() : nullptr;
        if ( top ) {
            setParent( top, Qt::Widget );
            top->installEventFilter( this );
            setGeometry( top->rect() );
        }
        setFocusPolicy( Qt::StrongFocus );
    }

    int exec()
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

    void accept() { if ( _loop ) _loop->exit( 1 ); }
    void reject() { if ( _loop ) _loop->exit( 0 ); }

protected:
    // Layout of child widgets/cached rectangles: computed from the frame
    // position, which depends on the editor window's size.
    virtual void layoutWidgets() {}

    // Base of the drawing buffer: background snapshot (like ImageRestorer in the
    // engine) + darkening. Without the snapshot the semi-transparent darkening
    // would accumulate on every overlay repaint.
    QImage makeBuffer() const
    {
        QImage buf( size(), QImage::Format_RGBA8888 );
        if ( _backdrop.size() == size() ) {
            buf = _backdrop;
            QPainter dim( &buf );
            dim.fillRect( buf.rect(), QColor( 0, 0, 0, 120 ) );
            dim.end();
        }
        else {
            buf.fill( QColor( 0, 0, 0, 120 ) );
        }
        return buf;
    }

    void showEvent( QShowEvent * ) override
    {
        if ( parentWidget() )
            setGeometry( parentWidget()->rect() );
        layoutWidgets();
        setFocus();
    }

    bool eventFilter( QObject * watched, QEvent * event ) override
    {
        if ( watched == parentWidget() && event->type() == QEvent::Resize && isVisible() ) {
            setGeometry( parentWidget()->rect() );
            layoutWidgets();
            _backdrop = grabBackdrop( this );
            update();
        }
        return QWidget::eventFilter( watched, event );
    }

    void keyPressEvent( QKeyEvent * event ) override
    {
        if ( event->key() == Qt::Key_Escape ) {
            reject();
            return;
        }
        QWidget::keyPressEvent( event );
    }

    QEventLoop * _loop = nullptr;
    QImage _backdrop; // background snapshot under the overlay (analog of ImageRestorer)
};

// --- Message window with a BUYBUILD frame (port of showMessage + ResizableFrameBox) ---
class FrameBoxDialog : public OverlayDialog
{
public:
    // contentHeight >= 0 — the content height is given by the caller (like the
    // height argument of Dialog::FrameBox); the button zone (40px) is added here,
    // like FrameBox( height, buttons = true ) in the engine (dialog_box.cpp:126).
    FrameBoxDialog( const Assets & assets, const QString & title, const QString & body, int buttons, QWidget * parent, int contentHeight = -1 )
        : OverlayDialog( parent )
        , _font( &assets )
        , _assets( assets )
    {
        // Height: title + text + buttons (unless the caller gave the exact
        // content height).
        int contentH = 0;
        if ( contentHeight >= 0 ) {
            contentH = contentHeight;
        }
        else {
            if ( !title.isEmpty() )
                contentH += _font.lineHeight( GameFont::Size::NORMAL ) + 10;
            if ( !body.isEmpty() )
                contentH += _font.lineHeight( GameFont::Size::SMALL ) + 10;
        }
        if ( buttons != MSG_ZERO )
            contentH += 40;

        const int middleH = std::max( 0, contentH - 2 * 35 );
        _frameW = 304; // 260 + (16+6)*2
        // Like in the engine: topHeight (99) + bottomHeight (81) + middleFragmentHeight.
        _frameH = 99 + middleH + 81;
        // Frame left column width — from BUYBUILD[4..6] sprites (leftWidth in
        // dialog_box.cpp). leftSideOffset = leftWidth − windowWidth/2: the content
        // (260px) is centered relative to the left half of the window, and the
        // window itself shifts left by leftSideOffset (ResizableFrameBox: _position.x).
        _leftW = std::max( assets.icnPixmap( "BUYBUILD.ICN", 4 ).width(),
                           std::max( assets.icnPixmap( "BUYBUILD.ICN", 5 ).width(), assets.icnPixmap( "BUYBUILD.ICN", 6 ).width() ) );
        // area.x = position.x + (windowWidth − area.width)/2 + leftSideOffset
        //        = position.x + 22 + (leftWidth − 152) = position.x + leftWidth − 130.
        _area = QRect( _leftW - _frameW / 2 + ( _frameW - BOX_AREA_W ) / 2, 0, BOX_AREA_W, 35 + middleH + 35 );

        // Buttons are textual with translation (like the fheroes2 non-original
        // resources: UNIFORM_GOOD_OKAY/CANCEL_BUTTON via getTextAdaptedSprite);
        // positions — in layoutWidgets, like fheroes2::ButtonGroup.
        if ( buttons == MSG_OK )
            addButton( uiButtonText( UiButton::Okay ), 1 );
        else if ( buttons == MSG_YES_NO ) {
            addButton( uiButtonText( UiButton::Yes ), 1 );
            addButton( uiButtonText( UiButton::No ), 0 );
        }
        else if ( buttons == MSG_OK_CANCEL ) {
            addButton( uiButtonText( UiButton::Okay ), 1 );
            addButton( uiButtonText( UiButton::Cancel ), 0 );
        }
    }

    // Position of the frame's top part in the window (ResizableFrameBox::_position.x =
    // center − leftSideOffset, dialog_box.cpp:120-122).
    QPoint framePos() const
    {
        return QPoint( ( width() - _frameW ) / 2 - ( _leftW - _frameW / 2 ), ( height() - _frameH ) / 2 );
    }
    QRect areaRect() const
    {
        const QPoint fp = framePos();
        return QRect( fp.x() + _area.x(), fp.y() + 99 - 35, _area.width(), _area.height() );
    }
    int buttonCount() const { return static_cast<int>( _buttons.size() ); }

    // Labels/buttons are configured in a subclass via paintEvent.
protected:
    // Port of fheroes2::ButtonGroup( area, buttonTypes ) (ui_button.cpp:480):
    // buttons stick to the bottom of the active area; one — centered, two —
    // with padding = free space / 4 from the area edges.
    void layoutWidgets() override
    {
        const QRect area = areaRect();
        if ( _buttons.size() == 1 ) {
            GameButton * b = _buttons[0];
            b->move( area.x() + ( area.width() - b->width() ) / 2, area.y() + area.height() - b->height() );
        }
        else if ( _buttons.size() == 2 ) {
            GameButton * left = _buttons[0];
            GameButton * right = _buttons[1];
            const int padding = ( area.width() - left->width() - right->width() ) / 4;
            left->move( area.x() + padding, area.y() + area.height() - left->height() );
            right->move( area.x() + area.width() - right->width() - padding, area.y() + area.height() - right->height() );
        }
    }

    void paintEvent( QPaintEvent * ) override
    {
        QImage buf = makeBuffer();
        QPainter p( &buf );

        const QPoint fp = framePos();
        const QPixmap leftTop = _assets.icnPixmap( "BUYBUILD.ICN", 4 );
        const QPixmap rightTop = _assets.icnPixmap( "BUYBUILD.ICN", 0 );
        const QPixmap leftMid = _assets.icnPixmap( "BUYBUILD.ICN", 5 );
        const QPixmap rightMid = _assets.icnPixmap( "BUYBUILD.ICN", 1 );
        const QPixmap leftBot = _assets.icnPixmap( "BUYBUILD.ICN", 6 );
        const QPixmap rightBot = _assets.icnPixmap( "BUYBUILD.ICN", 2 );

        // Port of ResizableFrameBox::redraw (dialog_box.cpp:135): the left column
        // [4]/[5]/[6] is aligned to the RIGHT edge of the window's left half
        // (leftWidth() = max of widths of [4],[5],[6]), the right one — right after it.
        // Middle chunks — by activeAreaHeight = 35, source (0,10): rows
        // 10..45 of the 45-high [5]/[1] sprite — exactly to the bottom edge.
        const int leftW = _leftW;
        int y = fp.y();
        p.drawPixmap( fp.x() + leftW - leftTop.width(), y, leftTop );
        p.drawPixmap( fp.x() + leftW, y, rightTop );
        y += leftTop.height();
        int middleH = _frameH - 99 - 81;
        while ( middleH > 0 ) {
            const int chunk = std::min( 35, middleH );
            p.drawPixmap( fp.x() + leftW - leftMid.width(), y, leftMid, 0, 10, leftMid.width(), chunk );
            p.drawPixmap( fp.x() + leftW, y, rightMid, 0, 10, rightMid.width(), chunk );
            y += chunk;
            middleH -= chunk;
        }
        p.drawPixmap( fp.x() + leftW - leftBot.width(), y, leftBot );
        p.drawPixmap( fp.x() + leftW, y, rightBot );

        drawContent( p );

        // Buttons — without shadows: in the engine ButtonGroup::draw() draws only
        // sprites, drawShadows() is not called in FrameBox dialogs.
        p.end();

        QPainter view( this );
        view.drawImage( 0, 0, buf );
    }

    virtual void drawContent( QPainter & p ) = 0;

    void addButton( const QString & text, int result )
    {
        GameButton * btn = makeGameTextButton( _assets, _font, text, this );
        const int r = result;
        connect( btn, &GameButton::clicked, this, [this, r]() {
            _result = r;
            accept();
        } );
        _buttons.push_back( btn );
    }

    int _result = 0;
    GameFont _font;
    const Assets & _assets;
    int _frameW = 304;
    int _frameH = 200;
    int _leftW = 161; // BUYBUILD left column width (per the sprites)
    QRect _area;
    std::vector<GameButton *> _buttons;
};

// Message: title (yellow), element (icon/frame), text, buttons.
// Port of fheroes2::showMessage + getDialogHeight (ui_dialog.cpp): title and
// text wrap by boxAreaWidthPx (260), lines are centered, the window height is
// computed from the number of lines, the element — like DialogElement.
class MessageDialog : public FrameBoxDialog
{
public:
    // Port of getDialogHeight (ui_dialog.cpp:238): content height without the button zone.
    static int contentHeightFor( const Assets & assets, const QString & title, const QString & body, const GameMessageElement & element )
    {
        constexpr int textOffsetY = 10;
        const GameFont font( &assets );

        const int headerHeight = title.isEmpty() ? 0
                                                 : font.textRows( title, GameFont::Size::NORMAL, BOX_AREA_W ) * font.lineHeight( GameFont::Size::NORMAL )
                                                       + textOffsetY;
        int overallTextHeight = headerHeight;

        // The body — a regular white font (normalWhite), like showStandardTextMessage.
        const int bodyTextHeight = body.isEmpty() ? 0
                                                  : font.textRows( body, GameFont::Size::NORMAL, BOX_AREA_W ) * font.lineHeight( GameFont::Size::NORMAL );
        if ( bodyTextHeight > 0 )
            overallTextHeight += bodyTextHeight + textOffsetY;

        int elementHeight = 0;
        const int elementAreaHeight = elementHeightOf( assets, element );
        if ( elementAreaHeight > 0 ) {
            if ( bodyTextHeight > 0 )
                elementHeight += textOffsetY;
            elementHeight += textOffsetY;
            elementHeight += elementAreaHeight;
        }

        return overallTextHeight + elementHeight;
    }

    static int elementHeightOf( const Assets & assets, const GameMessageElement & element )
    {
        switch ( element.kind ) {
        case GameMessageElement::ICON_TEXT:
            return 60;
        case GameMessageElement::SECONDARY_SKILL:
            return 71; // SECSKILL[15]
        case GameMessageElement::ARTIFACT:
            return 76; // RESOURCE[7]
        case GameMessageElement::PRIMARY_SKILL:
            return 105; // PRIMSKIL[4]
        case GameMessageElement::EXPERIENCE:
            return 64; // EXPMRL[4]
        case GameMessageElement::SPELL: {
            // Icon + 2 + name line (SpellDialogElement::_area).
            const QPixmap icn = spellIcon( assets, element.id );
            if ( icn.isNull() )
                return 0;
            const GameFont font( &assets );
            return icn.height() + 2 + font.lineHeight( GameFont::Size::SMALL );
        }
        default:
            return 0;
        }
    }

    MessageDialog( const Assets & assets, const QString & title, const QString & body, int buttons, const GameMessageElement & element, QWidget * parent )
        : FrameBoxDialog( assets, title, body, buttons, parent, contentHeightFor( assets, title, body, element ) )
        , _title( title )
        , _body( body )
        , _element( element )
    {}

    int result() const { return _result; }

protected:
    void drawContent( QPainter & p ) override
    {
        constexpr int textOffsetY = 10;
        const QRect area = areaRect();

        // Invisible text frame: all content is clipped to the dialog's active
        // area (Text::drawInRoi with imageRoi in fheroes2).
        p.save();
        p.setClipRect( area );

        // Title with wrapping by 260, lines are centered (Text::draw).
        const int headerHeight = _title.isEmpty()
                                     ? 0
                                     : _font.textRows( _title, GameFont::Size::NORMAL, BOX_AREA_W ) * _font.lineHeight( GameFont::Size::NORMAL )
                                           + textOffsetY;
        if ( !_title.isEmpty() )
            _font.drawTextWrapped( p, area.x(), area.y() + textOffsetY, BOX_AREA_W, _title, GameFont::Size::NORMAL, GameFont::Color::YELLOW );

        // Message text (normalWhite, wrapped by 260).
        if ( !_body.isEmpty() )
            _font.drawTextWrapped( p, area.x(), area.y() + textOffsetY + headerHeight, BOX_AREA_W, _body, GameFont::Size::NORMAL,
                                   GameFont::Color::WHITE );

        // Element: below the text, centered in the area (port of showMessage).
        if ( elementHeightOf( _assets, _element ) > 0 ) {
            int y = area.y() + headerHeight;
            if ( !_body.isEmpty() )
                y += _font.textRows( _body, GameFont::Size::NORMAL, BOX_AREA_W ) * _font.lineHeight( GameFont::Size::NORMAL ) + textOffsetY;
            y += textOffsetY;
            if ( !_body.isEmpty() )
                y += textOffsetY;
            drawElement( p, area, y );
        }

        p.restore();
    }

private:
    // Draws the element centered across the area's width; y — the top edge.
    void drawElement( QPainter & p, const QRect & area, int y )
    {
        const auto centered = [&]( const QString & text, GameFont::Size size, int rowY ) {
            QString t = text;
            const int maxWidth = 75; // skill icon width (fitToOneRow in the engine)
            if ( _font.textWidth( t, size ) > maxWidth )
                t = _font.elideText( t, size, maxWidth );
            const int w = _font.textWidth( t, size );
            _font.drawText( p, area.x() + ( area.width() - w ) / 2 - 1, y + rowY, t, size, GameFont::Color::WHITE );
        };

        switch ( _element.kind ) {
        case GameMessageElement::ICON_TEXT: {
            // Like before: icon + label side by side, centered.
            const QPixmap icn = _assets.icnPixmap( _element.iconIcn.toStdString(), _element.iconIndex );
            if ( icn.isNull() )
                break;
            const int tw = _font.textWidth( _element.text, GameFont::Size::SMALL );
            const int total = icn.width() + 8 + tw;
            const int x = area.x() + ( area.width() - total ) / 2;
            p.drawPixmap( x, y, icn );
            _font.drawText( p, x + icn.width() + 8, y + ( icn.height() - _font.lineHeight( GameFont::Size::SMALL ) ) / 2, _element.text,
                            GameFont::Size::SMALL, GameFont::Color::WHITE );
            break;
        }
        case GameMessageElement::SECONDARY_SKILL: {
            // Port of SecondarySkillDialogElement::draw (ui_dialog.cpp:759):
            // SECSKILL[15] frame + skill icon centered + name (y+8) and
            // level (y+56), the text is truncated to the icon width (fitToOneRow).
            const QPixmap bg = _assets.icnPixmap( ICN_SECSKILL, 15 );
            if ( bg.isNull() )
                break;
            const int x = area.x() + ( area.width() - bg.width() ) / 2;
            p.drawPixmap( x, y, bg );
            const QPixmap icn = _assets.icnPixmap( ICN_SECSKILL, _element.id );
            if ( !icn.isNull() )
                p.drawPixmap( x + ( bg.width() - icn.width() ) / 2, y + ( bg.height() - icn.height() ) / 2, icn );
            const QString name = QString::fromStdString( skillName( _element.id ) );
            centered( name, GameFont::Size::SMALL, 8 );
            const QString level = QString::fromStdString( skillLevelName( _element.level ) );
            centered( level, GameFont::Size::SMALL, 56 );
            break;
        }
        case GameMessageElement::ARTIFACT: {
            // Port of ArtifactDialogElement::draw (ui_dialog.cpp): RESOURCE[7]
            // frame + large ARTIFACT.ICN icon at (6,6).
            const QPixmap frame = _assets.icnPixmap( "RESOURCE.ICN", 7 );
            if ( frame.isNull() )
                break;
            const int x = area.x() + ( area.width() - frame.width() ) / 2;
            p.drawPixmap( x, y, frame );
            const QPixmap icn = _assets.icnPixmap( ICN_ARTIFACT, _element.id );
            if ( !icn.isNull() )
                p.drawPixmap( x + 6, y + 6, icn );
            break;
        }
        case GameMessageElement::PRIMARY_SKILL: {
            // Port of PrimarySkillDialogElement::draw: PRIMSKIL[4] frame,
            // skill icon centered, name at y+10.
            const QPixmap bg = _assets.icnPixmap( ICN_PRIMSKIL, 4 );
            if ( bg.isNull() )
                break;
            const int x = area.x() + ( area.width() - bg.width() ) / 2;
            p.drawPixmap( x, y, bg );
            const QPixmap icn = _assets.icnPixmap( ICN_PRIMSKIL, _element.id );
            if ( !icn.isNull() )
                p.drawPixmap( x + ( bg.width() - icn.width() ) / 2, y + ( bg.height() - icn.height() ) / 2, icn );
            const QString name = QString::fromStdString( primarySkillName( _element.id ) );
            const int w = _font.textWidth( name, GameFont::Size::SMALL );
            _font.drawText( p, x + ( bg.width() - w ) / 2, y + 10, name, GameFont::Size::SMALL, GameFont::Color::WHITE );
            break;
        }
        case GameMessageElement::EXPERIENCE: {
            // Port of ExperienceDialogElement::draw with zero experience: only
            // the EXPMRL[4] icon.
            const QPixmap icn = _assets.icnPixmap( "EXPMRL.ICN", 4 );
            if ( icn.isNull() )
                break;
            p.drawPixmap( area.x() + ( area.width() - icn.width() ) / 2, y, icn );
            break;
        }
        case GameMessageElement::SPELL: {
            // Port of SpellDialogElement::draw (ui_dialog.cpp): spell icon
            // centered, spell name below it (for mass spells — a doubled icon
            // via spellIcon).
            const QPixmap icn = spellIcon( _assets, _element.id );
            if ( icn.isNull() )
                break;
            const int x = area.x() + ( area.width() - icn.width() ) / 2;
            p.drawPixmap( x, y, icn );
            QString name = _element.text.isEmpty() ? QString::fromStdString( spellName( _element.id ) ) : _element.text;
            if ( _font.textWidth( name, GameFont::Size::SMALL ) > BOX_AREA_W )
                name = _font.elideText( name, GameFont::Size::SMALL, BOX_AREA_W );
            const int w = _font.textWidth( name, GameFont::Size::SMALL );
            _font.drawText( p, area.x() + ( area.width() - w ) / 2, y + icn.height() + 2, name, GameFont::Size::SMALL,
                            GameFont::Color::WHITE );
            break;
        }
        default:
            break;
        }
    }

    QString _title;
    QString _body;
    GameMessageElement _element;
};

int showGameMessageImpl( QWidget * parent, const Assets & assets, const QString & title, const QString & body, int buttons, const GameMessageElement & element )
{
    MessageDialog dlg( assets, title, body, buttons, element, parent );
    dlg.exec();
    return dlg.result();
}

// --- Count dialog (port of Dialog::SelectCount) ---
class SelectCountDialog : public FrameBoxDialog
{
public:
    // Height of the element above the selector (topUiElement in the engine).
    static int elementHeightOf( const SelectCountElement & element )
    {
        switch ( element.kind ) {
        case SelectCountElement::PRIMARY_SKILL:
            return 105; // PRIMSKIL[4]
        case SelectCountElement::EXPERIENCE:
            return 64; // EXPMRL[4]
        case SelectCountElement::MONSTER:
            return 105; // STRIP[12]
        default:
            return 0;
        }
    }

    // Content height — like in Dialog::SelectCount (dialog_selectcount.cpp:94):
    // headerHeight + 10 (elementOffset) + element height + 10 + selector
    // height (30 in the engine: selectionAreaHeight).
    static int contentHeightFor( const Assets & assets, const QString & header, const SelectCountElement & element )
    {
        const GameFont font( &assets );
        const int headerHeight = font.textRows( header, GameFont::Size::NORMAL, BOX_AREA_W ) * font.lineHeight( GameFont::Size::NORMAL );
        const int topH = elementHeightOf( element );
        return headerHeight + 10 + topH + ( topH > 0 ? 10 : 0 ) + 30;
    }

    SelectCountDialog( const Assets & assets, const QString & header, int min, int max, int value, QWidget * parent, const SelectCountElement & element )
        : FrameBoxDialog( assets, header, QString(), MSG_OK_CANCEL, parent, contentHeightFor( assets, header, element ) )
        , _header( header )
        , _min( min )
        , _max( max )
        , _value( value )
        , _element( element )
    {
        _editBox = _assets.icnPixmap( "TOWNWIND.ICN", 4 );
        for ( int i = 0; i < 4; ++i )
            _arrows[i] = _assets.icnPixmap( "TOWNWIND.ICN", 5 + i );
        // MIN/MAX button labels — from fheroes2 translations (non-original
        // resources: UNIFORM_GOOD_MAX/MIN_BUTTON are generated as text).
        _maxBtn = makeGameTextButton( assets, _font, uiButtonText( UiButton::Max ), this );
        _minBtn = makeGameTextButton( assets, _font, uiButtonText( UiButton::Min ), this );
        connect( _maxBtn, &GameButton::clicked, this, [this]() { setValue( _max ); } );
        connect( _minBtn, &GameButton::clicked, this, [this]() { setValue( _min ); } );
        setMouseTracking( true );
    }

    int value() const { return _value; }
    bool accepted() const { return _result == 1; }

protected:
    void drawContent( QPainter & p ) override
    {
        // Dialog::SelectCount geometry (dialog_selectcount.cpp:101-134):
        // title from the very top of the active area, element centered,
        // selector at (area.x + 38).
        const QRect area = areaRect();

        // Invisible text frame: the content is clipped to the active area.
        p.save();
        p.setClipRect( area );

        _font.drawTextWrapped( p, area.x(), area.y(), BOX_AREA_W, _header, GameFont::Size::NORMAL, GameFont::Color::WHITE );

        const int headerHeight = _font.textRows( _header, GameFont::Size::NORMAL, BOX_AREA_W ) * _font.lineHeight( GameFont::Size::NORMAL );
        int offsetY = headerHeight + 10;

        const int topH = elementHeightOf( _element );
        if ( topH > 0 ) {
            drawElement( p, area, area.y() + offsetY );
            offsetY += topH + 10;
        }

        // Selector (ValueSelectionDialogElement::getArea, ui_dialog.cpp:1038):
        // width = field + 6 + arrow, height = arrow height × 2 + 5.
        const int arrowW = _arrows[0].width();
        const int arrowH = _arrows[0].height();
        const int totalH = arrowH * 2 + 5;
        const int totalW = _editBox.width() + 6 + arrowW;
        const int sx = area.x() + 38;
        const int sy = area.y() + offsetY;
        _selector = QRect( sx, sy, totalW, totalH );
        _editRect = QRect( sx, sy + ( totalH - _editBox.height() ) / 2, _editBox.width(), _editBox.height() );
        p.drawPixmap( _editRect.topLeft(), _editBox );

        const QString text = QString::number( _value );
        const int tw = _font.textWidth( text, GameFont::Size::NORMAL );
        _font.drawText( p, _editRect.x() + ( _editRect.width() - tw ) / 2, _editRect.y() + ( _editRect.height() - 13 ) / 2, text, GameFont::Size::NORMAL,
                        GameFont::Color::WHITE );

        // Arrows: position minus the sprite's hotspot (3,2), like setPosition in the engine.
        const IcnSprite & arrowSprite = _assets.icnSprite( "TOWNWIND.ICN", 5 );
        const int arrowX = sx + _editBox.width() + 6 - arrowSprite.offsetX;
        _upRect = QRect( arrowX, sy - arrowSprite.offsetY, arrowW, arrowH );
        _downRect = QRect( arrowX, sy - arrowSprite.offsetY + arrowH + 5, arrowW, arrowH );
        p.drawPixmap( _upRect.topLeft(), _arrows[_upPressed ? 1 : 0] );
        p.drawPixmap( _downRect.topLeft(), _arrows[_downPressed ? 3 : 2] );

        // MIN/MAX to the right of the selector (dialog_selectcount.cpp:131).
        _maxBtn->move( sx + totalW + 6, sy );
        _minBtn->move( sx + totalW + 6, sy );
        _maxBtn->setVisible( _value <= _min );
        _minBtn->setVisible( _value > _min );

        p.restore();
    }

    // Top element (topUiElement): skill/experience/monster picture.
    void drawElement( QPainter & p, const QRect & area, int y )
    {
        switch ( _element.kind ) {
        case SelectCountElement::PRIMARY_SKILL: {
            // Port of PrimarySkillDialogElement::draw: PRIMSKIL[4] frame +
            // skill icon centered + name at y+10 (the value is not drawn —
            // it is empty in the map editor).
            const QPixmap bg = _assets.icnPixmap( ICN_PRIMSKIL, 4 );
            if ( bg.isNull() )
                break;
            const int x = area.x() + ( area.width() - bg.width() ) / 2;
            p.drawPixmap( x, y, bg );
            const QPixmap icn = _assets.icnPixmap( ICN_PRIMSKIL, _element.id );
            if ( !icn.isNull() )
                p.drawPixmap( x + ( bg.width() - icn.width() ) / 2, y + ( bg.height() - icn.height() ) / 2, icn );
            const QString name = QString::fromStdString( primarySkillName( _element.id ) );
            const int w = _font.textWidth( name, GameFont::Size::SMALL );
            _font.drawText( p, x + ( bg.width() - w ) / 2, y + 10, name, GameFont::Size::SMALL, GameFont::Color::WHITE );
            break;
        }
        case SelectCountElement::EXPERIENCE: {
            // Port of ExperienceDialogElement::draw with zero experience: EXPMRL[4].
            const QPixmap icn = _assets.icnPixmap( "EXPMRL.ICN", 4 );
            if ( icn.isNull() )
                break;
            p.drawPixmap( area.x() + ( area.width() - icn.width() ) / 2, y, icn );
            break;
        }
        case SelectCountElement::MONSTER: {
            // Port of MonsterDialogElement::draw (the current engine): STRIP[12]
            // frame + monster sprite at (6,6) accounting for its hotspot.
            const QPixmap bg = _assets.icnPixmap( "STRIP.ICN", 12 );
            if ( bg.isNull() )
                break;
            const int x = area.x() + ( area.width() - bg.width() ) / 2;
            p.drawPixmap( x, y, bg );
            char icnName[32];
            snprintf( icnName, sizeof( icnName ), "MONH%04d.ICN", _element.id - 1 );
            const IcnSprite & sprite = _assets.icnSprite( icnName, 0 );
            if ( sprite.isNull() )
                break;
            const QPixmap pm = _assets.icnPixmap( icnName, 0 );
            p.drawPixmap( x + 6 + sprite.offsetX, y + 6 + sprite.offsetY, pm );
            break;
        }
        default:
            break;
        }
    }

    void mousePressEvent( QMouseEvent * event ) override
    {
        if ( event->button() == Qt::LeftButton ) {
            if ( _upRect.contains( event->pos() ) ) {
                _upPressed = true;
                _holdTimer.start( 450 );
                setValue( _value + 1 );
                update();
                return;
            }
            if ( _downRect.contains( event->pos() ) ) {
                _downPressed = true;
                _holdTimer.start( 450 );
                setValue( _value - 1 );
                update();
                return;
            }
            if ( _editRect.contains( event->pos() ) ) {
                // Virtual numpad, like openVirtualNumpad in the game.
                if ( openNumpad( window(), _assets, _value, _min, _max ) )
                    update();
                return;
            }
        }
        if ( event->button() == Qt::RightButton && _editRect.contains( event->pos() ) ) {
            const QString text = gameText( "The available values range from %{min} to %{max}." )
                                     .replace( "%{min}", QString::number( _min ) )
                                     .replace( "%{max}", QString::number( _max ) );
            showGameMessage( window(), _assets, QString(), text, MSG_OK );
            return;
        }
        FrameBoxDialog::mousePressEvent( event );
    }

    void mouseReleaseEvent( QMouseEvent * event ) override
    {
        _upPressed = false;
        _downPressed = false;
        _holdTimer.stop();
        update();
        FrameBoxDialog::mouseReleaseEvent( event );
    }

    void wheelEvent( QWheelEvent * event ) override
    {
        if ( _selector.contains( event->position().toPoint() ) )
            setValue( _value + ( event->angleDelta().y() > 0 ? 1 : -1 ) );
    }

    void keyPressEvent( QKeyEvent * event ) override
    {
        if ( event->key() == Qt::Key_Up ) {
            setValue( _value + 1 );
            return;
        }
        if ( event->key() == Qt::Key_Down ) {
            setValue( _value - 1 );
            return;
        }
        FrameBoxDialog::keyPressEvent( event );
    }

private:
    void setValue( int v )
    {
        _value = std::clamp( v, _min, _max );
        update();
    }

    QString _header;
    int _min = 0;
    int _max = 0;
    int _value = 0;
    SelectCountElement _element;
    QPixmap _editBox;
    QPixmap _arrows[4];
    QRect _selector;
    QRect _editRect;
    QRect _upRect;
    QRect _downRect;
    bool _upPressed = false;
    bool _downPressed = false;
    GameButton * _maxBtn = nullptr;
    GameButton * _minBtn = nullptr;
    QTimer _holdTimer;
};

// --- Virtual numpad (port of openVirtualNumpad) ---
class NumpadDialog : public GameWindow
{
public:
    NumpadDialog( const Assets & assets, int & value, int min, int max, QWidget * parent )
        : GameWindow( &assets, QSize( 320, 291 ), parent )
        , _font( &assets )
        , _assets( assets )
        , _value( value )
        , _min( min )
        , _max( max )
    {
        setFocusPolicy( Qt::StrongFocus );
        _text = QString::number( value );
        _cursorPos = _text.size();

        // Input field (REQBKG 40,286,268×21); its position — in layoutWidgets.
        const QPixmap bg = assets.icnPixmap( "REQBKG.ICN", 0 );
        if ( !bg.isNull() )
            _inputBg = bg.copy( 40, 286, 268, 21 );

        // Keys: 1 2 3 / 4 5 6 / 7 8 9 / [empty][0][empty][Backspace].
        const QStringList rows = { "123", "456", "789" };
        for ( int r = 0; r < 3; ++r ) {
            for ( int c = 0; c < 3; ++c ) {
                GameButton * btn = makeGameTextButton( assets, _font, QString( rows[r][c] ), this, 30 );
                _keys.push_back( btn );
                connect( btn, &GameButton::clicked, this, [this, ch = rows[r][c]]() { insertDigit( ch ); } );
            }
        }
        _key0 = makeGameTextButton( assets, _font, QStringLiteral( "0" ), this, 30 );
        connect( _key0, &GameButton::clicked, this, [this]() { insertDigit( QLatin1Char( '0' ) ); } );
        _keyBack = makeGameTextButton( assets, _font, QStringLiteral( "<"), this, 54 );
        connect( _keyBack, &GameButton::clicked, this, [this]() {
            if ( _cursorPos > 0 ) {
                _text.remove( _cursorPos - 1, 1 );
                --_cursorPos;
                update();
            }
        } );

        _btnOk = makeGameTextButton( assets, _font, uiButtonText( UiButton::Okay ), this );
        connect( _btnOk, &GameButton::clicked, this, &NumpadDialog::accept );
    }

    int exec() override
    {
        const int result = GameWindow::exec();
        if ( result == 1 ) {
            bool ok = false;
            const int v = _text.toInt( &ok );
            if ( !ok ) {
                showGameMessage( window(), _assets, gameText( "Error" ), gameText( "The entered value is invalid." ), MSG_OK );
                return exec();
            }
            if ( v < _min || v > _max ) {
                const QString text = gameText( "The entered value is out of range.\nIt should be not less than %{minValue} and not more than %{maxValue}." )
                                         .replace( "%{minValue}", QString::number( _min ) )
                                         .replace( "%{maxValue}", QString::number( _max ) );
                showGameMessage( window(), _assets, gameText( "Error" ), text, MSG_OK );
                return exec();
            }
            _value = v;
            return 1;
        }
        return result;
    }

protected:
    void layoutWidgets() override
    {
        const QRect area = activeArea();
        _inputRect = QRect( area.x() + ( area.width() - 268 ) / 2, area.y() + 20, 268, 21 );
        // Key grid: rows y = 50, 91, 132; the last one — 173.
        for ( int r = 0; r < 3; ++r ) {
            int x = area.x() + 25 + ( 270 - ( 3 * 30 + 2 * 8 ) ) / 2;
            for ( int c = 0; c < 3; ++c ) {
                _keys[r * 3 + c]->setGeometry( x, area.y() + 50 + r * 41, 30, 25 );
                x += 30 + 8;
            }
        }
        int x = area.x() + 25 + ( 270 - ( 30 + 8 + 30 + 8 + 30 + 8 + 54 ) ) / 2;
        _key0->setGeometry( x + 30 + 8 + 30 + 8, area.y() + 173, 30, 25 );
        _keyBack->setGeometry( x + 30 + 8 + 30 + 8 + 30 + 8, area.y() + 173, 54, 25 );
        _btnOk->setGeometry( area.x() + ( area.width() - 95 ) / 2, area.y() + area.height() - 35, 95, 25 );
    }

    void paintEvent( QPaintEvent * event ) override
    {
        GameWindow::paintEvent( event );
        QPainter p( this );
        p.drawPixmap( _inputRect.topLeft(), _inputBg );
        // Text — white, centered on the dark REQBKG field (like TextInputField in
        // fheroes2: there is no white fill over the background).
        const int w = _font.textWidth( _text, GameFont::Size::NORMAL );
        _font.drawText( p, _inputRect.x() + std::max( 0, ( _inputRect.width() - w ) / 2 ), _inputRect.y() + 2, _text, GameFont::Size::NORMAL,
                        GameFont::Color::WHITE );
        const int cx = _inputRect.x() + ( _inputRect.width() - w ) / 2 + _font.textWidth( _text.left( _cursorPos ), GameFont::Size::NORMAL );
        p.fillRect( cx, _inputRect.y() + 2, 2, 15, QColor( 0, 0, 0, 220 ) );
        p.end();
    }

    void mousePressEvent( QMouseEvent * event ) override
    {
        // Click inside the field places the caret (nearest character boundary).
        if ( _inputRect.contains( event->position().toPoint() ) ) {
            const int w = _font.textWidth( _text, GameFont::Size::NORMAL );
            const int target = event->position().toPoint().x() - _inputRect.x() - ( _inputRect.width() - w ) / 2;
            int best = 0;
            int bestDist = target;
            for ( int i = 0; i <= _text.size(); ++i ) {
                const int width = _font.textWidth( _text.left( i ), GameFont::Size::NORMAL );
                const int dist = std::abs( width - target );
                if ( dist < bestDist ) {
                    bestDist = dist;
                    best = i;
                }
            }
            _cursorPos = best;
            update();
        }
        GameWindow::mousePressEvent( event );
    }

    void keyPressEvent( QKeyEvent * event ) override
    {
        switch ( event->key() ) {
        case Qt::Key_Left:
            if ( _cursorPos > 0 ) {
                --_cursorPos;
                update();
            }
            return;
        case Qt::Key_Right:
            if ( _cursorPos < _text.size() ) {
                ++_cursorPos;
                update();
            }
            return;
        case Qt::Key_Home:
            if ( _cursorPos != 0 ) {
                _cursorPos = 0;
                update();
            }
            return;
        case Qt::Key_End:
            if ( _cursorPos != _text.size() ) {
                _cursorPos = _text.size();
                update();
            }
            return;
        case Qt::Key_Backspace:
            if ( _cursorPos > 0 ) {
                _text.remove( _cursorPos - 1, 1 );
                --_cursorPos;
                update();
            }
            return;
        case Qt::Key_Delete:
            if ( _cursorPos < _text.size() ) {
                _text.remove( _cursorPos, 1 );
                update();
            }
            return;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            accept();
            return;
        default:
            break;
        }
        const QString t = event->text();
        if ( !t.isEmpty() && t.at( 0 ).isDigit() && _text.size() < 10 ) {
            _text.insert( _cursorPos, t );
            _cursorPos += t.size();
            update();
            return;
        }
        GameWindow::keyPressEvent( event );
    }

private:
    void insertDigit( QChar ch )
    {
        if ( _text.size() < 10 ) {
            _text.insert( _cursorPos, ch );
            ++_cursorPos;
            update();
        }
    }

    GameFont _font;
    const Assets & _assets;
    int & _value;
    int _min = 0;
    int _max = 0;
    QString _text;
    int _cursorPos = 0;
    QRect _inputRect;
    QPixmap _inputBg;
    std::vector<GameButton *> _keys;
    GameButton * _key0 = nullptr;
    GameButton * _keyBack = nullptr;
    GameButton * _btnOk = nullptr;
};

// --- Selection list (port of Dialog::ItemSelectionWindow + Interface::ListBox) ---
class ItemSelectionDialog : public GameWindow
{
public:
    struct Item {
        QPixmap icon;
        QString text;
        int id = 0;
    };

    ItemSelectionDialog( const Assets & assets, const QSize & size, const QString & title, const QString & description, int iconOffsetX, int textOffsetX,
                         int itemHeight, QWidget * parent )
        : GameWindow( &assets, size, parent )
        , _font( &assets )
        , _assets( assets )
        , _title( title )
        , _description( description )
        , _iconOffsetX( iconOffsetX )
        , _textOffsetX( textOffsetX )
        , _itemHeight( itemHeight )
    {
        setMouseTracking( true );
        // Buttons are textual with translation (fheroes2 ItemSelectionWindow:
        // BUTTON_OK/BUTTON_CANCEL, non-original resources are generated as text).
        _btnOk = makeGameTextButton( assets, _font, uiButtonText( UiButton::Okay ), this );
        _btnCancel = makeGameTextButton( assets, _font, uiButtonText( UiButton::Cancel ), this );
        connect( _btnOk, &GameButton::clicked, this, [this]() {
            _result = currentId();
            accept();
        } );
        connect( _btnCancel, &GameButton::clicked, this, &GameWindow::reject );

        _scroll = new GameScrollBar( &assets, this );
        connect( _scroll, &GameScrollBar::valueChanged, this, [this]( int value ) {
            _topId = value;
            update();
        } );
    }

    void setItems( const QVector<Item> & items, int currentId )
    {
        _items = items;
        _current = -1;
        for ( int i = 0; i < _items.size(); ++i ) {
            if ( _items[i].id == currentId )
                _current = i;
        }
        // The list geometry is known only after layoutWidgets (the window is
        // shown), so here we only mark the position as invalid: Verify() will
        // center the view on the selected item (interface_list.h:624).
        _topId = -1;
        update();
    }

    int currentId() const
    {
        return _current >= 0 && _current < _items.size() ? _items[_current].id : 0;
    }

    void setOnRightClickItem( std::function<void( int id )> handler ) { _onRightClickItem = std::move( handler ); }

    int result() const { return _result; }

protected:
    // Items area (SetAreaItems, dialog_selectitems.cpp:927).
    QRect itemsArea() const { return _listRect.adjusted( 5, 5, -5, -5 ); }

    // maxItems = rtAreaItems.height / _offsetY (dialog_selectitems.cpp:137).
    int maxItems() const { return std::max( 1, itemsArea().height() / _itemHeight ); }

    int maxTopId() const { return std::max( 0, static_cast<int>( _items.size() ) - maxItems() ); }

    // Port of ListBox::Verify (interface_list.h:624): fixes topId if it is out of
    // bounds — with a given selection the view is centered on it.
    void verify()
    {
        if ( _items.isEmpty() ) {
            _current = -1;
            _topId = -1;
            return;
        }
        if ( _current >= _items.size() )
            _current = -1;
        const int maxTop = maxTopId();
        if ( _topId < 0 || _topId > maxTop )
            _topId = _current < 0 ? 0 : std::clamp( _current - maxItems() / 2, 0, maxTop );
        syncScrollbar();
    }

    // Port of ListBox::SetCurrentVisible (interface_list.h:268).
    void setCurrentVisible()
    {
        verify();
        if ( _items.isEmpty() )
            return;
        if ( _current >= 0 && ( _topId > _current || _topId + maxItems() <= _current ) ) {
            if ( _topId > _current )
                _topId = _current;
            else
                _topId = _current + 1 - maxItems();
        }
        syncScrollbar();
    }

    void syncScrollbar()
    {
        _scroll->setRange( _items.size(), maxItems() );
        _scroll->setValue( std::max( 0, _topId ) );
    }

    int lastVisibleId() const { return std::min( _topId + maxItems(), static_cast<int>( _items.size() ) ) - 1; }

    void setTopId( int topId )
    {
        _topId = std::clamp( topId, 0, maxTopId() );
        syncScrollbar();
        update();
    }

    void layoutWidgets() override
    {
        // ItemSelectionWindow geometry (dialog_selectitems.cpp:911-940).
        const QRect area = activeArea();
        const int listOffsetY = _description.isEmpty() ? 0 : _font.lineHeight( GameFont::Size::NORMAL ) + 3;
        _listRect = QRect( area.x() + 10, area.y() + 30 + listOffsetY, area.width() - 40, area.height() - 70 - listOffsetY );

        // gapsFromEdges(20, 7) + Padding::BOTTOM_LEFT/RIGHT (ui_window.cpp:446).
        _btnOk->move( area.x() + 20, area.y() + area.height() - _btnOk->height() - 7 );
        _btnCancel->move( area.x() + area.width() - _btnCancel->width() - 20, area.y() + area.height() - _btnCancel->height() - 7 );

        // Scrollbar: x = area.x + area.width − 25, along the list height.
        _scroll->setGeometry( area.x() + area.width() - 25, _listRect.y(), 16, _listRect.height() );
        setCurrentVisible();
    }

    void paintEvent( QPaintEvent * event ) override
    {
        GameWindow::paintEvent( event );
        QPainter p( this );
        const QRect area = activeArea();
        drawCentered( p, _font, area.x(), area.y() + 10, area.width(), _title, GameFont::Size::NORMAL, GameFont::Color::YELLOW );
        if ( !_description.isEmpty() )
            drawCentered( p, _font, area.x(), area.y() + 30, area.width(), _description, GameFont::Size::NORMAL, GameFont::Color::WHITE );

        // Darkened list background (applyTextBackgroundShading).
        p.fillRect( _listRect, QColor( 0, 0, 0, 90 ) );

        // Rows: exactly maxItems of them, step — rtAreaItems.height / maxItems,
        // like ListBox::Redraw (interface_list.h:204-210).
        const QRect items = itemsArea();
        const int visible = maxItems();
        const int step = items.height() / visible;
        const int top = std::max( 0, _topId );
        p.setClipRect( items );
        for ( int i = 0; i < visible && top + i < _items.size(); ++i ) {
            const Item & item = _items[top + i];
            const int y = items.y() + i * step;
            const bool current = ( top + i == _current );
            if ( !item.icon.isNull() )
                p.drawPixmap( items.x() + _iconOffsetX - item.icon.width() / 2, y + _itemHeight / 2 - item.icon.height() / 2, item.icon );
            // The text is clipped to area.width − textOffsetX − 55 (renderText).
            _font.drawText( p, items.x() + _textOffsetX, y + _itemHeight / 2 - _font.lineHeight( GameFont::Size::NORMAL ) / 2 + 2, item.text,
                            GameFont::Size::NORMAL, current ? GameFont::Color::YELLOW : GameFont::Color::WHITE, area.width() - _textOffsetX - 55 );
        }
        p.setClipping( false );
        p.end();
    }

    int itemAt( const QPoint & pos ) const
    {
        const QRect items = itemsArea();
        if ( !items.contains( pos ) )
            return -1;
        // (mouse.y − rtAreaItems.y) * maxItems / rtAreaItems.height + topId.
        const int id = ( pos.y() - items.y() ) * maxItems() / items.height() + std::max( 0, _topId );
        return id < _items.size() ? id : -1;
    }

    void mousePressEvent( QMouseEvent * event ) override
    {
        setFocus();
        if ( event->button() == Qt::LeftButton && itemsArea().contains( event->pos() ) ) {
            // Start of a possible drag-scroll over the list (interface_list.h:497).
            _dragStartY = event->pos().y();
            _dragging = true;
            _dragScrolled = false;
            return;
        }
        GameWindow::mousePressEvent( event );
    }

    void mouseMoveEvent( QMouseEvent * event ) override
    {
        if ( !_dragging || ( event->buttons() & Qt::LeftButton ) == 0 ) {
            GameWindow::mouseMoveEvent( event );
            return;
        }
        const QRect items = itemsArea();
        const int delta = _dragStartY - event->pos().y();
        const int itemSize = items.height() / maxItems();
        if ( std::abs( delta ) > itemSize ) {
            setTopId( _topId + delta / itemSize );
            _dragStartY = event->pos().y();
            _dragScrolled = true; // releasing the button is not counted as a click on a row
        }
    }

    void mouseReleaseEvent( QMouseEvent * event ) override
    {
        if ( _dragging ) {
            _dragging = false;
            if ( !_dragScrolled && event->button() == Qt::LeftButton ) {
                const int idx = itemAt( event->pos() );
                if ( idx >= 0 && idx != _current ) {
                    _current = idx;
                    update();
                }
            }
            return;
        }
        // Right click on a row — a popup (ActionListPressRight in the engine).
        if ( event->button() == Qt::RightButton && itemsArea().contains( event->pos() ) && _onRightClickItem ) {
            const int idx = itemAt( event->pos() );
            if ( idx >= 0 && idx < _items.size() ) {
                _current = idx;
                update();
                _onRightClickItem( _items[idx].id );
            }
            return;
        }
        GameWindow::mouseReleaseEvent( event );
    }

    void mouseDoubleClickEvent( QMouseEvent * event ) override
    {
        // A double click accepts the selection only on the already highlighted
        // row (interface_list.h:558).
        const int idx = itemAt( event->pos() );
        if ( idx >= 0 && idx == _current ) {
            _result = currentId();
            accept();
            return;
        }
        if ( idx >= 0 ) {
            _current = idx;
            update();
            return;
        }
        GameWindow::mouseDoubleClickEvent( event );
    }

    void wheelEvent( QWheelEvent * event ) override
    {
        // The wheel works only over the list and over the scrollbar (localevent.h:291).
        const QPoint pos = event->position().toPoint();
        if ( !itemsArea().contains( pos ) && !_scroll->geometry().contains( pos ) )
            return;
        _wheelDelta += event->angleDelta().y();
        while ( _wheelDelta >= 120 ) {
            _wheelDelta -= 120;
            setTopId( _topId - 1 );
        }
        while ( _wheelDelta <= -120 ) {
            _wheelDelta += 120;
            setTopId( _topId + 1 );
        }
    }

    void keyPressEvent( QKeyEvent * event ) override
    {
        // Keyboard port of ListBox::QueueEventProcessing (interface_list.h:368-455).
        switch ( event->key() ) {
        case Qt::Key_Up:
            if ( _current > 0 ) {
                --_current;
                setCurrentVisible();
                update();
            }
            return;
        case Qt::Key_Down:
            if ( _current + 1 < _items.size() ) {
                ++_current;
                setCurrentVisible();
                update();
            }
            return;
        case Qt::Key_PageUp: {
            setCurrentVisible();
            if ( _topId > 0 ) {
                const int prevTop = _topId;
                _topId = _topId > maxItems() ? _topId - maxItems() : 0;
                syncScrollbar();
                _current = std::clamp( _current - ( prevTop - _topId ), _topId, lastVisibleId() );
            }
            else if ( _current != 0 ) {
                _current = 0;
            }
            update();
            return;
        }
        case Qt::Key_PageDown: {
            setCurrentVisible();
            if ( _topId + maxItems() < _items.size() ) {
                const int prevTop = _topId;
                _topId = std::min( _topId + maxItems(), maxTopId() );
                syncScrollbar();
                _current = std::clamp( _current + ( _topId - prevTop ), _topId, lastVisibleId() );
            }
            else if ( _current < _items.size() - 1 ) {
                _current = _items.size() - 1;
            }
            update();
            return;
        }
        case Qt::Key_Home:
            _topId = 0;
            _current = _items.isEmpty() ? -1 : 0;
            syncScrollbar();
            update();
            return;
        case Qt::Key_End:
            _topId = maxTopId();
            _current = static_cast<int>( _items.size() ) - 1;
            syncScrollbar();
            update();
            return;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            _result = currentId();
            accept();
            return;
        default:
            break;
        }
        GameWindow::keyPressEvent( event );
    }

    GameFont _font;
    const Assets & _assets;
    QString _title;
    QString _description;
    int _iconOffsetX = 22;
    int _textOffsetX = 50;
    int _itemHeight = 43;
    QRect _listRect;
    QVector<Item> _items;
    int _current = -1;
    int _topId = 0;
    int _result = 0;
    int _wheelDelta = 0;
    int _dragStartY = 0;
    bool _dragging = false;
    bool _dragScrolled = false;
    GameButton * _btnOk = nullptr;
    GameButton * _btnCancel = nullptr;
    GameScrollBar * _scroll = nullptr;
    std::function<void( int id )> _onRightClickItem; // right click on a row (ActionListPressRight)
};

// Spell icon (with mass-spell generation, see §18).
QPixmap spellIcon( const Assets & assets, int spellId )
{
    const int idx = spellIconIndex( spellId );
    if ( idx < 60 )
        return assets.icnPixmap( ICN_SPELLS, idx, 1.0 );

    int base = -1;
    switch ( idx ) {
    case 60: base = 6; break;
    case 61: base = 14; break;
    case 62: base = 1; break;
    case 63: base = 7; break;
    case 64: base = 3; break;
    case 65: base = 15; break;
    default:
        break;
    }
    if ( base >= 0 ) {
        const QPixmap basePm = assets.icnPixmap( ICN_SPELLS, base, 1.0 );
        if ( basePm.isNull() )
            return {};
        QPixmap out( basePm.width() + 8, basePm.height() + 8 );
        out.fill( Qt::transparent );
        QPainter p( &out );
        p.setOpacity( 128.0 / 255.0 );
        p.drawPixmap( 0, 0, basePm );
        p.setOpacity( 192.0 / 255.0 );
        p.drawPixmap( 4, 4, basePm );
        p.setOpacity( 1.0 );
        p.drawPixmap( 8, 8, basePm );
        p.end();
        return out;
    }
    if ( idx == 73 ) {
        const QPixmap magic = assets.icnPixmap( "MAGIC07.ICN", 2 );
        if ( magic.isNull() || magic.width() < 67 || magic.height() < 63 )
            return {};
        return magic.copy( 28, 27, 39, 36 );
    }
    return {};
}

// --- String input (port of Dialog::inputString) ---
// Content height for the name input dialog: title/body lines + the input
// field + a gap below it (the button zone is added by FrameBoxDialog).
int inputContentHeight( const Assets & assets, const QString & title, const QString & body )
{
    const GameFont font( &assets );
    int h = 0;
    if ( !title.isEmpty() )
        h += font.lineHeight( GameFont::Size::NORMAL ) + 10;
    if ( !body.isEmpty() )
        h += font.lineHeight( GameFont::Size::SMALL ) + 10;
    h += assets.icnPixmap( "BUYBUILD.ICN", 3 ).height() + 20;
    return h;
}

class InputStringDialog : public FrameBoxDialog
{
public:
    InputStringDialog( const Assets & assets, const QString & title, const QString & body, const QString & initial, int charLimit, QWidget * parent )
        : FrameBoxDialog( assets, title, body, MSG_OK_CANCEL, parent, inputContentHeight( assets, title, body ) )
        , _font( &assets )
        , _title( title )
        , _body( body )
        , _text( initial )
        , _charLimit( charLimit )
        , _cursorPos( initial.size() )
    {
        setFocusPolicy( Qt::StrongFocus );
        _cursorTimer.setInterval( 450 );
        connect( &_cursorTimer, &QTimer::timeout, this, [this]() {
            _cursorOn = !_cursorOn;
            update();
        } );
        _cursorTimer.start();
    }

    QString result() const { return _result == 1 ? _text : QString(); }

protected:
    void drawContent( QPainter & p ) override
    {
        const QRect area = areaRect();

        // Invisible text frame: the content is clipped to the active area.
        p.save();
        p.setClipRect( area );

        int y = area.y() + 10;
        if ( !_title.isEmpty() ) {
            drawCentered( p, _font, area.x(), y, area.width(), _title, GameFont::Size::NORMAL, GameFont::Color::YELLOW );
            y += _font.lineHeight( GameFont::Size::NORMAL ) + 10;
        }
        if ( !_body.isEmpty() ) {
            drawCentered( p, _font, area.x(), y, area.width(), _body, GameFont::Size::NORMAL, GameFont::Color::WHITE );
            y += _font.lineHeight( GameFont::Size::NORMAL ) + 10;
        }

        // Input field (BUYBUILD[3]).
        const QPixmap field = _assets.icnPixmap( "BUYBUILD.ICN", 3 );
        _fieldRect = QRect( area.x() + ( area.width() - field.width() ) / 2, y, field.width(), field.height() );
        p.drawPixmap( _fieldRect.topLeft(), field );
        const QRect textRect = _fieldRect.adjusted( 13, 1, -13, -3 );
        _font.drawText( p, textRect.x(), textRect.y(), _text, GameFont::Size::NORMAL, GameFont::Color::WHITE, textRect.width() );
        if ( _cursorOn && hasFocus() ) {
            const int cx = textRect.x() + _font.textWidth( _text.left( _cursorPos ), GameFont::Size::NORMAL );
            p.fillRect( cx, textRect.y() + 2, 2, textRect.height() - 4, QColor( 255, 255, 255, 200 ) );
        }

        p.restore();
    }

    void keyPressEvent( QKeyEvent * event ) override
    {
        // Cursor movement (like the engine's InsertKeySym + KEY_LEFT/KEY_RIGHT).
        switch ( event->key() ) {
        case Qt::Key_Left:
            if ( _cursorPos > 0 ) {
                --_cursorPos;
                update();
            }
            return;
        case Qt::Key_Right:
            if ( _cursorPos < _text.size() ) {
                ++_cursorPos;
                update();
            }
            return;
        case Qt::Key_Home:
            if ( _cursorPos != 0 ) {
                _cursorPos = 0;
                update();
            }
            return;
        case Qt::Key_End:
            if ( _cursorPos != _text.size() ) {
                _cursorPos = _text.size();
                update();
            }
            return;
        case Qt::Key_Backspace:
            if ( _cursorPos > 0 ) {
                _text.remove( _cursorPos - 1, 1 );
                --_cursorPos;
                update();
            }
            return;
        case Qt::Key_Delete:
            if ( _cursorPos < _text.size() ) {
                _text.remove( _cursorPos, 1 );
                update();
            }
            return;
        default:
            break;
        }
        const QString t = event->text();
        // 0x7F (DEL) is what macOS sends as text() for Backspace — exclude it
        // from printable characters.
        if ( !t.isEmpty() && t.at( 0 ).unicode() >= 0x20 && t.at( 0 ).unicode() != 0x7F && ( _charLimit == 0 || encodeCp1251( _text ).size() < _charLimit ) ) {
            _text.insert( _cursorPos, t );
            _cursorPos += t.size();
            update();
            return;
        }
        FrameBoxDialog::keyPressEvent( event );
    }

    void mousePressEvent( QMouseEvent * event ) override
    {
        setFocus();
        // Click inside the field places the caret (like
        // getCursorInTextPosition in the engine): the nearest character
        // boundary by glyph width.
        const QRect textRect = _fieldRect.adjusted( 13, 1, -13, -3 );
        if ( _fieldRect.contains( event->position().toPoint() ) ) {
            const int target = event->position().toPoint().x() - textRect.x();
            int best = 0;
            int bestDist = target;
            for ( int i = 0; i <= _text.size(); ++i ) {
                const int w = _font.textWidth( _text.left( i ), GameFont::Size::NORMAL );
                const int dist = std::abs( w - target );
                if ( dist < bestDist ) {
                    bestDist = dist;
                    best = i;
                }
            }
            if ( best != _cursorPos ) {
                _cursorPos = best;
                _cursorOn = true;
            }
            update();
        }
        FrameBoxDialog::mousePressEvent( event );
    }

    GameFont _font;
    QString _title;
    QString _body;
    QString _text;
    int _charLimit = 0;
    int _cursorPos = 0;
    QRect _fieldRect;
    QTimer _cursorTimer;
    bool _cursorOn = true;
};

// --- Troop window (port of Dialog::ArmyInfo, BUTTONS | DISMISS mode) ---
// Animated monster (RandomMonsterAnimation, CASTLE_UNIT_DELAY delay),
// DrawMonsterStats characteristics, name and count, DISMISS/EXIT buttons.
class ArmyInfoDialog : public OverlayDialog
{
public:
    ArmyInfoDialog( const Assets & assets, int monsterId, uint32_t count, QWidget * parent )
        : OverlayDialog( parent )
        , _font( &assets )
        , _assets( assets )
        , _monsterId( monsterId )
        , _count( count )
    {
        setMouseTracking( true );

        // DISMISS/EXIT buttons are textual with translation (fheroes2 with
        // non-original resources: BUTTON_SMALL_DISMISS/EXIT_GOOD are generated
        // as text on EMPTY_GOOD_BUTTON).
        _btnDismiss = makeGameTextButton( assets, _font, uiButtonText( UiButton::Dismiss ), this );
        connect( _btnDismiss, &GameButton::clicked, this, [this]() {
            const QString msg = gameText( "Are you sure you want to dismiss this army?" );
            if ( showGameMessage( window(), _assets, QString::fromStdString( monsterName( _monsterId ) ), msg, MSG_YES_NO ) == 1 ) {
                _dismissed = true;
                accept();
            }
        } );
        _btnExit = makeGameTextButton( assets, _font, uiButtonText( UiButton::Exit ), this );
        connect( _btnExit, &GameButton::clicked, this, &ArmyInfoDialog::accept );

        // Monster animation: a full-size ICN (like the engine's getMonsterData(id).icnId,
        // e.g. SWORDSMN.ICN), «standing» frames from *.FRM.BIN every 100 ms
        // (CASTLE_UNIT_DELAY). MONH%04d are cell mini-sprites, not for this window.
        // Port of RandomMonsterAnimation: STATIC repeated, then IDLE1.
        const MonsterAnimInfo & anim = monsterAnimInfo( monsterId );
        if ( anim.icn != nullptr && anim.icn[0] != '\0' )
            snprintf( _icnName, sizeof( _icnName ), "%s", anim.icn );
        else
            snprintf( _icnName, sizeof( _icnName ), "MONH%04d.ICN", monsterId - 1 );
        for ( int repeat = 0; repeat < 8; ++repeat )
            for ( int i = 0; i < anim.staticCount; ++i )
                _frames.push_back( anim.staticFrames[i] );
        for ( int i = 0; i < anim.idle1Count; ++i )
            _frames.push_back( anim.idle1Frames[i] );
        if ( _frames.empty() )
            _frames.push_back( 0 );
        _frame = 0;
        _animTimer.setInterval( 100 );
        connect( &_animTimer, &QTimer::timeout, this, [this]() {
            _frame = ( _frame + 1 ) % static_cast<int>( _frames.size() );
            update();
        } );
        _animTimer.start();
    }

    bool dismissed() const { return _dismissed; }

protected:
    QPoint dialogPos() const
    {
        const QPixmap dlg = _assets.icnPixmap( "VIEWARMY.ICN", 0 );
        return QPoint( ( width() - dlg.width() ) / 2, ( height() - dlg.height() ) / 2 );
    }

    void layoutWidgets() override
    {
        // Port of dialog_armyinfo.cpp: EXIT in the bottom right corner, DISMISS to the left.
        const QPoint pos = dialogPos();
        const QPixmap dlg = _assets.icnPixmap( "VIEWARMY.ICN", 0 );
        _btnDismiss->move( pos.x() + 280, pos.y() + 221 );
        _btnExit->move( pos.x() + dlg.width() - 58 - _btnExit->width() + 18, pos.y() + 221 );
    }

    void paintEvent( QPaintEvent * ) override
    {
        QImage buf = makeBuffer();
        QPainter p( &buf );

        const QPixmap dlg = _assets.icnPixmap( "VIEWARMY.ICN", 0 );
        const QPixmap shadow = _assets.icnPixmap( "VIEWARMY.ICN", 7 );
        const QPoint pos = dialogPos();
        // The engine draws sprites via Blit(Image, x, y) WITHOUT subtracting the
        // hotspot: the window frame — at pos (dialogOffset), the shadow — at
        // dialogOffset + (shadow.hotspot − dlg.hotspot) = (−16,+21). The content
        // (texts, monster, buttons) — from pos, like pos_rt in the engine.
        const IcnSprite & dlgSprite = _assets.icnSprite( "VIEWARMY.ICN", 0 );
        const IcnSprite & shadowSprite = _assets.icnSprite( "VIEWARMY.ICN", 7 );
        const QPoint dlgPos( pos.x(), pos.y() );
        const QPoint shadowPos( pos.x() + shadowSprite.offsetX - dlgSprite.offsetX, pos.y() + shadowSprite.offsetY - dlgSprite.offsetY );
        if ( !shadow.isNull() )
            p.drawPixmap( shadowPos, shadow );
        p.drawPixmap( dlgPos, dlg );

        // Monster name: yellow, centered (DrawMonsterInfo).
        const QString name = QString::fromStdString( monsterName( _monsterId ) );
        {
            const int w = _font.textWidth( name, GameFont::Size::NORMAL );
            _font.drawText( p, pos.x() + 29 + ( 227 - w ) / 2, pos.y() + 37 + 2, name, GameFont::Size::NORMAL, GameFont::Color::YELLOW );
        }

        // Statistics (DrawMonsterStats): «:» at x=dst.x−3, labels on the right
        // (fitToOneRow 123), values on the left (fitToOneRow 114).
        const MonsterStats * st = monsterStats( _monsterId );
        if ( st ) {
            int y = pos.y() + 37;
            const int offsetY = 16;
            const int colonX = pos.x() + 400 - 3;
            const auto row = [&]( const QString & label, const QString & value ) {
                _font.drawText( p, colonX, y, QStringLiteral( ":" ), GameFont::Size::NORMAL, GameFont::Color::WHITE );
                QString l = label;
                if ( _font.textWidth( l, GameFont::Size::NORMAL ) > 123 )
                    l = _font.elideText( l, GameFont::Size::NORMAL, 123 );
                QString v = value;
                if ( _font.textWidth( v, GameFont::Size::NORMAL ) > 114 )
                    v = _font.elideText( v, GameFont::Size::NORMAL, 114 );
                const int lw = _font.textWidth( l, GameFont::Size::NORMAL );
                _font.drawText( p, std::max( colonX - lw - 4, pos.x() + 400 - 123 ), y, l, GameFont::Size::NORMAL, GameFont::Color::WHITE );
                _font.drawText( p, pos.x() + 400 + 6, y, v, GameFont::Size::NORMAL, GameFont::Color::WHITE );
                y += offsetY;
            };
            row( gameText( "Attack Skill" ), QString::number( st->attack ) );
            row( gameText( "Defense Skill" ), QString::number( st->defense ) );
            if ( st->shots > 0 )
                row( gameText( "Shots" ), QString::number( st->shots ) );
            const QString dmg = st->dmgMin != st->dmgMax ? QString( "%1-%2" ).arg( st->dmgMin ).arg( st->dmgMax ) : QString::number( st->dmgMin );
            row( gameText( "Damage" ), dmg );
            row( gameText( "Hit Points" ), QString::number( st->hp ) );
            row( gameText( "Speed" ), QString::fromStdString( speedName( st->speed ) ) );
        }

        // Count — centered in the box (80, 223, 125×23).
        if ( _count != 0 ) {
            const QString text = QString::number( _count );
            const int w = _font.textWidth( text, GameFont::Size::NORMAL );
            const int h = _font.lineHeight( GameFont::Size::NORMAL );
            _font.drawText( p, pos.x() + 80 + ( 125 - w ) / 2, pos.y() + 223 + ( 23 - h ) / 2 + 2, text, GameFont::Size::NORMAL,
                            GameFont::Color::WHITE );
        }

        // Monster: an animation frame in the area (520/4+16, 175), clipped by the
        // dialog (like the engine's dialogRoi: 16..h−16). The sprite is drawn
        // with its top-left corner at pos + (146 + hotspot.x, 175 + hotspot.y) —
        // like outPos in DrawMonster (dialog_armyinfo.cpp:488-495).
        {
            const IcnSprite & sprite = _assets.icnSprite( _icnName, _frames[_frame] );
            if ( !sprite.isNull() ) {
                const QPixmap pm = _assets.icnPixmap( _icnName, _frames[_frame] );
                const QPoint mpos( pos.x() + 520 / 4 + 16 + sprite.offsetX, pos.y() + 175 + sprite.offsetY );
                p.save();
                p.setClipRect( pos.x(), pos.y() + 16, dlg.width(), dlg.height() - 32 );
                p.drawPixmap( mpos, pm );
                p.restore();
            }
        }

        // DISMISS/EXIT buttons are drawn without shadows (like in the engine's
        // dialog_armyinfo — see the info-unit-added.png reference); widgets on
        // top of the buffer.
        p.end();
        QPainter view( this );
        view.drawImage( 0, 0, buf );
        view.end();
    }

    GameFont _font;
    const Assets & _assets;
    int _monsterId = 0;
    uint32_t _count = 0;
    bool _dismissed = false;
    char _icnName[32] = {};
    std::vector<int> _frames;
    int _frame = 0;
    QTimer _animTimer;
    GameButton * _btnDismiss = nullptr;
    GameButton * _btnExit = nullptr;
};

// --- Spell book (port of SpellBook::Edit) ---
class SpellBookDialog : public OverlayDialog
{
public:
    SpellBookDialog( const Assets & assets, std::vector<int> & spells, QWidget * parent )
        : OverlayDialog( parent )
        , _font( &assets )
        , _assets( assets )
        , _spells( spells )
    {
        setMouseTracking( true );
        _page = assets.icnPixmap( "BOOK.ICN", 0 );
        _bmInfo = assets.icnPixmap( "BOOK.ICN", 6 );
        _bmAdv = assets.icnPixmap( "BOOK.ICN", 3 );
        _bmCmbt = assets.icnPixmap( "BOOK.ICN", 4 );
        _bmClose = assets.icnPixmap( "BOOK.ICN", 5 );

        // Book size: two pages wide, height by the bottom bookmark.
        const int h = std::max( { 273 + _bmInfo.height(), 269 + _bmAdv.height(), 276 + _bmCmbt.height(), 280 + _bmClose.height() } );
        _book = QRect( 0, 0, _page.width() * 2, h );

        // Bookmarks (click).
        _bmInfoRect = QRect( 123, 273, _bmInfo.width(), _bmInfo.height() );
        _bmAdvRect = QRect( 266, 269, _bmAdv.width(), _bmAdv.height() );
        _bmCmbtRect = QRect( 299, 276, _bmCmbt.width(), _bmCmbt.height() );
        // «Close» bookmark: drawn at (416,280) (bookmarkCloseOffset,
        // spell_book.cpp:57), and the click zone in edit mode — (420,284)
        // (SpellBook::Edit, spell_book.cpp:429).
        _bmClosePos = QPoint( 416, 280 );
        _bmCloseRect = QRect( 420, 284, _bmClose.width(), _bmClose.height() );
        _prevRect = QRect( 30, 8, 30, 25 );
        _nextRect = QRect( 410, 8, 30, 25 );
    }

    bool modified() const { return _modified; }

protected:
    // Like SpellBook::Open/Edit (spell_book.cpp:235): vertically the book is
    // centered by the PAGE height (306), not by the full book height with the
    // bookmarks (344).
    QPoint bookPos() const { return QPoint( ( width() - _book.width() ) / 2, ( height() - _page.height() ) / 2 ); }

    void paintEvent( QPaintEvent * ) override
    {
        QImage buf = makeBuffer();
        QPainter p( &buf );

        const QPoint bp = bookPos();

        // The book is assembled in a separate layer (like output in spellBookRedrawLists):
        // the LEFT page — a mirrored copy of the sprite (Blit(..., flip=true),
        // spell_book.cpp:154), the RIGHT one — as is (:162). Then the page-turn
        // arrows and the spine cut end up where the click zones expect them
        // _prevRect(30,8)/_nextRect(410,8).
        QImage book( _book.width(), _book.height(), QImage::Format_RGBA8888 );
        book.fill( Qt::transparent );
        {
            QPainter bookPainter( &book );
            bookPainter.drawImage( 0, 0, mirrorHorizontal( _page.toImage() ) );
            bookPainter.drawPixmap( _page.width(), 0, _page );
            bookPainter.drawPixmap( _bmInfoRect.topLeft(), _bmInfo );
            bookPainter.drawPixmap( _bmAdvRect.topLeft(), _bmAdv );
            bookPainter.drawPixmap( _bmCmbtRect.topLeft(), _bmCmbt );
            bookPainter.drawPixmap( _bmClosePos, _bmClose );
            bookPainter.end();
        }

        // Book shadow by silhouette: addShadow(output, {-16,16}, 3) (spell_book.cpp:189).
        p.end();
        addSilhouetteShadow( buf, bp, book, QPoint( -16, 16 ), 3 );
        p.begin( &buf );
        p.drawImage( bp, book );

        // Mana on the bookmark (three digits vertically).
        {
            const QPoint tp( bp.x() + 123 + 11, bp.y() + 273 + 11 );
            const auto digit = [&]( int d, int y ) {
                const QString t = d >= 0 ? QString::number( d ) : QStringLiteral( " " );
                _font.drawText( p, tp.x() - _font.textWidth( t, GameFont::Size::SMALL ) / 2, y, t, GameFont::Size::SMALL, GameFont::Color::WHITE );
            };
            digit( _mana >= 100 ? _mana / 100 : -1, tp.y() );
            digit( _mana >= 10 ? ( _mana % 100 ) / 10 : -1, tp.y() + _font.lineHeight( GameFont::Size::SMALL ) );
            digit( _mana % 10, tp.y() + 2 * _font.lineHeight( GameFont::Size::SMALL ) );
        }

        // Spells: 6 per page.
        _cells.clear();
        for ( int pageId = 0; pageId < 2; ++pageId ) {
            const int px = pageId == 0 ? 0 : 220;
            for ( int i = 0; i < 6; ++i ) {
                const size_t index = _startIndex + static_cast<size_t>( pageId * 6 + i );
                if ( index >= _spells.size() )
                    continue;
                const int spellId = _spells[index];
                const int col = i % 2;
                const int ox = 84 + 81 * col;
                const int extraY = ( ( col == 1 ) == ( pageId == 1 ) ) ? 0 : 5;
                const int oy = 71 + 78 * ( i / 2 ) - extraY;

                const QPixmap icn = spellIcon( _assets, spellId );
                const int vertOffset = std::min( 6, 49 - icn.height() );
                const int cx = bp.x() + px + ox - ( icn.width() + icn.width() % 2 ) / 2;
                const int cy = bp.y() + oy - icn.height() - vertOffset + 2;
                p.drawPixmap( cx, cy, icn );
                _cells.emplace_back( QRect( cx, cy, icn.width(), icn.height() + 10 ), spellId );

                // Label like in SpellBookRedrawSpells (spell_book.cpp:111-116):
                // «name [cost]» wrapped in 80px; if the name fits in a single
                // line, the cost goes to the second line.
                const QString name = QString::fromStdString( spellName( spellId ) );
                const int cost = spellCost( spellId );
                constexpr int maxTextWidth = 80;
                QString label = name;
                if ( cost > 0 ) {
                    const bool singleRow = _font.textRows( name, GameFont::Size::SMALL, maxTextWidth ) == 1;
                    label += ( singleRow ? QLatin1Char( '\n' ) : QLatin1Char( ' ' ) ) + QStringLiteral( "[%1]" ).arg( cost );
                }
                _font.drawTextWrapped( p, bp.x() + px + ox - 40, bp.y() + oy + 2, maxTextWidth, label, GameFont::Size::SMALL, GameFont::Color::WHITE );
            }
        }

        p.end();
        QPainter view( this );
        view.drawImage( 0, 0, buf );
    }

    void mousePressEvent( QMouseEvent * event ) override
    {
        if ( event->button() == Qt::LeftButton ) {
            const QPoint bp = bookPos();
            const QPoint local = event->pos() - bp;
            if ( _bmCloseRect.contains( local ) ) {
                accept();
                return;
            }
            if ( _startIndex > 0 && _prevRect.contains( local ) ) {
                _startIndex -= std::min<size_t>( _startIndex, 12 );
                update();
                return;
            }
            if ( _spells.size() > _startIndex + 12 && _nextRect.contains( local ) ) {
                _startIndex += 12;
                update();
                return;
            }
            // Click on a spell — description; on an empty spot — select a new one.
            for ( const auto & [rect, spellId] : _cells ) {
                if ( rect.contains( event->pos() ) ) {
                    // Spell popup (SpellDialogElement::showPopup): icon centered
                    // + name below it; for mass spells — a doubled icon via
                    // spellIcon().
                    GameMessageElement element;
                    element.kind = GameMessageElement::SPELL;
                    element.id = spellId;
                    showGameMessage( window(), _assets, QString::fromStdString( spellName( spellId ) ),
                                     QString::fromStdString( spellDescription( spellId ) ), MSG_OK, element );
                    update();
                    return;
                }
            }
            if ( _book.contains( local ) ) {
                std::set<int> exclude( _spells.begin(), _spells.end() );
                const int spellId = selectSpellDialog( window(), _assets, std::vector<int>( exclude.begin(), exclude.end() ) );
                if ( spellId > 0 ) {
                    _spells.push_back( spellId );
                    std::sort( _spells.begin(), _spells.end() );
                    _modified = true;
                    update();
                }
                return;
            }
        }
        if ( event->button() == Qt::RightButton ) {
            for ( const auto & [rect, spellId] : _cells ) {
                if ( rect.contains( event->pos() ) ) {
                    _spells.erase( std::remove( _spells.begin(), _spells.end(), spellId ), _spells.end() );
                    if ( _startIndex > 0 && _startIndex + 12 > _spells.size() )
                        _startIndex = _spells.size() > 12 ? _spells.size() - 12 : 0;
                    _modified = true;
                    update();
                    return;
                }
            }
        }
        OverlayDialog::mousePressEvent( event );
    }

    void wheelEvent( QWheelEvent * event ) override
    {
        const int delta = event->angleDelta().y() > 0 ? -1 : 1;
        if ( delta < 0 && _startIndex > 0 )
            _startIndex -= std::min<size_t>( _startIndex, 12 );
        else if ( delta > 0 && _spells.size() > _startIndex + 12 )
            _startIndex += 12;
        update();
    }

    void keyPressEvent( QKeyEvent * event ) override
    {
        if ( event->key() == Qt::Key_Left && _startIndex > 0 ) {
            _startIndex -= std::min<size_t>( _startIndex, 12 );
            update();
            return;
        }
        if ( event->key() == Qt::Key_Right && _spells.size() > _startIndex + 12 ) {
            _startIndex += 12;
            update();
            return;
        }
        OverlayDialog::keyPressEvent( event );
    }

    GameFont _font;
    const Assets & _assets;
    std::vector<int> & _spells;
    size_t _startIndex = 0;
    int _mana = 0;
    bool _modified = false;
    QPixmap _page;
    QPixmap _bmInfo;
    QPixmap _bmAdv;
    QPixmap _bmCmbt;
    QPixmap _bmClose;
    QRect _book;
    QRect _bmInfoRect;
    QRect _bmAdvRect;
    QRect _bmCmbtRect;
    QPoint _bmClosePos; // where the «close» bookmark is drawn
    QRect _bmCloseRect; // where its click is caught (like in SpellBook::Edit)
    QRect _prevRect;
    QRect _nextRect;
    std::vector<std::pair<QRect, int>> _cells;
};

} // namespace

// --- Public functions ---

int showGameMessage( QWidget * parent, const Assets & assets, const QString & title, const QString & body, int buttons, const GameMessageElement & element )
{
    return showGameMessageImpl( parent, assets, title, body, buttons, element );
}

bool selectCountDialog( QWidget * parent, const Assets & assets, const QString & header, int min, int max, int & value, int step,
                        const SelectCountElement & element )
{
    (void)step;
    SelectCountDialog dlg( assets, header, min, max, value, parent, element );
    dlg.exec();
    if ( dlg.accepted() )
        value = dlg.value();
    return dlg.accepted();
}

bool openNumpad( QWidget * parent, const Assets & assets, int & value, int min, int max )
{
    NumpadDialog dlg( assets, value, min, max, parent );
    return dlg.exec() == 1;
}

int selectMonsterDialog( QWidget * parent, const Assets & assets, int currentId )
{
    ItemSelectionDialog dlg( assets, QSize( 320, listDialogHeight( parent ) ), gameText( "Select Monster:" ), QString(), 22, 50, 43, parent );
    QVector<ItemSelectionDialog::Item> items;
    for ( int id : editableMonsters() ) {
        ItemSelectionDialog::Item item;
        item.icon = assets.icnPixmap( ICN_MONS32, id - 1, 1.0 );
        item.text = QString::fromStdString( monsterName( id ) );
        item.id = id;
        items.push_back( item );
    }
    dlg.setItems( items, currentId );
    return dlg.exec() == 1 ? dlg.result() : 0;
}

int selectArtifactDialog( QWidget * parent, const Assets & assets, int currentId, const std::vector<int> & excludeIds )
{
    ItemSelectionDialog dlg( assets, QSize( 370, listDialogHeight( parent ) ), gameText( "Select Artifact:" ), QString(), 22, 50, 42, parent );
    QVector<ItemSelectionDialog::Item> items;
    const auto excluded = [&]( int id ) { return std::find( excludeIds.begin(), excludeIds.end(), id ) != excludeIds.end(); };

    // The spell book — first, then the rest (like Dialog::selectArtifact).
    if ( !excluded( ARTIFACT_MAGIC_BOOK ) ) {
        ItemSelectionDialog::Item item;
        item.icon = assets.icnPixmap( ICN_ARTFX, ARTIFACT_MAGIC_BOOK - 1, 1.0 );
        item.text = QString::fromStdString( artifactName( ARTIFACT_MAGIC_BOOK ) );
        item.id = ARTIFACT_MAGIC_BOOK;
        items.push_back( item );
    }
    for ( int id = 1; id <= 81; ++id ) {
        if ( id == ARTIFACT_MAGIC_BOOK || excluded( id ) )
            continue;
        ItemSelectionDialog::Item item;
        item.icon = assets.icnPixmap( ICN_ARTFX, id - 1, 1.0 );
        item.text = QString::fromStdString( artifactName( id ) );
        item.id = id;
        items.push_back( item );
    }
    dlg.setItems( items, currentId );
    return dlg.exec() == 1 ? dlg.result() : -1;
}

int selectHeroDialog( QWidget * parent, const Assets & assets, int currentId )
{
    ItemSelectionDialog dlg( assets, QSize( 300, listDialogHeight( parent ) ), gameText( "Select Hero:" ), QString(), 22, 50, 35, parent );
    QVector<ItemSelectionDialog::Item> items;
    for ( int id = 1; id <= 71; ++id ) {
        auto it = heroDefaultNames().find( id );
        ItemSelectionDialog::Item item;
        item.icon = assets.icnPixmap( ICN_MINIPORT, id - 1, 1.0 );
        item.text = it != heroDefaultNames().end() ? QString::fromStdString( it->second ) : QStringLiteral( "#%1" ).arg( id );
        item.id = id;
        items.push_back( item );
    }
    dlg.setItems( items, currentId );
    return dlg.exec() == 1 ? dlg.result() : 0;
}

int selectSpellDialog( QWidget * parent, const Assets & assets, const std::vector<int> & excludeSpells )
{
    ItemSelectionDialog dlg( assets, QSize( 340, listDialogHeight( parent ) ), gameText( "Select Spell:" ), QString(), 37, 80, 55, parent );
    QVector<ItemSelectionDialog::Item> items;
    for ( int id = 1; id <= SPELL_COUNT; ++id ) {
        if ( std::find( excludeSpells.begin(), excludeSpells.end(), id ) != excludeSpells.end() )
            continue;
        ItemSelectionDialog::Item item;
        item.icon = spellIcon( assets, id );
        item.text = QString::fromStdString( spellName( id ) );
        item.id = id;
        items.push_back( item );
    }
    dlg.setItems( items, 0 );
    // Right click on a row — spell popup (SelectSpell::ActionListPressRight
    // → SpellDialogElement::showPopup).
    dlg.setOnRightClickItem( [&]( int itemId ) {
        GameMessageElement element;
        element.kind = GameMessageElement::SPELL;
        element.id = itemId;
        showGameMessage( dlg.window(), assets, QString::fromStdString( spellName( itemId ) ),
                         QString::fromStdString( spellDescription( itemId ) ), MSG_OK, element );
    } );
    return dlg.exec() == 1 ? dlg.result() : 0;
}

bool selectSecondarySkillDialog( QWidget * parent, const Assets & assets, int & skillId, int & level, const std::vector<int> & excludeSkillIds )
{
    // Port of Dialog::selectSecondarySkill + SelectEnumSecSkill
    // (dialog_selectitems.cpp:414): MINISS icons (GetIndexSprite2 = id−1),
    // 42-high rows, right click — skill popup (ActionListPressRight).
    // Skills already added to the hero do not appear in the list (all their levels).
    ItemSelectionDialog dlg( assets, QSize( 350, listDialogHeight( parent ) ), gameText( "Select Skill:" ), QString(), 22, 50, 42, parent );
    QVector<ItemSelectionDialog::Item> items;
    for ( int id = 1; id <= 14; ++id ) {
        if ( std::find( excludeSkillIds.begin(), excludeSkillIds.end(), id ) != excludeSkillIds.end() )
            continue;
        for ( int lvl = 1; lvl <= 3; ++lvl ) {
            ItemSelectionDialog::Item item;
            item.icon = assets.icnPixmap( ICN_MINISS, id - 1, 1.0 );
            // The row — the skill name with its level (skill.GetName() in the engine).
            item.text = QString::fromStdString( skillNameWithLevel( id, lvl ) );
            item.id = id * 10 + lvl;
            items.push_back( item );
        }
    }
    dlg.setItems( items, 0 );
    dlg.setOnRightClickItem( [&]( int itemId ) {
        GameMessageElement element;
        element.kind = GameMessageElement::SECONDARY_SKILL;
        element.id = itemId / 10;
        element.level = itemId % 10;
        showGameMessage( dlg.window(), assets, QString::fromStdString( skillNameWithLevel( itemId / 10, itemId % 10 ) ),
                         QString::fromStdString( skillDescription( itemId / 10, itemId % 10 ) ), MSG_OK, element );
    } );
    if ( dlg.exec() != 1 )
        return false;
    const int v = dlg.result();
    skillId = v / 10;
    level = v % 10;
    return true;
}

QString inputStringDialog( QWidget * parent, const Assets & assets, const QString & title, const QString & body, const QString & initial, int charLimit )
{
    InputStringDialog dlg( assets, title, body, initial, charLimit, parent );
    dlg.exec();
    return dlg.result();
}

void showArmyInfoDialog( QWidget * parent, const Assets & assets, int monsterId, uint32_t count, bool & dismissed )
{
    ArmyInfoDialog dlg( assets, monsterId, count, parent );
    dlg.exec();
    dismissed = dlg.dismissed();
}

bool spellBookDialog( QWidget * parent, const Assets & assets, std::vector<int> & spells )
{
    SpellBookDialog dlg( assets, spells, parent );
    dlg.exec();
    return dlg.modified();
}

} // namespace fh2
