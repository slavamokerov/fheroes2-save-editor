#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace fh2 {

struct MonsterInfo {
    std::string name; // English msgid, localized via the game translation (.mo)
    int hp = 0;
};

// Monster::MonsterType (monster.h)
constexpr int MONSTER_UNKNOWN = 0;
constexpr int MONSTER_BLACK_DRAGON = 38;
constexpr int MONSTER_COUNT = 72; // id 0..71

const std::map<int, MonsterInfo> & monsters();
std::string monsterName( int id );
int monsterHp( int id );
std::vector<int> editableMonsters(); // 1..66

// Heroes enum (heroes.h)
constexpr int HEROES_COUNT = 73;
const std::map<int, std::string> & heroDefaultNames();

struct PlayerColor {
    int mask = 0;
    std::string name; // English msgid
};

const std::map<int, PlayerColor> & playerColors();

// Secondary skills 1..14 (Secondary enum from skill.h).
struct SkillInfo {
    std::string name; // English msgid
};
const std::map<int, SkillInfo> & skills();
std::string skillName( int id );
// Secondary skill level: 1..3 → Basic/Advanced/Expert (engine msgids "skill|Basic" etc.).
std::string skillLevelName( int level );
// Skill name with level ("Advanced Pathfinding"), like Skill::Secondary::GetName in fheroes2.
std::string skillNameWithLevel( int id, int level );
// Hero class (Race, bit mask).
std::string raceName( int race );

inline constexpr int CONTROL_HUMAN = 1;
inline constexpr int CONTROL_AI = 4;

// UI pixel-art scale. Integer multipliers only: nearest-neighbor gives a
// crisp picture; with fractional ones the pixel art "swims" (see README).
inline constexpr double SPRITE_SCALE = 1.0;       // sprites at natural scale (as in the game)
inline constexpr double LIST_SPRITE_SCALE = 1.0;  // portraits in lists

// Maximum sprite sizes from AGG (measured on HEROES2.AGG):
// MONH0000..0065.ICN — up to 76×93 (dragons), PORT0000..0070.ICN — 101×93.
inline constexpr int MONSTER_MAX_W = 76;
inline constexpr int MONSTER_MAX_H = 93;
inline constexpr int PORTRAIT_W = 101;
inline constexpr int PORTRAIT_H = 93;

// === Hero Screen 1:1, see FH2_SAVE_FORMAT.md §15 ===
// Base is 640×480; the editor uses natural scale (as in the game).
inline constexpr double SCREEN_SCALE = 1.0;
inline constexpr int SCREEN_W = 640;
inline constexpr int SCREEN_H = 480;

inline int screenX( int x ) { return static_cast<int>( x * SCREEN_SCALE ); }
inline int screenS( int s ) { return static_cast<int>( s * SCREEN_SCALE ); }

// Army cell in the hero window (ArmyBar, 5×1): 82×93 frame, 88 step.
inline constexpr int ARMY_CELL_W = 82;
inline constexpr int ARMY_CELL_H = 93;
inline constexpr int ARMY_CELL_STEP = 88; // 82 + 6 gap

// Artifact cell (ArtifactsBar): the ARTIFACT.ICN[0] sprite is 64×64, 79 step (64 + 15 gap).
inline constexpr int ARTIFACT_CELL_W = 64;
inline constexpr int ARTIFACT_CELL_H = 64;
inline constexpr int ARTIFACT_CELL_STEP = 79;

