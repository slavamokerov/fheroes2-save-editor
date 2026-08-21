#pragma once

#include <memory>
#include <vector>

#include <QMainWindow>

namespace fh2 {

class Assets;
class GameButton;
class GameFont;
class HeroPanel;
class SaveFile;
struct HeroRecord;

// Central widget: stone background (STONEBAK), game buttons at the top,
// map name in the game font, hero screen in the center.
class CentralWidget : public QWidget
{
    Q_OBJECT
public:
    CentralWidget( QWidget * parent = nullptr );

    void setAssets( const Assets * assets );
    void setMapText( const QString & text );
    void setInfoText( const QString & text );
    void setOpenEnabled( bool enabled );
    HeroPanel * heroPanel() { return _heroPanel; }
    void setLayoutBounds();

Q_SIGNALS:
    void openClicked();
    void saveClicked();
    void dataDirClicked();
    void quitClicked();

protected:
    void paintEvent( QPaintEvent * event ) override;
    void resizeEvent( QResizeEvent * event ) override;

private:
    void rebuildButtons();

    const Assets * _assets = nullptr;
    std::unique_ptr<GameFont> _font;
    QString _mapText;
    QString _infoText;
    bool _openEnabled = false;
    GameButton * _btnOpen = nullptr;
    GameButton * _btnSave = nullptr;
    GameButton * _btnData = nullptr;
    GameButton * _btnQuit = nullptr;
    HeroPanel * _heroPanel = nullptr;

    // Layout, recalculated in setLayoutBounds: the left edge of the hero panel
    // and its top (the panel is centered in the area BELOW the hints block, so
    // that the save/warning lines never overlap the panel).
    int _panelX = 8;
    int _panelY = 74;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow( QWidget * parent = nullptr );
    ~MainWindow() override;

    void openPath( const QString & path ) { open( path ); }

    // Debug UI (FH2_DEBUG_DIALOG): open a dialog by name to snapshot it.
    // Returns false if the name is unknown or no resources are loaded.
    bool openDebugDialog( const QString & name );

private Q_SLOTS:
    void openDialog();
    void saveFile();
    void pickDataDir();

private:
    void open( const QString & path );
    void fillHeroes();
    void showHero( int index );
    void markDirty();
    void updateActions();
    bool confirmDiscard();
    void warnIfGameRunning();
    void selectPrevHero();
    void selectNextHero();
    bool pickGameDataDir( bool force );
    void showError( const QString & message );
    void showEvent( QShowEvent * event ) override;
    void keyPressEvent( QKeyEvent * event ) override;
    void closeEvent( QCloseEvent * event ) override;
    // Classic cheat code typed anywhere in the window: adds 5 Black Dragons
    // to the current hero's army.
    void applyCheatCode();

    std::unique_ptr<Assets> _assets;
    std::unique_ptr<SaveFile> _saveFile;
    bool _dirty = false;
    std::vector<const HeroRecord *> _shownHeroes;
    int _currentHero = -1;
    QString _dataDir;
    QString _path;
    QString _cheatBuf; // trailing digits typed so far

    CentralWidget * _central = nullptr;
};

} // namespace fh2
