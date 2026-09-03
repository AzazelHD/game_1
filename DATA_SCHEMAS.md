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
- `isMagical` (bool, default: `false`)
- `element` (string enum, default: `neutral`)
  - supported: `fire`, `ice`, `lightning`, `holy`, `dark`, `neutral`
- `skillAccuracy` (int, default: `100`)
- `range` (int, default: `1`)
- `area` (int, default: `0`)
- `mpCost` (int, default: `0`)
- `castOncePerArea` (bool, default: `false`)
- `effectTypes` (array of string enums, default: `["damage"]`)
  - supported values map to: `damage`, `heal`, `buff`, `debuff`
- `progressionCategory` (string enum, progression metadata: `active`, `passive`, `reaction`)
- `requiredPromotion` (string enum, progression metadata: `Templar`, `Knight`, `Paladin`)
- `autoGranted` (bool, progression metadata; true for current promotion placeholders)
- `placeholder` (bool, progression metadata; true means content is not balanced)

The current `SkillLoader` intentionally ignores progression-only metadata. The
isolated `SkillProgression` catalog owns eligibility, point costs, learning,
promotion grants, and equip limits until combat/UI wiring is requested.

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
  "effectTypes": ["damage"]
}
```

## 3) Battle Definition Data (`config/BattleCatalog.*`)

Currently authored in C++ (not external JSON yet).

`BattleDefinition` includes:

- identity and map path
- camera/background/music defaults
- player and enemy composition
- objectives, rewards
- trigger-action events
- victory/defeat rule structs

If this moves to JSON later, this C++ struct layout should be used as the source schema.

## 4) Map Data (`assets/maps/*.tmj` + tilesets)

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

## Validation Notes

1. Skill IDs should be unique across all files in `assets/skills`.
2. Unit `skills` values should reference valid skill IDs.
3. Race/gender strings are strict enums; unknown values currently throw load errors.
4. Map object classes should match what battle setup expects, otherwise placements may fail silently.

## 5) Generic Items and Inventory

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

`PartyContext` owns the shared campaign `Inventory` and `GearCatalog`.
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

## 6) Gear and Equipment Rules

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

All Gear instances force the inherited `ItemDefinition.stackable` to `false`
and the inherited `maxStackSize` to `1`. Gear does not add duplicate item or
stack fields.

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

`RosterSystem` consumes these rules through atomic equip/unequip transactions. A resolved `EquipmentLoadout` remains a transient view; campaign save/load serialization remains deferred.
It shows a unit portrait, name, level, EXP, class, effective core stats, and
every equipment slot, including empty slots. Battle inspection remains separate.

Elf and Elin currently reuse the Human slot shape. Numeric Elf/Elin static and
growth biases, and all non-Human promotion growth/skills, are clearly marked as
placeholder balance content in code pending a balance pass. Elin Archer/Mage
promotion names intentionally reuse the Elf placeholder names.
