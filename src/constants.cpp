#include "constants.h"

#include <cmath>

#include "gettextmo.h"

namespace fh2 {

namespace {
// id -> (name, HP) from monster.h + monster_info.cpp; names are English msgids,
// localized at runtime via the game translation (.mo).
const std::map<int, MonsterInfo> MONSTERS = {
    { 0, { "Unknown", 0 } },
    { 1, { "Peasant", 1 } },
    { 2, { "Archer", 10 } },
    { 3, { "Ranger", 10 } },
    { 4, { "Pikeman", 15 } },
    { 5, { "Veteran Pikeman", 20 } },
    { 6, { "Swordsman", 25 } },
    { 7, { "Master Swordsman", 30 } },
    { 8, { "Cavalry", 30 } },
    { 9, { "Champion", 40 } },
    { 10, { "Paladin", 50 } },
    { 11, { "Crusader", 65 } },
    { 12, { "Goblin", 3 } },
    { 13, { "Orc", 10 } },
    { 14, { "Orc Chief", 15 } },
    { 15, { "Wolf", 20 } },
    { 16, { "Ogre", 40 } },
    { 17, { "Ogre Lord", 60 } },
    { 18, { "Troll", 40 } },
    { 19, { "War Troll", 40 } },
    { 20, { "Cyclops", 80 } },
    { 21, { "Sprite", 2 } },
    { 22, { "Dwarf", 20 } },
    { 23, { "Battle Dwarf", 20 } },
    { 24, { "Elf", 15 } },
    { 25, { "Grand Elf", 15 } },
    { 26, { "Druid", 25 } },
    { 27, { "Greater Druid", 25 } },
    { 28, { "Unicorn", 40 } },
    { 29, { "Phoenix", 100 } },
    { 30, { "Centaur", 5 } },
    { 31, { "Gargoyle", 15 } },
    { 32, { "Griffin", 25 } },
    { 33, { "Minotaur", 35 } },
    { 34, { "Minotaur King", 45 } },
    { 35, { "Hydra", 75 } },
    { 36, { "Green Dragon", 200 } },
    { 37, { "Red Dragon", 250 } },
    { 38, { "Black Dragon", 300 } },
    { 39, { "Halfling", 3 } },
    { 40, { "Boar", 15 } },
    { 41, { "Iron Golem", 30 } },
    { 42, { "Steel Golem", 35 } },
    { 43, { "Roc", 40 } },
    { 44, { "Mage", 30 } },
    { 45, { "Archmage", 35 } },
    { 46, { "Giant", 150 } },
    { 47, { "Titan", 300 } },
    { 48, { "Skeleton", 4 } },
    { 49, { "Zombie", 15 } },
    { 50, { "Mutant Zombie", 20 } },
    { 51, { "Mummy", 25 } },
    { 52, { "Royal Mummy", 30 } },
    { 53, { "Vampire", 30 } },
    { 54, { "Vampire Lord", 40 } },
    { 55, { "Lich", 25 } },
    { 56, { "Power Lich", 35 } },
    { 57, { "Bone Dragon", 150 } },
    { 58, { "Rogue", 4 } },
    { 59, { "Nomad", 20 } },
    { 60, { "Ghost", 20 } },
    { 61, { "Genie", 50 } },
    { 62, { "Medusa", 35 } },
    { 63, { "Earth Elemental", 50 } },
    { 64, { "Air Elemental", 35 } },
    { 65, { "Fire Elemental", 40 } },
    { 66, { "Water Elemental", 45 } },
};

const std::map<int, std::string> HEROES = {
    { 1, "Lord Kilburn" }, { 2, "Sir Gallant" }, { 3, "Ector" }, { 4, "Gwenneth" }, { 5, "Tyro" },
    { 6, "Ambrose" }, { 7, "Ruby" }, { 8, "Maximus" }, { 9, "Dimitry" }, { 10, "Thundax" },
    { 11, "Fineous" }, { 12, "Jojosh" }, { 13, "Crag Hack" }, { 14, "Jezebel" }, { 15, "Jaclyn" },
    { 16, "Ergon" }, { 17, "Tsabu" }, { 18, "Atlas" }, { 19, "Astra" }, { 20, "Natasha" },
    { 21, "Troyan" }, { 22, "Vatawna" }, { 23, "Rebecca" }, { 24, "Gem" }, { 25, "Ariel" },
    { 26, "Carlawn" }, { 27, "Luna" }, { 28, "Arie" }, { 29, "Alamar" }, { 30, "Vesper" },
    { 31, "Crodo" }, { 32, "Barok" }, { 33, "Kastore" }, { 34, "Agar" }, { 35, "Falagar" },
    { 36, "Wrathmont" }, { 37, "Myra" }, { 38, "Flint" }, { 39, "Dawn" }, { 40, "Halon" },
    { 41, "Myrini" }, { 42, "Wilfrey" }, { 43, "Sarakin" }, { 44, "Kalindra" }, { 45, "Mandigal" },
    { 46, "Zom" }, { 47, "Darlana" }, { 48, "Zam" }, { 49, "Ranloo" }, { 50, "Charity" },
    { 51, "Rialdo" }, { 52, "Roxana" }, { 53, "Sandro" }, { 54, "Celia" }, { 55, "Roland" },
    { 56, "Lord Corlagon" }, { 57, "Sister Eliza" }, { 58, "Archibald" }, { 59, "Lord Halton" },
    { 60, "Brother Brax" }, { 61, "Solmyr" }, { 62, "Dainwin" }, { 63, "Mog" }, { 64, "Uncle Ivan" },
    { 65, "Joseph" }, { 66, "Gallavant" }, { 67, "Elderian" }, { 68, "Ceallach" }, { 69, "Drakonia" },
    { 70, "Martine" }, { 71, "Jarkonas" },
};

const std::map<int, PlayerColor> COLORS = {
    { 0, { 0, "None" } },
    { 1, { 1, "Blue" } },
    { 2, { 2, "Green" } },
    { 4, { 4, "Red" } },
    { 8, { 8, "Yellow" } },
    { 16, { 16, "Orange" } },
    { 32, { 32, "Purple" } },
};

// Secondary skills: id -> name (English msgid from fheroes2 skill.cpp GetName).
const std::map<int, SkillInfo> SKILLS = {
    { 1, { "Pathfinding" } }, { 2, { "Archery" } }, { 3, { "Logistics" } },
    { 4, { "Scouting" } }, { 5, { "Diplomacy" } }, { 6, { "Navigation" } },
    { 7, { "Leadership" } }, { 8, { "Wisdom" } }, { 9, { "Mysticism" } },
    { 10, { "Luck" } }, { 11, { "Ballistics" } }, { 12, { "Eagle Eye" } },
    { 13, { "Necromancy" } }, { 14, { "Estates" } },
};

// Artifacts 1..82 (id, name). Order and English names are from
// fheroes2 src/fheroes2/resource/artifact_info.cpp; icon is ARTFX[id-1].
const std::map<int, ArtifactInfo> ARTIFACTS = {
    { 1, { "Ultimate Book of Knowledge" } },
    { 2, { "Ultimate Sword of Dominion" } },
    { 3, { "Ultimate Cloak of Protection" } },
    { 4, { "Ultimate Wand of Magic" } },
    { 5, { "Ultimate Shield" } },
    { 6, { "Ultimate Staff" } },
    { 7, { "Ultimate Crown" } },
    { 8, { "Golden Goose" } },
    { 9, { "Arcane Necklace of Magic" } },
    { 10, { "Caster's Bracelet of Magic" } },
    { 11, { "Mage's Ring of Power" } },
    { 12, { "Witch's Broach of Magic" } },
    { 13, { "Medal of Valor" } },
    { 14, { "Medal of Courage" } },
    { 15, { "Medal of Honor" } },
    { 16, { "Medal of Distinction" } },
    { 17, { "Fizbin of Misfortune" } },
    { 18, { "Thunder Mace of Dominion" } },
    { 19, { "Armored Gauntlets of Protection" } },
    { 20, { "Defender Helm of Protection" } },
    { 21, { "Giant Flail of Dominion" } },
    { 22, { "Ballista of Quickness" } },
    { 23, { "Stealth Shield of Protection" } },
    { 24, { "Dragon Sword of Dominion" } },
    { 25, { "Power Axe of Dominion" } },
    { 26, { "Divine Breastplate of Protection" } },
    { 27, { "Minor Scroll of Knowledge" } },
    { 28, { "Major Scroll of Knowledge" } },
    { 29, { "Superior Scroll of Knowledge" } },
    { 30, { "Foremost Scroll of Knowledge" } },
    { 31, { "Endless Sack of Gold" } },
    { 32, { "Endless Bag of Gold" } },
    { 33, { "Endless Purse of Gold" } },
    { 34, { "Nomad Boots of Mobility" } },
    { 35, { "Traveler's Boots of Mobility" } },
    { 36, { "Lucky Rabbit's Foot" } },
    { 37, { "Golden Horseshoe" } },
    { 38, { "Gambler's Lucky Coin" } },
    { 39, { "Four-Leaf Clover" } },
    { 40, { "True Compass of Mobility" } },
    { 41, { "Sailor's Astrolabe of Mobility" } },
    { 42, { "Evil Eye" } },
    { 43, { "Enchanted Hourglass" } },
    { 44, { "Gold Watch" } },
    { 45, { "Skullcap" } },
    { 46, { "Ice Cloak" } },
    { 47, { "Fire Cloak" } },
    { 48, { "Lightning Helm" } },
    { 49, { "Evercold Icicle" } },
    { 50, { "Everhot Lava Rock" } },
    { 51, { "Lightning Rod" } },
    { 52, { "Snake-Ring" } },
    { 53, { "Ankh" } },
    { 54, { "Book of Elements" } },
    { 55, { "Elemental Ring" } },
    { 56, { "Holy Pendant" } },
    { 57, { "Pendant of Free Will" } },
    { 58, { "Pendant of Life" } },
    { 59, { "Serenity Pendant" } },
    { 60, { "Seeing-eye Pendant" } },
    { 61, { "Kinetic Pendant" } },
    { 62, { "Pendant of Death" } },
    { 63, { "Wand of Negation" } },
    { 64, { "Golden Bow" } },
    { 65, { "Telescope" } },
    { 66, { "Statesman's Quill" } },
    { 67, { "Wizard's Hat" } },
    { 68, { "Power Ring" } },
    { 69, { "Ammo Cart" } },
    { 70, { "Tax Lien" } },
    { 71, { "Hideous Mask" } },
    { 72, { "Endless Pouch of Sulfur" } },
    { 73, { "Endless Vial of Mercury" } },
    { 74, { "Endless Pouch of Gems" } },
    { 75, { "Endless Cord of Wood" } },
    { 76, { "Endless Cart of Ore" } },
    { 77, { "Endless Pouch of Crystal" } },
    { 78, { "Spiked Helm" } },
    { 79, { "Spiked Shield" } },
    { 80, { "White Pearl" } },
    { 81, { "Black Pearl" } },
    { 82, { "Magic Book" } },
};

// Spells 1..65: id -> (name, level, SPELLS.ICN icon).
// Names/levels are from fheroes2 spell.cpp (spell_info, Spell::Level).
const std::map<int, SpellInfo> SPELLS = {
    { 1, { "Fireball", 3, 8 } },
    { 2, { "Fireblast", 4, 9 } },
    { 3, { "Lightning Bolt", 2, 4 } },
    { 4, { "Chain Lightning", 4, 5 } },
    { 5, { "Teleport", 3, 10 } },
    { 6, { "Cure", 1, 6 } },
    { 7, { "Mass Cure", 4, 60 } },
    { 8, { "Resurrect", 4, 13 } },
    { 9, { "Resurrect True", 5, 12 } },
    { 10, { "Haste", 1, 14 } },
    { 11, { "Mass Haste", 3, 61 } },
    { 12, { "Slow", 1, 1 } },
    { 13, { "Mass Slow", 4, 62 } },
    { 14, { "Blind", 2, 21 } },
    { 15, { "Bless", 1, 7 } },
    { 16, { "Mass Bless", 3, 63 } },
    { 17, { "Stoneskin", 1, 31 } },
    { 18, { "Steelskin", 2, 30 } },
    { 19, { "Curse", 1, 3 } },
    { 20, { "Mass Curse", 3, 64 } },
    { 21, { "Holy Word", 3, 22 } },
    { 22, { "Holy Shout", 4, 23 } },
    { 23, { "Anti-Magic", 3, 17 } },
    { 24, { "Dispel Magic", 1, 18 } },
    { 25, { "Mass Dispel", 3, 73 } },
    { 26, { "Magic Arrow", 1, 38 } },
    { 27, { "Berserker", 4, 19 } },
    { 28, { "Armageddon", 5, 16 } },
    { 29, { "Elemental Storm", 4, 11 } },
    { 30, { "Meteor Shower", 4, 24 } },
    { 31, { "Paralyze", 3, 20 } },
    { 32, { "Hypnotize", 5, 37 } },
    { 33, { "Cold Ray", 2, 36 } },
    { 34, { "Cold Ring", 3, 35 } },
    { 35, { "Disrupting Ray", 2, 34 } },
    { 36, { "Death Ripple", 2, 29 } },
    { 37, { "Death Wave", 3, 28 } },
    { 38, { "Dragon Slayer", 2, 32 } },
    { 39, { "Blood Lust", 1, 27 } },
    { 40, { "Animate Dead", 3, 25 } },
    { 41, { "Mirror Image", 5, 26 } },
    { 42, { "Shield", 1, 15 } },
    { 43, { "Mass Shield", 4, 65 } },
    { 44, { "Summon Earth Elemental", 5, 56 } },
    { 45, { "Summon Air Elemental", 5, 57 } },
    { 46, { "Summon Fire Elemental", 5, 58 } },
    { 47, { "Summon Water Elemental", 5, 59 } },
    { 48, { "Earthquake", 3, 33 } },
    { 49, { "View Mines", 1, 39 } },
    { 50, { "View Resources", 1, 40 } },
    { 51, { "View Artifacts", 2, 41 } },
    { 52, { "View Towns", 3, 42 } },
    { 53, { "View Heroes", 3, 43 } },
    { 54, { "View All", 4, 44 } },
    { 55, { "Identify Hero", 3, 45 } },
    { 56, { "Summon Boat", 2, 46 } },
    { 57, { "Dimension Door", 5, 47 } },
    { 58, { "Town Gate", 4, 48 } },
    { 59, { "Town Portal", 5, 49 } },
    { 60, { "Visions", 2, 50 } },
    { 61, { "Haunt", 2, 51 } },
    { 62, { "Set Earth Guardian", 4, 52 } },
    { 63, { "Set Air Guardian", 4, 53 } },
    { 64, { "Set Fire Guardian", 4, 54 } },
    { 65, { "Set Water Guardian", 4, 55 } },
};
} // namespace

const std::map<int, MonsterInfo> & monsters() { return MONSTERS; }

std::string monsterName( int id )
{
    const auto it = MONSTERS.find( id );
    if ( it == MONSTERS.end() )
        return "?";
    return trGame( it->second.name );
}

int monsterHp( int id )
{
    auto it = MONSTERS.find( id );
    return it == MONSTERS.end() ? 0 : it->second.hp;
}

std::vector<int> editableMonsters()
{
    // Black Dragon and Ghost are pinned as the first two rows of the list.
    std::vector<int> out = { MONSTER_BLACK_DRAGON, 60 };
    for ( int id = 1; id <= 66; ++id ) {
        if ( id != MONSTER_BLACK_DRAGON && id != 60 )
            out.push_back( id );
    }
    return out;
}

const std::map<int, std::string> & heroDefaultNames() { return HEROES; }

const std::map<int, PlayerColor> & playerColors() { return COLORS; }

const std::map<int, SkillInfo> & skills()
{
    return SKILLS;
}

std::string skillName( int id )
{
    const auto it = SKILLS.find( id );
    if ( it == SKILLS.end() )
        return "?";
    return trGame( it->second.name );
}

std::string skillLevelName( int level )
{
    switch ( level ) {
    case 1:
        return trGame( "skill|Basic" );
    case 2:
        return trGame( "skill|Advanced" );
    case 3:
        return trGame( "skill|Expert" );
    default:
        return "?";
    }
}

// "Name with level" (Skill::Secondary::GetName in fheroes2): the combined
// strings ("Basic Pathfinding", "Advanced Archery", ...) are ready-made
// engine msgids, so compose the English one and translate it.
std::string skillNameWithLevel( int id, int level )
{
    if ( id < 1 || id > 14 || level < 1 || level > 3 )
        return "?";
    static const char * const levelEn[] = { "", "Basic", "Advanced", "Expert" };
    std::string out = levelEn[level];
    out += ' ';
    const auto it = skills().find( id );
    out += ( it == skills().end() ) ? std::string( "?" ) : it->second.name;
    return trGame( out );
}

std::string raceName( int race )
{
    switch ( race ) {
    case 1:
        return trGame( "Knight" );
    case 2:
        return trGame( "Barbarian" );
    case 4:
        return trGame( "Sorceress" );
    case 8:
        return trGame( "Warlock" );
    case 16:
        return trGame( "Wizard" );
    case 32:
        return trGame( "Necromancer" );
    case 64:
        return trGame( "Multi" );
    case 128:
        return trGame( "Random" );
    default:
        return "?";
    }
}

const std::map<int, ArtifactInfo> & artifacts() { return ARTIFACTS; }

std::string artifactName( int id )
{
    const auto it = ARTIFACTS.find( id );
    if ( it == ARTIFACTS.end() )
        return "?";
    return trGame( it->second.name );
}

std::string primarySkillName( int index )
{
    switch ( index ) {
    case 0:
        return trGame( "Attack Skill" );
    case 1:
        return trGame( "Defense Skill" );
    case 2:
        return trGame( "Spell Power" );
    case 3:
        return trGame( "Knowledge" );
    default:
        return "?";
    }
}

std::vector<std::pair<int, int>> secondarySkillDefaults( int race )
{
    // heroInitialSecondarySkills from game_static.cpp: 14 values (levels)
    // for skills 1..14 by race KNGT..NECR.
    static const int table[6][14] = {
        { 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 }, // KNGT: Archery 1, Leadership 1
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0 }, // BARB: Ballistics 2
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1 }, // SORC: Mysticism 2, Estates 1
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1 }, // WRLK: Necromancy 2, Estates 1
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 }, // WZRD: Estates 2
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1 }, // NECR: Luck 1, Estates 1
    };
    int row = -1;
    switch ( race ) {
    case 1: row = 0; break;
    case 2: row = 1; break;
    case 4: row = 2; break;
    case 8: row = 3; break;
    case 16: row = 4; break;
    case 32: row = 5; break;
    default: break;
    }
    std::vector<std::pair<int, int>> out;
    if ( row >= 0 ) {
        for ( int skill = 1; skill <= 14; ++skill ) {
            if ( table[row][skill - 1] > 0 )
                out.emplace_back( skill, table[row][skill - 1] );
        }
    }
    return out;
}

