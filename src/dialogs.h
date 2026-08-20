#pragma once

#include <QString>
#include <QWidget>
#include <vector>

namespace fh2 {

class Assets;

// Message buttons (like Dialog:: in fheroes2).
enum : int {
    MSG_ZERO = 0,
    MSG_OK = 1,
    MSG_YES_NO = 2,
    MSG_OK_CANCEL = 3,
};

// Message element (port of fheroes2::DialogElement from ui_dialog.h):
// an «icon + label» above the text, like in the engine's showStandardTextMessage.
struct GameMessageElement {
    enum Kind {
        NONE = 0,
        ICON_TEXT,         // ICN icon + label (spell: SPELLS[id] + name)
        SECONDARY_SKILL,   // SECSKILL[15] + skill icon + name + level
        ARTIFACT,          // RESOURCE[7] + ARTIFACT[id]
        PRIMARY_SKILL,     // PRIMSKIL[4] + PRIMSKIL[id] icon + skill name
        EXPERIENCE,        // EXPMRL[4]
        SPELL,             // spell icon centered + its name below (SpellDialogElement)
    };

    Kind kind = NONE;
    QString iconIcn; // for ICON_TEXT
    int iconIndex = 0;
    int id = 0;   // skill/artifact/spell id / primary skill index
    int level = 0; // secondary skill level (for the label in the element)
    QString text; // label for ICON_TEXT
};

// --- Message (port of showStandardTextMessage + DialogElement) ---
// Title and text wrap at width 260 (boxAreaWidthPx), lines are centered;
// the window height is computed from the number of lines (Text::height).
int showGameMessage( QWidget * parent, const Assets & assets, const QString & title, const QString & body, int buttons = MSG_OK,
                     const GameMessageElement & element = {} );

// Element in the top part of the count dialog (port of Dialog::SelectCount
// with topUiElement, dialog_selectcount.cpp): skill/experience/monster picture.
struct SelectCountElement {
    enum Kind {
        NONE = 0,
        PRIMARY_SKILL, // PRIMSKIL[4] + PRIMSKIL[id] icon + skill name
        EXPERIENCE,    // EXPMRL[4]
        MONSTER,       // STRIP[12] + monster sprite (id — monsterId)
    };

    Kind kind = NONE;
    int id = 0;
};

// --- Count dialog (port of Dialog::SelectCount + ValueSelectionDialogElement) ---
// Returns true if accepted; value — the selected value (0 on cancel).
bool selectCountDialog( QWidget * parent, const Assets & assets, const QString & header, int min, int max, int & value, int step,
                        const SelectCountElement & element = {} );

// --- Virtual numpad (port of openVirtualNumpad) ---
bool openNumpad( QWidget * parent, const Assets & assets, int & value, int min, int max );

// --- Selection lists (port of ItemSelectionWindow and Dialog::select*) ---
// Return 0/-1 on cancel.
int selectMonsterDialog( QWidget * parent, const Assets & assets, int currentId );
int selectArtifactDialog( QWidget * parent, const Assets & assets, int currentId, const std::vector<int> & excludeIds );
int selectHeroDialog( QWidget * parent, const Assets & assets, int currentId );
int selectSpellDialog( QWidget * parent, const Assets & assets, const std::vector<int> & excludeSpells );
// Secondary skill selection (id + level 1..3); true if selected.
// excludeSkillIds — skill ids that must not appear in the list (ones already
// added to the hero; all levels of the skill are excluded in that case).
bool selectSecondarySkillDialog( QWidget * parent, const Assets & assets, int & skillId, int & level,
                                 const std::vector<int> & excludeSkillIds = {} );

// --- String input (port of Dialog::inputString) ---
QString inputStringDialog( QWidget * parent, const Assets & assets, const QString & title, const QString & body, const QString & initial, int charLimit );

// --- Troop window (port of Dialog::ArmyInfo, BUTTONS|DISMISS mode) ---
// Animated monster, stats, DISMISS/EXIT buttons. On «Dismiss» — a confirmation
// and dismissed = true. Buttons are VIEWARMY[1..4] sprites with baked-in text
// (the texts are English in the original resources).
void showArmyInfoDialog( QWidget * parent, const Assets & assets, int monsterId, uint32_t count, bool & dismissed );

// --- Spell book (port of SpellBook::Edit) ---
// Click on a spell — description, right click — remove, click on an empty slot — select.
// Returns true if the book changed; spells — the new list.
bool spellBookDialog( QWidget * parent, const Assets & assets, std::vector<int> & spells );

} // namespace fh2
