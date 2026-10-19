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
├── AGENTS.md                # agent rules (this file; tools require it at root)
├── README.md                # project overview + doc index
├── docs/                    # all prose (section 13 has the map)
│   ├── ARCHITECTURE.md      # authoritative module/ownership map
│   ├── ROADMAP.md           # mission, slice status, remaining work
│   ├── DATA_SCHEMAS.md      # JSON + struct data contracts
│   ├── ENGINE_API.md        # engine public surface cheat-sheet
│   ├── TUNING.md            # where to change a tunable value
│   ├── TODO.md              # live bugs, verification, QA backlog
│   ├── CHANGELOG.md         # milestone log
│   ├── design/              # UNIT_PROGRESSION.md, RACES.md, FRAMED_LAYOUT.md
│   └── archive/             # MILESTONES.md, RESOLVED_ISSUES.md
├── graphify-out/            # knowledge graph (graph.json, GRAPH_REPORT.md,
│                            # .graphify_labels.json); regenerate, never hand-edit
├── CMakeLists.txt           # orchestrator only
├── game/CMakeLists.txt      # SINGLE source of truth for game sources
├── tools/build_debug.bat
├── tools/build_release.bat
└── game/
    ├── assets/              # maps, units, skills, quests, shop, tavern, town,
    │                        # intro, fonts, sprites, audio
    └── src/
        ├── main.cpp
        ├── scenes/          # BootState, IntroState, MainMenuState,
        │                    # WorldMapState, BattleState, ResultState,
        │                    # BattleLoader
        ├── battle/          # BattleSession, controllers/, combat/, map/,
        │                    # unit/, systems/, ui/
        ├── ai/              # EnemyAI
        ├── config/          # BattleCatalog, GameConstants, UiTuning
        ├── data/            # SettingsManager, UnitLoader, SkillLoader,
        │                    # SaveManager, QuestData/QuestLoader, ShopStock,
        │                    # TavernData
        ├── events/          # BattleEventSystem
        ├── inventory/       # Item, Inventory, Gear, EquipRules,
        │                    # GearCatalog
        ├── renderer/        # BattleRenderer, BattleRendererContext,
        │                    # BattleWorldProjection
        ├── settings/        # SettingsState, Graphics/Audio, SettingsRowWindow
        ├── systems/         # RosterSystem, PartySystem, PartyContext,
        │                    # QuestLog
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
     `endLogicalPass()`. The world pass now renders into a **native-sized buffer**
     set by `Renderer::setWorldBufferSize()` (the framed layout's play rect) and
     is blitted 1:1, so culling and the backdrop must use the **camera viewport**
     (`Camera::getViewportSize()`), never `GameConstants::VIEW_*`.
   - Native UI / pixel-aligned overlays (e.g. `DamagePreview`) go through the
     **native pass** with `toNativeRect` / `toNativePos` (the 1280×720 design →
     window transform).
   - The world pass ends with `endWorldPass()`; a full-window **backdrop** (the
     map's sky, later parallax) is drawn natively between it and
     `blitWorldBuffer()`, and content outside the design play field (the
     permanent battle **rails** frame) draws in raw output pixels via
     `beginNativeOverlay()` / `endNativeOverlay()`. Prefer `endLogicalPass()`
     only when there is no backdrop to interleave. See `docs/design/FRAMED_LAYOUT.md`
     and `docs/ARCHITECTURE.md` → "Render Pipeline".
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
  `IntroState` (New Game narrator → tutorial duel) → `WorldMapState` →
  `BattleState` → `ResultState`). `BattleLoader` builds battles from a
  `BattleRequest`.
- `battle/BattleSession` — owns runtime units, turn queue integration,
  victory/defeat evaluation.
- `battle/controllers/` — `HumanTurnController`, `BattleMenuController`,
  `AttackResolutionController`, `DeploymentPhaseController`,
  `DialogueController`, `MovementAnimationController`.
- `battle/combat/` — hit formulas (`CombatSystem`), turn queue, pending targets.
- `battle/map/` — `Grid`, `BattleMap`, `MovementRange`, `Pathfinder`, `GameTile`.
- `battle/unit/` — runtime `Unit`, `UnitData`, `UnitProgression`,
  `SkillProgression` (skill points/unlocks/levels), `StatusEffect`
  (statuses, buffs/debuffs), `RecruitmentRules`, `UnitFactory`.
- `battle/systems/` — deployment, combat animation queue, `CombatVfxLibrary`,
  `MovementCommitSystem`, `BattleParticipantsBuilder`,
  `BattleCameraController` (the single battle camera authority: a central 80%
  **camera dead zone** keyed on the cursor's height-adjusted walkable floor
  surface — a minimum-displacement pan only when the surface leaves it,
  per-axis gated so an axis the board fits on never pans, plus the cast/target
  focus beats).
- `battle/ui/` — battle-specific windows (deployment, inspect, panel).
- `ai/EnemyAI` — enemy movement + attack plan/act.
- `config/BattleCatalog` — static battle definitions, per-map rules/events.
- `config/GameConstants` — viewport / shared game constants.
- `config/UiTuning` — designer-facing tuning knobs (camera dead zone, cursor
  marker, rails); documented in `TUNING.md`.
- `data/` — `SettingsManager`, `UnitLoader`, `SkillLoader`, `SaveManager`
  (campaign saves, format v4), `QuestData`/`QuestLoader`, `ShopStock`,
  `TavernData`.
- `events/BattleEventSystem` — trigger→action dispatcher
  (`OnTurnStart`, `OnUnitDeath`, `OnVictory`, …).
- `renderer/BattleRenderer` + `BattleRendererContext` — read-only render
  contract; `BattleLayout` — framed rails/play-rect/integer art-scale math;
  `BattleWorldProjection` — shared tile→screen anchor math.
- `inventory/` — `Item`, `Inventory`, `Gear`, `EquipRules`, `GearCatalog`.
- `systems/` — `RosterSystem` (recruitable + persistent equipped `ItemId`),
  `PartySystem` (active battle/world subset), `PartyContext` (campaign-lifetime
  owner of roster + party + shared `Inventory` + `GearCatalog` + gold),
  `QuestLog` (active/completed quest state).
- `ui/` — `UIManager`, `Cursor`, `DamagePreview`, `FloatingTextSystem`,
  `UITheme`, `UIScale`, `UIUtils`, `UIWindow`, `UIKeyRepeat`, `UnitPortrait`,
  `ConfirmDialogs`, shared widgets.
- `ui/windows/` — `ButtonMenuWindow`, `ConfirmWindow`, `DialogWindow`,
  `InventoryWindow`, `PartyWindow`, `ShopWindow`, `SkillProgressionWindow`,
  `UnitDetailWindow`.
  `ButtonMenuWindow` supports `setScrollable(false)` (panel grows to fit every
  row, nothing scrolls) and `setWrappable(false)` (focus stops at the ends,
  instead of cycling to the other end); the world-map main menu uses
  `setScrollable(false)` and keeps the default wrapping.
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

Facing semantics (see `battle/combat/CombatSystem.h`):

- Facings are the four iso diagonals (`IsoDirection`): NE/SE/SW/NW, with tile
  deltas NE=(0,-1), SE=(1,0), SW=(0,1), NW=(-1,0).
- Attack side is always measured against the **defender's** facing:
  `CombatSystem::attackSideFrom(attacker, defender)` → `Front` (×0.90 dmg,
  +0 ACC), `Side` (×1.0, +10 ACC), `Rear` (×1.10, +20 ACC). Supportive
  (heal/buff) skills stay `Side`. Basic attacks, offensive skills, AI, and
  the damage preview must share this single computation.
- Facing input mirrors `Cursor::readMovementInput` (Up/W→NW, Right/D→NE,
  Down/S→SE, Left/A→SW). Only "Wait" lets you re-face in combat (no Back);
  deployment facing choice can be undone with Back/ESC, which grabs the unit
  back onto the cursor.

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
  tutorial duel uses contextual one-line tips and other battles show the
  static controls dialog on first combat.

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
  `effectTypes`, `heightTolerance`, `splashHeightTolerance` (`-1` = AOE splash
  unconstrained by height), `requiresLineOfSight`, `splashRespectsWalls`,
  `statKey`, `statAmount`, `buffTurns`,
  plus progression metadata (`progressionCategory`,
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
    `Accessory`) are **stackable** (`maxStackSize` defaults to 99): identical
    gear merges into one inventory stack (e.g. two rings = single `x2` entry),
    each equip consumes one unit, unequip returns one. Fully-equipped items
    (0 free copies) still appear in the equip list, grayed-out and sorted
    alphabetically with the rest; the rows stay focusable but Enter is a
    no-op.
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
- **Arrow/WASD parity**: arrow keys and WASD are equivalent for every
  direction-using control (cursor, facing choices, menu navigation, lists).
  Check **both** `KeyCode`s at every call site (`Up`/`W`, `Down`/`S`,
  `Left`/`A`, `Right`/`D`) rather than aliasing them in the engine — this is
  a deliberate project decision, not a refactor target.

---

## 8. Build, Run, Verify

Two supported flows:

- **DEV** — game Debug + engine consumed from `engine/build` package export.
- **FINAL** — game Release + engine consumed from installed package in `../install`.

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

Clean configure when the cache is stale:

```bat
cd /d H:\Coding\Games\TRPG\game_1
if exist build\debug   rmdir /s /q build\debug
if exist build\release rmdir /s /q build\release
```

VS Code tasks (already wired in `.vscode/tasks.json`):

- Engine DEV / Engine FINAL (engine workspace)
- Game DEV / Game FINAL / Run Game (game_1 workspace)

Per `.clinerules/instructions.md`, the agent **must** read `.vscode/tasks.json`
before doing any build/run/configure work and summarize the existing tasks
first. Add new tasks there instead of inventing ad-hoc CLI invocations.

Shell host policy (per `.github/copilot-instructions.md`): use `cmd.exe` for
all repository commands, builds, tests, CMake, MSVC, Git, and native
toolchain setup. Even when the host terminal is PowerShell, prefer the
`engine/tools/vsenv.bat` / `game_1/tools/build_*.bat` wrappers and avoid
localized path names.

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
6. **Stale-save check**: if the change invalidates campaign saves (see
   §9.1 "Save-file hygiene"), delete the old `user.dat` copies before
   declaring done.

If you cannot run the full smoke run in the current environment, say so
explicitly in the PR/change description rather than claiming "done".

### 9.1 Save-file hygiene (delete stale saves on important changes)

When a change makes existing campaign saves stale, misleading, or invalid,
**delete all old save files before handing off** and say so in the change
description. A stale `user.dat` can hide new content or produce confusing
state (e.g. an old roster missing newly seeded units).

Treat these as "important" (invalidation) triggers:

- The save format/schema changes — a required field is added/renamed
  (`SaveManager.h` roster/inventory/party payloads) or `formatVersion` bumps.
- The default roster seed changes (`RosterSystem::ensureDefaultUnits`) — e.g.
  new units added, existing ones removed/renamed.
- Progression/leveling semantics change (`RosterUnit`, `Unit::growToLevel`,
  `BattleState::awardVictoryBonusXp`).
- Any gameplay-level change where an old campaign would demonstrably be
  wrong or crash — or the user explicitly asks to wipe saves.

Rules of thumb:

- The game writes `user.dat` (`kSaveFilePath`) in the **process working
  directory**, so it can exist in several places: `build/debug/game/Debug/`,
  `build/release/...`, and the repo root. Delete **every** copy.
- One-liner: run `tools\clear_saves.bat` from `game_1` (recursively removes
  every `user.dat` under the repo). Raw equivalent:
  `Get-ChildItem -Path <workspaceRoot> -Recurse -Filter user.dat | Remove-Item`.
- Only delete when a trigger above applies — the file holds the player's
  campaign, so it is destructive. Never delete as routine cleanup.
- After deleting, note that the next run must start a **New Game** (the
  fresh roster/progression is seeded there).

---

## 10. What Is Deferred — Do NOT Build

M1–M5 (economy/save v4, quests, town hub, skill progression, status effects,
buffs, terrain, skill power scaling, boss battle) have **shipped**; see
`docs/ROADMAP.md` and `docs/CHANGELOG.md`. What remains deferred — do not
implement unless the user explicitly asks:

- Monster / `Undead` units as player recruits. `Undead` is a placeholder
  for future monster units only (FFTA-style fixed movesets, likely
  different slot model). Do not build monster unit / skill tree / equip
  rules yet (`docs/design/RACES.md` § Not yet defined).
- Elin-specific Archer/Mage specialization names (currently reusing Elf
  placeholders).
- Races beyond Human/Elf/Elin.
- Promotion class tree and passive/reaction skill effects (`docs/design/UNIT_PROGRESSION.md`;
  `SkillLoader` ignores `progressionCategory`, `requiredPromotion`,
  `autoGranted`, `placeholder`).
- Enemy skill casting (AI uses basic attacks only), cursed terrain, extra
  content statuses.
- Multi-cast skill chains.
- Real sprite art, a full combat animation/VFX package beyond
  `CombatAnimationSystem` + `CombatVfxLibrary`, and any audio backend (the
  engine has none).
- Gamepad support.

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

## 12. Agent Anti-Patterns (pointer)

The prohibitions are the rules already stated in this file; there are no
additional ones hiding outside their sections. Before acting, keep these in
mind (full wording is in the cited section):

- Engine edits, render-pass bypass, `BattleState.cpp` piling, UI polling,
  include/ tree, globbing → **§2**.
- Cursor reset, Manhattan-distance movement cost → **§5**.
- Magic strings, code comments → **§7**.
- Deferred features (monster units, promotions, enemy skill casting,
  multi-cast chains, audio) → **§10**.

---

## 13. Documentation Map

Everything except this file lives under `docs/`. When the agent needs ground
truth, read in this order:

1. `docs/ARCHITECTURE.md` — module ownership, battle flow, render pipeline.
2. `docs/ROADMAP.md` — mission, current slice status, remaining work.
3. `docs/DATA_SCHEMAS.md` — JSON / struct contracts.
4. `docs/TODO.md` — live bugs, verification items, QA backlog; the shipped
   milestone records are in `docs/archive/MILESTONES.md`.
5. `docs/CHANGELOG.md` — what shipped and when.
6. `docs/TUNING.md` — where to change a tunable value (camera, UI, layout).
7. `docs/design/UNIT_PROGRESSION.md` — progression design (M4/M5 slices
   shipped; promotions and passives still deferred).
8. `docs/design/RACES.md` — race/class design pitch; the build ships
   Human/Elf/Elin only, `Undead` is a placeholder for future monsters.
9. `docs/design/FRAMED_LAYOUT.md` — framed battle layout (M1 shipped).
10. `docs/ENGINE_API.md` — engine public surface: what to call, what to reuse.
11. Engine architecture: `engine/ARCHITECTURE.md`.
12. `graphify-out/GRAPH_REPORT.md` — generated code graph; a derived view of
    the source, not a replacement for the docs above.

Update `docs/TODO.md` and `docs/CHANGELOG.md` when finishing items. Update
`docs/ARCHITECTURE.md` when ownership/responsibilities change. Update
`docs/DATA_SCHEMAS.md` when a JSON contract or runtime struct shape changes.

---

## 14. Engine Public API and Tool Files

- **Engine API reference** (public surface, module summary, isometric projection
  formula, do-not-duplicate checklist): [`docs/ENGINE_API.md`](docs/ENGINE_API.md).
- **Tool-specific instruction files**: section 14.1 below.

### 14.1 Tool-specific instruction files (do not duplicate, do not delete)

This file is the repo-level single source of truth for agent
conventions, but three other files exist because different tools load
different files first. Keep all three present and in sync; each loads
for a different audience:

- **`AGENTS.md`** (this file) — picked up by `AGENTS.md`-aware tools
  (opencode, Aider, and similar). The canonical, full version of
  every rule.
- **`.github/copilot-instructions.md`** — picked up by GitHub
  Copilot/Coding Agent. Currently scoped to shell-host policy
  (use `cmd.exe`, prefer the `*.bat` wrappers, never localized
  path names). Cross-link to AGENTS.md when adding rules; do not
  duplicate the full table of contents here.
- **`.clinerules/instructions.md`** — picked up by Cline. Currently a
  short pre-flight rule: "before any build/run/configure task, read
  `.vscode/tasks.json` first and summarize the existing tasks".
  Enforce this rule when acting under Cline; new build-related rules
  should land both in AGENTS.md §8 and in `.clinerules/instructions.md`
  if they are Cline-relevant.

The old `USEFUL_COMMANDS.txt` held only the isometric formula; it was deleted
and that formula now lives in `docs/ENGINE_API.md` §3. Do not reintroduce a
scratch command file — build recipes live in §8 of this file.