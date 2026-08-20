#pragma once

#include <functional>
#include <memory>

#include <QPixmap>
#include <QWidget>

#include "gamefont.h"

class QContextMenuEvent;
class QMouseEvent;
class QPaintEvent;
class QTimer;
class QRect;
class QSize;
class QPoint;
class QString;
class QPainter;

namespace fh2 {

class Assets;
class SaveFile;
struct HeroRecord;

// Hero screen, a 1:1 port of the Hero Screen window from fheroes2
// (src/fheroes2/heroes/heroes_dialog.cpp, map editor mode).
// 640×480, natural scale. Drawing and hit tests are done manually.
class HeroPanel : public QWidget
{
    Q_OBJECT
public:
    explicit HeroPanel( const Assets * assets, QWidget * parent = nullptr );
    ~HeroPanel() override;

    void setHero( SaveFile * saveFile, HeroRecord * hero );
    void clearHero();
    void setAssets( const Assets * assets );
    // Shows a message in the in-game style status bar at the bottom of the panel.
    void setStatusMessage( const QString & text );

    // The "previous/next hero" buttons are disabled when there are fewer than 2 heroes.
    void setNavigationEnabled( bool enabled );

    QSize screenSize() const;

Q_SIGNALS:
    void changed();
    void prevClicked();
    void nextClicked();

protected:
    void paintEvent( QPaintEvent * event ) override;
    void mousePressEvent( QMouseEvent * event ) override;
    void mouseReleaseEvent( QMouseEvent * event ) override;
    void mouseDoubleClickEvent( QMouseEvent * event ) override;
    void mouseMoveEvent( QMouseEvent * event ) override;
    void leaveEvent( QEvent * event ) override;
    void contextMenuEvent( QContextMenuEvent * event ) override;

private:
    enum HitArea {
        HIT_NONE = 0,
        HIT_TITLE,
        HIT_PORTRAIT,
        HIT_CREST,
        HIT_PRIMARY,      // + index 0..3
        HIT_EXP,
        HIT_MANA,
        HIT_MORALE,
        HIT_LUCK,
        HIT_ARMY,       // + index 0..4
        HIT_SECSKILL,   // + index 0..7
        HIT_ARTIFACT,   // + index 0..13
        HIT_PREV,
        HIT_NEXT,
    };

    struct Hit {
        HitArea area = HIT_NONE;
        int index = -1;
    };

    Hit hitTest( int x, int y ) const;

    QRect titleRect() const;
    QRect portraitRect() const;
    QRect crestRect() const;
    QRect primaryRect( int i ) const;
    QRect expRect() const;
    QRect manaRect() const;
    QRect moraleRect() const;
    QRect luckRect() const;
    QRect armyRect( int i ) const;
    QRect secSkillRect( int i ) const;
    QRect artifactRect( int i ) const;
    QRect prevRect() const;
    QRect nextRect() const;
    QRect statusBarRect() const;

    void refreshAssets();
    void startHold( std::function<void()> action );
    void stopHold();
    void editPrimarySkill( int index );
    void resetPrimarySkillsToDefaults();
    void editExperience();
    void resetExperience();
    void editSpellPoints();
    void resetSpellPoints();
    void clickArmySlot( int slot );
    void showArmyInfo( int slot );
    void editTitle();
    void pickPortrait();
    void pickMonsterForSlot( int slot );
    void clearArmySlot( int slot );
    void editSecondarySkill( int slot );
    void clickArtifactSlot( int slot );
    void showArtifactInfo( int slot );
    void pickArtifact( int slot );
    void clearArtifact( int slot );
    void editSpellBook();
    QString moraleLuckText( bool luck ) const;
    void setStatus( const QString & text );

    QPixmap dimmed( QPixmap pm ) const;
    int occupiedSlots() const;
    int moraleLuckValue( bool luck ) const;

    const Assets * _assets = nullptr;
    SaveFile * _saveFile = nullptr;
    HeroRecord * _hero = nullptr;
    bool _navEnabled = true;
    std::unique_ptr<GameFont> _font;

    QPixmap _background;
    QPixmap _hsbtns[9];    // HSBTNS 0..8
    QPixmap _recruit[4];   // RECRUIT 0..3
    QPixmap _hsicons[12];  // HSICONS 0..11
    QPixmap _primskil[4];  // PRIMSKIL 0..3
    QPixmap _secskill[15]; // SECSKILL 0..14 (0 — frame, 1..14 — skills)
    QPixmap _artfx[82];    // ARTFX 0..81 (artifacts 1..82)
    QPixmap _artifact[83]; // ARTIFACT.ICN 1..82 (large artifact icons)
    QPixmap _artifactCell; // ARTIFACT.ICN 0 (artifact cell backdrop)
    QPixmap _strip[11];    // STRIP 4..10 (army cell frames per race)
    QPixmap _stripEmpty;   // STRIP[2] (empty army slot placeholder, as in the map editor)
    QPixmap _stripSel;     // STRIP[1] (selected army slot frame)
    QPixmap _artifactCursor; // selected artifact frame (spcursor 70×70, ArtifactsBar)
    QPixmap _crest[6];     // CREST 0..5 (player crests)

    Hit _pressed;          // held-down area
    Hit _hovered;          // area under the cursor (for highlighting)
    int _armySelected = -1; // selected army slot (clicking a second slot — swap/move)
    int _artifactSelected = -1; // selected artifact (like ArtifactsBar::spcursor)
    QString _status;
    std::unique_ptr<QTimer> _holdTimer;
    std::function<void()> _holdAction;
    bool _holdInitial = false;
};

} // namespace fh2