// ICN names for the UI (all present in HEROES2.AGG).
inline constexpr const char * ICN_HEROBKG = "HEROBKG.ICN";
inline constexpr const char * ICN_HEROEXTG = "HEROEXTG.ICN";
inline constexpr const char * ICN_HSBTNS = "HSBTNS.ICN";
inline constexpr const char * ICN_RECRUIT = "RECRUIT.ICN";
inline constexpr const char * ICN_STRIP = "STRIP.ICN";
inline constexpr const char * ICN_MINISS = "MINISS.ICN";
inline constexpr const char * ICN_CREST = "CREST.ICN";
inline constexpr const char * ICN_HSICONS = "HSICONS.ICN";
inline constexpr const char * ICN_PRIMSKIL = "PRIMSKIL.ICN";
inline constexpr const char * ICN_SECSKILL = "SECSKILL.ICN";
inline constexpr const char * ICN_ARTFX = "ARTFX.ICN";
inline constexpr const char * ICN_MONS32 = "MONS32.ICN";
inline constexpr const char * ICN_MINIPORT = "MINIPORT.ICN";
inline constexpr const char * ICN_FONT = "FONT.ICN";
inline constexpr const char * ICN_SMALFONT = "SMALFONT.ICN";
inline constexpr const char * ICN_STONEBAK = "STONEBAK.ICN";
inline constexpr const char * ICN_SURDRBKG = "SURDRBKG.ICN";
inline constexpr const char * ICN_WINLOSE = "WINLOSE.ICN";
inline constexpr const char * ICN_SCROLL = "SCROLL.ICN";
inline constexpr const char * ICN_CPANEL = "CPANEL.ICN";
inline constexpr const char * ICN_SPANBTN = "SPANBTN.ICN";
inline constexpr const char * ICN_ARTIFACT = "ARTIFACT.ICN";
inline constexpr const char * ICN_SPELLS = "SPELLS.ICN";
inline constexpr const char * ICN_ADVBORD = "ADVBORD.ICN";

// TIL tiles. CLOF32 is a fully fogged (undiscovered) map tile:
// 4 variants of 32×32 "starry sky", the engine picks (x + y) % 4
// (drawFog, maps_tiles_render.cpp:709).
inline constexpr const char * TIL_CLOF32 = "CLOF32.TIL";
inline constexpr int FOG_TILE_SIZE = 32;
inline constexpr int FOG_TILE_VARIANTS = 4;

// Hero level from experience (GetExperienceFromLevel table from fheroes2 heroes.cpp).
int heroLevel( uint32_t experience );

// Army cell frame: ICN STRIP, index by hero race (renderMonsterFrame).
int raceStripIndex( int race );

// Player crest: ICN CREST, index by color mask (Color::GetIndex).
int crestIndexForColor( int colorMask );

// Artifacts 1..82 (id, name) — icons ARTFX[id-1] (32×32) and ARTIFACT.ICN[id] (large).
struct ArtifactInfo {
    std::string name; // English msgid
};
const std::map<int, ArtifactInfo> & artifacts();
std::string artifactName( int id );

// Primary skills (display order: attack, defense, power, knowledge).
std::string primarySkillName( int index );

// Hero's starting primary skills by race (heroInitialPrimarySkills from the
// engine's game_static.cpp, order: attack, defense, power, knowledge) — for
// "reset to defaults" (right-click on a skill, as in the map editor).
int primarySkillDefault( int race, int index );

// Starting secondary skills by race (heroInitialSecondarySkills):
// a list of (skillId, level). Empty means the race has no starting skills.
std::vector<std::pair<int, int>> secondarySkillDefaults( int race );

// Spells 1..65 (Spell enum from fheroes2). Icon is SPELLS.ICN[iconIndex].
struct SpellInfo {
    std::string name; // English msgid
    int level = 0;     // 1..5
    int iconIndex = 0; // index in SPELLS.ICN (Spell::IndexSprite)
};
const std::map<int, SpellInfo> & spells();
std::string spellName( int id );
int spellLevel( int id );
int spellIconIndex( int id );

// Artifact id of the Magic Book.
inline constexpr int ARTIFACT_MAGIC_BOOK = 82;
// Maximum spells in a hero's book (all known ones, 1..65).
inline constexpr int SPELL_COUNT = 65;

} // namespace fh2
