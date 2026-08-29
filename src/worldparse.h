#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fh2 {

// Version of the shared core (fh2core). External consumers (the
// fheroes2-screenshot project) check this number at build time to detect an
// out-of-sync vendored copy. Increment on any public API change.
inline constexpr int FH2CORE_VERSION = 2;

struct WorldParseError : std::runtime_error {
    explicit WorldParseError( const std::string & msg )
        : std::runtime_error( msg )
    {}
};

// One object part of a tile (Maps::ObjectPart).
struct ObjectPart {
    uint32_t uid = 0;
    uint8_t layerType = 0; // ObjectLayerType: 0 object, 1 background, 2 shadow, 3 terrain
    uint8_t icnType = 0;   // ObjectIcnType
    uint8_t icnIndex = 0;
};

// Maps::Tile (for format versions >= 10032; see FH2_SAVE_FORMAT.md §5, §6).
struct WorldTile {
    int32_t index = 0;
    uint16_t terrainImageIndex = 0; // -> GROUND32
    uint8_t terrainFlags = 0;
    uint16_t passability = 0;
    ObjectPart mainPart;
    uint16_t mainObjectType = 0; // MP2::MapObjectType
    uint8_t fogColors = 0;       // fog per player (bitmask)
    uint32_t metadata[3] = { 0, 0, 0 };
    uint8_t occupantHeroId = 0;
    uint8_t markedAsRoad = 0;
    std::vector<ObjectPart> groundParts;
    std::vector<ObjectPart> topParts;
    uint8_t boatOwnerColor = 0;
};

struct RouteStep {
    int32_t from = -1;
    int32_t direction = 0;
    uint32_t penalty = 0;
};

struct HeroRoute {
    bool hide = true;
    std::vector<RouteStep> steps;
};

// A hero record read sequentially from the World section (AllHeroes, 73 records).
// Only the fields needed for the poster are kept.
struct WorldHero {
    int32_t id = -1;
    uint8_t color = 0; // PlayerColor bitmask
    std::string name;  // cp1251
    int16_t centerX = 0;
    int16_t centerY = 0;
    uint32_t modes = 0;
    uint32_t spellPoints = 0;
    uint32_t movePoints = 0;
    uint32_t race = 0; // Race bitmask
    int32_t portrait = 0;
    int32_t primary[4] = { 0, 0, 0, 0 }; // attack, defense, knowledge, power (stream order)
    std::vector<int32_t> spells;         // spell book
    std::vector<std::pair<int32_t, int32_t>> artifacts; // (id, ext)
    int32_t monsterId[5] = { 0, 0, 0, 0, 0 };
    uint32_t monsterCount[5] = { 0, 0, 0, 0, 0 };
    int32_t experience = 0;
    int32_t direction = 0;
    int32_t spriteIndex = 0;
    HeroRoute route;
};

// Castle (vec_castles of the World section).
struct WorldCastle {
    int16_t x = 0;
    int16_t y = 0;
    uint32_t modes = 0;
    uint32_t race = 0; // Race bitmask
    uint32_t constructedBuildings = 0;
    uint32_t disabledBuildings = 0;
    uint8_t color = 0; // PlayerColor bitmask
    std::string name;  // cp1251
    // Captain (HeroBase).
    int32_t captainPrimary[4] = { 0, 0, 0, 0 };
    uint32_t captainSpellPoints = 0;
    uint32_t captainMovePoints = 0;
    std::vector<int32_t> captainSpells;
    // Mage guild: general and library spell lists.
    std::vector<int32_t> mageGuildGeneral;
    std::vector<int32_t> mageGuildLibrary;
    std::vector<uint32_t> dwelling; // creature counts per level
    // Garrison (Army, 5 slots).
    int32_t garrisonMonsterId[5] = { 0, 0, 0, 0, 0 };
    uint32_t garrisonCount[5] = { 0, 0, 0, 0, 0 };
};

// Kingdom (vec_kingdoms of the World section).
struct WorldKingdom {
    uint32_t modes = 0;
    uint8_t color = 0; // PlayerColor bitmask
    uint32_t resources[7] = { 0, 0, 0, 0, 0, 0, 0 }; // wood, mercury, ore, sulfur, crystal, gems, gold
    size_t resourcesOffset = 0;                       // stream offset of resources[0] (wood)
    int32_t lostTownDays = 0;
    std::vector<int32_t> castleIds; // castle indices in the WorldData::castles order
    std::vector<int32_t> heroIds;   // hero ids
    int32_t recruitIds[2] = { -1, -1 };
    uint32_t recruitDays[2] = { 0, 0 };
    std::vector<std::pair<int32_t, uint16_t>> visited;
    std::string puzzleBits;         // 48 chars of '0'/'1'
    std::vector<uint8_t> puzzleZones[4];
    int32_t visitedTents = 0;
    int32_t topCastle = 0;
    int32_t topHero = 0;
    std::vector<int32_t> monstersUnderVision; // format 10034 only
};

// EventDate (vec_eventsday of the World section).
struct WorldEvent {
    uint32_t resources[7] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t forAI = 0;
    uint32_t day = 0;
    uint32_t period = 0;
    uint8_t colors = 0;
    std::string message;
    std::string title;
};

struct WorldData {
    int width = 0;
    int height = 0;
    std::vector<WorldTile> tiles;
    std::vector<WorldHero> heroes; // all 73 (including not-yet-hired)
    std::vector<WorldCastle> castles;
    std::vector<WorldKingdom> kingdoms;
    std::vector<std::string> rumors;
    std::vector<WorldEvent> events;
    uint32_t day = 0;
    uint32_t week = 0;
    uint32_t month = 0;
    size_t dayOffset = 0;   // stream offset of day (u32)
    size_t weekOffset = 0;  // stream offset of week (u32)
    size_t monthOffset = 0; // stream offset of month (u32)
    int32_t winHeroId = -1;
    int32_t lossHeroId = -1;
    size_t offsetAfterCaptureObj = 0; // start of map_objects (diagnostics)
    size_t offsetAfterEvents = 0;     // start of map_captureobj (diagnostics)
    uint32_t capturedCount = 0;       // map_captureobj element count (diagnostics)
    int32_t ultimateArtifactId = 0;
    int32_t ultimateArtifactIndex = -1; // tile index where the ultimate artifact is hidden
};

// Sequentially parses the World section from the decompressed save stream
// (raw starts with the World section right after the file header).
WorldData parseWorld( const std::vector<uint8_t> & raw, int formatVersion );

} // namespace fh2