int primarySkillDefault( int race, int index )
{
    // heroInitialPrimarySkills from fheroes2 game_static.cpp:
    // order (attack, defense, power, knowledge) by race KNGT..NECR.
    static const int table[6][4] = {
        { 2, 2, 1, 1 }, // KNGT (1)
        { 3, 1, 1, 1 }, // BARB (2)
        { 0, 0, 2, 3 }, // SORC (4)
        { 0, 0, 3, 2 }, // WRLK (8)
        { 0, 1, 2, 2 }, // WZRD (16)
        { 1, 0, 2, 2 }, // NECR (32)
    };
    int row = -1;
    switch ( race ) {
    case 1: row = 0; break;
    case 2: row = 1; break;
    case 4: row = 2; break;
    case 8: row = 3; break;
    case 16: row = 4; break;
    case 32: row = 5; break;
    default: break;
    }
    if ( row < 0 || index < 0 || index > 3 )
        return 0;
    return table[row][index];
}

const std::map<int, SpellInfo> & spells()
{
    return SPELLS;
}

std::string spellName( int id )
{
    const auto it = SPELLS.find( id );
    if ( it == SPELLS.end() )
        return "?";
    return trGame( it->second.name );
}

int spellLevel( int id )
{
    auto it = SPELLS.find( id );
    return it == SPELLS.end() ? 0 : it->second.level;
}

