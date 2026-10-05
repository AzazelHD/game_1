# TRPG Game Data Schemas

This document describes the current data contracts used by `game_1`.

## 1) Unit JSON (`assets/units/*.json`)

Parsed by `UnitLoader` into `UnitData`.

### Required fields

- `name` (string)
- `race` (string enum): `Human`, `Elf`, `Elin`, `Undead`
- `gender` (string enum): `Male`, `Female`, `None`

### Optional fields (with defaults)

- `className` (string, default: `Unknown`)
- `spriteSetId` (string, default: empty)
- `acquisitionIndex` (int, default: `0`)
- `level` (int, default: `1`)
- `maxHp` (int, default: `30`)
- `maxMp` (int, default: `8`)
- `attack` (int, default: `10`)
- `defense` (int, default: `5`)
- `magic` (int, default: `5`)
- `magicDefense` (int, default: `5`)
- `moveRange` (int, default: `4`)
- `atkRange` (int, default: `1`)
- `evasion` (int, default: `10`)
- `jump` (int, default: `1`)
- `speed` (int, default: `25`)
- `team` (int, default: `0`)
- `skills` (array of skill IDs, default: empty)
- `baseClass` (string enum, default: `Soldier`; `Soldier`, `Archer`, `Mage`, `Scout`, `Duelist`)
- `promotion` (string enum, default: `None`; additive progression metadata)

Race recruitment eligibility is defined in `RecruitmentRules`:

- Human: Soldier, Archer, Mage
- Elf: Archer, Mage, Scout
- Elin: Archer, Mage, Duelist
- Undead: disabled placeholder for future monster units; not playable recruits

### Example

```json
{
  "name": "Soldier",
  "race": "Human",
  "gender": "Male",
  "level": 1,
  "maxHp": 30,
  "maxMp": 8,
  "attack": 12,
  "defense": 8,
  "magic": 3,
  "magicDefense": 4,
  "moveRange": 4,
  "atkRange": 1,
  "evasion": 10,
  "jump": 2,
  "speed": 20,
  "skills": ["slash", "fireball"]
}
```

## 2) Skill JSON (`assets/skills/*.json`)

Parsed by `SkillLoader` into `SkillData`.

### Required fields

- `id` (string, unique across all loaded skills)
- `name` (string)

### Optional fields (with defaults)

- `description` (string, default: empty)
- `basePower` (int, default: `0`)
- `powerPerLevel` (int, default: `0`) — damage/heal power gained per skill
  **level** above 1. Only meaningful for damage/heal skills; the learned level
  comes from `SkillProgression` (player units), so JSON enemies without
  progression always fight at authored `basePower`. Scaling is applied in
  `BattleState::makeHitContext` (shared by preview, AI and real hits).
- `isMagical` (bool, default: `false`)
- `element` (string enum, default: `neutral`)
  - supported: `fire`, `ice`, `lightning`, `holy`, `dark`, `neutral`
- `skillAccuracy` (int, default: `100`)
- `range` (int, default: `1`)
- `area` (int, default: `0`)
- `mpCost` (int, default: `0`)
- `castOncePerArea` (bool, default: `false`)
- `effectTypes` (array of string enums, default: `["damage"]`)
  - supported values map to: `damage`, `heal`, `buff`, `debuff`, `ko`
  - A skill may declare several; `ko` is fatal and takes precedence over
    `damage`. Skills with **no** `damage` effect show `---` as their damage
    readout in the battle preview.
- `visualOnHit` (string **or** object, default: absent = no animation) — a named
  combat animation played on each target that the skill connects to, **before**
  the skill's consequence is applied. The target stays alive and on screen for
  the duration, which is what lets a lethal effect have a death animation.
  - As a string: `"visualOnHit": "skull"` (duration defaults to 1.0s).
  - As an object:
    - `id` (string) — effect id from `CombatVfxLibrary` (`combat_vfx::*`)
    - `duration` (float, seconds, default `1.0`)
  - Effect ids must name an entry in the built-in VFX table
    (`src/battle/systems/CombatVfxLibrary.cpp`); an unknown id falls back to the
    default expanding ring.
