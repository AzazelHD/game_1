# AGENTS.md — `game_1` (TRPG Game Layer)

Instructions for AI coding agents (opencode / Copilot / similar) working on
the `game_1` project. This file is the **single source of truth** for agent
behavior on this repo. Update it when conventions change.

`game_1` is the tactical RPG game layer that consumes the `engine` through
its public headers. All gameplay logic lives here, never in the engine.

---

## 1. Project Layout

```
game_1/
├── ARCHITECTURE.md          # authoritative module/ownership map
├── PLAN.md                  # mission, status, done-vs-pending
├── DATA_SCHEMAS.md          # JSON + struct data contracts
├── UNIT_DESIGN.md           # progression design (deferred)
├── RACE_DESIGN.md           # race/class eligibility
├── TODO.md                  # active bugs, regressions, polish
├── CHANGELOG.md             # milestone log
├── USEFUL_COMMANDS.txt      # build/run commands
├── CMakeLists.txt           # orchestrator only
├── game/CMakeLists.txt      # SINGLE source of truth for game sources
├── tools/build_debug.bat
├── tools/build_release.bat
└── game/
    ├── assets/              # maps, units, skills, fonts, sprites, audio
    └── src/
        ├── main.cpp
        ├── scenes/          # BootState, MainMenuState, WorldMapState,
        │                    # BattleState, ResultState
        ├── battle/          # BattleSession, controllers/, combat/, map/,
        │                    # unit/, systems/, ui/
        ├── ai/              # EnemyAI
        ├── config/          # BattleCatalog, GameConstants
        ├── data/            # SettingsManager, UnitLoader, SkillLoader
        ├── events/          # BattleEventSystem
        ├── inventory/       # Item, Inventory, Gear, EquipRules,
        │                    # GearCatalog
        ├── renderer/        # BattleRenderer, BattleRendererContext
        ├── scenes/BattleLoader.{h,cpp}
        ├── settings/        # SettingsState, Graphics/Audio, SettingsRowWindow
        ├── systems/         # RosterSystem, PartySystem, PartyContext
        ├── ui/              # UIManager, Cursor, DamagePreview, theme,
        │                    # scaling, floating text, shared windows
        └── world/           # WorldGraph, WorldPathfinding
```

When working in `game/src/`, mirrors `engine/include/` (engine public headers
are pulled in via `../engine/include` include path; see `game/CMakeLists.txt`).

---

## 2. Hard Architectural Rules

These are non-negotiable. Violating them breaks the engine/game boundary
and the modular design.

1. **Engine boundary**
   - Game code uses **engine public headers only**.
   - Never modify anything under `engine/`. All gameplay logic stays in
     `game/src/`.
   - Never duplicate engine-side logic in `game/src/`; call the engine API
     instead.

2. **`UIManager` is the sole UI authority**
   - It is the single source of truth for modal / input-blocking behavior.
   - Controllers and states must react to drained `UIEvent`s, not poll window
     internals.
   - If `UIManager::hasBlockingWindow()` is true, tactical input must not
     progress combat state.

3. **`BattleState` is an orchestration shell**
   - New behavior goes into focused controllers/systems first.
   - Do not pile new responsibilities directly into `BattleState.cpp`.

4. **Render passes**
   - World-layer effects (`CombatAnimationSystem::render`, `DebugRenderer::flush`)
     go through the **world/logical pass** with `beginLogicalPass()` /
     `endLogicalPass()`.
   - Native UI / pixel-aligned overlays (e.g. `DamagePreview`) go through the
     **native pass** with `toNativeRect` / `toNativePos`.
   - Text must remain crisp: do not reintroduce linear-filter stretching of a
     low-res canvas.

5. **Headless / cache contract**
   - `game/CMakeLists.txt` is the **only** place that lists game sources and
     headers. Adding a new `.cpp` or `.h` requires editing it explicitly —
     globbing is intentional anti-pattern here.

6. **Public include tree policy**
   - `src/` is the active implementation tree for the game target.
   - Do **not** introduce an `include/` tree unless a real public game API
     requires it. Prefer keeping headers under `src/` next to their `.cpp`.

---

## 3. Module Ownership (quick reference)

For detailed module responsibilities see `ARCHITECTURE.md`. Quick roles:

- `scenes/` — top-level state flow (`BootState` → `MainMenuState` →
  `WorldMapState` → `BattleState` → `ResultState`).
- `battle/BattleSession` — owns runtime units, turn queue integration,
  victory/defeat evaluation.
- `battle/controllers/` — `HumanTurnController`, `BattleMenuController`,
  `AttackResolutionController`, `DeploymentPhaseController`,
  `DialogueController`, `MovementAnimationController`.