int spellIconIndex( int id )
{
    auto it = SPELLS.find( id );
    return it == SPELLS.end() ? 0 : it->second.iconIndex;
}

namespace {

// Experience threshold for a level (GetExperienceFromLevel from fheroes2 heroes.cpp;
// levels above 39 use the formula: exp(L) = exp(L-1) + round((exp(L-1)-exp(L-2))*1.2/100)*100).
uint32_t experienceForLevel( int level )
{
    if ( level <= 0 )
        return 0;
    switch ( level ) {
    case 1: return 1000;
    case 2: return 2000;
    case 3: return 3200;
    case 4: return 4500;
    case 5: return 6000;
    case 6: return 7700;
    case 7: return 9000;
    case 8: return 11000;
    case 9: return 13200;
    case 10: return 15500;
    case 11: return 18500;
    case 12: return 22100;
    case 13: return 26400;
    case 14: return 31600;
    case 15: return 37800;
    case 16: return 45300;
    case 17: return 54200;
    case 18: return 65000;
    case 19: return 78000;
    case 20: return 93600;
    case 21: return 112300;
    case 22: return 134700;
    case 23: return 161600;
    case 24: return 193900;
    case 25: return 232700;
    case 26: return 279300;
    case 27: return 335200;
    case 28: return 402300;
    case 29: return 482800;
    case 30: return 579400;
    case 31: return 695300;
    case 32: return 834400;
    case 33: return 1001300;
    case 34: return 1201600;
    case 35: return 1442000;
    case 36: return 1730500;
    case 37: return 2076700;
    case 38: return 2492100;
    case 39: return 2990600;
    default: {
        const uint32_t l1 = experienceForLevel( level - 1 );
        return l1 + static_cast<uint32_t>( std::lround( ( l1 - experienceForLevel( level - 2 ) ) * 1.2 / 100 ) * 100 );
    }
    }
}

} // namespace

int heroLevel( uint32_t experience )
{
    for ( int lvl = 1; lvl < 255; ++lvl ) {
        if ( experience < experienceForLevel( lvl ) )
            return lvl;
    }
    return 0;
}

int raceStripIndex( int race )
{
    switch ( race ) {
    case 1: return 4;   // KNGT
    case 2: return 5;   // BARB
    case 4: return 6;   // SORC
    case 8: return 7;   // WRLK
    case 16: return 8;  // WZRD
    case 32: return 9;  // NECR
    default: return 10; // multi/random
    }
}

int crestIndexForColor( int colorMask )
{
    switch ( colorMask ) {
    case 1: return 0;   // BLUE
    case 2: return 1;   // GREEN
    case 4: return 2;   // RED
    case 8: return 3;   // YELLOW
    case 16: return 4;  // ORANGE
    case 32: return 5;  // PURPLE
    default: return 0;
    }
}

} // namespace fh2
