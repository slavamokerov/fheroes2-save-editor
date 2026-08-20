#include "gamedata.h"

#include <cstdio>
#include <cstring>

#include "constants.h"
#include "gettextmo.h"

namespace fh2 {

namespace {

struct SpellData {
    int cost;
    int extra;
    const char * desc; // English msgid (engine spell.cpp), localized via game .mo
};

const SpellData SPELLS[66] = {
    { 9, 10, "Causes a giant fireball to strike the selected area, damaging all nearby creatures." }, // 1
    { 15, 10, "An improved version of fireball, fireblast affects two hexes around the center point of the spell, rather than one." }, // 2
    { 7, 25, "Causes a bolt of electrical energy to strike the selected creature." }, // 3
    { 9, 0, "Teleports the creature you select to any open position on the battlefield." }, // 4
    { 6, 5, "Removes all negative spells cast upon one of your units, and restores up to %{count} HP per level of spell power." }, // 5
    { 15, 5, "Removes all negative spells cast upon your forces, and restores up to %{count} HP per level of spell power, per creature." }, // 6
    { 12, 50, "Resurrects creatures from a damaged or dead unit until end of combat." }, // 7
    { 15, 50, "Resurrects creatures from a damaged or dead unit permanently." }, // 8
    { 3, 2, "Increases the speed of any creature by %{count}." }, // 9
    { 10, 2, "Increases the speed of all of your creatures by %{count}." }, // 10
    { 3, 0, "Slows target to half movement rate." }, // 11
    { 15, 0, "Slows all enemies to half movement rate." }, // 12
    { 6, 50, "Clouds the affected creatures' eyes, preventing them from moving and reduces the damage when retaliating by %{count} percent." }, // 13
    { 3, 0, "Causes the selected creatures to inflict maximum damage." }, // 14
    { 12, 0, "Causes all of your units to inflict maximum damage." }, // 15
    { 3, 3, "Magically increases the defense skill of the selected creatures." }, // 16
    { 6, 5, "Increases the defense skill of the targeted creatures. This is an improved version of Stoneskin." }, // 17
    { 3, 0, "Causes the selected creatures to inflict minimum damage." }, // 18
    { 12, 0, "Causes all enemy troops to inflict minimum damage." }, // 19
    { 9, 10, "Damages all undead in the battle." }, // 20
    { 12, 20, "Damages all undead in the battle. This is an improved version of Holy Word." }, // 21
    { 7, 0, "Prevents any magic against the selected creatures." }, // 22
    { 5, 0, "Removes all magic spells from a single target." }, // 23
    { 12, 0, "Removes all magic spells from all creatures." }, // 24
    { 3, 10, "Causes a magic arrow to strike the selected target." }, // 25
    { 12, 0, "Causes a creature to attack its nearest neighbor." }, // 26
    { 20, 50, "Holy terror strikes the battlefield, causing severe damage to all creatures." }, // 27
    { 15, 25, "Magical elements pour down on the battlefield, damaging all creatures." }, // 28
    { 15, 25, "A rain of rocks strikes an area of the battlefield, damaging all nearby creatures." }, // 29
    { 9, 0, "The targeted creatures are paralyzed, unable to move or retaliate." }, // 30
    { 15, 25, "Brings a single enemy unit under your control if its hits are less than %{count} times the caster's spell power." }, // 31
    { 6, 20, "Drains body heat from a single enemy unit." }, // 32
    { 9, 10, "Drains body heat from all units surrounding the center point, but not including the center point." }, // 33
    { 7, 3, "Reduces the defense rating of an enemy unit by three." }, // 34
    { 6, 5, "Damages all living (non-undead) units in the battle." }, // 35
    { 10, 10, "Damages all living (non-undead) units in the battle. This spell is an improved version of Death Ripple." }, // 36
    { 6, 5, "Greatly increases a unit's attack skill vs. Dragons." }, // 37
    { 3, 3, "Increases a unit's attack skill." }, // 38
    { 10, 50, "Resurrects creatures from a damaged or dead undead unit permanently." }, // 39
    { 3, 2, "Halves damage received from ranged attacks for a single unit. Does not affect damage received from Turrets or Ballistae." }, // 40
    { 7, 0, "Halves damage received from ranged attacks for all of your units. Does not affect damage received from Turrets or Ballistae." }, // 41
    { 30, 3, "Summons Earth Elementals to fight for your army." }, // 42
    { 30, 3, "Summons Air Elementals to fight for your army." }, // 43
    { 30, 3, "Summons Fire Elementals to fight for your army." }, // 44
    { 30, 3, "Summons Water Elementals to fight for your army." }, // 45
    { 15, 0, "Damages castle walls." }, // 46
    { 1, 0, "Causes all mines across the land to become visible." }, // 47
    { 1, 0, "Causes all resources across the land to become visible." }, // 48
    { 2, 0, "Causes all artifacts across the land to become visible." }, // 49
    { 2, 0, "Causes all towns and castles across the land to become visible." }, // 50
    { 2, 0, "Causes all Heroes across the land to become visible." }, // 51
    { 3, 0, "Causes the entire land to become visible." }, // 52
    { 3, 0, "Allows the caster to view detailed information on enemy Heroes." }, // 53
    { 10, 0, "Allows the caster to magically transport to a nearby location." }, // 54
    { 10, 0, "Returns the caster to the nearest town or castle currently owned. This spell cannot be cast if the hero is already in a town or a castle." }, // 55
    { 20, 0, "Returns the hero to the town or castle of choice, provided it is controlled by you." }, // 56
    { 6, 3, "Visions predicts the likely outcome of an encounter with a neutral army camp." }, // 57
    { 8, 4, "Haunts a mine you control with Ghosts. This mine stops producing resources. (If I can't keep it, nobody will!)" }, // 58
    { 15, 4, "Sets Earth Elementals to guard a mine against enemy armies." }, // 59
    { 15, 4, "Sets Air Elementals to guard a mine against enemy armies." }, // 60
    { 15, 4, "Sets Fire Elementals to guard a mine against enemy armies." }, // 61
    { 15, 4, "Sets Water Elementals to guard a mine against enemy armies." }, // 62
    { 0, 0, "Randomly selected spell of any level." }, // 63
    { 0, 0, "Randomly selected 1st level spell." }, // 64
    { 0, 0, "Randomly selected 2nd level spell." }, // 65
};

const MonsterStats MONSTERS[72] = {
    { 0, 0, 0, 0, 0, 2, 0 },
    { 1, 1, 1, 1, 1, 2, 0 },
    { 5, 3, 2, 3, 10, 2, 12 },
    { 5, 3, 2, 3, 10, 4, 24 },
    { 5, 9, 3, 4, 15, 4, 0 },
    { 5, 9, 3, 4, 20, 5, 0 },
    { 7, 9, 4, 6, 25, 4, 0 },
    { 7, 9, 4, 6, 30, 5, 0 },
    { 10, 9, 5, 10, 30, 6, 0 },
    { 10, 9, 5, 10, 40, 7, 0 },
    { 11, 12, 10, 20, 50, 5, 0 },
    { 11, 12, 10, 20, 65, 6, 0 },
    { 3, 1, 1, 2, 3, 4, 0 },
    { 3, 4, 2, 3, 10, 2, 8 },
    { 3, 4, 3, 4, 15, 3, 16 },
    { 6, 2, 3, 5, 20, 6, 0 },
    { 9, 5, 4, 6, 40, 2, 0 },
    { 9, 5, 5, 7, 60, 4, 0 },
    { 10, 5, 5, 7, 40, 4, 8 },
    { 10, 5, 7, 9, 40, 5, 16 },
    { 12, 9, 12, 24, 80, 5, 0 },
    { 4, 2, 1, 2, 2, 4, 0 },
    { 6, 5, 2, 4, 20, 2, 0 },
    { 6, 6, 2, 4, 20, 4, 0 },
    { 4, 3, 2, 3, 15, 4, 24 },
    { 5, 5, 2, 3, 15, 6, 24 },
    { 7, 5, 5, 8, 25, 5, 8 },
    { 7, 7, 5, 8, 25, 6, 16 },
    { 10, 9, 7, 14, 40, 5, 0 },
    { 12, 10, 20, 40, 100, 7, 0 },
    { 3, 1, 1, 2, 5, 4, 8 },
    { 4, 7, 2, 3, 15, 6, 0 },
    { 6, 6, 3, 5, 25, 4, 0 },
    { 9, 8, 5, 10, 35, 4, 0 },
    { 9, 8, 5, 10, 45, 6, 0 },
    { 8, 9, 6, 12, 75, 2, 0 },
    { 12, 12, 25, 50, 200, 4, 0 },
    { 13, 13, 25, 50, 250, 5, 0 },
    { 14, 14, 25, 50, 300, 6, 0 },
    { 2, 1, 1, 3, 3, 3, 12 },
    { 5, 4, 2, 3, 15, 6, 0 },
    { 5, 10, 4, 5, 30, 2, 0 },
    { 7, 10, 4, 5, 35, 3, 0 },
    { 7, 7, 4, 8, 40, 4, 0 },
    { 11, 7, 7, 9, 30, 5, 12 },
    { 12, 8, 7, 9, 35, 6, 24 },
    { 13, 10, 20, 30, 150, 4, 0 },
    { 15, 15, 20, 30, 300, 6, 24 },
    { 4, 3, 2, 3, 4, 4, 0 },
    { 5, 2, 2, 3, 15, 2, 0 },
    { 5, 2, 2, 3, 20, 4, 0 },
    { 6, 6, 3, 4, 25, 4, 0 },
    { 6, 6, 3, 4, 30, 5, 0 },
    { 8, 6, 5, 7, 30, 4, 0 },
    { 8, 6, 5, 7, 40, 5, 0 },
    { 7, 12, 8, 10, 25, 5, 12 },
    { 7, 13, 8, 10, 35, 6, 24 },
    { 11, 9, 25, 45, 150, 4, 0 },
    { 6, 1, 1, 2, 4, 5, 0 },
    { 7, 6, 2, 5, 20, 6, 0 },
    { 8, 7, 4, 6, 20, 5, 0 },
    { 10, 9, 20, 30, 50, 6, 0 },
    { 8, 9, 6, 10, 35, 4, 0 },
    { 8, 8, 4, 5, 50, 3, 0 },
    { 7, 7, 2, 8, 35, 6, 0 },
    { 8, 6, 4, 6, 40, 5, 0 },
    { 6, 8, 3, 7, 45, 4, 0 },
    { 0, 0, 0, 0, 0, 2, 0 },
    { 0, 0, 0, 0, 0, 2, 0 },
    { 0, 0, 0, 0, 0, 2, 0 },
    { 0, 0, 0, 0, 0, 2, 0 },
    { 0, 0, 0, 0, 0, 2, 0 },
};

// Speed names. "Slow"/"Fast"/"Very Fast" are engine msgids (game .mo), the
// rest are the editor's own strings (editor .po).
const char * SPEED_NAMES[10] = { nullptr, nullptr, "Very Slow", "Slow", "Average", "Fast", "Very Fast", "Ultra Fast", nullptr, nullptr };

// Skill values per level (secondarySkillValuesPerLevel, game_static.cpp).
const int SKILL_VALUE[15][3] = {
    { 0, 0, 0 },
    { 25, 50, 100 },  // 1 Pathfinding
    { 10, 25, 50 },   // 2 Archery
    { 10, 20, 30 },   // 3 Logistics
    { 1, 2, 3 },      // 4 Scouting
    { 25, 50, 100 },  // 5 Diplomacy
    { 33, 66, 100 },  // 6 Navigation
    { 1, 2, 3 },      // 7 Leadership
    { 3, 4, 5 },      // 8 Wisdom
    { 1, 2, 3 },      // 9 Mysticism
    { 1, 2, 3 },      // 10 Luck
    { 0, 0, 0 },      // 11 Ballistics
    { 20, 30, 40 },   // 12 Eagle Eye
    { 10, 20, 30 },   // 13 Necromancy
    { 100, 250, 500 } // 14 Estates
};

// Skill description sentences from skill.cpp; English msgids, localized at
// runtime via the game translation (.mo).
const char * SENT_PATH_B = "%{skill} reduces the movement penalty for rough terrain by %{count} percent.";
const char * SENT_PATH_E = "%{skill} eliminates the movement penalty for rough terrain.";
const char * SENT_ARCH = "%{skill} increases the damage done by the hero's range attacking creatures by %{count} percent, and eliminates the %{penalty} percent penalty when shooting past obstacles (e.g. castle walls).";
const char * SENT_LOGI = "%{skill} increases the hero's movement points by %{count} percent.";
const char * SENT_SCOUT1 = "%{skill} increases the hero's viewable area by one square.";
const char * SENT_DIPL = "%{skill} allows the hero to negotiate with monsters who are weaker than their army, and reduces the cost of surrender.";
const char * SENT_DIPL_BA = "Approximately %{count} percent of the creatures may offer to join the hero.";
const char * SENT_DIPL_E = "All of the creatures may offer to join the hero.";
const char * SENT_DIPL_C = "The cost of surrender is reduced to %{percent} percent of the total cost of troops in the army.";
const char * SENT_NAVI = "%{skill} increases the hero's movement points over water by %{count} percent.";
const char * SENT_LEAD = "%{skill} increases the hero's troops morale by %{count}.";
const char * SENT_WIS3 = "%{skill} allows the hero to learn third level spells.";
const char * SENT_WIS4 = "%{skill} allows the hero to learn fourth level spells.";
const char * SENT_WIS5 = "%{skill} allows the hero to learn fifth level spells.";
const char * SENT_MYST1 = "%{skill} regenerates one additional spell point per day to the hero.";
const char * SENT_LUCK = "%{skill} increases the hero's luck by %{count}.";
const char * SENT_BALL_B = "%{skill} gives the hero's catapult shots a greater chance to hit and do damage to castle walls.";
const char * SENT_BALL_A = "%{skill} gives the hero's catapult an extra shot, and each shot has a greater chance to hit and do damage to castle walls.";
const char * SENT_BALL_E = "%{skill} gives the hero's catapult an extra shot, and each shot automatically destroys any wall, except a fortified wall in a Knight castle.";
const char * SENT_EAGLE_B = "%{skill} gives the hero a %{count} percent chance to learn any given 1st or 2nd level spell that was cast by an enemy during combat.";
const char * SENT_EAGLE_A = "%{skill} gives the hero a %{count} percent chance to learn any given 3rd level spell (or below) that was cast by an enemy during combat.";
const char * SENT_EAGLE_E = "%{skill} gives the hero a %{count} percent chance to learn any given 4th level spell (or below) that was cast by an enemy during combat.";
const char * SENT_NECRO = "%{skill} allows %{count} percent of the creatures killed in combat to be brought back from the dead as Skeletons.";
const char * SENT_ESTATES = "The hero produces %{count} gold pieces per day as tax revenue from estates.";

} // namespace

const MonsterStats * monsterStats( int monsterId )
{
    if ( monsterId < 1 || monsterId >= 72 )
        return nullptr;
    return &MONSTERS[monsterId];
}

std::string speedName( int speed )
{
    if ( speed < 2 || speed > 7 )
        return "?";
    return trGameOrEditor( SPEED_NAMES[speed] );
}

int spellCost( int spellId )
{
    if ( spellId < 1 || spellId > 65 )
        return 0;
    return SPELLS[spellId].cost;
}

int spellExtraValue( int spellId )
{
    if ( spellId < 1 || spellId > 65 )
        return 0;
    return SPELLS[spellId].extra;
}

std::string spellDescription( int spellId )
{
    if ( spellId < 1 || spellId > 65 )
        return {};
    const SpellData & s = SPELLS[spellId];
    std::string out = trGame( s.desc );
    char buf[16];
    snprintf( buf, sizeof( buf ), "%d", s.extra );
    size_t pos = 0;
    while ( ( pos = out.find( "%{count}" ) ) != std::string::npos )
        out.replace( pos, 8, buf );
    return out;
}

std::string skillDescription( int skillId, int level )
{
    if ( skillId < 1 || skillId > 14 || level < 1 || level > 3 )
        return {};

    const int count = SKILL_VALUE[skillId][level - 1];
    const int castleWallPenalty = 50;
    const int diplomacyDiscount[3] = { 40, 30, 20 };

    const auto fmt = [&]( const char * en, int cnt, int penalty, int percent ) {
        std::string s = trGame( en );
        // In fheroes2 %{skill} is substituted with the SKILL NAME WITH LEVEL
        // (GetDescription → name = GetName()).
        const std::string skill = skillNameWithLevel( skillId, level );
        const std::string cs = std::to_string( cnt );
        const std::string ps = std::to_string( penalty );
        const std::string pc = std::to_string( percent );
        for ( const auto & rep : { std::pair<const char *, const std::string *>{ "%{skill}", &skill }, { "%{count}", &cs },
                                   { "%{penalty}", &ps }, { "%{percent}", &pc } } ) {
            size_t pos = 0;
            while ( ( pos = s.find( rep.first ) ) != std::string::npos )
                s.replace( pos, strlen( rep.first ), *rep.second );
        }
        return s;
    };
    const auto sent = [&]( const char * en ) {
        return fmt( en, count, castleWallPenalty, diplomacyDiscount[level - 1] );
    };
    // Plural sentences (_n in the engine): the lookup key is the SINGULAR
    // msgid (that is how _n pairs are stored in the .mo), the plural form is
    // picked by the locale rules of the active language.
    const auto sentPlural = [&]( const char * singularMsgid, size_t n ) {
        std::string s = trGamePlural( singularMsgid, n );
        const std::string skill = skillNameWithLevel( skillId, level );
        const std::string cs = std::to_string( count );
        for ( const auto & rep : { std::pair<const char *, const std::string *>{ "%{skill}", &skill }, { "%{count}", &cs } } ) {
            size_t pos = 0;
            while ( ( pos = s.find( rep.first ) ) != std::string::npos )
                s.replace( pos, strlen( rep.first ), *rep.second );
        }
        return s;
    };

    std::string out;
    switch ( skillId ) {
    case 1:
        out = ( level == 3 ) ? sent( SENT_PATH_E ) : sent( SENT_PATH_B );
        break;
    case 2:
        out = sent( SENT_ARCH );
        break;
    case 3:
        out = sent( SENT_LOGI );
        break;
    case 4:
        out = sentPlural( SENT_SCOUT1, static_cast<size_t>( count ) );
        break;
    case 5: {
        out = sent( SENT_DIPL );
        out += "\n\n";
        out += ( level == 3 ) ? sent( SENT_DIPL_E ) : sent( SENT_DIPL_BA );
        out += "\n\n";
        out += sent( SENT_DIPL_C );
        break;
    }
    case 6:
        out = sent( SENT_NAVI );
        break;
    case 7:
        out = sent( SENT_LEAD );
        break;
    case 8:
        out = sent( level == 1 ? SENT_WIS3 : ( level == 2 ? SENT_WIS4 : SENT_WIS5 ) );
        break;
    case 9:
        out = sentPlural( SENT_MYST1, static_cast<size_t>( count ) );
        break;
    case 10:
        out = sent( SENT_LUCK );
        break;
    case 11:
        out = sent( level == 1 ? SENT_BALL_B : ( level == 2 ? SENT_BALL_A : SENT_BALL_E ) );
        break;
    case 12:
        out = sent( level == 1 ? SENT_EAGLE_B : ( level == 2 ? SENT_EAGLE_A : SENT_EAGLE_E ) );
        break;
    case 13:
        out = sent( SENT_NECRO );
        break;
    case 14:
        out = sent( SENT_ESTATES );
        break;
    default:
        break;
    }
    return out;
}

struct ArtifactDescriptionData {
    const char * desc; // English msgid (artifact_info.cpp) with %{name}/%{count}
    int count = 0;     // value for %{count} (0 — the msgid has no placeholder)
};

// Artifact descriptions 1..82 (Artifact::GetDescription, artifact_info.cpp);
// %{name}/%{count}/%{spell} are already substituted.
const ArtifactDescriptionData ARTIFACT_DESCRIPTIONS[83] = {
    { nullptr, 0 }, // 0
    { "The %{name} increases the hero's knowledge by %{count}.", 12 }, // 1
    { "The %{name} increases the hero's attack skill by %{count}.", 12 }, // 2
    { "The %{name} increases the hero's defense skill by %{count}.", 12 }, // 3
    { "The %{name} increases the hero's spell power by %{count}.", 12 }, // 4
    { "The %{name} increases the hero's attack and defense skills by %{count} each.", 6 }, // 5
    { "The %{name} increases the hero's spell power and knowledge by %{count} each.", 6 }, // 6
    { "The %{name} increases each of the hero's basic skills by %{count} points.", 4 }, // 7
    { "The %{name} brings in an income of %{count} gold per day.", 10000 }, // 8
    { "The %{name} increases the hero's spell power by %{count}.", 4 }, // 9
    { "The %{name} increases the hero's spell power by %{count}.", 2 }, // 10
    { "The %{name} increases the hero's spell power by %{count}.", 2 }, // 11
    { "The %{name} increases the hero's spell power by %{count}.", 3 }, // 12
    { "The %{name} increases the morale of the hero's army by %{count}.", 1 }, // 13
    { "The %{name} increases the morale of the hero's army by %{count}.", 1 }, // 14
    { "The %{name} increases the morale of the hero's army by %{count}.", 1 }, // 15
    { "The %{name} increases the morale of the hero's army by %{count}.", 1 }, // 16
    { "The %{name} greatly decreases the morale of the hero's army by %{count}.", 2 }, // 17
    { "The %{name} increases the hero's attack skill by %{count}.", 1 }, // 18
    { "The %{name} increase the hero's defense skill by %{count}.", 1 }, // 19
    { "The %{name} increases the hero's defense skill by %{count}.", 1 }, // 20
    { "The %{name} increases the hero's attack skill by %{count}.", 1 }, // 21
    { "The %{name} gives the hero's catapult one extra shot per combat round.", 0 }, // 22
    { "The %{name} increases the hero's defense skill by %{count}.", 2 }, // 23
    { "The %{name} increases the hero's attack skill by %{count}.", 3 }, // 24
    { "The %{name} increases the hero's attack skill by %{count}.", 2 }, // 25
    { "The %{name} increases the hero's defense skill by %{count}.", 3 }, // 26
    { "The %{name} increases the hero's knowledge by %{count}.", 2 }, // 27
    { "The %{name} increases the hero's knowledge by %{count}.", 3 }, // 28
    { "The %{name} increases the hero's knowledge by %{count}.", 4 }, // 29
    { "The %{name} increases the hero's knowledge by %{count}.", 5 }, // 30
    { "The %{name} provides the hero with %{count} gold per day.", 1000 }, // 31
    { "The %{name} provides the hero with %{count} gold per day.", 750 }, // 32
    { "The %{name} provides the hero with %{count} gold per day.", 500 }, // 33
    { "The %{name} increase the hero's movement on land.", 0 }, // 34
    { "The %{name} increase the hero's movement on land.", 0 }, // 35
    { "The %{name} increases the luck of the hero's army by %{count}.", 1 }, // 36
    { "The %{name} increases the luck of the hero's army by %{count}.", 1 }, // 37
    { "The %{name} increases the luck of the hero's army by %{count}.", 1 }, // 38
    { "The %{name} increases the luck of the hero's army by %{count}.", 1 }, // 39
    { "The %{name} increases the hero's movement on land and sea.", 0 }, // 40
    { "The %{name} increases the hero's movement on sea.", 0 }, // 41
    { "The %{name} reduces the casting cost of curse spells by half.", 0 }, // 42
    { "The %{name} extends the duration of all the hero's spells by %{count} turns.", 2 }, // 43
    { "The %{name} doubles the effectiveness of the hero's hypnotize spells.", 0 }, // 44
    { "The %{name} halves the casting cost of all mind influencing spells.", 0 }, // 45
    { "The %{name} halves all damage the hero's troops receive from cold spells.", 0 }, // 46
    { "The %{name} halves all damage the hero's troops receive from fire spells.", 0 }, // 47
    { "The %{name} halves all damage the hero's troops receive from lightning spells.", 0 }, // 48
    { "The %{name} causes the hero's cold spells to do %{count} percent more damage to enemy troops.", 50 }, // 49
    { "The %{name} causes the hero's fire spells to do %{count} percent more damage to enemy troops.", 50 }, // 50
    { "The %{name} causes the hero's lightning spells to do %{count} percent more damage to enemy troops.", 50 }, // 51
    { "The %{name} halves the casting cost of all of the hero's bless spells.", 0 }, // 52
    { "The %{name} doubles the effectiveness of all of the hero's resurrect and animate spells.", 0 }, // 53
    { "The %{name} doubles the effectiveness of all of the hero's summoning spells.", 0 }, // 54
    { "The %{name} halves the casting cost of all summoning spells.", 0 }, // 55
    { "The %{name} makes all of the hero's troops immune to curse spells.", 0 }, // 56
    { "The %{name} makes all of the hero's troops immune to hypnotize spells.", 0 }, // 57
    { "The %{name} makes all of the hero's troops immune to death spells.", 0 }, // 58
    { "The %{name} makes all of the hero's troops immune to berserk spells.", 0 }, // 59
    { "The %{name} makes all of the hero's troops immune to blindness spells.", 0 }, // 60
    { "The %{name} makes all of the hero's troops immune to paralyze spells.", 0 }, // 61
    { "The %{name} makes all of the hero's troops immune to holy spells.", 0 }, // 62
    { "The %{name} makes all of the hero's troops immune to dispel magic spells.", 0 }, // 63
    { "The %{name} eliminates the %{count} percent penalty for the hero's troops shooting past obstacles (e.g. castle walls).", 50 }, // 64
    { "The %{name} increases the amount of terrain the hero reveals when adventuring by %{count} extra square.", 1 }, // 65
    { "The %{name} reduces the cost of surrender to %{count} percent of the total cost of troops the hero has in their army.", 10 }, // 66
    { "The %{name} increases the duration of the hero's spells by %{count} turns.", 10 }, // 67
    { "The %{name} returns %{count} extra spell points per day to the hero.", 2 }, // 68
    { "The %{name} provides endless ammunition for all of the hero's troops that shoot.", 0 }, // 69
    { "The %{name} costs the hero %{count} gold pieces per day.", 250 }, // 70
    { "The %{name} prevents all 'wandering' armies from joining the hero.", 0 }, // 71
    { "The %{name} provides %{count} unit of sulfur per day.", 1 }, // 72
    { "The %{name} provides %{count} unit of mercury per day.", 1 }, // 73
    { "The %{name} provides %{count} unit of gems per day.", 1 }, // 74
    { "The %{name} provides %{count} unit of wood per day.", 1 }, // 75
    { "The %{name} provides %{count} unit of ore per day.", 1 }, // 76
    { "The %{name} provides %{count} unit of crystal per day.", 1 }, // 77
    { "The %{name} increases the hero's attack and defense skills by %{count} each.", 1 }, // 78
    { "The %{name} increases the hero's attack and defense skills by %{count} each.", 2 }, // 79
    { "The %{name} increases the hero's spell power and knowledge by %{count} each.", 1 }, // 80
    { "The %{name} increases the hero's spell power and knowledge by %{count} each.", 2 }, // 81
    { "The %{name} enables the hero to cast spells.", 0 }, // 82
};

std::string artifactDescription( int artifactId )
{
    if ( artifactId < 1 || artifactId > 82 )
        return {};
    const ArtifactDescriptionData & d = ARTIFACT_DESCRIPTIONS[artifactId];
    if ( d.desc == nullptr )
        return {};
    // Like ArtifactData::getDescription in artifact_info.cpp: the msgid carries
    // %{name}/%{count}, they are replaced after the translation lookup.
    std::string out = trGame( d.desc );
    const auto replaceAll = []( std::string & text, const char * placeholder, const std::string & value ) {
        size_t pos = 0;
        while ( ( pos = text.find( placeholder ) ) != std::string::npos )
            text.replace( pos, std::strlen( placeholder ), value );
    };
    replaceAll( out, "%{name}", trGame( artifactName( artifactId ) ) );
    if ( d.count > 0 )
        replaceAll( out, "%{count}", std::to_string( d.count ) );
    return out;
}

// Full-size monster animations for the troop window. Generated from
// fheroes2 (monster_info.cpp monsterIcnIds/binFileName + *.FRM.BIN from
// HEROES2.AGG): engine icnId, STATIC frames (index 7 in FRM) and IDLE1 (8).
// MONH%04d are MINI-sprites of army cells; for the troop window the engine
// uses full-size animations (SWORDSMN.ICN etc.).
static const MonsterAnimInfo MONSTER_ANIMATIONS[67] = {
    { nullptr, { 0 }, 0, { 0 }, 0 },                                  // 0
    { "PEASANT.ICN", { 1 }, 1, { 2 }, 1 },                            // 1
    { "ARCHER.ICN", { 1 }, 1, { 2, 3, 4, 3 }, 4 },                    // 2
    { "ARCHER2.ICN", { 1 }, 1, { 2, 3, 4, 3 }, 4 },                   // 3
    { "PIKEMAN.ICN", { 1 }, 1, { 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3 }, 16 }, // 4
    { "PIKEMAN2.ICN", { 1 }, 1, { 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3 }, 16 }, // 5
    { "SWORDSMN.ICN", { 1 }, 1, { 39, 40, 41 }, 3 },                  // 6
    { "SWORDSM2.ICN", { 1 }, 1, { 39, 40, 41 }, 3 },                  // 7
    { "CAVALRYR.ICN", { 1 }, 1, { 19, 20, 19 }, 3 },                  // 8
    { "CAVALRYB.ICN", { 1 }, 1, { 19, 20, 19 }, 3 },                  // 9
    { "PALADIN.ICN", { 1 }, 1, { 2, 3, 4, 5, 6 }, 5 },                // 10
    { "PALADIN2.ICN", { 1 }, 1, { 2, 3, 4, 5, 6 }, 5 },               // 11
    { "GOBLIN.ICN", { 1 }, 1, { 35, 36, 35 }, 3 },                    // 12
    { "ORC.ICN", { 1 }, 1, { 2 }, 1 },                                // 13
    { "ORC2.ICN", { 1 }, 1, { 2 }, 1 },                               // 14
    { "WOLF.ICN", { 1 }, 1, { 20 }, 1 },                              // 15
    { "OGRE.ICN", { 1 }, 1, { 2, 3, 2 }, 3 },                         // 16
    { "OGRE2.ICN", { 1 }, 1, { 2, 3, 2 }, 3 },                        // 17
    { "TROLL.ICN", { 1 }, 1, { 16, 17, 18 }, 3 },                     // 18
    { "TROLL2.ICN", { 1 }, 1, { 16, 17, 18 }, 3 },                    // 19
    { "CYCLOPS.ICN", { 1 }, 1, { 33, 34, 33 }, 3 },                   // 20
    { "SPRITE.ICN", { 1 }, 1, { 16, 17, 16, 18 }, 4 },                // 21
    { "DWARF.ICN", { 1 }, 1, { 44, 45, 46, 45, 44 }, 5 },             // 22
    { "DWARF2.ICN", { 1 }, 1, { 44, 45, 46, 45, 44 }, 5 },            // 23
    { "ELF.ICN", { 1 }, 1, { 42 }, 1 },                               // 24
    { "ELF2.ICN", { 1 }, 1, { 42 }, 1 },                              // 25
    { "DRUID.ICN", { 1 }, 1, { 46 }, 1 },                             // 26
    { "DRUID2.ICN", { 1 }, 1, { 46 }, 1 },                            // 27
    { "UNICORN.ICN", { 1 }, 1, { 4, 5 }, 2 },                         // 28
    { "PHOENIX.ICN", { 1 }, 1, { 30, 31, 32 }, 3 },                   // 29
    { "CENTAUR.ICN", { 1 }, 1, { 64, 65, 66 }, 3 },                   // 30
    { "GARGOYLE.ICN", { 1 }, 1, { 2 }, 1 },                           // 31
    { "GRIFFIN.ICN", { 1 }, 1, { 16, 17, 18, 19 }, 4 },               // 32
    { "MINOTAUR.ICN", { 1 }, 1, { 2 }, 1 },                           // 33
    { "MINOTAU2.ICN", { 1 }, 1, { 2 }, 1 },                           // 34
    { "HYDRA.ICN", { 1 }, 1, { 28, 29, 30, 29, 28 }, 5 },             // 35
    { "DRAGGREE.ICN", { 1 }, 1, { 47, 48, 49, 50, 51, 52, 53 }, 7 },  // 36
    { "DRAGRED.ICN", { 1 }, 1, { 47, 48, 49, 50, 51, 52, 53 }, 7 },   // 37
    { "DRAGBLAK.ICN", { 1 }, 1, { 47, 48, 49, 50, 51, 52, 53 }, 7 },  // 38
    { "HALFLING.ICN", { 1 }, 1, { 2 }, 1 },                           // 39
    { "BOAR.ICN", { 1 }, 1, { 2 }, 1 },                               // 40
    { "GOLEM.ICN", { 1 }, 1, { 34, 35, 34 }, 3 },                     // 41
    { "GOLEM2.ICN", { 1 }, 1, { 34, 35, 34 }, 3 },                    // 42
    { "ROC.ICN", { 1 }, 1, { 18, 1, 19 }, 3 },                        // 43
    { "MAGE1.ICN", { 1 }, 1, { 6, 7 }, 2 },                           // 44
    { "MAGE2.ICN", { 1 }, 1, { 6, 7 }, 2 },                           // 45
    { "TITANBLU.ICN", { 1 }, 1, { 2, 3, 4, 2 }, 4 },                  // 46
    { "TITANBLA.ICN", { 1 }, 1, { 2, 3, 4, 2 }, 4 },                  // 47
    { "SKELETON.ICN", { 1 }, 1, { 35, 36, 37 }, 3 },                  // 48
    { "ZOMBIE.ICN", { 1 }, 1, { 14, 15, 16, 17, 18, 19 }, 6 },        // 49
    { "ZOMBIE2.ICN", { 1 }, 1, { 14, 15, 16, 17, 18, 19 }, 6 },       // 50
    { "MUMMYW.ICN", { 1 }, 1, { 2 }, 1 },                             // 51
    { "MUMMY2.ICN", { 1 }, 1, { 2 }, 1 },                             // 52
    { "VAMPIRE.ICN", { 1 }, 1, { 2 }, 1 },                            // 53
    { "VAMPIRE2.ICN", { 1 }, 1, { 2 }, 1 },                           // 54
    { "LICH.ICN", { 1 }, 1, { 2, 3, 4 }, 3 },                         // 55
    { "LICH2.ICN", { 1 }, 1, { 2, 3, 4 }, 3 },                        // 56
    { "DRAGBONE.ICN", { 1 }, 1, { 22, 23, 24, 25 }, 4 },              // 57
    { "ROGUE.ICN", { 1 }, 1, { 2, 3, 4, 3, 2 }, 5 },                  // 58
    { "NOMAD.ICN", { 1 }, 1, { 8, 9 }, 2 },                           // 59
    { "GHOST.ICN", { 1 }, 1, { 2, 3, 4, 5, 9, 7, 8 }, 7 },            // 60
    { "GENIE.ICN", { 1 }, 1, { 5 }, 1 },                              // 61
    { "MEDUSA.ICN", { 1 }, 1, { 17, 18, 19, 17, 18 }, 5 },            // 62
    { "EELEM.ICN", { 1 }, 1, { 2, 3, 4 }, 3 },                        // 63
    { "AELEM.ICN", { 1 }, 1, { 2, 3, 4 }, 3 },                        // 64
    { "FELEM.ICN", { 1 }, 1, { 2, 3, 4 }, 3 },                        // 65
    { "WELEM.ICN", { 1 }, 1, { 2, 3, 4 }, 3 },                        // 66
};

const MonsterAnimInfo & monsterAnimInfo( int monsterId )
{
    static const MonsterAnimInfo EMPTY = { nullptr, { 0 }, 0, { 0 }, 0 };
    if ( monsterId < 0 || monsterId > 66 )
        return EMPTY;
    return MONSTER_ANIMATIONS[monsterId];
}

} // namespace fh2