- `neverMiss` (bool, default: `false`) — **debug/scripted only.** The skill
  always lands: evasion, the defender's facing bonus and terrain evasion are all
  bypassed, and the preview reports 100% accuracy.
- `heightTolerance` (int, default: `0`) — max |height diff| between caster and
  target tile; `-1` = ignore height entirely (cast anywhere), `0` = same height only
- `splashHeightTolerance` (int, default: `-1`) — AoE splash tiles may be no more
  than this many levels from the CAST/CENTER tile; `-1` = ignore splash height
  (legacy behavior), `0` = same height as center only
- `statKey` (string enum, default: empty — ignored unless a buff/debuff effect)
  - supported: `attack`, `defense`, `magic`, `magicDefense`, `speed`, `evasion`
- `statAmount` (int, default: `0`) — flat signed stat change (negative = debuff)
- `buffTurns` (int, default: `0`) — duration in the target's own turns; the mod
  ticks down at each start of the target's turn
- `statuses` (array of objects, default: empty) — status effects the skill
  inflicts. Each entry:
  - `type` (string enum): `poison`, `burn`, `stun`, `sleep`, `barrier`,
    `attack_up`, `armor_down`, `attack_down`
  - `turns` (int, default: `0`) — duration in the target's own turns
  - `damagePerTick` (int, default: `0`) — DoT damage per turn (poison/burn)
  - `chance` (int, default: `100`) — 0–100, rolled once per target
  Applied on a **landed hit** for offensive skills and on any **supportive
  cast** target. DoT statuses stack damagePerTick and refresh duration on
  reapplication; stun/sleep consume the target's turn while present and only
  refresh their duration. Sleep additionally breaks on taking real combat
  damage. Buffs (`barrier`, `attack_up`) and curses (`armor_down`,
  `attack_down`) refresh only and modify stats **while present** without
  stacking: buffs grant **+25% of the base stat**, curses **−10% of the
  total stat** (base + race/gender + gear). Statuses are transient battle
  state (not persisted).
- `progressionCategory` (string enum, progression metadata: `active`, `passive`, `reaction`)
- `requiredPromotion` (string enum, progression metadata: `Templar`, `Knight`, `Paladin`)
- `autoGranted` (bool, progression metadata; true for current promotion placeholders)
- `placeholder` (bool, progression metadata; true means content is not balanced)