- `battle/combat/` — hit formulas (`CombatSystem`), turn queue, pending targets.
- `battle/map/` — `Grid`, `BattleMap`, `MovementRange`, `Pathfinder`, `GameTile`.
- `battle/unit/` — runtime `Unit`, `UnitData`, `UnitProgression`,
  `SkillProgression`, `RecruitmentRules`, `UnitFactory`.
- `battle/systems/` — deployment, combat animation queue,
  `BattleParticipantsBuilder`.
- `battle/ui/` — battle-specific windows (deployment, inspect, panel).
- `ai/EnemyAI` — enemy movement + attack plan/act.
- `config/BattleCatalog` — static battle definitions, per-map rules/events.
- `config/GameConstants` — viewport / shared game constants.
- `data/` — `SettingsManager`, `UnitLoader`, `SkillLoader`.
- `events/BattleEventSystem` — trigger→action dispatcher
  (`OnTurnStart`, `OnUnitDeath`, `OnVictory`, …).
- `renderer/BattleRenderer` + `BattleRendererContext` — read-only render
  contract.
- `inventory/` — `Item`, `Inventory`, `Gear`, `EquipRules`, `GearCatalog`.
- `systems/` — `RosterSystem` (recruitable + persistent equipped `ItemId`),
  `PartySystem` (active battle/world subset), `PartyContext` (campaign-lifetime
  owner of roster + party + shared `Inventory` + `GearCatalog`).
- `ui/` — `UIManager`, `Cursor`, `DamagePreview`, `FloatingTextSystem`,
  `UITheme`, `UIScale`, `UIUtils`, `UIWindow`, shared widgets.
- `ui/windows/` — `ButtonMenuWindow`, `ConfirmWindow`, `DialogWindow`,
  `InventoryWindow`, `PartyWindow`, `UnitDetailWindow`.
- `settings/` — `SettingsState`, `GraphicsSettings`, `AudioSettings`,
  `SettingsRowWindow`.
- `world/` — `WorldGraph`, `WorldPathfinding`.

---

## 4. Battle Flow (ownership chain)

When changing battle behavior, the canonical flow is:

1. `BattleState` enters deployment mode.
2. `DeploymentPhaseController` validates placement and starts combat.
3. `BattleSession` drives active unit and queue state.
4. `HumanTurnController` (or `EnemyAI` on enemy turn) chooses actions.
5. `AttackResolutionController` resolves hit sequence and state transitions.
6. `BattleEventSystem` emits side effects (dialogue, rewards, spawns,
   end-battle).
7. On end → `ResultState` → return to world map.

Turn budget semantics (see `battle/unit/Unit.h` header comment for canonical
doc):

- **Movement** is an independent points pool; multi-step moves allowed in the
  same turn as long as points remain and no MajorAction has been taken since
  the last move.
- **MajorAction**: at most ONE of `Attack` / `Skill` / `Defend` per turn. Once
  taken, undo of earlier movement is disabled, but `Attack`/`Skill` still
  allow moving afterward.
- Only `Wait` and `Defend` end a turn. `Defend` counts as the MajorAction AND
  ends the turn immediately (no post-Defend movement).
- Running out of movement or taking the MajorAction does NOT auto-end the
  turn; `BattleState` must receive an explicit `Wait` or `Defend`.

---

## 5. Turn System Rules

- All units act via a single turn queue; humans use `HumanTurnController`,
  enemies use `EnemyAI`.
- Dead units are filtered out of the queue immediately.
- Movement cost is **real path cost** from the reachable cost map, not
  Manhattan distance. Do not reintroduce Manhattan-based movement cost.
- Cursor resets to the active unit when the battle menu reopens or after
  attack resolution commits.
- Pre-battle scripted dialogue hooks exist in `DialogueController`; the
  always-on sample sequence is still pending.

---

## 6. Data Contracts

Authoritative schemas live in `DATA_SCHEMAS.md`. Summary:

- **Units** (`assets/units/*.json`) → `UnitData` via `UnitLoader`.
  Required: `name`, `race` (`Human`/`Elf`/`Elin`/`Undead`), `gender`
  (`Male`/`Female`/`None`). Optional: stat fields with documented defaults,
  `skills` (IDs), `baseClass`, `promotion`. Race eligibility per
  `RecruitmentRules`.
- **Skills** (`assets/skills/*.json`) → `SkillData` via `SkillLoader`.
  Required: `id` (unique), `name`. Optional: `basePower`, `isMagical`,
  `element`, `skillAccuracy`, `range`, `area`, `mpCost`, `castOncePerArea`,
  `effectTypes`, plus progression metadata (`progressionCategory`,
  `requiredPromotion`, `autoGranted`, `placeholder`). `SkillLoader`
  intentionally ignores progression-only metadata — owned by
  `SkillProgression` catalog.
