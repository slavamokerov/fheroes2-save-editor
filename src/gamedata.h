#pragma once

#include <string>

namespace fh2 {

// Data ported from fheroes2 (spell.cpp, monster_info.cpp, skill.cpp,
// artifact_info.cpp) — for description windows, the spell book and monster
// stats. All texts are English msgids, localized at runtime via the game
// translation (.mo file of the fheroes2 installation).
struct MonsterStats {
    int attack = 0;
    int defense = 0;
    int dmgMin = 0;
    int dmgMax = 0;
    int hp = 0;
    int speed = 0;  // 2..7 (Speed: VERYSLOW..ULTRAFAST)
    int shots = 0;
};

const MonsterStats * monsterStats( int monsterId ); // nullptr — no data
std::string speedName( int speed );

int spellCost( int spellId );            // magic points per spell
std::string spellDescription( int spellId ); // %{count} already replaced
int spellExtraValue( int spellId );

// Secondary skill description for level 1..3 (port of Skill::Secondary::GetDescription).
std::string skillDescription( int skillId, int level );

// Artifact description 1..82 (port of Artifact::GetDescription, artifact_info.cpp):
// %{name}/%{count} are already substituted. Empty — no data.
std::string artifactDescription( int artifactId );

// Full-size monster animation for the troop window (port of
// getMonsterData(id).icnId + *.FRM.BIN, engine's MonsterAnimInfo::STATIC/IDLE1):
// icn — full-size ICN name (like ICN::SWORDSMN), "standing" frames from the
// BIN file (STATIC — almost always {1}, IDLE1 — the "waiting" cycle).
struct MonsterAnimInfo {
    const char * icn;
    int staticFrames[2];
    int staticCount;
    int idle1Frames[16];
    int idle1Count;
};

const MonsterAnimInfo & monsterAnimInfo( int monsterId ); // id 0..66; empty for 0

} // namespace fh2