The `SkillLoader` intentionally ignores progression-only metadata (`SkillData`
stays combat-only). The isolated `SkillProgression` catalog owns eligibility,
point costs, learning, and equip limits for the **active** skills table; battle
units import the roster's learned/equipped actives via
`Unit::importRosterProgression` (which grafts equipped actives' IDs onto the
unit's combat skill list). Passive/reaction skills and per-level power scaling
are deferred to a later milestone.

### Example

```json
{
  "id": "fireball",
  "name": "Fireball",
  "description": "Launches a ball of fire.",
  "basePower": 15,
  "isMagical": true,
  "element": "fire",
  "skillAccuracy": 85,
  "range": 4,
  "area": 1,
  "mpCost": 5,
  "effectTypes": ["damage"],
  "heightTolerance": -1,
  "splashHeightTolerance": 2,
  "requiresLineOfSight": true,
  "splashRespectsWalls": true
}
```

## 3) Battle Definition Data (`config/BattleCatalog.*`)

Currently authored in C++ (not external JSON yet).

`BattleDefinition` includes:

- identity and map path
- camera/background/music defaults
- player and enemy composition
- objectives, rewards (`BattleRewards`: `xp` per-unit victory XP on top of
  `GameConstants::BATTLE_VICTORY_XP`, `gold` granted once to the campaign on
  victory, `items` = gear catalog keys granted one copy each to the shared
  inventory on victory, resolved via `GearCatalog::findByKey`)
- `enemies[]` — each `EnemyDefinition{id, templatePath, level, team}`; `level`
  is honoured for non-roster spawns (`UnitSpawn.level` → `Unit::growToLevel`)
- trigger-action events — `BattleEvent{trigger, minTurn, once, actions[]}`:
  - `trigger` (enum): `OnBattleStart`, `OnUnitDeath`, `OnTurnStart`,
    `OnTurnEnd`, `OnVictory`, `OnDefeat`, `OnTileEnter`
  - `minTurn` (int, default `0`) — event only fires once the battle turn
    counter (`BattleSession::m_turnsElapsed`) reaches this value; used with
    `OnTurnStart`/`OnTurnEnd` to stage reinforcement **waves**
  - `once` (bool, default `false`) — when true the event executes at most once
    even if its trigger fires repeatedly
  - `actions[]` (enum `BattleActionType`): `SpawnUnit` (needs
    `unitTemplatePath`, one unit per action), `ShowDialogue` (`text`),
    `GiveReward` (`value` = XP), `PlayAnimation` (`text` = tag),
    `ModifyVariable`, `StartCutscene`, `EndBattle` (`flag`)
- victory/defeat rule structs (`VictoryConditionType`: `KillAll`, `KillBoss`,
  `SurviveTurns`; `DefeatConditionType`: `AllAlliesDead`, `CriticalUnitDied`)

If this moves to JSON later, this C++ struct layout should be used as the source schema.

## 4) Quest JSON (`assets/quests/*.json`)

Parsed by `QuestLoader` into `QuestDefinition`; runtime state tracked by
`QuestLog` (owned by `PartyContext`).

### Required fields

- `id` (string, unique across all loaded quests)
- `title` (string)
- `objective` (object)

### Optional fields (with defaults)

- `brief` (string, default: empty) — journal one-liner
- `description` (string, default: empty) — journal full-text flavour
- `isMain` (bool, default: `false`) — main vs side quest
- `offeredAtTavern` (bool, default: `false`) — when true the quest is **not**
  auto-accepted on New Game and only appears on the tavern quest board
  (`assets/tavern` quest board). Journey journal hides un-accepted tavern
  quests until picked up.
- `client` (string, default: empty) — name of the quest-giver, shown as the
  speaker on the quest-board info window (title line).
- `cost` (int, default: `0`) — gold fee **charged on accept** from the tavern
  quest board (`0` = free). Main/auto-accepted quests never pay this; the
  board shows "Cost: none" when zero and refuses the purchase with a
  not-enough-gold dialog when the party can't cover it.
- `reward` (object, default: empty) — see below
- `unlocks` (array of ints, default: empty) — world-map node ids gated behind
  completing this quest

### `objective` object

- `type` (string enum, default: `victory`)
  - `victory` — win the battle at `nodeId`
  - `reach` — arrive at `nodeId` (no battle requirement)
- `nodeId` (int, default: `-1`) — world-map node id

### `reward` object

- `gold` (int, default: `0`) — currency granted to the campaign purse
- `xp` (int, default: `0`) — XP granted to each current party member
- `items` (array of gear catalog keys, default: empty) — granted one copy each
  to the shared inventory, resolved via `GearCatalog::findByKey` (mirrors
  `BattleRewards.items`)

### Gating semantics

`QuestLog::isNodeUnlocked(nodeId)` is false when any **incomplete** quest lists
`nodeId` in `unlocks`. Nodes referenced by no quest are always unlocked. Quest
state persists in the campaign save (`formatVersion` 3+, `questLog` payload; current format is v4).

### Example

```json
{
  "id": "first_watch",
  "title": "Claim the First Watch",
  "brief": "Drive off the scouts at the watchtower and secure the approach.",
  "description": "The road ahead is surveyed by an enemy picket. Win the skirmish at the watchtower to open the way inland.",
  "isMain": true,
  "objective": { "type": "victory", "nodeId": 2 },
  "reward": { "gold": 80, "xp": 20, "items": [] },
  "unlocks": [6]
}
```

## 5) Map Data (`assets/maps/*.tmj` + tilesets)

Loaded through engine `TiledJsonLoader` into `TileMapData`, then adapted by game `BattleMap`/`Grid`.

### Required map-level fields (Tiled)

- `width`, `height`
- `tilewidth`, `tileheight`
- `layers`
- `tilesets`

### Supported layer kinds

- `tilelayer`: tile GID matrix for visual and collision metadata lookup
- `objectgroup`: spawn points and markers consumed by game systems

### Objects used by battle setup

Object layer entries are converted to `MapObject` with:

- `id`
- `name`
- `class` (className)
- `x`, `y`
- point/shape type flags

Typical class names include spawn-oriented tags such as player/enemy spawn markers.

### Tileset support

- Embedded or external tilesets are supported by the loader.
- Tile class/type metadata can be used by gameplay systems through engine map data structures.
- The 0.5H iso tileset (`0.5H_IsoTiles.tsx`) types tiles that feed combat:
  `grass` (plain), `grass_eva` (tile ids 16–23 → GIDs 17–24), `grass_mag`
  (ids 24–31 → 25–32), `grass_atk` (ids 32–39 → 33–40). `BattleMap` converts
  the cell's topmost non-zero layer's type to `TileClass` flags, and
  `CombatSystem::applyTileBonuses` consumes them at resolution/preview time:
  attacker ground `grass_atk`/`grass_mag` adds flat attack/magic
  (`TILE_ATK_BONUS` 2 / `TILE_MAG_BONUS` 3), defender ground `grass_eva` adds
  evasion (`TILE_EVA_BONUS` 5). (firstgid is `1`, so GID = tile id + 1.)

## Validation Notes

1. Skill IDs should be unique across all files in `assets/skills`.
2. Unit `skills` values should reference valid skill IDs.
3. Race/gender strings are strict enums; unknown values currently throw load errors.
4. Map object classes should match what battle setup expects, otherwise placements may fail silently.

## 6) Generic Items and Inventory

The inventory subsystem is intentionally independent of gear and equipment
rules. Future gear items and crafting materials can both use the same generic
item definition.

### Item definition (runtime)

`ItemDefinition` contains:

- `id`: unsigned 16-bit item ID (`0` through `65535`)
- `key`: stable internal identifier
- `displayName`: player-facing name
- `description`: optional descriptive text
- `stackable`: whether multiple units may share a stack
- `maxStackSize`: per-stack cap, default `99`
- `goldValue`: merchant worth in gold. The town shop pays this when **buying
  from the player** and charges `goldValue * 2` when **selling to the player**.
  `0` = not sold/bought by the shop (items below this value are filtered from
  the shop lists). Authored in code for every gear item (starter + debug).

### Inventory rules

- Default total capacity is `65536` individual item units.
- Capacity is not a distinct-ID or stack-entry limit.
- A stack of quantity `99` consumes `99` capacity.
- Stackable items may create multiple entries for one item ID, such as `99 + 30`.
- Non-stackable items always use quantity `1` and separate entries.
- Add operations are atomic and fail without modifying the inventory when the
  requested quantity does not fit.
- Gear subclasses, equipment slots, and discard prompts are intentionally not
  part of this subsystem.

### Inventory serialization

The isolated `Inventory::toJson()` format is:

```json
{
  "capacity": 65536,
  "stacks": [
    { "itemId": 100, "quantity": 99 },
    { "itemId": 100, "quantity": 30 },
    { "itemId": 200, "quantity": 1 }
  ]
}
```

Serialization stores item references and quantities only. `fromJson()` uses a
caller-provided item-definition resolver, allowing a future campaign save
system to own the item catalog without coupling it to inventory.

### Campaign ownership and equipment reservations

`PartyContext` owns the shared campaign `Inventory` and `GearCatalog`, plus the
campaign purse `gold` (seeded to `kStartingGold` on New Game, awarded by battle
victory rewards, and serialized with the campaign save). The campaign save
payload stores `gold` as an integer alongside roster/party/inventory; both the
campaign and battle save formats persist it (`formatVersion` 2+, forwards and
backwards compatible). Missing `gold` on a legacy read defaults to `0`.
`formatVersion` 3 adds the `questLog` payload (active + completed quest ids,
see §4). `formatVersion` 4 adds each roster unit's progression fields —
`baseClass` (template-derived `BaseClass` int), `skillPoints` (int), and
`skills` (learned levels + equipped sets, see §SkillProgression). Legacy saves
(v1–v3) load with the template's base class, zero skill points, and nothing
learned.
Each `RosterUnit`, identified by its stable `instanceId`, stores persistent
equipped `ItemId` references for every normal gear slot plus two amulet slots.

Equipped gear is reserved rather than duplicated:

1. Equipping removes exactly one item unit from shared inventory.
2. The roster unit stores its `ItemId` in the selected slot.
3. Replacing gear returns the displaced item to inventory in the same
   `RosterSystem` transaction.
4. Unequipping returns the item only when inventory capacity accepts it.

Consequently an `ItemId` cannot be equipped by two roster units unless the
inventory originally contains more than one unit of that item. Battle `Unit`
objects remain temporary; they receive a resolved Gear-pointer loadout snapshot
from their roster entry when spawned.

## 7) Gear and Equipment Rules

Gear is an additive specialization of `ItemDefinition`; it does not replace
the generic item or inventory systems.

### Gear types

The current subclasses are:

- `Sword`: one-handed, non-ranged weapon
- `Mace`: one-handed, non-ranged weapon
- `Bow`: two-handed, ranged weapon
- `Shield`: off-hand weapon (requires specific main-hand conditions)
- `Helmet`: head armor
- `Chest`: body armor
- `Accessory`: wearable item with subtype (Ring, Necklace, Gloves, Shoes)

All Gear instances are **stackable** (the inherited `ItemDefinition.stackable`
defaults to `true`, `maxStackSize` defaults to `99`): identical gear merges into
one inventory stack (e.g. two rings = a single `x2` entry), and equipping a
unit consumes one unit of the stack while unequipping returns one. Gear does
not add duplicate item or stack fields.

### Gear modifiers and effects

`GearStatModifiers` supplies optional additive integer modifiers, all defaulting
to `0`: `maxHp`, `maxMp`, `attack`, `defense`, `magic`, `magicDefense`,
`evasion`, `speed`, `moveRange`, and `jump`.

When a `Unit` is bound to its externally owned `EquipmentLoadout`, its effective
in-battle stat getters sum base `UnitData`, existing race/gender bonuses, and
all equipped Gear modifiers. This does not add a save/persistence model.

`GearSpecialEffect` is a generic tag enum. Only `TeleportMovement` is currently
implemented. A unit with this effect:

- Uses normal movement budget, terrain cost, map bounds, and jump-height rules.
- Ignores occupied tiles while determining a route to a destination.
- Cannot land on an occupied tile.
- Commits directly to the destination without A\* walking reconstruction or
  walking animation.

No position-derived front/side/rear flanking calculation exists currently;
teleport does not introduce one.

### Data-driven slot rules

`EquipRules` owns race-specific slot configuration separately from Gear class
definitions. Current Human configuration:

| Slot      | Capacity | Rule                                                                                                 |
| --------- | -------- | ---------------------------------------------------------------------------------------------------- |
| Weapon    | 1        | Main-hand weapon (melee or ranged)                                                                   |
| Offhand   | 1        | Shield (requires main-hand: one-handed, non-ranged) OR secondary weapon (one-handed non-ranged only) |
| Head      | 1        | Helmet                                                                                               |
| Body      | 1        | Chest armor                                                                                          |
| Accessory | 2        | Each slot accepts Ring, Necklace, Gloves, or Shoes                                                   |

The Offhand slot has special eligibility rules:

- If the main-hand Weapon is two-handed or ranged, the Offhand slot is disabled
- If the main-hand Weapon is one-handed and non-ranged, the Offhand slot accepts either a Shield or a secondary one-handed non-ranged Weapon
- Dual-wielding two one-handed weapons is currently allowed via placeholder implementation; future versions may add class/skill gating

Replacing a weapon while a shield is equipped is rejected unless the new weapon supports that shield. Other races currently have disabled placeholder configs until their equipment rules are designed.

`UnitDetailWindow` provides interactive equipment management through a three-state UI:

- Details state: displays unit info and action menu (Equip, Abilities, Dismiss)
- SlotSelect state: shows 6 equippable positions with current items
- ItemSelect state: filters inventory by slot and shows list view or stat-delta preview

`RosterSystem` consumes these rules through atomic equip/unequip transactions. A resolved `EquipmentLoadout` remains a transient view; campaign saves persist roster equipment and the shared inventory (see Campaign ownership above).
It shows a unit portrait, name, level, EXP, class, effective core stats, and
every equipment slot, including empty slots. Battle inspection remains separate.

Elf and Elin currently reuse the Human slot shape. Numeric Elf/Elin static and
growth biases, and all non-Human promotion growth/skills, are clearly marked as
placeholder balance content in code pending a balance pass. Elin Archer/Mage
promotion names intentionally reuse the Elf placeholder names.

## 8) Shop data (`assets/shop/stock.json`)

Parsed by `ShopStock::load` into a list of gear catalog **keys**. The town
shop's Buy list resolves each key through `GearCatalog::findByKey()`; keys that
don't resolve in the current build (e.g. debug gear in Release) are skipped
silently. Prices are derived from `ItemDefinition.goldValue` (§6), never stored
in this file.

```json
{
  "stock": ["starter_sword", "starter_shield"]
}
```

- Buy price per item      = `goldValue * 2`
- Sell price per item     = `goldValue`
- Items with `goldValue <= 0` never appear on either list.

## 9) Tavern data (`assets/tavern/*.json`)

Parsed by `TavernData` into `TavernCandidate` / `TavernLine` structs.

### `candidates.json` — recruitable units

```json
{
  "candidates": [
    {
      "templatePath": "assets/units/soldier.json",
      "customName": "Kael",
      "cost": 150,
      "flavor": "A veteran of the border wars..."
    }
  ]
}
```

- `templatePath` — unit JSON used to build the recruit (see §1).
- `customName` — recruit's display name (overrides the template default name).
- `cost` — gold required to hire; hiring spends from the campaign purse.
- `flavor` — one-line description used by the UI.
- A candidate is hidden from the list once a roster unit with the same
  `templatePath` + `customName` exists (recruit-once, no save-format change).
- Hiring appends a fresh `RosterUnit` and adds it to the active party when the
  party has fewer than 6 members.

### `lore.json` — bar-room dialogue

```json
{
  "lines": [
    { "speaker": "Barkeep", "text": "The Eastern Reach has been quiet lately." }
  ]
}
```

- `speaker` — name shown as the dialog speaker.
- `text` — line of flavor dialogue. All lines play sequentially when the player
  picks Talk in the tavern.

## 10) Town talk (`assets/town/talk.json`)

Same shape as tavern `lore.json` (`{ "lines": [{ "speaker", "text" }] }`), loaded
with `TavernData::loadLore`. These are the townsfolk lines played when the
player picks **Talk** on the town-hub menu.