- **Maps** (`assets/maps/*.tmj`/`.tmx`) → loaded via engine `TiledJsonLoader`,
  adapted by `BattleMap`/`Grid`. Object layer entries become `MapObject` with
  `id`, `name`, `class`, `x`, `y`, point/shape flags.
- **Battle definitions** are currently authored in C++ via `BattleCatalog`.
  If/when migrated to JSON, the existing C++ struct layout is the source schema.
- **Inventory / Items**:
  - `ItemDefinition`: `id` (uint16, 0–65535), `key`, `displayName`,
    `description`, `stackable`, `maxStackSize` (default 99).
  - Default capacity 65536 individual item units; capacity is per-unit, not
    per-stack or per-ID.
  - Add operations are atomic (fail without mutating if quantity doesn't fit).
  - Gear subclasses (`Sword`, `Mace`, `Bow`, `Shield`, `Helmet`, `Chest`,
    `Accessory`) force `stackable=false` and `maxStackSize=1`.
  - `GearStatModifiers`: additive integer modifiers, all default to 0.
  - `GearSpecialEffect`: only `TeleportMovement` is implemented.
  - `EquipRules`: race-specific slot configuration; Human default is
    Weapon×1, Offhand×1 (Shield OR secondary 1H non-ranged), Head×1,
    Body×1, Accessory×2. Elf/Elin reuse Human shape with placeholder biases.
  - `RosterSystem` equip/unequip is atomic; one shared inventory, equipped
    gear is **reserved not duplicated** (equipping removes one unit, replacing
    returns the displaced item in the same transaction).
  - Battle `Unit` objects are temporary and receive a resolved
    `EquipmentLoadout` snapshot from their roster entry on spawn.
- **Item serialization** (`Inventory::toJson()`):
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
  `fromJson()` requires a caller-provided item-definition resolver
  (catalog ownership belongs to the future save system, not inventory).

---

## 7. Code Style and Conventions

Observed in this repo:

- **Language standard**: C++20 (`set(CMAKE_CXX_STANDARD 20)`).
- **Headers**: `#pragma once`; include order: STL → engine headers
  (`engine/...`) → local game headers (`battle/...`, `ui/...`).
- **Naming**:
  - Types: `PascalCase` (`BattleSession`, `HumanTurnController`,
    `UnitData`, `MajorAction`).
  - Enums: `enum class` with `PascalCase` values (`MajorAction::Attack`).
  - Methods / functions: `camelCase` (`takeDamage`, `getMoveRangeLeft`,
    `hasActed`).
  - Members: `m_camelCase` (`m_moveRangeLeft`, `m_currentHp`).
  - Constants / statics: `PascalCase` or `kCamelCase` (match nearest file).
- **File layout**: `.h` and `.cpp` colocated in the same module subfolder;
  one type per file pair unless clearly cohesive (e.g. `Item`/`Inventory`/
  `Gear`/`EquipRules`/`GearCatalog` form one subsystem).
- **Forward declarations**: prefer them over extra includes in headers.
- **Comments**: do **not** add comments unless asked. Existing files contain
  explanatory header comments documenting turn-budget semantics and the
  engine boundary — preserve those when editing nearby code.
- **No magic strings**: skill IDs, item IDs, and unit names flow through
  data loaders; do not hardcode them in controllers.
- **Strict enums**: race/gender are strict enums; unknown values currently
  throw load errors. Keep that contract.

---

## 8. Build, Run, Verify

Build flows (full recipes in `USEFUL_COMMANDS.txt`):

```bat
:: DEV flow
cd /d H:\Coding\Games\TRPG\engine
tools\build_debug.bat

cd /d H:\Coding\Games\TRPG\game_1
tools\build_debug.bat
```

```bat
:: FINAL flow
cd /d H:\Coding\Games\TRPG\engine
tools\build_release.bat
tools\vsenv.bat cmake --install build --config Release

cd /d H:\Coding\Games\TRPG\game_1
tools\build_release.bat
```

```bat
:: Run debug build
H:\Coding\Games\TRPG\game_1\build\debug\game\Debug\game.exe
```

Clean configure when cache is stale:

```bat
cd /d H:\Coding\Games\TRPG\game_1
if exist build\debug   rmdir /s /q build\debug
if exist build\release rmdir /s /q build\release
```

Engineering rules to enforce:

- Compile after every architectural change (`TODO.md`).
- Maintain zero-warning build where practical.
- Keep at least one map fully playable at all times.

---

## 9. Required Verification After Changes

After any code change, before declaring done:

1. **Reconfigure + build** with the matching DEV or FINAL preset. A change
   that breaks the build is not "done".
2. **Smoke run** (manually, when feasible):
   `Boot → Main Menu → World Map → Deployment → Combat → Attack → Victory
   → Return`. This is still listed as TODO verification in `TODO.md`.
3. **Camera regression**: windowed + borderless, deployment + combat,
   cursor at all four map corners, clamping behavior.
4. **Button menu verification**: marker positioning, width sizing,
   alignment across action/skill/system/inspect menus.
5. **Render-pass spot-check** when touching `BattleState::render()` or any
   code that draws via `BattleRenderer`: text stays crisp, coordinate
   spaces match the pass (`logical` vs `native`).

If you cannot run the full smoke run in the current environment, say so
explicitly in the PR/change description rather than claiming "done".

---

## 10. What Is Deferred — Do NOT Build

These are explicitly out of scope for the current vertical slice. Do not
implement them unless the user explicitly asks:

- Full campaign save/load serialization for roster equipment, inventory,
  and EXP (`ARCHITECTURE.md` § Current Gaps).
- Monster / `Undead` units as player recruits. `Undead` is a placeholder
  for future monster units only (FFTA-style fixed movesets, likely
  different slot model). Do not build monster unit / skill tree / equip
  rules yet (`RACE_DESIGN.md` § Not yet defined).
- Elin-specific Archer/Mage specialization names (currently reusing Elf
  placeholders).
- Races beyond Human/Elf/Elin.
- Unit progression point investment / leveling / promotion wiring beyond
  the existing `SkillProgression` placeholder catalog (`UNIT_DESIGN.md` is
  the design doc but progression is intentionally deferred —
  `SkillLoader` ignores `progressionCategory`, `requiredPromotion`,
  `autoGranted`, `placeholder`).
- Combat animation and VFX package beyond the current
  `CombatAnimationSystem` queue + placeholder rendering.
- City/NPC interactions and full recruitment UX.
- Multi-cast skill chains, status effect pipelines.

---

## 11. Coding Workflow Expectations

For any non-trivial task the agent should:

1. **Read first**: skim `ARCHITECTURE.md`, then the relevant module's `.h`
   before touching `.cpp`. Mirror existing style.
2. **Plan**: for multi-step work, write a short plan before edits and
   prefer focused PR-sized changes.
3. **Localize**: change the smallest set of files. New behavior goes into
   the matching controller/system; do not extend `BattleState.cpp` unless
   the change is genuinely orchestration-level.
4. **Wire CMake explicitly**: when adding/removing a source or header,
   edit `game/CMakeLists.txt`. Re-run the configure + build.
5. **Cite locations**: when explaining changes, use
   `game/src/path/file.cpp:line` references.
6. **Do not commit** unless explicitly asked. Inspect `git status` and
   `git diff` first.
7. **Do not** touch secrets, git config, hooks, or push without an
   explicit instruction.

---

## 12. Agent Anti-Patterns (do not do this)

- Editing `engine/` to make a game-side change easier.
- Adding new render passes or bypassing `beginLogicalPass()`/
  `endLogicalPass()` for world-layer effects.
- Polling window internals from controllers instead of draining
  `UIEvent`s.
- Putting new logic directly into `BattleState.cpp` when a controller or
  system exists for it.
- Hardcoding skill IDs, item IDs, or unit names inside controllers.
- Reverting cursor reset after attack resolution or menu re-entry.
- Using Manhattan distance instead of real path cost for movement.
- Adding an `include/` tree under `game/` "just in case".
- Globbing the source tree in `game/CMakeLists.txt`.
- Adding code comments to changed lines unless explicitly asked (matches
  global agent instruction).
- Implementing deferred features (progression wiring, monster units, save
  serialization, multi-cast chains) without an explicit ask.

---

## 13. Documentation Map

When the agent needs ground truth, read in this order:

1. `ARCHITECTURE.md` — module ownership and contracts.
2. `PLAN.md` — current vertical slice status, done vs pending.
3. `DATA_SCHEMAS.md` — JSON / struct contracts.
4. `TODO.md` — active bugs and regressions.
5. `CHANGELOG.md` — what shipped and when.
6. `UNIT_DESIGN.md` — progression design (deferred, do not implement).
7. `RACE_DESIGN.md` — race/class eligibility (deferred, do not implement).
8. `USEFUL_COMMANDS.txt` — build/run commands.
9. Engine architecture: `engine/ARCHITECTURE.md`.

Update `TODO.md` and `CHANGELOG.md` when finishing items. Update
`ARCHITECTURE.md` when ownership/responsibilities change. Update
`DATA_SCHEMAS.md` when a JSON contract or runtime struct shape changes.