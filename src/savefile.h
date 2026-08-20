#pragma once

#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "constants.h"

namespace fh2 {

constexpr int MIN_SUPPORTED_VERSION = 10032;
constexpr int MAX_SUPPORTED_VERSION = 10034;

struct SaveError : std::runtime_error {
    explicit SaveError( const std::string & msg )
        : std::runtime_error( msg )
    {}
};

struct MapInfo {
    std::string filename;
    std::string name;
    std::string description;
    int width = 0;
    int height = 0;
    int difficulty = 0;
    int kingdomMax = 0;
    std::vector<int> races;
    std::vector<int> unions;
    int kingdomColors = 0;
    int colorsAvailableForHumans = 0;
    int colorsAvailableForComp = 0;
    int colorsOfRandomRaces = 0;
    int victoryConditionType = 0;
    bool compAlsoWins = false;
    bool allowNormalVictory = false;
    int victoryConditionParams[2] = { 0, 0 };
    int lossConditionType = 0;
    int lossConditionParams[2] = { 0, 0 };
    uint32_t timestamp = 0;
    bool startWithHeroInFirstCastle = false;
    int gameVersion = 0;
    uint32_t worldDay = 0;
    uint32_t worldWeek = 0;
    uint32_t worldMonth = 0;
    int mainLanguage = 0;
    std::string creatorNotes;
};

struct Troop {
    int32_t monsterId = 0;
    uint32_t count = 0;
};

// Primary skills in display order: attack, defense, power, knowledge.
struct PrimarySkills {
    int attack = 0;
    int defense = 0;
    int power = 0;
    int knowledge = 0;

    int value( int index ) const
    {
        switch ( index ) {
        case 0: return attack;
        case 1: return defense;
        case 2: return power;
        case 3: return knowledge;
        default: return 0;
        }
    }
};

struct ArtifactSlot {
    int id = 0; // 0 = empty; 1..81
    int ext = 0;
};

struct HeroRecord {
    size_t pos = 0;            // offset of the name in the decompressed stream
    std::string name;          // cp1251
    int color = 0;             // PlayerColor mask
    uint32_t experience = 0;
    std::vector<std::pair<int, int>> skills;
    Troop slots[5];
    size_t armyOffset = 0;     // offset of the first slot
    int heroId = 0;
    int portrait = 0;
    int race = 0;

    // HeroBase (§6.1): fields are stored BEFORE the name; found by backward scanning.
    // If parsing fails, all values are zero and all offsets are 0.
    bool heroBaseParsed = false;
    PrimarySkills primary;
    int spellPoints = 0;
    int artifactCount = 0; // size of the artifact bag in the stream (0..14)
    ArtifactSlot artifacts[14];
    std::vector<int> spells; // spell ids in the book (in stream order)
    size_t primaryOffset = 0;     // offset of attack (skill offsets: attack+0, defense+4, knowledge+8, power+12)
    size_t spellPointsOffset = 0;
    size_t artifactsOffset = 0;   // offset of the first artifact (id)
    size_t experienceOffset = 0;  // offset of experience
    size_t nameOffset = 0;        // offset of the u32 name length (= pos)
    size_t raceOffset = 0;        // offset of the race (i32 after the portrait)
    size_t spellBookOffset = 0;   // offset of the u32 spell list length
    size_t spellBookSize = 0;     // size of the spell block in the stream (4 + 4×count)

    // Offset of a primary skill value in the stream (file order: attack, defense, knowledge, power).
    size_t primarySkillOffset( int index ) const
    {
        switch ( index ) {
        case 0: return primaryOffset;
        case 1: return primaryOffset + 4;
        case 2: return primaryOffset + 12; // power
        case 3: return primaryOffset + 8;  // knowledge
        default: return 0;
        }
    }
};

struct PlayerInfo {
    int color = 0;
    int control = 0;
    int race = 0;
    int friends = 0;
    std::string name; // cp1251
    int focus[2] = { 0, 0 };
    int personality = 0;
    int handicap = 0;

    bool isHuman() const { return ( control & CONTROL_HUMAN ) != 0; }
    bool isAI() const { return ( control & CONTROL_AI ) != 0; }
};

class SaveFile
{
public:
    static SaveFile load( const std::string & path );

