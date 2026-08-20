#include "heropanel.h"

#include <QContextMenuEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

#include <algorithm>
#include <cmath>

#include "assets.h"
#include "constants.h"
#include "gameui.h"
#include "gettextmo.h"
#include "dialogs.h"
#include "gamedata.h"
#include "savefile.h"
#include "textutil.h"
#include "translations.h"

namespace fh2 {

namespace {

QString heroDisplayName( const HeroRecord & hero )
{
    QString name = decodeCp1251( hero.name );
    if ( name.isEmpty() ) {
        auto it = heroDefaultNames().find( hero.heroId );
        if ( it != heroDefaultNames().end() )
            name = QString::fromStdString( trGame( it->second ) ); // localized default name
        else
            name = editorText( "Hero #%1" ).arg( hero.heroId );
    }
    return name;
}

// StandardWindow frame around the hero screen: borderSize = 16 (ui_window.cpp).
// The main window has no shadow, so the content is offset exactly by the border.
constexpr int FRAME_BORDER = 16;
QPoint contentOffset() { return QPoint( FRAME_BORDER, FRAME_BORDER ); }

QString monsterIcnName( int monsterId )
{
    char buf[32];
    snprintf( buf, sizeof( buf ), "MONH%04d.ICN", monsterId - 1 );
    return QString::fromLatin1( buf );
}

// The background EXIT button baked into HEROBKG (the button was removed, but
// its image remained in the background). We cover it with a mirrored copy of
// the column to the left of the artifacts (over their full height) — the right
// side becomes a reflection of the left frame. The patch starts right after
// the artifact grid (51 + 6×79 + 64 = 589). The copy is taken from the already
// painted buffer in paintEvent (the column may be partially transparent in
// HEROBKG).
QRect exitPatchSrc() { return QRect( 0, 306, 51, 145 ); }
QRect exitPatchDest() { return QRect( 589, 306, 51, 145 ); }

} // namespace

HeroPanel::HeroPanel( const Assets * assets, QWidget * parent )
    : QWidget( parent )
    , _assets( assets )
{
    setFixedSize( screenSize() );
    setMouseTracking( true );
    _font = std::make_unique<GameFont>( assets );

    _holdTimer = std::make_unique<QTimer>( this );
    connect( _holdTimer.get(), &QTimer::timeout, this, [this]() {
        if ( _holdAction )
            _holdAction();
        _holdInitial = false;
        _holdTimer->start( 90 );
    } );

    refreshAssets();
}

HeroPanel::~HeroPanel() = default;

QSize HeroPanel::screenSize() const
{
    // Hero screen + StandardWindow frame (16px on each side). The main window
    // has NO shadow (only dialogs cast one), so we reserve no space for it:
    // the content is drawn at (16,16), the frame — at (0,0).
    return QSize( screenS( SCREEN_W ) + FRAME_BORDER * 2, screenS( SCREEN_H ) + FRAME_BORDER * 2 );
}

void HeroPanel::setAssets( const Assets * assets )
{
    _assets = assets;
    _font = std::make_unique<GameFont>( assets );
    refreshAssets();
    update();
}

void HeroPanel::setNavigationEnabled( bool enabled )
{
    _navEnabled = enabled;
    update();
}

void HeroPanel::refreshAssets()
{
    _background = QPixmap();
    for ( auto & p : _hsbtns )
        p = QPixmap();
    for ( auto & p : _recruit )
        p = QPixmap();
    for ( auto & p : _hsicons )
        p = QPixmap();
    for ( auto & p : _primskil )
        p = QPixmap();
    for ( auto & p : _secskill )
        p = QPixmap();
    for ( auto & p : _artfx )
        p = QPixmap();
    for ( auto & p : _artifact )
        p = QPixmap();
    _artifactCell = QPixmap();
    for ( auto & p : _strip )
        p = QPixmap();
    _stripEmpty = QPixmap();
    for ( auto & p : _crest )
        p = QPixmap();
    _stripSel = QPixmap();
    _artifactCursor = QPixmap();

    if ( !_assets )
        return;

    // Background: HEROBKG[0] (640×461) + HEROEXTG[0] frame on top (as in the game).
    QPixmap bg = _assets->icnPixmap( ICN_HEROBKG, 0, SCREEN_SCALE );
    const QPixmap frame = _assets->icnPixmap( ICN_HEROEXTG, 0, SCREEN_SCALE );
    if ( !bg.isNull() && !frame.isNull() ) {
        QPainter p( &bg );
        p.drawPixmap( 0, 0, frame );
        p.end();
        _background = bg;
    }

    for ( int i = 0; i < 9; ++i )
        _hsbtns[i] = _assets->icnPixmap( ICN_HSBTNS, i, SCREEN_SCALE );
    for ( int i = 0; i < 4; ++i )
        _recruit[i] = _assets->icnPixmap( ICN_RECRUIT, i, SCREEN_SCALE );
    for ( int i = 0; i < 12; ++i )
        _hsicons[i] = _assets->icnPixmap( ICN_HSICONS, i, SCREEN_SCALE );
    for ( int i = 0; i < 4; ++i )
        _primskil[i] = _assets->icnPixmap( ICN_PRIMSKIL, i, SCREEN_SCALE );
    for ( int i = 0; i < 15; ++i )
        _secskill[i] = _assets->icnPixmap( ICN_SECSKILL, i, SCREEN_SCALE );
    for ( int i = 0; i < 82; ++i )
        _artfx[i] = _assets->icnPixmap( ICN_ARTFX, i, SCREEN_SCALE );
    for ( int i = 1; i <= 82; ++i )
        _artifact[i] = _assets->icnPixmap( ICN_ARTIFACT, i, SCREEN_SCALE );
    _artifactCell = _assets->icnPixmap( ICN_ARTIFACT, 0, SCREEN_SCALE );
    for ( int i = 0; i < 11; ++i )
        _strip[i] = _assets->icnPixmap( ICN_STRIP, i + 4, SCREEN_SCALE );
    _stripEmpty = _assets->icnPixmap( ICN_STRIP, 2, SCREEN_SCALE );
    _stripSel = _assets->icnPixmap( ICN_STRIP, 1, SCREEN_SCALE );

    // Selected artifact frame: 70×70 spcursor made of three rectangles of
    // color 190/180/190 (ArtifactsBar, resource/artifact.cpp:1132).
    _artifactCursor = QPixmap( 70, 70 );
    _artifactCursor.fill( Qt::transparent );
    {
        QPainter p( &_artifactCursor );
        p.setPen( QPen( _assets->paletteColor( 190 ), 1 ) );
        p.drawRect( 0, 0, 69, 69 );
        p.setPen( QPen( _assets->paletteColor( 180 ), 1 ) );
        p.drawRect( 1, 1, 67, 67 );
        p.setPen( QPen( _assets->paletteColor( 190 ), 1 ) );
        p.drawRect( 2, 2, 65, 65 );
        p.end();
    }

    for ( int i = 0; i < 6; ++i )
        _crest[i] = _assets->icnPixmap( ICN_CREST, i, SCREEN_SCALE );

}

void HeroPanel::setHero( SaveFile * saveFile, HeroRecord * hero )
{
    _saveFile = saveFile;
    _hero = hero;
    setStatus( gameText( "Hero Screen" ) );
    update();
}

void HeroPanel::clearHero()
{
    _hero = nullptr;
    _saveFile = nullptr;
    stopHold();
    setStatus( gameText( "Hero Screen" ) );
    update();
}

// --- geometry (640×480, from heroes_dialog.cpp) ---

QRect HeroPanel::titleRect() const { return QRect( 60, 2, 519, 17 ); }

QRect HeroPanel::portraitRect() const { return QRect( 49, 31, 101, 93 ); }

QRect HeroPanel::crestRect() const { return QRect( 49, 130, 101, 93 ); }

QRect HeroPanel::primaryRect( int i ) const { return QRect( 156 + i * 88, 31, 82, 93 ); }

// As in fheroes2 (editor mode): experience (512, 76), mana (550, 78), 35×36.
QRect HeroPanel::expRect() const { return QRect( 512, 76, 35, 36 ); }

QRect HeroPanel::manaRect() const { return QRect( 550, 78, 35, 36 ); }

QRect HeroPanel::armyRect( int i ) const { return QRect( 156 + i * 88, 130, 82, 93 ); }

QRect HeroPanel::moraleRect() const { return QRect( 514, 44, 34, 19 ); } // y=44 in the editor

QRect HeroPanel::luckRect() const { return QRect( 550, 44, 33, 21 ); } // y=44 in the editor

QRect HeroPanel::secSkillRect( int i ) const { return QRect( 3 + i * 80, 233, 75, 65 ); }

// Artifact cell 64×64 (size of ARTIFACT.ICN[0]), step 79 = 64 + 15 gap
// (like setInBetweenItemsOffset({15,15}) in the game's ArtifactsBar).
QRect HeroPanel::artifactRect( int i ) const
{
    return QRect( 51 + ( i % 7 ) * ARTIFACT_CELL_STEP, 308 + ( i / 7 ) * ARTIFACT_CELL_STEP, ARTIFACT_CELL_W, ARTIFACT_CELL_H );
}

QRect HeroPanel::prevRect() const { return QRect( 0, 460, 22, 20 ); }

QRect HeroPanel::nextRect() const { return QRect( 618, 460, 22, 20 ); }

QRect HeroPanel::statusBarRect() const { return QRect( 22, 460, 596, 20 ); }

HeroPanel::Hit HeroPanel::hitTest( int x, int y ) const
{
    Hit hit;
    if ( titleRect().contains( x, y ) )
        hit.area = HIT_TITLE;
    else if ( portraitRect().contains( x, y ) )
        hit.area = HIT_PORTRAIT;
    else if ( crestRect().contains( x, y ) )
        hit.area = HIT_CREST;
    else if ( expRect().contains( x, y ) )
        hit.area = HIT_EXP;
    else if ( manaRect().contains( x, y ) )
        hit.area = HIT_MANA;
    else if ( moraleRect().contains( x, y ) )
        hit.area = HIT_MORALE;
    else if ( luckRect().contains( x, y ) )
        hit.area = HIT_LUCK;
    else if ( prevRect().contains( x, y ) )
        hit.area = HIT_PREV;
    else if ( nextRect().contains( x, y ) )
        hit.area = HIT_NEXT;
    else {
        for ( int i = 0; i < 4; ++i ) {
            if ( primaryRect( i ).contains( x, y ) ) {
                hit.area = HIT_PRIMARY;
                hit.index = i;
                break;
            }
        }
        if ( hit.area == HIT_NONE ) {
            for ( int i = 0; i < 5; ++i ) {
                if ( armyRect( i ).contains( x, y ) ) {
                    hit.area = HIT_ARMY;
                    hit.index = i;
                    break;
                }
            }
        }
        if ( hit.area == HIT_NONE ) {
            for ( int i = 0; i < 8; ++i ) {
                if ( secSkillRect( i ).contains( x, y ) ) {
                    hit.area = HIT_SECSKILL;
                    hit.index = i;
                    break;
                }
            }
        }
        if ( hit.area == HIT_NONE ) {
            for ( int i = 0; i < 14; ++i ) {
                if ( artifactRect( i ).contains( x, y ) ) {
                    hit.area = HIT_ARTIFACT;
                    hit.index = i;
                    break;
                }
            }
        }
    }
    return hit;
}

// --- mouse events ---

void HeroPanel::mousePressEvent( QMouseEvent * event )
{
    const QPoint local = event->position().toPoint() - contentOffset();
    const Hit hit = hitTest( static_cast<int>( local.x() / SCREEN_SCALE ), static_cast<int>( local.y() / SCREEN_SCALE ) );

    if ( event->button() == Qt::LeftButton ) {
        _pressed = hit;
        update();
        if ( hit.area == HIT_PREV && _navEnabled ) {
            stopHold();
            Q_EMIT prevClicked();
            startHold( [this]() { Q_EMIT prevClicked(); } );
        }
        else if ( hit.area == HIT_NEXT && _navEnabled ) {
            stopHold();
            Q_EMIT nextClicked();
            startHold( [this]() { Q_EMIT nextClicked(); } );
        }
        return;
    }

    if ( event->button() == Qt::RightButton ) {
        switch ( hit.area ) {
        case HIT_PRIMARY:
            // As in the map editor: right click resets all skills to default values.
            resetPrimarySkillsToDefaults();
            break;
        case HIT_EXP:
            resetExperience();
            break;
        case HIT_MANA:
            resetSpellPoints();
            break;
        case HIT_ARMY:
            // Like "reset unit" in the editor: right click resets the troop.
            if ( _hero && _saveFile ) {
                clearArmySlot( hit.index );
                if ( _armySelected == hit.index )
                    _armySelected = -1;
                update();
            }
            break;
        case HIT_SECSKILL:
            // Right click on an occupied skill removes it (the list shifts left).
            if ( _hero && _saveFile && hit.index < static_cast<int>( _hero->skills.size() ) ) {
                try {
                    _saveFile->removeSecondarySkill( *_hero, hit.index );
                    Q_EMIT changed();
                    update();
                }
                catch ( const SaveError & e ) {
                    setStatus( QString::fromStdString( e.what() ) );
                }
            }
            break;
        case HIT_ARTIFACT:
            clearArtifact( hit.index );
            break;
        case HIT_TITLE: {
            if ( !_hero || !_saveFile )
                break;
            auto it = heroDefaultNames().find( _hero->heroId );
            if ( it == heroDefaultNames().end() )
                break;
            const std::string def = it->second;
            if ( def.size() == _hero->name.size() ) {
                _saveFile->setName( *_hero, def );
                Q_EMIT changed();
                update();
            }
            else {
                setStatus( editorText( "Default name length mismatch — reset unavailable" ) );
            }
            break;
        }
        case HIT_PORTRAIT:
            if ( _hero && _saveFile && _hero->portrait != _hero->heroId ) {
                _saveFile->setPortrait( *_hero, _hero->heroId );
                Q_EMIT changed();
                update();
            }
            break;
        case HIT_CREST:
            // The hero's race is not editable.
            break;
        default:
            break;
        }
    }
}

void HeroPanel::mouseReleaseEvent( QMouseEvent * event )
{
    if ( event->button() == Qt::RightButton )
        return;

    const QPoint local = event->position().toPoint() - contentOffset();
    const Hit releaseHit = hitTest( static_cast<int>( local.x() / SCREEN_SCALE ), static_cast<int>( local.y() / SCREEN_SCALE ) );
    const Hit pressedHit = _pressed;
    stopHold();
    _pressed = {};
    update();

    if ( releaseHit.area == pressedHit.area && releaseHit.index == pressedHit.index && pressedHit.area != HIT_NONE ) {
        switch ( pressedHit.area ) {
        case HIT_TITLE:
            editTitle();
            break;
        case HIT_PORTRAIT:
            pickPortrait();
            break;
        case HIT_CREST:
            // The hero's race is not editable.
            break;
        case HIT_PRIMARY:
            editPrimarySkill( pressedHit.index );
            break;
        case HIT_EXP:
            editExperience();
            break;
        case HIT_MANA:
            editSpellPoints();
            break;
        case HIT_ARMY:
            // As in the editor: click on an empty slot picks a monster and
            // count; on an occupied one selects it (a click on another slot
            // then swaps/moves).
            if ( _hero && _saveFile )
                clickArmySlot( pressedHit.index );
            break;
        case HIT_SECSKILL:
            editSecondarySkill( pressedHit.index );
            break;
        case HIT_ARTIFACT:
            clickArtifactSlot( pressedHit.index );
            break;
        default:
            break;
        }
    }
}

void HeroPanel::mouseMoveEvent( QMouseEvent * event )
{
    const QPoint local = event->position().toPoint() - contentOffset();
    const Hit hit = hitTest( static_cast<int>( local.x() / SCREEN_SCALE ), static_cast<int>( local.y() / SCREEN_SCALE ) );
    if ( hit.area != _hovered.area || hit.index != _hovered.index ) {
        _hovered = hit;
        update();
    }
    switch ( hit.area ) {
    case HIT_PREV:
        setStatus( _navEnabled ? gameText( "Show previous hero" )
                               : gameText( "Hero Screen" ) );
        break;
    case HIT_NEXT:
        setStatus( _navEnabled ? gameText( "Show next hero" )
                               : gameText( "Hero Screen" ) );
        break;
    case HIT_TITLE:
        setStatus( gameText( "Click to change hero's name. Right-click to reset to default." ) );
        break;
    case HIT_PORTRAIT:
        setStatus( gameText( "Set hero's portrait. Right-click to reset to default." ) );
        break;
    case HIT_CREST:
        // The hero's race is not editable.
        setStatus( gameText( "Hero Screen" ) );
        break;
    case HIT_PRIMARY:
        setStatus( replaceName( gameText( "Set %{skill} base value. Right-click to reset all skills to default." ), "%{skill}",
                             QString::fromStdString( primarySkillName( hit.index ) ) ) );
        break;
    case HIT_EXP:
        setStatus( gameText( "Change Experience value. Right-click to reset to default value." ) );
        break;
    case HIT_MANA:
        setStatus( gameText( "Change Spell Points value. Right-click to reset to default value." ) );
        break;
    case HIT_MORALE:
        setStatus( moraleLuckText( false ) );
        break;
    case HIT_LUCK:
        setStatus( moraleLuckText( true ) );
        break;
    case HIT_ARMY: {
        if ( !_hero ) {
            setStatus( gameText( "Hero Screen" ) );
            break;
        }
        const int idx = hit.index;
        const int mid = _hero->slots[idx].monsterId;
        if ( _armySelected >= 0 ) {
            // Port of ArmyBar::ActionBarCursor (army_bar.cpp:322).
            const int selId = _hero->slots[_armySelected].monsterId;
            if ( idx == _armySelected && mid != 0 ) {
                setStatus( replaceName( gameText( "View %{name}" ), "%{name}", QString::fromStdString( monsterName( mid ) ) ) );
            }
            else if ( mid == 0 ) {
                setStatus( replaceName( gameText( "Move the %{name} " ), "%{name}", QString::fromStdString( monsterName( selId ) ) ) );
            }
            else if ( mid == selId ) {
                setStatus( replaceName( gameText( "Combine %{name} armies" ), "%{name}", QString::fromStdString( monsterName( mid ) ) ) );
            }
            else {
                setStatus( replaceName( replaceName( gameText( "Exchange %{name2} with %{name}" ), "%{name}",
                                                 QString::fromStdString( monsterName( mid ) ) ),
                                    "%{name2}", QString::fromStdString( monsterName( selId ) ) ) );
            }
        }
        else if ( mid != 0 ) {
            setStatus( replaceName( gameText( "Select %{name}" ), "%{name}", QString::fromStdString( monsterName( mid ) ) ) );
        }
        else {
            setStatus( gameText( "Set hero's Army. Right-click to reset unit." ) );
        }
        break;
    }
    case HIT_SECSKILL:
        if ( _hero && hit.index < static_cast<int>( _hero->skills.size() ) && _hero->skills[hit.index].first > 0 ) {
            setStatus( replaceName( gameText( "View %{skill} Info" ), "%{skill}",
                             QString::fromStdString( skillName( _hero->skills[hit.index].first ) ) ) );
        }
        else if ( _hero && hit.index == static_cast<int>( _hero->skills.size() ) ) {
            setStatus( editorText( "Set hero's Secondary Skills. Right-click to remove skill." ) );
        }
        else {
            setStatus( editorText( "Fill the previous skill slots first" ) );
        }
        break;
    case HIT_ARTIFACT: {
        if ( !_hero ) {
            setStatus( gameText( "Hero Screen" ) );
            break;
        }
        const int idx = hit.index;
        const int id = idx < _hero->artifactCount ? _hero->artifacts[idx].id : 0;
        if ( _artifactSelected >= 0 && _artifactSelected < _hero->artifactCount ) {
            // Port of ArtifactsBar::ActionBarCursor (resource/artifact.cpp:1330).
            const int selId = _hero->artifacts[_artifactSelected].id;
            if ( idx == _artifactSelected && id != 0 ) {
                setStatus( replaceName( gameText( "View %{name} Info" ), "%{name}", QString::fromStdString( artifactName( id ) ) ) );
            }
            else if ( id == ARTIFACT_MAGIC_BOOK ) {
                setStatus( gameText( "Cannot move the Spellbook" ) );
            }
            else if ( id == 0 ) {
                setStatus( replaceName( gameText( "Move %{name}" ), "%{name}", QString::fromStdString( artifactName( selId ) ) ) );
            }
            else {
                setStatus( replaceName( replaceName( gameText( "Exchange %{name2} with %{name}" ), "%{name}",
                                                 QString::fromStdString( artifactName( id ) ) ),
                                    "%{name2}", QString::fromStdString( artifactName( selId ) ) ) );
            }
        }
        else if ( id == ARTIFACT_MAGIC_BOOK ) {
            setStatus( gameText( "View Spells" ) );
        }
        else if ( id != 0 ) {
            setStatus( replaceName( gameText( "Select %{name}" ), "%{name}", QString::fromStdString( artifactName( id ) ) ) );
        }
        else if ( idx <= _hero->artifactCount ) {
            // An empty cell inside the bag (a hole) or the slot right after it.
            setStatus( gameText( "Set hero's Artifacts. Right-click to reset Artifact." ) );
        }
        else {
            setStatus( editorText( "Fill the previous artifact slots first" ) );
        }
        break;
    }
    default:
        setStatus( gameText( "Hero Screen" ) );
        break;
    }
}

void HeroPanel::mouseDoubleClickEvent( QMouseEvent * event )
{
    // Double click: on an occupied army slot — the troop window with animation
    // and buttons (Dialog::ArmyInfo); on an occupied artifact — an info hint
    // (ArtifactDialogElement). As in the engine (ArmyBar/ArtifactsBar).
    if ( event->button() != Qt::LeftButton ) {
        QWidget::mouseDoubleClickEvent( event );
        return;
    }
    const QPoint local = event->position().toPoint() - contentOffset();
    const Hit hit = hitTest( static_cast<int>( local.x() / SCREEN_SCALE ), static_cast<int>( local.y() / SCREEN_SCALE ) );
    stopHold();
    _pressed = {};
    switch ( hit.area ) {
    case HIT_ARMY:
        if ( _hero && _saveFile )
            showArmyInfo( hit.index );
        break;
    case HIT_ARTIFACT:
        if ( _hero )
            showArtifactInfo( hit.index );
        break;
    default:
        break;
    }
    update();
}

void HeroPanel::leaveEvent( QEvent * )
{
    _hovered = {};
    setStatus( gameText( "Hero Screen" ) );
    update();
}

void HeroPanel::contextMenuEvent( QContextMenuEvent * )
{
    // The right click is handled in mousePressEvent per area.
}

// --- hold (auto repeat) ---

void HeroPanel::startHold( std::function<void()> action )
{
    _holdAction = std::move( action );
    _holdInitial = true;
    _holdTimer->start( 450 );
}

void HeroPanel::stopHold()
{
    _holdTimer->stop();
    _holdAction = nullptr;
}

// --- edit actions ---

void HeroPanel::editPrimarySkill( int index )
{
    if ( !_hero || !_saveFile || !_assets )
        return;
    if ( !_hero->heroBaseParsed ) {
        setStatus( editorText( "Primary skills unavailable (hero record not recognized)" ) );
        return;
    }
    // As in the map editor: clicking a skill opens the count dialog with the
    // skill plate (PrimarySkillDialogElement): "Set %skill Skill:"; spell
    // power/knowledge start at 1, attack/defense at 0.
    const int current = _hero->primary.value( index );
    const int min = ( index == 2 || index == 3 ) ? 1 : 0;
    const QString title = replaceName( gameText( "Set %{skill} Skill:" ), "%{skill}", QString::fromStdString( primarySkillName( index ) ) );
    int value = current;
    SelectCountElement element;
    element.kind = SelectCountElement::PRIMARY_SKILL;
    element.id = index;
    if ( !selectCountDialog( this, *_assets, title, min, 99, value, 1, element ) || value == current )
        return;
    _saveFile->setPrimarySkill( *_hero, index, value );
    Q_EMIT changed();
    update();
}

void HeroPanel::resetPrimarySkillsToDefaults()
{
    if ( !_hero || !_saveFile )
        return;
    if ( !_hero->heroBaseParsed ) {
        setStatus( editorText( "Primary skills unavailable (hero record not recognized)" ) );
        return;
    }
    for ( int i = 0; i < 4; ++i )
        _saveFile->setPrimarySkill( *_hero, i, primarySkillDefault( _hero->race, i ) );
    Q_EMIT changed();
    update();
}

void HeroPanel::editExperience()
{
    if ( !_hero || !_saveFile || !_assets )
        return;
    int value = static_cast<int>( _hero->experience );
    // Experience icon above the selector (ExperienceDialogElement{0} in the engine).
    SelectCountElement element;
    element.kind = SelectCountElement::EXPERIENCE;
    if ( !selectCountDialog( this, *_assets, gameText( "Set Experience value:" ), 0, 2990600, value, 1, element )
         || value == static_cast<int>( _hero->experience ) )
        return;
    _saveFile->setExperience( *_hero, static_cast<uint32_t>( value ) );
    Q_EMIT changed();
    update();
}

void HeroPanel::resetExperience()
{
    if ( !_hero || !_saveFile )
        return;
    if ( _hero->experience == 0 )
        return;
    _saveFile->setExperience( *_hero, 0 );
    Q_EMIT changed();
    update();
}

void HeroPanel::editSpellPoints()
{
    if ( !_hero || !_saveFile || !_assets )
        return;
    if ( !_hero->heroBaseParsed ) {
        setStatus( editorText( "Spell points unavailable (hero record not recognized)" ) );
        return;
    }
    int value = _hero->spellPoints;
    if ( !selectCountDialog( this, *_assets, gameText( "Set Spell Points value:" ), 0, 9999, value, 1 )
         || value == _hero->spellPoints )
        return;
    _saveFile->setSpellPoints( *_hero, value );
    Q_EMIT changed();
    update();
}

void HeroPanel::resetSpellPoints()
{
    if ( !_hero || !_saveFile )
        return;
    if ( !_hero->heroBaseParsed ) {
        setStatus( editorText( "Spell points unavailable (hero record not recognized)" ) );
        return;
    }
    // "Reset to default value" — maximum mana (knowledge × 10), as in the game.
    const int max = std::max( 1, _hero->primary.knowledge * 10 );
    if ( _hero->spellPoints == max )
        return;
    _saveFile->setSpellPoints( *_hero, max );
    Q_EMIT changed();
    update();
}

// Double click on an occupied army slot — the troop window with animation and
// DISMISS/EXIT buttons (port of Dialog::ArmyInfo from ArmyBar::ActionBarLeftMouseDoubleClick).
void HeroPanel::showArmyInfo( int slot )
{
    if ( !_hero || !_saveFile || !_assets )
        return;
    const Troop & t = _hero->slots[slot];
    if ( t.monsterId == 0 )
        return;
    _armySelected = -1;
    update();

    bool dismissed = false;
    showArmyInfoDialog( this, *_assets, t.monsterId, t.count, dismissed );
    if ( dismissed ) {
        // The hero's last troop cannot be dismissed (a save without an army
        // would not load).
        if ( occupiedSlots() <= 1 ) {
            setStatus( editorText( "The last hero's troop cannot be removed" ) );
            return;
        }
        _saveFile->setSlot( *_hero, slot, 0, 0 );
        Q_EMIT changed();
        update();
    }
}

void HeroPanel::editTitle()
{
    if ( !_hero || !_saveFile || !_assets )
        return;
    const QString name = inputStringDialog( this, *_assets, gameText( "Enter hero's name:" ), QString(),
                                             heroDisplayName( *_hero ), 64 );
    if ( name.isEmpty() )
        return;
    const std::string encoded = encodeCp1251( name );
    if ( encoded.size() != _hero->name.size() ) {
        showGameMessage( this, *_assets, editorText( "Hero name" ),
                         editorText( "The name must be the same length (%1 character(s)) to avoid shifting the save data." )
                             .arg( _hero->name.size() ) );
        return;
    }
    if ( encoded == _hero->name )
        return;
    _saveFile->setName( *_hero, encoded );
    Q_EMIT changed();
    update();
}

void HeroPanel::pickPortrait()
{
    if ( !_hero || !_saveFile || !_assets )
        return;
    const int portrait = selectHeroDialog( this, *_assets, _hero->portrait );
    if ( portrait > 0 && portrait != _hero->portrait ) {
        _saveFile->setPortrait( *_hero, portrait );
        Q_EMIT changed();
        update();
    }
}

int HeroPanel::occupiedSlots() const
{
    if ( !_hero )
        return 0;
    int n = 0;
    for ( int i = 0; i < 5; ++i ) {
        if ( _hero->slots[i].monsterId != 0 )
            ++n;
    }
    return n;
}

void HeroPanel::pickMonsterForSlot( int slot )
{
    // As in the editor: pick a monster (the current one is the race's starting
    // monster), then the count with the monster frame element.
    if ( !_hero || !_saveFile || !_assets )
        return;
    int current = 0;
    switch ( _hero->race ) {
    case 1: current = 1; break;   // Peasant
    case 2: current = 12; break;  // Goblin
    case 4: current = 21; break;  // Sprite
    case 8: current = 30; break;  // Centaur
    case 16: current = 39; break; // Halfling
    case 32: current = 48; break; // Skeleton
    default: break;
    }
    const int mid = selectMonsterDialog( this, *_assets, current );
    if ( mid <= 0 )
        return;

    const QString title = replaceName( gameText( "Set %{monster} Count:" ), "%{monster}", QString::fromStdString( monsterName( mid ) ) );
    int count = 1;
    // Monster plate above the selector (MonsterDialogElement in the engine).
    SelectCountElement element;
    element.kind = SelectCountElement::MONSTER;
    element.id = mid;
    if ( !selectCountDialog( this, *_assets, title, 1, 500000, count, 1, element ) )
        return;
    _saveFile->setSlot( *_hero, slot, mid, count );
    _armySelected = -1;
    Q_EMIT changed();
    update();
}

void HeroPanel::clickArmySlot( int slot )
{
    if ( !_hero || !_saveFile )
        return;
    const int selected = _armySelected;
    const Troop & sel = selected >= 0 ? _hero->slots[selected] : Troop{};
    const Troop & cur = _hero->slots[slot];

    if ( selected < 0 ) {
        if ( cur.monsterId == 0 ) {
            pickMonsterForSlot( slot );
        }
        else {
            // Select the troop (a click on another slot then swaps/moves).
            _armySelected = slot;
            update();
        }
        return;
    }

    if ( selected == slot ) {
        _armySelected = -1;
        update();
        return;
    }

    if ( sel.monsterId == 0 ) {
        _armySelected = -1;
        update();
        return;
    }

    if ( cur.monsterId == 0 ) {
        // Move the troop into the empty slot.
        _saveFile->setSlot( *_hero, slot, sel.monsterId, sel.count );
        _saveFile->setSlot( *_hero, selected, 0, 0 );
    }
    else if ( cur.monsterId == sel.monsterId ) {
        // Combine identical troops.
        _saveFile->setSlot( *_hero, slot, sel.monsterId, std::min<uint32_t>( 999999, sel.count + cur.count ) );
        _saveFile->setSlot( *_hero, selected, 0, 0 );
    }
    else {
        // Swap the troops.
        _saveFile->setSlot( *_hero, selected, cur.monsterId, cur.count );
        _saveFile->setSlot( *_hero, slot, sel.monsterId, sel.count );
    }
    _armySelected = -1;
    Q_EMIT changed();
    update();
}

void HeroPanel::clearArmySlot( int slot )
{
    if ( !_hero || !_saveFile )
        return;
    if ( _hero->slots[slot].monsterId == 0 )
        return;
    if ( occupiedSlots() <= 1 ) {
        setStatus( editorText( "The last hero's troop cannot be removed" ) );
        return;
    }
    _saveFile->setSlot( *_hero, slot, 0, 0 );
    Q_EMIT changed();
    update();
}

void HeroPanel::editSecondarySkill( int slot )
{
    if ( !_hero || !_saveFile || !_assets )
        return;
    if ( slot > static_cast<int>( _hero->skills.size() ) || slot >= 8 ) {
        setStatus( editorText( "Fill the previous skill slots first" ) );
        return;
    }

    // An occupied cell shows the skill description (like SecondarySkillDialogElement):
    // the title and text use the skill name including the level.
    if ( slot < static_cast<int>( _hero->skills.size() ) && _hero->skills[slot].first > 0 ) {
        const int skillId = _hero->skills[slot].first;
        const int level = _hero->skills[slot].second;
        GameMessageElement element;
        element.kind = GameMessageElement::SECONDARY_SKILL;
        element.id = skillId;
        element.level = level;
        showGameMessage( this, *_assets, QString::fromStdString( skillNameWithLevel( skillId, level ) ),
                         QString::fromStdString( skillDescription( skillId, level ) ), MSG_OK, element );
        return;
    }

    // An empty cell picks a skill (id + level), like Dialog::selectSecondarySkill.
    // Skills already given to the hero are excluded from the list (all levels).
    int skillId = 0;
    int level = 0;
    std::vector<int> taken;
    for ( const auto & s : _hero->skills ) {
        if ( s.first > 0 )
            taken.push_back( s.first );
    }
    if ( !selectSecondarySkillDialog( this, *_assets, skillId, level, taken ) )
        return;
    try {
        _saveFile->setSecondarySkill( *_hero, slot, skillId, level );
    }
    catch ( const SaveError & e ) {
        setStatus( QString::fromStdString( e.what() ) );
        return;
    }
    Q_EMIT changed();
    update();
}

void HeroPanel::pickArtifact( int slot )
{
    if ( !_hero || !_saveFile || !_assets )
        return;
    if ( !_hero->heroBaseParsed ) {
        setStatus( editorText( "Artifacts unavailable (hero record not recognized)" ) );
        return;
    }

    // Slots are filled in order: occupied cells, empty cells inside the bag
    // (holes) and the first slot right after the bag are available.
    if ( slot > _hero->artifactCount || slot >= 14 ) {
        setStatus( editorText( "Fill the previous artifact slots first" ) );
        return;
    }

    const int curId = slot < _hero->artifactCount ? _hero->artifacts[slot].id : 0;
    // Artifact duplicates are allowed (all except multiple spell books).
    const int id = selectArtifactDialog( this, *_assets, curId, {} );
    if ( id < 0 )
        return;
    if ( id == curId )
        return;

    // Only one spell book is allowed (SpellBookActivate in the engine).
    if ( id == ARTIFACT_MAGIC_BOOK ) {
        for ( int i = 0; i < _hero->artifactCount; ++i ) {
            if ( _hero->artifacts[i].id == ARTIFACT_MAGIC_BOOK ) {
                showGameMessage( this, *_assets, gameText( "Magic Book" ),
                                 gameText( "You cannot have multiple spell books." ) );
                return;
            }
        }
        if ( _hero->artifactCount >= 14 ) {
            showGameMessage( this, *_assets, gameText( "Artifact" ),
                             gameText( "You cannot pick up this artifact, you already have a full load!" ) );
            return;
        }
    }

    try {
        // An empty cell inside the bag replaces a hole; a slot after the bag —
        // appends to the end.
        _saveFile->setArtifact( *_hero, slot, id );
    }
    catch ( const SaveError & e ) {
        setStatus( QString::fromStdString( e.what() ) );
        return;
    }
    Q_EMIT changed();
    update();
}

// Single click on an artifact cell (port of ArtifactsBar::ActionBarLeftMouseSingleClick):
// a book — open it; an occupied cell — select (frame); a second click — move/swap;
// an empty available cell — pick an artifact from the list.
void HeroPanel::clickArtifactSlot( int slot )
{
    if ( !_hero || !_saveFile )
        return;

    // Unavailable slots (out of order) cannot be moved to or selected.
    if ( slot > _hero->artifactCount || slot >= 14 ) {
        _artifactSelected = -1;
        setStatus( editorText( "Fill the previous artifact slots first" ) );
        update();
        return;
    }

    // A spell book opens the spell editor.
    if ( slot < _hero->artifactCount && _hero->artifacts[slot].id == ARTIFACT_MAGIC_BOOK ) {
        _artifactSelected = -1;
        editSpellBook();
        return;
    }

    const int selected = _artifactSelected;
    if ( selected >= 0 ) {
        if ( selected == slot ) {
            _artifactSelected = -1;
            update();
            return;
        }
        const bool curIsBook = slot < _hero->artifactCount && _hero->artifacts[slot].id == ARTIFACT_MAGIC_BOOK;
        const bool selIsBook = selected < _hero->artifactCount && _hero->artifacts[selected].id == ARTIFACT_MAGIC_BOOK;
        if ( curIsBook || selIsBook ) {
            _artifactSelected = -1;
            setStatus( gameText( "Cannot move the Spellbook" ) );
            update();
            return;
        }
        const int curId = slot < _hero->artifactCount ? _hero->artifacts[slot].id : 0;
        try {
            if ( curId == 0 ) {
                // Move into the empty slot: the artifact goes to the chosen
                // cell, the old place becomes empty.
                _saveFile->moveArtifact( *_hero, selected, slot );
            }
            else {
                _saveFile->swapArtifacts( *_hero, selected, slot );
            }
        }
        catch ( const SaveError & e ) {
            _artifactSelected = -1;
            setStatus( QString::fromStdString( e.what() ) );
            update();
            return;
        }
        _artifactSelected = -1;
        Q_EMIT changed();
        update();
        return;
    }

    const int curId = slot < _hero->artifactCount ? _hero->artifacts[slot].id : 0;
    if ( curId != 0 ) {
        // Select the artifact (red frame).
        _artifactSelected = slot;
        update();
        return;
    }

    pickArtifact( slot );
}

// Double click on an occupied artifact cell — info hint with the description
// (port of ArtifactsBar::ActionBarLeftMouseDoubleClick → ArtifactDialogElement).
void HeroPanel::showArtifactInfo( int slot )
{
    if ( !_hero || !_assets )
        return;
    _artifactSelected = -1;
    if ( slot >= _hero->artifactCount || _hero->artifacts[slot].id == 0 )
        return;
    const int id = _hero->artifacts[slot].id;
    if ( id == ARTIFACT_MAGIC_BOOK ) {
        editSpellBook();
        return;
    }
    GameMessageElement element;
    element.kind = GameMessageElement::ARTIFACT;
    element.id = id;
    showGameMessage( this, *_assets, QString::fromStdString( artifactName( id ) ),
                     QString::fromStdString( artifactDescription( id ) ), MSG_OK, element );
    update();
}

void HeroPanel::clearArtifact( int slot )
{
    if ( !_hero || !_saveFile )
        return;
    if ( !_hero->heroBaseParsed || slot >= _hero->artifactCount || _hero->artifacts[slot].id == 0 )
        return;
    // The spell book is not removed.
    if ( _hero->artifacts[slot].id == ARTIFACT_MAGIC_BOOK )
        return;
    _artifactSelected = -1;
    try {
        _saveFile->setArtifact( *_hero, slot, 0 );
    }
    catch ( const SaveError & e ) {
        setStatus( QString::fromStdString( e.what() ) );
        return;
    }
    Q_EMIT changed();
    update();
}

void HeroPanel::editSpellBook()
{
    if ( !_hero || !_saveFile || !_assets )
        return;
    if ( !_hero->heroBaseParsed ) {
        setStatus( editorText( "Spell book unavailable (hero record not recognized)" ) );
        return;
    }
    std::vector<int> spells = _hero->spells;
    if ( !spellBookDialog( this, *_assets, spells ) )
        return;
    try {
        _saveFile->setSpells( *_hero, spells );
    }
    catch ( const SaveError & e ) {
        setStatus( QString::fromStdString( e.what() ) );
        return;
    }
    Q_EMIT changed();
    update();
}

QString HeroPanel::moraleLuckText( bool luck ) const
{
    if ( !_hero )
        return {};

    // As in the game: short "%level Morale" / "%level Luck" (MoraleString/LuckString).
    // Levels are fheroes2 context keys (morale|X / luck|X in .po).
    const int value = moraleLuckValue( luck );
    const QString level = luck
                              ? ( value > 0 ? gameText( "luck|Good" )
                                            : ( value < 0 ? gameText( "luck|Bad" ) : gameText( "luck|Normal" ) ) )
                              : ( value > 1 ? gameText( "morale|Great" )
                                            : ( value == 1 ? gameText( "morale|Good" )
                                                           : ( value == 0 ? gameText( "morale|Normal" )
                                                                          : ( value == -1 ? gameText( "morale|Poor" )
                                                                                          : ( value == -2 ? gameText( "morale|Awful" )
                                                                                                          : gameText( "morale|Treason" ) ) ) ) ) );
    return luck ? gameText( "%{luck} Luck" ).replace( "%{luck}", level ) : gameText( "%{morale} Morale" ).replace( "%{morale}", level );
}

int HeroPanel::moraleLuckValue( bool luck ) const
{
    if ( !_hero )
        return 0;

    // Simplified calculation: morale/luck are not stored in the save, the game
    // computes them from the army, skills and artifacts. Here — an estimate
    // based on the army and skills.
    const auto factionOf = []( int monsterId ) {
        if ( monsterId <= 0 )
            return 0;
        if ( monsterId <= 11 )
            return 1; // Knight
        if ( monsterId <= 20 )
            return 2; // Barbarian
        if ( monsterId <= 29 )
            return 3; // Sorceress
        if ( monsterId <= 38 )
            return 4; // Warlock
        if ( monsterId <= 47 )
            return 5; // Wizard
        if ( monsterId <= 57 )
            return 6; // Necromancer
        return 0;     // neutral
    };

    int value = 0;
    int factions = 0;
    int factionMask = 0;
    int troops = 0;
    for ( int i = 0; i < 5; ++i ) {
        const int f = factionOf( _hero->slots[i].monsterId );
        if ( _hero->slots[i].monsterId != 0 ) {
            ++troops;
            if ( f != 0 )
                factionMask |= 1 << f;
        }
    }
    for ( int f = 1; f <= 6; ++f ) {
        if ( factionMask & ( 1 << f ) )
            ++factions;
    }

    if ( luck ) {
        // Luck: the luck skill (+1..+3) and artifacts 36..39 (+1), Fizbin (−2).
        for ( const auto & s : _hero->skills ) {
            if ( s.first == 10 )
                value += s.second;
        }
        for ( int i = 0; i < _hero->artifactCount; ++i ) {
            const int id = _hero->artifacts[i].id;
            if ( id >= 36 && id <= 39 )
                value += 1;
            if ( id == 17 )
                value -= 2;
        }
    }
    else if ( troops > 0 ) {
        // Morale: an all-undead army is neutral; otherwise +1 for a single
        // faction, −1 for each additional one; the leadership skill gives +1..+3.
        if ( ( factionMask & ~( 1 << 6 ) ) == 0 )
            value = 0;
        else if ( factions <= 1 )
            value = 1;
        else
            value = -( factions - 1 );
        for ( const auto & s : _hero->skills ) {
            if ( s.first == 7 )
                value += s.second;
        }
    }
    return value;
}

void HeroPanel::setStatus( const QString & text )
{
    if ( _status == text )
        return;
    _status = text;
    update();
}

void HeroPanel::setStatusMessage( const QString & text )
{
    setStatus( text );
}

// --- painting ---

void HeroPanel::paintEvent( QPaintEvent * )
{
    // Painting into an offscreen buffer: the hero screen (offset for the
    // frame), then the StandardWindow frame and its shadow — like a window in
    // the map editor.
    QImage buf( screenSize(), QImage::Format_RGBA8888 );
    buf.fill( Qt::transparent );
    QPainter p( &buf );
    p.translate( contentOffset() );

    // Backdrop with the same tile grid as the main window background (the
    // "starry sky" of an unexplored map): HEROBKG contains transparent windows
    // through which it is visible — like the game screen under the hero dialog.
    {
        // Raw buffer coordinates: the tile grid is tied to the window, not to
        // the offset content.
        p.save();
        p.resetTransform();
        drawFogBackground( p, _assets, buf.size(), mapTo( window(), QPoint( 0, 0 ) ) );
        p.restore();
    }

    if ( !_background.isNull() ) {
        p.drawPixmap( 0, 0, _background );

        // The background EXIT button: cover it with a mirrored copy of the
        // column to the left of the artifacts (flipped horizontally — like the
        // right frame). The buffer uses raw coordinates; add the content offset.
        const QImage patch = mirrorHorizontal( buf.copy( exitPatchSrc().translated( contentOffset().x(), contentOffset().y() ) ) );
        p.drawImage( exitPatchDest().topLeft(), patch );
    }
    else if ( !_assets ) {
        p.fillRect( rect(), QColor( "#7a6a5a" ) );
    }

    if ( _hero ) {
        // Title: "Name Class (Level N)" centered in the area (60,2,519,17).
        const int level = heroLevel( _hero->experience );
        const QString title = replaceName( replaceName( gameText( "%{name} the %{race} (Level %{level})" )
                                                       .replace( "%{level}", QString::number( level ) ),
                                                    "%{name}", heroDisplayName( *_hero ) ),
                                      "%{race}", QString::fromStdString( raceName( _hero->race ) ) );
        {
            const QRect tr_ = titleRect();
            const int w = _font->textWidth( title, GameFont::Size::NORMAL );
            _font->drawText( p, tr_.x() + ( tr_.width() - w ) / 2, tr_.y() + 1, title, GameFont::Size::NORMAL, GameFont::Color::WHITE );
        }

        // Portrait.
        const QPixmap portrait = _assets ? _assets->portraitPixmap( _hero->portrait, SCREEN_SCALE ) : QPixmap();
        if ( !portrait.isNull() ) {
            const QRect pr = portraitRect();
            p.drawPixmap( pr.x(), pr.y(), portrait );
        }

        // Crest.
        const int crestIdx = crestIndexForColor( _hero->color );
        if ( !_crest[crestIdx].isNull() ) {
            const QRect cr = crestRect();
            p.drawPixmap( cr.x(), cr.y(), _crest[crestIdx] );
        }

        // Primary skills: name on top + value below (like PrimarySkillsBar).
        for ( int i = 0; i < 4; ++i ) {
            const QRect r = primaryRect( i );
            if ( !_primskil[i].isNull() )
                p.drawPixmap( r.topLeft(), _primskil[i] );
            if ( _hero->heroBaseParsed ) {
                const QString name = QString::fromStdString( primarySkillName( i ) );
                _font->drawText( p, r.x() + ( r.width() - _font->textWidth( name, GameFont::Size::SMALL ) ) / 2, r.y() + 6, name,
                                 GameFont::Size::SMALL, GameFont::Color::WHITE );
                const QString value = QString::number( _hero->primary.value( i ) );
                const int w = _font->textWidth( value, GameFont::Size::NORMAL );
                _font->drawText( p, r.x() + ( r.width() - w ) / 2, r.bottom() - 17, value, GameFont::Size::NORMAL, GameFont::Color::WHITE );
            }
        }

        // Morale/luck: icons by sign (like MoraleIndicator/LuckIndicator).
        {
            const auto drawIndicator = [&]( bool isLuck, const QRect & area ) {
                const int value = moraleLuckValue( isLuck );
                int iconIdx = 6; // neutral
                if ( !isLuck ) {
                    iconIdx = value > 0 ? 4 : ( value < 0 ? 5 : 7 );
                }
                else {
                    iconIdx = value > 0 ? 2 : ( value < 0 ? 3 : 6 );
                }
                if ( _hsicons[iconIdx].isNull() )
                    return;
                const int count = std::max( 1, std::abs( value ) );
                const int inter = 6;
                int cx = area.x() + ( area.width() - ( _hsicons[iconIdx].width() + inter * ( count - 1 ) ) ) / 2;
                const int cy = area.y() + ( area.height() - _hsicons[iconIdx].height() ) / 2;
                for ( int k = 0; k < count; ++k ) {
                    p.drawPixmap( cx, cy, _hsicons[iconIdx] );
                    cx += inter;
                }
            };
            drawIndicator( false, moraleRect() );
            drawIndicator( true, luckRect() );
        }

        // Experience and mana: icons + values below them (like
        // ExperienceIndicator / SpellPointsIndicator in fheroes2, editor mode).
        {
            const QRect er = expRect();
            const QRect mr = manaRect();
            if ( !_hsicons[1].isNull() )
                p.drawPixmap( er.topLeft(), _hsicons[1] );
            if ( !_hsicons[8].isNull() )
                p.drawPixmap( mr.topLeft(), _hsicons[8] );

            // Experience: the number below the icon; on overflow — "x.xxM" as in the game.
            if ( _hero ) {
                QString expText = QString::number( _hero->experience );
                if ( _font->textWidth( expText, GameFont::Size::SMALL ) > 33 ) {
                    const uint32_t millions = _hero->experience / 1000000;
                    if ( _hero->experience < 10000000 )
                        expText = QString( "%1.%2M" ).arg( millions ).arg( ( _hero->experience - millions * 1000000 ) / 10000, 2, 10, QLatin1Char( '0' ) );
                    else
                        expText = QString( "%1.%2M" ).arg( millions ).arg( ( _hero->experience - millions * 1000000 ) / 100000, 1, 10, QLatin1Char( '0' ) );
                }
                const int ew = _font->textWidth( expText, GameFont::Size::SMALL );
                _font->drawText( p, er.x() + ( er.width() - ew ) / 2, er.y() + 25, expText, GameFont::Size::SMALL, GameFont::Color::WHITE );

                // Mana: "current/maximum" (maximum = 10×knowledge).
                const int maxSp = _hero->heroBaseParsed ? std::max( 1, _hero->primary.knowledge * 10 ) : 0;
                const QString spText = QString( "%1/%2" ).arg( _hero->spellPoints ).arg( maxSp );
                const int sw = _font->textWidth( spText, GameFont::Size::SMALL );
                if ( sw <= 33 ) {
                    _font->drawText( p, mr.x() + ( mr.width() - sw ) / 2, mr.y() + 23, spText, GameFont::Size::SMALL, GameFont::Color::WHITE );
                }
                else {
                    const QString cur = QString::number( _hero->spellPoints );
                    const QString max = QString::number( maxSp );
                    _font->drawText( p, mr.x() + ( mr.width() - _font->textWidth( cur, GameFont::Size::SMALL ) ) / 2, mr.y() + 14, cur,
                                     GameFont::Size::SMALL, GameFont::Color::WHITE );
                    _font->drawText( p, mr.x() + mr.width() - _font->textWidth( max, GameFont::Size::SMALL ) - 2, mr.y() + 24, max,
                                     GameFont::Size::SMALL, GameFont::Color::WHITE );
                }
            }
        }

        // Army: 5 cells. An empty slot — the STRIP[2] placeholder (like
        // RedrawBackground in the engine's map editor), an occupied one — the
        // race frame STRIP[4..10], the monster at its hotspot and the counter
        // in the bottom right corner (ArmyBar::RedrawItem).
        {
            const int stripIdx = raceStripIndex( _hero->race ) - 4;
            for ( int i = 0; i < 5; ++i ) {
                const QRect cell = armyRect( i );

                const int mid = _hero->slots[i].monsterId;
                if ( mid == 0 ) {
                    // Empty slot placeholder.
                    if ( !_stripEmpty.isNull() )
                        p.drawPixmap( cell.topLeft(), _stripEmpty );
                }
                else {
                    if ( stripIdx >= 0 && stripIdx < 11 && !_strip[stripIdx].isNull() )
                        p.drawPixmap( cell.topLeft(), _strip[stripIdx] );

                    if ( _assets ) {
                        const QString icn = monsterIcnName( mid );
                        const IcnSprite & sprite = _assets->icnSprite( icn.toStdString(), 0 );
                        if ( !sprite.isNull() ) {
                            const QPixmap pm = _assets->icnPixmap( icn.toStdString(), 0, SCREEN_SCALE );
                            p.drawPixmap( cell.x() + sprite.offsetX, cell.y() + sprite.offsetY, pm );
                        }
                    }

                    // Counter in the bottom right corner of the cell (as in ArmyBar::RedrawItem).
                    const uint32_t count = _hero->slots[i].count;
                    if ( count != 0 ) {
                        const QString text = QString::number( count );
                        const int w = _font->textWidth( text, GameFont::Size::NORMAL );
                        const int th = _font->lineHeight( GameFont::Size::NORMAL );
                        _font->drawText( p, cell.right() - w - 3, cell.y() + cell.height() - th + 1, text,
                                         GameFont::Size::NORMAL, GameFont::Color::WHITE );
                    }
                }

                // Selected slot frame (spcursor = STRIP[1], like ArmyBar).
                if ( i == _armySelected && !_stripSel.isNull() )
                    p.drawPixmap( cell.topLeft(), _stripSel );
            }
        }

        // Secondary skills: frames for all 8 cells + icons/name/level for occupied ones.
        for ( int i = 0; i < 8; ++i ) {
            const QRect cell = secSkillRect( i );
            if ( !_secskill[0].isNull() )
                p.drawPixmap( cell.topLeft(), _secskill[0] );
            if ( i < static_cast<int>( _hero->skills.size() ) ) {
                const int skillId = _hero->skills[i].first;
                const int skillLvl = _hero->skills[i].second;
                if ( skillId >= 1 && skillId <= 14 && !_secskill[skillId].isNull() )
                    p.drawPixmap( cell.topLeft(), _secskill[skillId] );

                // The text is cut to the cell width with "..." (fitToOneRow in
                // the engine, ui_text.cpp:495) and drawn within the cell clip
                // (drawInRoi): the name at y+5, the level at y+53, like
                // SecondarySkillsBar::RedrawItem (skill_bar.cpp:368).
                const auto drawCellText = [&]( const QString & raw, int y ) {
                    QString text = raw;
                    if ( _font->textWidth( text, GameFont::Size::SMALL ) > cell.width() )
                        text = _font->elideText( text, GameFont::Size::SMALL, cell.width() );
                    const int w = _font->textWidth( text, GameFont::Size::SMALL );
                    p.save();
                    p.setClipRect( cell );
                    _font->drawText( p, cell.x() + ( cell.width() - w ) / 2 - 1, cell.y() + y, text, GameFont::Size::SMALL,
                                     GameFont::Color::WHITE );
                    p.restore();
                };
                drawCellText( QString::fromStdString( skillName( skillId ) ), 5 );
                drawCellText( QString::fromStdString( skillLevelName( skillLvl ) ), 53 );
            }

        }

        // Artifacts 7×2: the cell backdrop for all 14 slots + the large
        // ARTIFACT.ICN[id] sprite (like ArtifactsBar in the game).
        for ( int i = 0; i < 14; ++i ) {
            const QRect cell = artifactRect( i );
            const int artId = _hero->heroBaseParsed ? ( i < _hero->artifactCount ? _hero->artifacts[i].id : 0 ) : 0;
            if ( !_artifactCell.isNull() ) {
                p.drawPixmap( cell.topLeft(), _artifactCell );
            }
            else {
                p.fillRect( cell, QColor( 0, 0, 0, 60 ) );
            }
            if ( artId >= 1 && artId <= 82 ) {
                if ( !_artifact[artId].isNull() )
                    p.drawPixmap( cell.topLeft(), _artifact[artId] );
                else if ( !_artfx[artId - 1].isNull() )
                    p.drawPixmap( cell.topLeft(), _artfx[artId - 1] );
            }

            // Selected artifact frame (spcursor 70×70, like ArtifactsBar).
            if ( i == _artifactSelected && !_artifactCursor.isNull() )
                p.drawPixmap( cell.x() - 3, cell.y() - 3, _artifactCursor );
        }

        // Status bar (without the prev/next buttons — they are below, after the shadows).
        {
            const QRect bar = statusBarRect();
            if ( !_hsbtns[8].isNull() )
                p.drawPixmap( bar.topLeft(), _hsbtns[8] );
            // Like StatusBar::updateMessage in fheroes2: NORMAL white, cut to
            // the bar width (fitToOneRow), centered, +3px vertically.
            QString text = _status;
            if ( _font->textWidth( text, GameFont::Size::NORMAL ) > bar.width() )
                text = _font->elideText( text, GameFont::Size::NORMAL, bar.width() );
            const int w = _font->textWidth( text, GameFont::Size::NORMAL );
            _font->drawText( p, bar.x() + ( bar.width() - w ) / 2, bar.y() + 3, text, GameFont::Size::NORMAL, GameFont::Color::WHITE );
        }

        // Shadows of the prev/next buttons.
        {
            addGradientShadow( buf, prevRect().topLeft() + contentOffset(), _hsbtns[4].toImage(), QPoint( -5, 5 ) );
            addGradientShadow( buf, nextRect().topLeft() + contentOffset(), _hsbtns[6].toImage(), QPoint( -5, 5 ) );
        }

        // Prev/next buttons on top of the shadows.

        {
            // The prev/next buttons dim only when navigation is unavailable.
            const bool prevPressed = _pressed.area == HIT_PREV;
            const bool nextPressed = _pressed.area == HIT_NEXT;
            QPixmap prevPm = _hsbtns[prevPressed ? 5 : 4];
            QPixmap nextPm = _hsbtns[nextPressed ? 7 : 6];
            if ( !_navEnabled ) {
                prevPm = dimmed( prevPm );
                nextPm = dimmed( nextPm );
            }
            p.drawPixmap( prevRect().topLeft(), prevPm );
            p.drawPixmap( nextRect().topLeft(), nextPm );
        }
    }

    p.end();

    // Standard window frame around the hero screen (port of StandardWindow).
    // The shadow is NOT drawn: only dialog windows cast one.
    const QPixmap frame = makeStandardFrame( _assets, QSize( screenS( SCREEN_W ), screenS( SCREEN_H ) ), false );
    {
        QPainter fp( &buf );
        fp.drawPixmap( 0, 0, frame );
        fp.end();
    }

    QPainter view( this );
    view.drawImage( 0, 0, buf );
}

QPixmap HeroPanel::dimmed( QPixmap pm ) const
{
    if ( pm.isNull() )
        return pm;
    QImage img = pm.toImage();
    for ( int y = 0; y < img.height(); ++y ) {
        for ( int x = 0; x < img.width(); ++x ) {
            QColor c = img.pixelColor( x, y );
            if ( c.alpha() != 0 )
                img.setPixelColor( x, y, QColor( c.red() / 2, c.green() / 2, c.blue() / 2, c.alpha() ) );
        }
    }
    return QPixmap::fromImage( img );
}

} // namespace fh2
