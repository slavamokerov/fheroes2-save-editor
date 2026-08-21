# fheroes2 save file format (.sav / .savc / .savh / .savm)

A complete description of the format, verified on real saves and cross-checked against the sources at
https://github.com/ihhub/fheroes2 (master branch, format versions 10032–10034 — see
[§12.6](#126-format-versions-save_format_versionh), game 1.1.11–1.1.8x).

Purpose of this document: to provide everything needed to write an fheroes2 save editor.

---

## 1. File overview

A save file consists of two parts:

| Part | Field | Type | Value |
|---|---|---|---|
| 1. Header (uncompressed) | magic | u16 | `0xFF03` |
| | versionString | str | e.g. `"10032"` |
| | formatVersion | u16 | e.g. 10032 |
| | requirements | u16 | `0` or `0x4000` (requires PoL resources) |
| | info | Maps::FileInfo | map information |
| | gameType | i32 | game type (see [§3.3](#33-gametype)) |
| 2. Compressed data block | rawSize | u32 | decompressed data size |
| | zipSize | u32 | compressed data size |
| | compressionVersion | u16 | `0` |
| | unused | u16 | `0` |
| | zlib data | zipSize bytes | zlib compress (RFC 1950) |

There is nothing after the compressed block in the file (no trailer/checksum).

## 2. Basic serialization rules

Serializer: `src/engine/serialize.h/.cpp`. **Everything is written big-endian (network byte order).**

| C++ type | Bytes | Comment |
|---|---|---|
| `bool` | 1 | 0 or 1 |
| `int8_t` / `uint8_t` / `char` | 1 | |
| `int16_t` / `uint16_t` | 2 | big-endian |
| `int32_t` / `uint32_t` | 4 | big-endian |
| `enum` | size of the underlying type | written as a number |
| `std::string` / `string_view` | 4 (u32 length) + bytes | **no terminator and no alignment** |
| `std::vector<T>` / `std::list<T>` | u32 count + elements back-to-back | |
| `std::set<T>` / `std::map<K,V>` | u32 count + elements | |
| `std::array<T,N>` | u32 count (=N) + elements | |
| `std::pair<A,B>` | A, B back-to-back | |
| `std::optional<T>` | 1 byte (0/1) + T if present | |
| `fheroes2::Point` | i32 x, i32 y | 8 bytes |

Structures are written "flat" — no padding and no headers, exactly in the field order of the
corresponding `operator<<` in the sources.

Important: the format version affects reading of old saves (in `operator>>` there are branches
`if (Game::GetVersionOfCurrentSaveFile() < ...)`). For saves of version **10032+** the simple
rules from this document apply.

## 3. Header

### 3.1 magic and version

- `u16 magic = 0xFF03` — the same for all saves.
- `std::string versionString` — ASCII string representation of the format number (u32 length + bytes).
- `u16 formatVersion` — the same number as an integer. For existing saves: 10032 (1.1.11), 10033 (1.1.15), 10034 (1.1.80)
  (the full version table is in [§12.6](#126-format-versions-save_format_versionh)).
- `u16 requirements` — bit flags. `0x4000` = REQUIRES_POL_RESOURCES (the game requires "The Price of Loyalty" resources).

Example from a real save:
```
ff 03                      magic
00 00 00 05                string length = 5
31 30 30 33 32             "10032"
27 30                      u16 10032
40 00                      requirements = 0x4000
```

### 3.2 Maps::FileInfo (serialization order)

Source: `src/fheroes2/maps/maps_fileinfo.cpp` — `operator<<(OStreamBase&, const FileInfo&)`.

| # | Field | Type | Comment |
|---|---|---|---|
| 1 | filename | string | file name only, without path |
| 2 | name | string | map name |
| 3 | description | string | map description |
| 4 | width | u16 | map width |
| 5 | height | u16 | map height |
| 6 | difficulty | u8 | |
| 7 | kingdommax | u8 | number of kingdoms/players |
| 8 | races[] | u8 × kingdommax | race of each slot |
| 9 | unions[] | u8 × kingdommax | unions PlayerColorsSet |
| 10 | kingdomColors | u8 | bitmask of used colors (see PlayerColor) |
| 11 | colorsAvailableForHumans | u8 | |
| 12 | colorsAvailableForComp | u8 | |
| 13 | colorsOfRandomRaces | u8 | |
| 14 | victoryConditionType | u8 | enum VictoryCondition |
| 15 | compAlsoWins | bool | |
| 16 | allowNormalVictory | bool | |
| 17 | victoryConditionParams | u16 × 2 | |
| 18 | lossConditionType | u8 | enum LossCondition |
| 19 | lossConditionParams | u16 × 2 | |
| 20 | timestamp | u32 | unix time of save creation |
| 21 | startWithHeroInFirstCastle | bool | |
| 22 | version | i32 | enum GameVersion: SUCCESSION_WARS=0, PRICE_OF_LOYALTY=1, RESURRECTION=2 |
| 23 | worldDay | u32 | day at the time of saving |
| 24 | worldWeek | u32 | week |
| 25 | worldMonth | u32 | month |
| 26 | mainLanguage | enum (i32) | `fheroes2::SupportedLanguage` (only for version ≥ 10025) |
| 27 | creatorNotes | string | only for version ≥ 10033 |

Note: `maxNumOfPlayers = 6`, but only `kingdommax` records of races/unions are written to the file.

### 3.3 gameType

`i32` — a bitmask of the game type. Determines the save extension:
`.sav` = TYPE_STANDARD, `.savc` = TYPE_CAMPAIGN, `.savh` = TYPE_HOTSEAT, `.savm` = multiplayer.
In the header it is usually: 1 (standard) or 2 (campaign), etc.

## 4. Compressed block and uncompressed stream

The uncompressed stream has the following structure:

```
World                    ([§5](#5-world))
Settings                 ([§9](#9-settings-and-players))
GameOver::Result         ([§10](#10-gameoverresult))
[CampaignSaveData]       (only if gameType is campaign, [§11](#11-campaignsavedata))
u16 0xFF03               end-of-data marker (integrity check)
```

The `0xFF03` marker is a mandatory last element; it is convenient for verifying that the stream
was decompressed correctly (last 2 bytes).

## 5. World

Source: `src/fheroes2/world/world.cpp` — `operator<<(OStreamBase&, const World&)`.
World is the first object of the stream and stores the whole game state.

Field order:

| # | Field | Type |
|---|---|---|
| 1 | width | u32 (in newer versions; in older < 10011 — u16) |
| 2 | height | u32 |
| 3 | vec_tiles | vector<Maps::Tile> — u32 count + tiles (each tile is complex; count = width×height) |
| 4 | vec_heroes | AllHeroes — **all heroes of the game** ([§6](#6-heroes--the-most-important-section-for-an-army-editor)) |
| 5 | vec_castles | vector<Castle*> — castles |
| 6 | vec_kingdoms | Kingdoms ([§7](#7-kingdom)) |
| 7 | _customRumors | vector<string> |
| 8 | vec_eventsday | list<EventDate> |
| 9 | map_captureobj | map<i32, CapturedObject> — captured objects (mines etc.) |
| 10 | _ultimateArtifact | Artifact (i32 id + i32 ext = 8 bytes) |
| 11 | _day | u32 |
| 12 | _week | u32 |
| 13 | _month | u32 |
| 14 | heroIdAsWinCondition | i32 |
| 15 | heroIdAsLossCondition | i32 |
| 16 | map_objects | MapObjects |
| 17 | _seed | u32 |

AllHeroes: `u32 count` (= HEROES_COUNT = 73) + 73 Heroes records back-to-back ([§6](#6-heroes--the-most-important-section-for-an-army-editor)).
This is exactly the section where hero armies live.

## 6. Heroes — the most important section for an army editor

Source: `src/fheroes2/heroes/heroes.cpp` — `operator<<(OStreamBase&, const Heroes&)`,
`src/fheroes2/heroes/heroes_base.cpp`, `src/fheroes2/army/army.cpp`, `src/fheroes2/army/army_troop.cpp`.

A hero record = HeroBase + the Heroes fields. The serialization order is exactly as follows
(all numbers big-endian):

### 6.1 HeroBase

| # | Field | Type | Size |
|---|---|---|---|
| 1 | attack | i32 | 4 |
| 2 | defense | i32 | 4 |
| 3 | knowledge | i32 | 4  ← **note: knowledge comes BEFORE power** |
| 4 | power | i32 | 4 |
| 5 | center | **i16 x, i16 y** (MapPosition) | 4  (not i32×2!) |
| 6 | modes | u32 | 4  (bit flags, e.g. SHIPMASTER=1, PATROL, JAIL…) |
| 7 | _spellPoints | u32 | 4 |
| 8 | _movePoints | u32 | 4 |
| 9 | _spellBook | vector<Spell>: u32 count + count × i32 spellId | variable (count 0..64) |
| 10 | _bagArtifacts | vector<Artifact>: u32 count + count × (i32 id, i32 ext) | variable (**count 0..14**, not always 14!) |

> Note: the position center is **two int16**, not two int32
> (see `Maps::operator<<` in `src/fheroes2/maps/position.cpp`).

### 6.2 Heroes (after HeroBase)

| # | Field | Type | Size |
|---|---|---|---|
| 11 | _name | string (u32 len + bytes) | variable |
| 12 | _color | u8 PlayerColor (a bitmask!) | 1 |
| 13 | _experience | u32 | 4 |
| 14 | _secondarySkills | vector<Secondary>: u32 count + count × (i32 skillId, i32 level) | variable |
| 15 | _army | Army ([§6.3](#63-army)) | variable |
| 16 | _id | i32 | 4  (Hero ID, see [§12.3](#123-hero-id-int32-heroes-enum-heroesh)) |
| 17 | _portrait | i32 | 4 |
| 18 | _race | i32 (Race, bitmask) | 4 |
| 19 | _objectTypeUnderHero | u16 (MP2::MapObjectType; versions < 10016 had a different size!) | 2 |
| 20 | _path | Route::Path: bool hide (1 byte) + u32 count + count × (i32 from, i32 direction, u32 penalty) | variable |
| 21 | _direction | i32 | 4 |
| 22 | _spriteIndex | i32 | 4 |
| 23 | _patrolCenter | Point (i32, i32) | 8 |
| 24 | _patrolDistance | u32 | 4 |
| 25 | _visitedObjects | list<IndexObject>: u32 count + count × (i32 index, u16 objectType) | variable |
| 26 | _lastGroundRegion | u32 | 4 |

### 6.3 Army

| # | Field | Type | Size |
|---|---|---|---|
| 1 | size | u32 (= 5, number of slots) | 4 |
| 2–6 | 5 × Troop | each: i32 monsterId + u32 count | 5 × 8 = 40 |
| 7 | _isSpreadCombatFormation | bool (1 byte) | 1 |
| 8 | _color | u8 (PlayerColor; for a hero it usually matches the hero's color) | 1 |

Troop: `monsterId = 0` (UNKNOWN) + `count = 0` means an **empty slot**.
`monsterId = 0` with count > 0 never occurs; an empty slot is both zeros.

**Total hero army size: 4 + 40 + 1 + 1 = 46 bytes.**

### 6.4 How to find a specific hero (scanning algorithm)

Heroes are not indexed — linear scanning with validation is required.
The algorithm is implemented in [src/savefile.cpp](src/savefile.cpp)
(`tryParseHero` + `tryParseHeroBase`):

1. Iterate position `p` over all bytes of the uncompressed stream.
2. Read u32 len; if `1 ≤ len ≤ 64` and the name bytes are printable — a candidate.
3. Then parse sequentially: color (u8, 0..63 — a color mask),
   experience (u32), secSkills (u32 count ≤ 8, each skill: i32 id 0..14, i32 level 0..3),
   armySize (u32 == 5), 5 slots (i32 id 0..72, u32 count ≤ 999999),
   spread (0/1), armyColor (0..63), _id (0..72), _portrait (0..72), _race (0..255).
4. If everything matches — this is a hero record. Then HeroBase ([§6.1](#61-herobase)) is recovered by
   **backward scanning** from the start of the name: bagArtifacts (count 0..14, size
   4 + 8×count, strictly adjacent to the name; **iterate count from 14 DOWNWARD** — otherwise
   at count=0 the ext of the last artifact (==0) is read and the bag is mistakenly
   empty), spellBook (u32 count ≤ 64 + i32×count),
   then the fixed movePoints, spellPoints, modes, center (i16×2), power,
   knowledge, defense, attack. Value validation (skills 0..99, center within the
   map, mana ≤ 9999) filters out false positives.

The algorithm is verified on real saves: it finds all AllHeroes records and
recognizes HeroBase for every hero.

### 6.5 Important specifics

- **PlayerColor is a bitmask**, not an index! Values: NONE=0x00, BLUE=0x01,
  GREEN=0x02, RED=0x04, YELLOW=0x08, ORANGE=0x10, PURPLE=0x20, UNUSED=0x80.
  That is why a hero can have a "strange" color of 32 — that is PURPLE (the purple player).

- Hero names: if a hero is not renamed, the name comes from the game localization.
  In Russian saves the names are in **CP1251** (single-byte, bytes 0x80–0xFF
  may not be printable ASCII). When scanning, do not discard non-ASCII bytes,
  only zero bytes.

- Recruit heroes (not yet hired) live in the same AllHeroes and have color=NONE(0).
  Their army is also available for editing (the standard starting one).

- A hero's `_id` is the ID from the Heroes list ([§12.3](#123-hero-id-int32-heroes-enum-heroesh)), `_portrait` is the portrait ID
  (matches _id if the portrait was not changed).

## 7. Kingdom

Source: `src/fheroes2/kingdom/kingdom.cpp`. Field order:

| # | Field | Type |
|---|---|---|
| 1 | modes | u32 |
| 2 | _color | u8 PlayerColor (in versions < 10031 it was i32) |
| 3 | resource | Funds/Resource — 7 resources |
| 4 | lost_town_days | i32 |
| 5 | castles | vector<i32> — castle indices |
| 6 | heroes | vector<i32> — **Hero IDs of the kingdom's active heroes** |
| 7 | recruits | Recruits (list available for hiring) |
| 8 | visit_object | list<IndexObject> |
| 9 | puzzle_maps | Puzzle (the obelisk puzzle map) |
| 10 | _visitedTentsColors | i32 |
| 11 | _topCastleInKingdomView | i32 |
| 12 | _topHeroInKingdomView | i32 |
| 13 | _monstersUnderVision | (only ≥ 10034) |

Kingdoms = `u32 count` + Kingdom records.

## 8. Castle (briefly)

Serialized in `vec_castles` (vector<Castle*>). Format in `src/fheroes2/castle/castle.cpp`
(`operator<<`). Contains the owner, buildings (bitmask/array), the garrison army
(Army — same format as [§6.3](#63-army)), the captain (HeroBase), etc. Not required for a hero army
editor; for a full editor — see the castle.cpp sources.

## 9. Settings and Players

`operator<<(OStreamBase&, const Settings&)` (src/fheroes2/system/settings.cpp):

| # | Field | Type |
|---|---|---|
| 1 | _gameLanguage | enum (i32) SupportedLanguage |
| 2 | _currentMapInfo | Maps::FileInfo (the full structure from [§3.2](#32-mapsfileinfo-serialization-order)!) |
| 3 | _gameDifficulty | i32 |
| 4 | game_type | i32 |
| 5 | players | Players (class from kingdom/players.*) |

## 10. GameOver::Result

Source: `src/fheroes2/game/game_over.cpp`:

| # | Field | Type |
|---|---|---|
| 1 | _colors | u8 PlayerColorsSet (in versions < 10031 it was i32) |
| 2 | result | map/set (high score table: names and scores) |

This is exactly where the "high scores" strings live (that is why near the end of the stream
one can encounter names like "Free stuff for computer.").

## 11. CampaignSaveData

Present only in campaign saves (.savc); written after GameOver::Result, before the
0xFF03 marker. Source: `src/fheroes2/campaign/campaign_savedata.cpp`.
Contains campaign data (completed scenarios, awards, hero carry-over information).
**If this block exists, it goes strictly after GameOver::Result and before the marker.**

## 12. Reference tables of constants

### 12.1 Monster ID (int32, `Monster::MonsterType`, monster.h)

| ID | Monster | ID | Monster |
|---|---|---|---|
| 0 | UNKNOWN | 37 | RED_DRAGON |
| 1 | PEASANT | 38 | **BLACK_DRAGON** |
| 2 | ARCHER | 39 | HALFLING |
| 3 | RANGER | 40 | BOAR |
| 4 | PIKEMAN | 41 | IRON_GOLEM |
| 5 | VETERAN_PIKEMAN | 42 | STEEL_GOLEM |
| 6 | SWORDSMAN | 43 | ROC |
| 7 | MASTER_SWORDSMAN | 44 | MAGE |
| 8 | CAVALRY | 45 | ARCHMAGE |
| 9 | CHAMPION | 46 | GIANT |
| 10 | PALADIN | 47 | TITAN |
| 11 | CRUSADER | 48 | SKELETON |
| 12 | GOBLIN | 49 | ZOMBIE |
| 13 | ORC | 50 | MUTANT_ZOMBIE |
| 14 | ORC_CHIEF | 51 | MUMMY |
| 15 | WOLF | 52 | ROYAL_MUMMY |
| 16 | OGRE | 53 | VAMPIRE |
| 17 | OGRE_LORD | 54 | VAMPIRE_LORD |
| 18 | TROLL | 55 | LICH |
| 19 | WAR_TROLL | 56 | POWER_LICH |
| 20 | CYCLOPS | 57 | BONE_DRAGON |
| 21 | SPRITE | 58 | ROGUE |
| 22 | DWARF | 59 | NOMAD |
| 23 | BATTLE_DWARF | 60 | GHOST |
| 24 | ELF | 61 | GENIE |
| 25 | GRAND_ELF | 62 | MEDUSA |
| 26 | DRUID | 63 | EARTH_ELEMENT |
| 27 | GREATER_DRUID | 64 | AIR_ELEMENT |
| 28 | UNICORN | 65 | FIRE_ELEMENT |
| 29 | PHOENIX | 66 | WATER_ELEMENT |
| 30 | CENTAUR | 67 | RANDOM_MONSTER |
| 31 | GARGOYLE | 68 | RANDOM_MONSTER_LEVEL_1 |
| 32 | GRIFFIN | 69 | RANDOM_MONSTER_LEVEL_2 |
| 33 | MINOTAUR | 70 | RANDOM_MONSTER_LEVEL_3 |
| 34 | MINOTAUR_KING | 71 | RANDOM_MONSTER_LEVEL_4 |
| 35 | HYDRA | 72 | MONSTER_COUNT (not a monster) |
| 36 | GREEN_DRAGON | | |

### 12.2 Race (int32, bitmask)

NONE=0x00, KNGT=0x01 (knights), BARB=0x02 (barbarians), SORC=0x04 (sorceresses),
WRLK=0x08 (warlocks), WZRD=0x10 (wizards), NECR=0x20 (necromancers),
MULT=0x40, RAND=0x80.

### 12.3 Hero ID (int32, Heroes enum, heroes.h)

| ID | Hero | ID | Hero | ID | Hero |
|---|---|---|---|---|---|
| 0 | UNKNOWN | 25 | Ariel | 50 | Charity |
| 1 | Lord Kilburn | 26 | Carlawn | 51 | Rialdo |
| 2 | Sir Gallant | 27 | Luna | 52 | Roxana |
| 3 | Ector | 28 | Arie | 53 | Sandro |
| 4 | Gwenneth | 29 | Alamar | 54 | Celia |
| 5 | Tyro | 30 | Vesper | 55 | Roland |
| 6 | Ambrose | 31 | Crodo | 56 | Lord Corlagon |
| 7 | Ruby | 32 | Barok | 57 | Sister Eliza |
| 8 | Maximus | 33 | Kastore | 58 | Archibald |
| 9 | Dimitry | 34 | Agar | 59 | Lord Halton |
| 10 | Thundax | 35 | Falagar | 60 | Brother Brax |
| 11 | Fineous | 36 | Wrathmont | 61 | Solmyr |
| 12 | Jojosh | 37 | Myra | 62 | Dainwin |
| 13 | Crag Hack | 38 | Flint | 63 | Mog |
| 14 | Jezebel | 39 | Dawn | 64 | Uncle Ivan |
| 15 | Jaclyn | 40 | Halon | 65 | Joseph |
| 16 | Ergon | 41 | Myrini | 66 | Gallavant |
| 17 | Tsabu | 42 | Wilfrey | 67 | Elderian |
| 18 | Atlas | 43 | Sarakin | 68 | Ceallach |
| 19 | Astra | 44 | Kalindra | 69 | Drakonia |
| 20 | Natasha | 45 | Mandigal | 70 | Martine |
| 21 | Troyan | 46 | Zom | 71 | Jarkonas |
| 22 | Vatawna | 47 | Darlana | 72 | DEBUG_HERO |
| 23 | Rebecca | 48 | Zam | | |
| 24 | Gem | 49 | Ranloo | | |

HEROES_COUNT = 73 (all AllHeroes records). A hero's default name comes from
`defaultHeroNames` (localized via gettext).

### 12.4 Secondary skills (Skill::Secondary, int32)

UNKNOWN=0, PATHFINDING=1, ARCHERY=2, LOGISTICS=3, SCOUTING=4, DIPLOMACY=5,
NAVIGATION=6, LEADERSHIP=7, WISDOM=8, MYSTICISM=9, LUCK=10, BALLISTICS=11,
EAGLE_EYE=12, NECROMANCY=13, ESTATES=14.
Levels (Skill::Level): NONE=0, BASIC=1, ADVANCED=2, EXPERT=3.

### 12.5 PlayerColor (u8, bitmask)

NONE=0x00, BLUE=0x01, GREEN=0x02, RED=0x04, YELLOW=0x08, ORANGE=0x10, PURPLE=0x20, UNUSED=0x80.
Standard in-game colors: 1 (blue), 2 (green), 4 (red), 8 (yellow), 16 (orange), 32 (purple).

### 12.6 Format versions (save_format_version.h)

| Number | Release |
|---|---|
| 10034 | 1.1.80 (CURRENT at the time of writing) |
| 10033 | 1.1.15 |
| 10032 | 1.1.11 |
| 10031 | 1.1.9 |
| 10030 | 1.1.8 |
| 10010 | 1.0.5 (LAST_SUPPORTED) |

The game will refuse to load a file if the version > CURRENT or < LAST_SUPPORTED.
In the file the version is stored twice: as a string and as a u16.

## 13. Pitfalls (verified in practice)

1. **Big-endian everywhere** — including inside the zlib stream.
2. **string is u32 length + bytes** (not u16 and not null-terminated).
3. **In Skill::Primary the order is: attack, defense, knowledge, power** (not attack/defense/power/knowledge!).
4. **PlayerColor is a bitmask**, not an ordinal color number.
5. Hero names in the Russian localization are CP1251; when scanning, do not filter out the high bit.
6. In campaign saves (.savc), CampaignSaveData follows GameOver::Result — do not confuse its strings with hero data.
7. A save cannot be edited while the game is running (AUTOSAVE will overwrite the changes).
8. Changing the **values** of army slots does not change the size of the uncompressed stream and
   requires no other field edits. When recompressing, zipSize changes — it must be rewritten
   into the block header.
9. The end-of-data marker is `0xFF03` at the end of the uncompressed stream. Useful as an
   integrity check after any edits.
10. `vec_tiles` (map tiles) take ~90% of the stream size and contain many "random" sequences —
    so a "pattern" hero search must use full structural validation ([§6.4](#64-how-to-find-a-specific-hero-scanning-algorithm)), not just a name search.
11. For "sleeping"/inactive heroes `_path` is empty (count=0) and `_visitedObjects` is empty —
    do not be alarmed by zero lengths.
12. When replacing an army slot, keep in mind: the army speed (the slowest unit) affects the
    hero's movement points (`_movePoints`), but they are recalculated by the game on the next
    day; editing them is not required.

## 14. Practical recipes (C++)

The whole core lives in the `fh2core` library (no Qt; [src/savefile.cpp](src/savefile.cpp)/[src/savefile.h](src/savefile.h),
[src/constants.cpp](src/constants.cpp), [src/gettextmo.cpp](src/gettextmo.cpp)). Recipes:

- **Reading/decompressing**: `SaveFile::load(path)` — parses the header, finds the
  zlib block (`findZlibBlock`), scans heroes and players.
- **Editing an army**: `SaveFile::setSlot(hero, slot, monsterId, count)` — writes
  values at the same offsets, the stream size does not change.
- **Editing HeroBase**: `setPrimarySkill`, `setSpellPoints`, `setExperience`,
  `setArtifact`, `setSecondarySkill`, `setRace`, `setPortrait`, `setName`
  (the name — only of the same length!).
- **Saving**: `SaveFile::save()` (recompresses the stream, assembles the file).
- **CLI without GUI**:
  `fheroes2-save-editor --add <file.sav> <hero_name> <monster_id> <count>` —
  writes a troop into a slot with the same monster, otherwise into an empty slot,
  otherwise into the first one; creates a `.bak` backup. Hero names are matched
  by the default names of [§12.3](#123-hero-id-int32-heroes-enum-heroesh), monster IDs are in
  [§12.1](#121-monster-id-int32-monstermonstertype-monsterh). Example:
  `fheroes2-save-editor --add AUTOSAVE.sav Gem 38 10` adds 10 Black Dragons to Gem.
- **Tests**: `FH2_SAVE_DIR=<folder with saves> ctest --test-dir build` — core_test
  runs a round-trip of all setters on real saves (the test suite itself is
  developer-local and runs against your own save files).

## 15. Useful source files

The reference is the fheroes2 repository. The paths below are relative to its root.

| What | File |
|---|---|
| Serializer (base rules) | `src/engine/serialize.h`, `serialize.cpp` |
| Save writing/reading, header | `src/fheroes2/game/game_io.cpp` |
| zlib block | `src/engine/zzlib.h`, `zzlib.cpp` |
| Format versions | `src/fheroes2/system/save_format_version.h` |
| World | `src/fheroes2/world/world.cpp` (operator<< World) |
| Heroes / AllHeroes | `src/fheroes2/heroes/heroes.cpp` (operator<< Heroes, AllHeroes) |
| HeroBase | `src/fheroes2/heroes/heroes_base.cpp` |
| Army / Troops | `src/fheroes2/army/army.cpp`, `army/army_troop.cpp` |
| Kingdom | `src/fheroes2/kingdom/kingdom.cpp` |
| Castle | `src/fheroes2/castle/castle.cpp` |
| FileInfo | `src/fheroes2/maps/maps_fileinfo.cpp` |
| Settings | `src/fheroes2/system/settings.cpp` |
| GameOver::Result | `src/fheroes2/game/game_over.cpp` |
| CampaignSaveData | `src/fheroes2/campaign/campaign_savedata.cpp` |
| Monster ID | `src/fheroes2/monster/monster.h` |
| Hero IDs / names | `src/fheroes2/heroes/heroes.h` (defaultHeroNames) |
| Skill | `src/fheroes2/heroes/skill.h`, `skill.cpp` |
| Race | `src/fheroes2/kingdom/race.h` |
| PlayerColor | `src/fheroes2/kingdom/color.h` |
| IndexObject / ObjectColor | `src/fheroes2/maps/pairs.h` |
| Point | `src/engine/math_base.h` |

## 16. Resource formats (AGG / ICN / KB.PAL)

The project does not depend on the fheroes2 code: game resources are read by the
project's own loader ([src/aggicn.cpp](src/aggicn.cpp)). The formats below were verified by
pixel-by-pixel comparison of the render with a build on the fheroes2 engine
(100% match).

### 16.1 AGG container (HEROES2.AGG / HEROES2X.AGG)

A simple concatenation of files:

| Offset | Field | Type |
|---|---|---|
| 0 | number of files | u16 LE |
| 2 | count × records: hash, offset, size | u32 LE × 3 (12 bytes) |
| end − 15×count | 8.3 ASCIIZ names, 15 bytes per name (13 characters + alignment) | bytes |

- The name hash — `calculateAggFilenameHash` (agg_file.cpp): a backward pass over the
  name, `hash = (hash << 5) + (hash >> 25); sum += c; hash += sum + c;`
  (c — uppercased). A mismatch = the file is corrupted.
- File data — the bytes `[offset, offset + size)`.

### 16.2 ICN sprites

ICN header: u16 count, u32 blockSize, then frames of 13 bytes:
`i16 x, i16 y, u16 w, u16 h, u8 animationFrames, u32 dataOffset` (all LE,
dataOffset — from the start of the data after the header). Frame data size =
`offsetData[next] − offsetData[i]` (or `blockSize − offsetData` for the last one).

Frame data (RLE opcodes, as decodeICNSprite in image_tool.cpp):

| Opcode | Meaning |
|---|---|
| `0x00` | end of row (the rest of the row is transparent) |
| `0x01..0x7F` | N literal pixels (color from the data, transform=0) |
| `0x80` | end of image |
| `0x81..0xBF` | (n−0x80) transparent pixels (transform=1) |
| `0xC0` | transform run: byte b; count = b&3, if 0 — count in the next byte; if b&0x40 — transform = ((b&0x3C)>>2)+2 (2..15, darkening/glow), otherwise transparent; then skip byte b |
| `0xC1` | count in the next byte, then the color (fill, transform=0) |
| `0xC2..0xFF` | count = op−0xC0, then the color (fill, transform=0) |

- `animationFrames & 0x20` — a monochrome sprite: opcodes `0x00`/`0x80`/transparent,
  and `0x01..0x7F` — N **black** pixels (color 0, transform=0).
- transform: 1 — transparent; 0 — the color is taken from the image layer;
  **2..15 — there is NO color**: the engine darkens (2..5) or lightens (6..9)
  the already drawn background using the `transformTable` rows (image.cpp:43).
  This is how frame shadows (BUYBUILD/SURDRBKG/WINLOSE) and letter shadows in
  FONT.ICN are made.
- Our decoder ([src/aggicn.cpp](src/aggicn.cpp)) stores three planes: RGBA (`image`), palette
  indices (`idx`) and **transform (`tf`)**. In RGBA, transform pixels become
  semi-transparent black (2..5) or white (6..9) with alpha `255·(1 − k)`,
  `k = 1.15 − 0.17·t` — visually these are the same shadows. Index assemblies
  (buttons) use the `tf` plane and apply `transformTable` like the engine.
  **Gotcha:** transform pixels must be drawn as shadows, not opaque black —
  otherwise dialogs get black slabs around them instead of shadows, and buttons
  get black dirt around the letters.

### 16.2.1 TIL tiles (GROUND32 / CLOF32 / STON)

Header: `u16 count, u16 width, u16 height`, then `count × width × height`
raw palette indices without transparency (the `GetMaximumTILIndex` port,
game_assets.cpp:5700). Integrity check: `6 + count·w·h == file size`.
The engine additionally keeps 4 "shapes" (shapeId 0..3 — X/Y reflections).

- `CLOF32.TIL` — 4 tiles of 32×32 "fully fogged" (unrevealed) map tile:
  an almost black background (index 35) with rare light and blue dots —
  that very "starry sky". The engine picks the variant `(mp.x + mp.y) % 4`
  (`drawFog`, maps_tiles_render.cpp:709), shapeId = 0.
- `GROUND32.TIL` — 432 tiles of 32×32 (ground), `STON.TIL` — 36 tiles (roads).
- In the save editor `CLOF32` is used as the application background and as a
  backdrop under the hero panel (it is visible through the transparent "windows"
  of HEROBKG); the tile grid is computed from the top-level window coordinates,
  so there is no seam at the panel edge.

### 16.3 KB.PAL palette

768 bytes: 256 colors × 3 channels, **6-bit** (0..63). Normalization — `<< 2`.
Plus copies of the cycling colors (like setGamePalette):
water 231..233, 235 → 246..249; lava 214..217 → 250..253.