    const std::string & path() const { return _path; }
    int formatVersion() const { return _formatVersion; }
    int requirements() const { return _requirements; }
    const MapInfo & mapInfo() const { return _mapInfo; }
    int gameType() const { return _gameType; }
    const std::vector<HeroRecord> & heroes() const { return _heroes; }
    std::vector<HeroRecord> & heroes() { return _heroes; }
    const std::vector<PlayerInfo> & players() const { return _players; }
    int playersCurrentColor() const { return _playersCurrentColor; }
    int playersColorsMask() const { return _playersColorsMask; }
    std::vector<int> humanColors() const;

    std::vector<HeroRecord *> heroesByColor( int color );
    std::vector<const HeroRecord *> heroesByColor( int color ) const;

    // Writes an army slot (changes only the values, the stream size stays the same).
    void setSlot( HeroRecord & hero, int slotIndex, int monsterId, int count );
    // +N black dragons: into the slot with dragons, otherwise an empty one,
    // otherwise instead of the weakest one. Returns the slot index.
    int addBlackDragons( HeroRecord & hero, int count = 5 );

    // HeroBase setters — they change only the values at existing offsets
    // (the stream size stays the same). They require heroBaseParsed.
    void setPrimarySkill( HeroRecord & hero, int index, int value );
    void setSpellPoints( HeroRecord & hero, int value );
    void setExperience( HeroRecord & hero, uint32_t value );
    // id=0 removes the artifact from the slot (the bag is rebuilt with empty
    // cells squeezed out), otherwise replace or add (slot == artifactCount,
    // count < 14 — append to the end). Duplicates are allowed (except UI limits).
    void setArtifact( HeroRecord & hero, int slot, int id );
    // Moves an artifact to an empty slot (a hole inside the bag or the slot
    // right after it): an empty cell (id=0 in the stream) stays at the old place.
    void moveArtifact( HeroRecord & hero, int fromSlot, int toSlot );
    // Swaps two artifacts "at the same offsets" (in the game artifacts move
    // between slots; the spell book is not involved).
    void swapArtifacts( HeroRecord & hero, int slotA, int slotB );
    // Replaces a skill (skillIndex < skills.size()) or appends to the end
    // (skillIndex == skills.size(), count < 8). Duplicates are forbidden.
    void setSecondarySkill( HeroRecord & hero, int skillIndex, int skillId, int level );
    // Removes a skill (the skills block is rebuilt with the list shifted left).
    void removeSecondarySkill( HeroRecord & hero, int skillIndex );
    // Full replacement of the spell book (the spellBook list is rebuilt).
    void setSpells( HeroRecord & hero, const std::vector<int> & spellIds );
    void setRace( HeroRecord & hero, int race );
    void setPortrait( HeroRecord & hero, int portrait );
    // The name can be changed only to one of the same length (otherwise the whole stream would shift).
    void setName( HeroRecord & hero, const std::string & cp1251Name );

    void save( const std::string & outPath ) const;
    void save() const { save( _path ); }

    bool isDirty() const { return _dirty; }
    void markClean() { _dirty = false; }

private:
    // Replaces the range [start, start+oldLen) with new bytes in the decompressed
    // stream, recalculating all hero offsets after the replaced region.
    // skipOwned: fields of the region "owner" that point INSIDE the region
    // itself and therefore must not be shifted (1 = artifactsOffset,
    // 2 = spellBookOffset).
    void resizeRegion( size_t start, size_t oldLen, const std::vector<uint8_t> & newData, HeroRecord * owner, unsigned skipOwned = 0 );
    // Shifts all offsets of a single hero by delta (if they lie after from).
    static void shiftHeroOffsets( HeroRecord & hero, size_t from, ptrdiff_t delta, unsigned skipMask = 0 );

    std::string _path;
    std::vector<uint8_t> _data; // original file
    int _formatVersion = 0;
    int _requirements = 0;
    MapInfo _mapInfo;
    int _gameType = 0;
    size_t _zpos = 0;           // offset of the zlib block in the file
    std::vector<uint8_t> _raw;  // decompressed stream
    std::vector<HeroRecord> _heroes;
    std::vector<PlayerInfo> _players;
    int _playersColorsMask = 0;
    int _playersCurrentColor = 0;
    bool _dirty = false;
};

} // namespace fh2
