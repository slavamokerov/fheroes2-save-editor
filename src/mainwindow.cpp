#include "mainwindow.h"

#include <cstdio>

#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QDateTime>
#include <QFileInfo>
#include <QKeyEvent>
#include <QPainter>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>

#include "assets.h"
#include "constants.h"
#include "gamedata.h"
#include "gamefont.h"
#include "gameui.h"
#include "gettextmo.h"
#include "heropanel.h"
#include "dialogs.h"
#include "savefile.h"
#include "textutil.h"
#include "translations.h"

namespace fh2 {

namespace {

QString defaultSaveDir()
{
    const QString home = QDir::homePath();
    const QStringList candidates = {
        home + "/Library/Application Support/fheroes2/files/save",
        home + "/.local/share/fheroes2/files/save",
        home + "/AppData/Local/fheroes2/files/save",
    };
    for ( const QString & c : candidates ) {
        if ( QFileInfo::exists( c ) )
            return c;
    }
    return home;
}

} // namespace

// --- CentralWidget ---

CentralWidget::CentralWidget( QWidget * parent )
    : QWidget( parent )
{
    _heroPanel = new HeroPanel( nullptr, this );
}

void CentralWidget::setAssets( const Assets * assets )
{
    _assets = assets;
    _font = std::make_unique<GameFont>( assets );
    _heroPanel->setAssets( assets );
    rebuildButtons();
    setLayoutBounds();
    update();
}

void CentralWidget::setMapText( const QString & text )
{
    _mapText = text;
    update();
}

void CentralWidget::setOpenEnabled( bool enabled )
{
    _openEnabled = enabled;
    if ( _btnSave )
        _btnSave->setEnabled( enabled );
}

void CentralWidget::setInfoText( const QString & text )
{
    _infoText = text;
    update();
}

void CentralWidget::rebuildButtons()
{
    // The buttons are created only once: recreating them from the handler of
    // their own signal (e.g. "GAME DATA" → setAssets → rebuildButtons) deleted
    // the button while clicked() was being emitted and crashed the app. The
    // resources no longer depend on the data folder after the first creation.
    if ( _btnOpen )
        return;

    if ( !_assets || !_font->valid() )
        return;

    // The whole toolbar uses the dark interface theme (palette-swapped
    // sprites, like the engine's EVIL button variants) so that the buttons
    // match the "starry sky" background of the window.
    _btnOpen = makeGameTextButton( *_assets, *_font, uiButtonText( UiButton::OpenSave ), this, 0, true );
    // Save button — floppy disk icon from the game (map editor: "Map file
    // settings", EDITBTNS[20..21] 48×36), scaled down to the height of the
    // other toolbar buttons (25px).
    QPixmap saveRel = evilThemePixmap( *_assets, "EDITBTNS.ICN", 20 ).scaled( 33, 25, Qt::IgnoreAspectRatio, Qt::FastTransformation );
    QPixmap savePress = evilThemePixmap( *_assets, "EDITBTNS.ICN", 21 ).scaled( 33, 25, Qt::IgnoreAspectRatio, Qt::FastTransformation );
    _btnSave = new GameButton( saveRel, savePress, this );
    _btnData = makeGameTextButton( *_assets, *_font, uiButtonText( UiButton::GameData ), this, 0, true );
    _btnQuit = makeGameTextButton( *_assets, *_font, uiButtonText( UiButton::Exit ), this, 0, true );

    connect( _btnOpen, &GameButton::clicked, this, &CentralWidget::openClicked );
    connect( _btnSave, &GameButton::clicked, this, &CentralWidget::saveClicked );
    connect( _btnData, &GameButton::clicked, this, &CentralWidget::dataDirClicked );
    connect( _btnQuit, &GameButton::clicked, this, &CentralWidget::quitClicked );
    _btnSave->setEnabled( false );
}

void CentralWidget::setLayoutBounds()
{
    const int margin = 8;
    const int y = margin;

    // The hints block (save line + warning) occupies y = 40..66; the hero panel
    // is centered in the area BELOW it, so the hints never overlap the panel
    // at any window size.
    constexpr int hintsBottom = 66;
    constexpr int panelTopMin = hintsBottom + 8;

    // The hero panel is centered horizontally; the button block is aligned to
    // its left edge, and "GAME DATA"/"EXIT" — to the right.
    int px = margin;
    int panelW = 0;
    if ( _heroPanel ) {
        const QSize panel = _heroPanel->screenSize();
        panelW = panel.width();
        px = std::max( margin, ( width() - panelW ) / 2 );
        _panelY = std::max( panelTopMin, panelTopMin + ( height() - panelTopMin - panel.height() ) / 2 );
        _heroPanel->setGeometry( px, _panelY, panelW, panel.height() );
        _heroPanel->raise();
    }
    _panelX = px;

    // Left: open, save.
    int x = px;
    for ( GameButton * b : { _btnOpen, _btnSave } ) {
        if ( !b )
            continue;
        b->setGeometry( x, y, b->width(), b->height() );
        x += b->width() + 6;
    }
    // Right: exit and game data (game data is to the left of exit).
    if ( _btnQuit )
        _btnQuit->setGeometry( px + panelW - _btnQuit->width(), y, _btnQuit->width(), _btnQuit->height() );
    if ( _btnData && _btnQuit )
        _btnData->setGeometry( _btnQuit->x() - 6 - _btnData->width(), y, _btnData->width(), _btnData->height() );
}

void CentralWidget::paintEvent( QPaintEvent * )
{
    QImage buf( size(), QImage::Format_RGBA8888 );
    QPainter p( &buf );
    // Background — the "starry sky" of an unexplored map (CLOF32.TIL). The
    // tile grid is computed from the top-level window coordinates so that it
    // matches the backdrop inside the hero panel (visible through the
    // transparent HEROBKG "windows").
    drawFogBackground( p, _assets, size(), mapTo( window(), QPoint( 0, 0 ) ) );
    p.end();

    // No button shadows: the opaque button slabs on the "starry sky" would
    // cast visible dark rectangles under themselves (in the engine they are
    // invisible on the matching stone background).

    QPainter p2( this );
    p2.drawImage( 0, 0, buf );

    // Header lines are elided to the panel width so that long map/file names
    // never run past the window edge.
    const int textMaxWidth = std::max( 0, width() - _panelX - 8 );
    if ( _font && _font->valid() && !_mapText.isEmpty() ) {
        _font->drawText( p2, _panelX, 40, _mapText, GameFont::Size::SMALL, GameFont::Color::YELLOW, textMaxWidth );
    }
    if ( _font && _font->valid() && !_infoText.isEmpty() ) {
        // Warning line — right below the save line (map/date).
        _font->drawText( p2, _panelX, 55, _infoText, GameFont::Size::SMALL, GameFont::Color::WHITE, textMaxWidth );
    }
    p2.end();
}

void CentralWidget::resizeEvent( QResizeEvent * )
{
    setLayoutBounds();
}

// --- MainWindow ---

MainWindow::MainWindow( QWidget * parent )
    : QMainWindow( parent )
{
    setWindowTitle( tr( "fheroes2 Save Editor" ) );
    // Without a focused widget Qt does not deliver KeyPress events at all,
    // so the 32167 cheat code (keyPressEvent below) never fired.
    setFocusPolicy( Qt::StrongFocus );

    _central = new CentralWidget( this );
    setCentralWidget( _central );

    connect( _central, &CentralWidget::openClicked, this, &MainWindow::openDialog );
    connect( _central, &CentralWidget::saveClicked, this, &MainWindow::saveFile );
    connect( _central, &CentralWidget::dataDirClicked, this, &MainWindow::pickDataDir );
    connect( _central, &CentralWidget::quitClicked, this, &QWidget::close );
    connect( _central->heroPanel(), &HeroPanel::changed, this, &MainWindow::markDirty );
    connect( _central->heroPanel(), &HeroPanel::prevClicked, this, &MainWindow::selectPrevHero );
    connect( _central->heroPanel(), &HeroPanel::nextClicked, this, &MainWindow::selectNextHero );

    QTimer::singleShot( 0, this, [this]() {
        const QSize panel = _central->heroPanel()->screenSize();
        resize( panel + QSize( 40, 110 ) );
    } );

    pickGameDataDir( false );
    if ( _assets ) {
        _central->setAssets( _assets.get() );
        _central->heroPanel()->setAssets( _assets.get() );
        if ( _assets->icnPixmap( ICN_HEROBKG, 0 ).isNull() ) {
            showError( editorText( "Game graphics not found — press GAME DATA and choose the folder with HEROES2.AGG." ) );
        }
    }
}

bool MainWindow::pickGameDataDir( bool force )
{
    try {
        // The default folder is tried only at startup (widgets are not drawn
        // yet). With force, the old Assets is NOT touched until a new folder
        // is chosen: CentralWidget/HeroPanel reference it and get repainted
        // while the modal dialog is open (this used to be a use-after-free).
        if ( !force && !_assets ) {
            QString dir = QString::fromStdString( Assets::defaultDataDir() );
            if ( !dir.isEmpty() ) {
                auto assets = std::make_unique<Assets>( Assets::load( dir.toStdString() ) );
                if ( assets->valid() ) {
                    _assets = std::move( assets );
                    _dataDir = dir;
                    // The game data folder may also carry the game's own
                    // translation files (files/lang/<lang>.mo).
                    loadGameTranslationFromDataDir( dir.toStdString() );
                }
            }
        }
        if ( !_assets || force ) {
            QString chosen = QFileDialog::getExistingDirectory(
                this, editorText( "Choose the game data folder (HEROES2.AGG)" ),
                force ? QDir::homePath() : defaultSaveDir() );
            if ( !chosen.isEmpty() ) {
                auto newAssets = std::make_unique<Assets>( Assets::load( chosen.toStdString() ) );
                if ( newAssets->valid() ) {
                    // First repoint the widgets to the NEW object, and only
                    // then destroy the old one (through _assets).
                    _central->setAssets( newAssets.get() );
                    _central->heroPanel()->setAssets( newAssets.get() );
                    _assets = std::move( newAssets );
                    _dataDir = chosen;
                    loadGameTranslationFromDataDir( chosen.toStdString() );
                    return true;
                }
                showError( editorText( "The chosen folder has no HEROES2.AGG" ) );
            }
        }
    }
    catch ( const std::exception & e ) {
        fprintf( stderr, "pickGameDataDir failed: %s\n", e.what() );
        showError( editorText( "Failed to load game resources" ) );
    }
    return false;
}

void MainWindow::pickDataDir()
{
    // The native modal dialog is opened deferred (not inside the button click
    // handler) — otherwise Qt/macOS may crash in a nested event loop.
    QTimer::singleShot( 0, this, [this]() {
        if ( pickGameDataDir( true ) ) {
            if ( _saveFile )
                fillHeroes();
            _central->setInfoText( editorText( "Game resources loaded from %1" ).arg( QDir( _dataDir ).dirName() ) );
        }
    } );
}

void MainWindow::openDialog()
{
    QString path = QFileDialog::getOpenFileName(
        this, editorText( "Open fheroes2 save" ), defaultSaveDir(),
        editorText( "fheroes2 saves (*.sav *.savc *.savm *.savh);;All files (*)" ) );
    if ( path.isEmpty() )
        return;
    open( path );
}

void MainWindow::open( const QString & path )
{
    if ( !confirmDiscard() )
        return;
    try {
        _saveFile = std::make_unique<SaveFile>( SaveFile::load( path.toStdString() ) );
    }
    catch ( const SaveError & e ) {
        showError( QString::fromStdString( e.what() ) );
        return;
    }
    _dirty = false;
    _path = path;

    setWindowTitle( tr( "fheroes2 Save Editor" ) + QStringLiteral( " — " ) + QFileInfo( path ).fileName() );

    const MapInfo & info = _saveFile->mapInfo();
    QString mapName = decodeCp1251( info.name );
    if ( mapName.isEmpty() )
        mapName = QFileInfo( path ).fileName();
    _central->setMapText( editorText( "%1  —  month %2, week %3, day %4  (format %5)" )
                              .arg( mapName )
                              .arg( info.worldMonth )
                              .arg( info.worldWeek )
                              .arg( info.worldDay )
                              .arg( _saveFile->formatVersion() ) );

    // The info line under the map name: file name, map size, difficulty and
    // the save time (24h). Warnings ("Saved...", "fheroes2 is running...")
    // temporarily replace it.
    QString difficultyName;
    switch ( info.difficulty ) {
    case 0: difficultyName = gameText( "difficulty|Easy" ); break;
    case 1: difficultyName = gameText( "difficulty|Normal" ); break;
    case 2: difficultyName = gameText( "difficulty|Hard" ); break;
    case 3: difficultyName = gameText( "difficulty|Expert" ); break;
    case 4: difficultyName = gameText( "difficulty|Impossible" ); break;
    default: break;
    }
    QString infoLine = QFileInfo( path ).fileName();
    infoLine += QStringLiteral( " · %1x%2" ).arg( info.width ).arg( info.height );
    if ( !difficultyName.isEmpty() )
        infoLine += QStringLiteral( " · " ) + difficultyName;
    if ( info.timestamp > 0 )
        infoLine += QStringLiteral( " · " ) + QDateTime::fromSecsSinceEpoch( static_cast<qint64>( info.timestamp ) ).toString( QStringLiteral( "HH:mm" ) );
    _central->setInfoText( infoLine );

    fillHeroes();
    // FH2_DEBUG_HERO=<name> — show the given hero right after opening (debug snapshots).
    const QByteArray debugHero = qgetenv( "FH2_DEBUG_HERO" );
    if ( !debugHero.isEmpty() ) {
        const QString name = QString::fromUtf8( debugHero );
        for ( int i = 0; i < static_cast<int>( _shownHeroes.size() ); ++i ) {
            if ( QString::fromStdString( _shownHeroes[i]->name ) == name ) {
                showHero( i );
                break;
            }
        }
    }
    updateActions();
    // Debug snapshots (FH2_DEBUG_SHOT) show the clean header with the save
    // info — skip the transient "fheroes2 is running" warning.
    if ( qgetenv( "FH2_DEBUG_SHOT" ).isEmpty() )
        warnIfGameRunning();
}

void MainWindow::fillHeroes()
{
    _shownHeroes.clear();
    _currentHero = -1;
    if ( !_saveFile ) {
        _central->heroPanel()->clearHero();
        return;
    }
    const std::vector<int> human = _saveFile->humanColors();
    if ( !human.empty() ) {
        for ( int c : human ) {
            for ( const HeroRecord * h : _saveFile->heroesByColor( c ) )
                _shownHeroes.push_back( h );
        }
    }
    else {
        for ( const HeroRecord & h : _saveFile->heroes() ) {
            if ( h.color != 0 )
                _shownHeroes.push_back( &h );
        }
    }
    if ( _shownHeroes.empty() ) {
        showGameMessage( this, *_assets, gameText( "Heroes" ), editorText( "This save has no heroes with an assigned color." ) );
        _central->heroPanel()->clearHero();
        _central->heroPanel()->setNavigationEnabled( false );
        return;
    }
    _central->heroPanel()->setNavigationEnabled( _shownHeroes.size() > 1 );
    showHero( 0 );
}

void MainWindow::showHero( int index )
{
    if ( !_saveFile || index < 0 || index >= static_cast<int>( _shownHeroes.size() ) )
        return;
    _currentHero = index;
    _central->heroPanel()->setHero( &*_saveFile, const_cast<HeroRecord *>( _shownHeroes[index] ) );
}

void MainWindow::selectPrevHero()
{
    if ( _shownHeroes.size() < 2 )
        return;
    const int n = static_cast<int>( _shownHeroes.size() );
    showHero( ( _currentHero - 1 + n ) % n );
}

void MainWindow::selectNextHero()
{
    if ( _shownHeroes.size() < 2 )
        return;
    const int n = static_cast<int>( _shownHeroes.size() );
    showHero( ( _currentHero + 1 ) % n );
}

void MainWindow::markDirty()
{
    _dirty = true;
    updateActions();
}

void MainWindow::updateActions()
{
    _central->setOpenEnabled( _saveFile != nullptr );
}

bool MainWindow::confirmDiscard()
{
    if ( !_dirty )
        return true;
    if ( !_assets )
        return true;
    return showGameMessage( this, *_assets, gameText( "Unsaved changes" ),
                            editorText( "There are unsaved changes. Open another file without saving?" ),
                            MSG_YES_NO )
           == 1;
}

void MainWindow::warnIfGameRunning()
{
#ifdef Q_OS_MACOS
    QProcess proc;
    proc.start( "pgrep", { "-x", "fheroes2" } );
    proc.waitForFinished( 5000 );
    if ( !proc.readAllStandardOutput().trimmed().isEmpty() ) {
        _central->setInfoText( editorText( "Warning: fheroes2 is running — AUTOSAVE may overwrite your changes!" ) );
    }
#else
    Q_UNUSED( 0 )
#endif
}

void MainWindow::saveFile()
{
    if ( !_saveFile )
        return;
    const QString path = QString::fromStdString( _saveFile->path() );
    const QString backup = path + ".bak";
    if ( !QFileInfo::exists( backup ) )
        QFile::copy( path, backup );
    try {
        _saveFile->save();
    }
    catch ( const SaveError & e ) {
        showError( QString::fromStdString( e.what() ) );
        return;
    }
    _dirty = false;
    updateActions();
    _central->setInfoText( editorText( "Saved: %1 (backup: .bak)" ).arg( QFileInfo( path ).fileName() ) );
}

void MainWindow::showError( const QString & message )
{
    if ( _assets ) {
        showGameMessage( this, *_assets, gameText( "Error" ), message );
    }
    else {
        // Without graphics — a system window.
        QMessageBox::critical( this, gameText( "Error" ), message );
    }
}

bool MainWindow::openDebugDialog( const QString & name )
{
    // UI debugging only: FH2_DEBUG_DIALOG=<name> + FH2_DEBUG_SHOT=<file.png>
    // opens the dialog so that it can be captured with a screenshot.
    if ( !_assets )
        return false;

    if ( name == QLatin1String( "book" ) ) {
        // Use the current hero's real spell book when a save is open.
        std::vector<int> spells = { 6, 8, 10, 15, 21, 31, 42, 53 };
        if ( _saveFile && _currentHero >= 0 && _currentHero < static_cast<int>( _shownHeroes.size() ) )
            spells = _shownHeroes[_currentHero]->spells;
        spellBookDialog( this, *_assets, spells );
        return true;
    }
    if ( name == QLatin1String( "monster" ) ) {
        selectMonsterDialog( this, *_assets, 38 );
        return true;
    }
    if ( name == QLatin1String( "artifact" ) ) {
        selectArtifactDialog( this, *_assets, 1, {} );
        return true;
    }
    if ( name == QLatin1String( "hero" ) ) {
        selectHeroDialog( this, *_assets, 1 );
        return true;
    }
    if ( name == QLatin1String( "spell" ) ) {
        selectSpellDialog( this, *_assets, {} );
        return true;
    }
    if ( name == QLatin1String( "skill" ) ) {
        int skillId = 1;
        int level = 1;
        selectSecondarySkillDialog( this, *_assets, skillId, level );
        return true;
    }
    if ( name == QLatin1String( "count" ) ) {
        int value = 100;
        selectCountDialog( this, *_assets, QStringLiteral( "Set Monster Count:" ), 1, 500000, value, 1 );
        return true;
    }
    if ( name == QLatin1String( "message" ) ) {
        showGameMessage( this, *_assets, QStringLiteral( "Title" ), QStringLiteral( "Message body text." ), MSG_OK_CANCEL );
        return true;
    }
    if ( name == QLatin1String( "inputname" ) ) {
        inputStringDialog( this, *_assets, gameText( "Enter hero's name:" ), QString(), QStringLiteral( "Harokin" ), 64 );
        return true;
    }
    if ( name == QLatin1String( "numpad" ) ) {
        int value = 123;
        openNumpad( this, *_assets, value, 1, 500000 );
        return true;
    }
    if ( name == QLatin1String( "armyinfo" ) ) {
        bool dismissed = false;
        // Swordsman (id=6) — as in the reference original-screens/info-unit-added.png.
        showArmyInfoDialog( this, *_assets, 6, 42, dismissed );
        return true;
    }
    if ( name == QLatin1String( "spellinfo" ) ) {
        GameMessageElement element;
        element.kind = GameMessageElement::SPELL;
        element.id = 15; // mass blessing (double icon)
        showGameMessage( this, *_assets, QString::fromStdString( spellName( 15 ) ),
                         QString::fromStdString( spellDescription( 15 ) ), MSG_OK, element );
        return true;
    }
    if ( name == QLatin1String( "skillinfo" ) ) {
        GameMessageElement element;
        element.kind = GameMessageElement::SECONDARY_SKILL;
        element.id = 8;
        element.level = 3;
        showGameMessage( this, *_assets, QString::fromStdString( skillNameWithLevel( 8, 3 ) ),
                         QString::fromStdString( skillDescription( 8, 3 ) ), MSG_OK, element );
        return true;
    }
    if ( name == QLatin1String( "artifactinfo" ) ) {
        GameMessageElement element;
        element.kind = GameMessageElement::ARTIFACT;
        element.id = 22;
        showGameMessage( this, *_assets, QString::fromStdString( artifactName( 22 ) ),
                         QString::fromStdString( artifactDescription( 22 ) ), MSG_OK, element );
        return true;
    }
    if ( name == QLatin1String( "primcount" ) ) {
        int value = 5;
        SelectCountElement element;
        element.kind = SelectCountElement::PRIMARY_SKILL;
        element.id = 2;
        selectCountDialog( this, *_assets, replaceName( gameText( "Set %{skill} Skill:" ), "%{skill}", QString::fromStdString( primarySkillName( 2 ) ) ), 1, 99, value, 1,
                           element );
        return true;
    }
    if ( name == QLatin1String( "expcount" ) ) {
        int value = 1000;
        SelectCountElement element;
        element.kind = SelectCountElement::EXPERIENCE;
        selectCountDialog( this, *_assets, QStringLiteral( "Set Experience value:" ), 0, 2990600, value, 1, element );
        return true;
    }
    if ( name == QLatin1String( "monstercount" ) ) {
        int value = 12;
        SelectCountElement element;
        element.kind = SelectCountElement::MONSTER;
        element.id = 9;
        selectCountDialog( this, *_assets, replaceName( gameText( "Set %{monster} Count:" ), "%{monster}", QString::fromStdString( monsterName( 9 ) ) ), 1, 500000, value,
                           1, element );
        return true;
    }
    if ( name == QLatin1String( "ghostcount" ) ) {
        int value = 9000;
        SelectCountElement element;
        element.kind = SelectCountElement::MONSTER;
        element.id = 60; // Ghost
        selectCountDialog( this, *_assets, replaceName( gameText( "Set %{monster} Count:" ), "%{monster}", QString::fromStdString( monsterName( 60 ) ) ), 1, 500000,
                           value, 1, element );
        return true;
    }
    return false;
}

void MainWindow::showEvent( QShowEvent * event )
{
    QMainWindow::showEvent( event );
    // Keep the keyboard focus on the window (game widgets do not take focus),
    // so key events — including the cheat code — are always delivered.
    setFocus( Qt::OtherFocusReason );
}

void MainWindow::keyPressEvent( QKeyEvent * event )
{
    // Classic cheat code (adds 5 Black Dragons). The application-level event
    // filter fires TWICE per event on this Qt/macOS combination, so the code
    // lives here: keyPressEvent receives each key exactly once. Modal dialogs
    // have their own focus, so the cheat is naturally ignored while one is open.
    const QString text = event->text();
    if ( !text.isEmpty() && text.at( 0 ).isDigit() ) {
        _cheatBuf += text.at( 0 );
        if ( _cheatBuf.size() > 5 )
            _cheatBuf = _cheatBuf.right( 5 );
        if ( _cheatBuf == QLatin1String( "32167" ) ) {
            _cheatBuf.clear();
            applyCheatCode();
        }
    }
    QMainWindow::keyPressEvent( event );
}

void MainWindow::closeEvent( QCloseEvent * event )
{
    event->accept();
}

MainWindow::~MainWindow() = default;

void MainWindow::applyCheatCode()
{
    if ( !_saveFile || _currentHero < 0 || _currentHero >= static_cast<int>( _shownHeroes.size() ) )
        return;
    HeroRecord * hero = const_cast<HeroRecord *>( _shownHeroes[_currentHero] );
    const int slot = _saveFile->addBlackDragons( *hero );
    showHero( _currentHero );
    _central->heroPanel()->setStatusMessage(
        editorText( "Cheat activated: +5 Black Dragons (slot %1)." ).arg( slot + 1 ) );
    markDirty();
}

} // namespace fh2
