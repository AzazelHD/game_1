# TRPG Game Architecture

## Purpose

`game_1` is the tactical RPG game layer that consumes `engine` through public engine headers.

The game owns all domain logic:

- battle rules
- world progression
- AI behavior
- content loading and balancing
- player-facing scene flow

## Scene Flow

1. `BootState`
2. `MainMenuState` (Continue / New Game / Load Game / Options / Quit)
3. `IntroState` (New Game only: narrator script, then tutorial duel via `BattleState`)
4. `WorldMapState` (node graph, town hub, quest journal, node events)
5. `BattleState`
6. `ResultState` (or return to world map depending on outcome flow)

## Module Responsibilities

### `scenes/`

- `BootState`: startup setup and first transition.
- `MainMenuState`: top-level navigation, Continue/Load, and settings entry.
- `IntroState`: data-driven narrator shots (`assets/intro/script.json`) leading into the tutorial duel.
- `BattleLoader`: builds a battle from a `BattleRequest`.
- `WorldMapState`: node graph cursor/movement, battle launch requests, town hub (shop/tavern/talk), and FFTA-style node event chooser.
- `BattleState`: orchestration shell for deployment/combat/render/event integration.
- `ResultState`: outcome display and transition onward.

### `battle/`

#### Core runtime state

- `BattleSession`: owns units, turn queue integration, and victory/defeat checks.

#### Controllers

- `HumanTurnController`: player input phases and tactical action initiation.
- `BattleMenuController`: action/skill/system/inspect menu state and event handling.
- `AttackResolutionController`: selected targets, preview updates, and hit resolution sequencing.
- `DeploymentPhaseController`: pre-combat unit placement lifecycle.
- `DialogueController`: battle-script dialogue sequencing and speaker tracking.
- `MovementAnimationController`: visual movement interpolation independent from logical move commit.

#### Combat and map primitives

- `combat/`: hit formulas (`CombatSystem`), turn queue, pending target sets.
- `map/`: grid occupancy, movement range, map tile data, and pathfinding.
- `unit/`: unit runtime model, progression, `SkillProgression` (skill points/levels), `StatusEffect` (statuses, buffs/debuffs), and template-driven factory construction.
- `systems/`: deployment, combat animation event queue, `CombatVfxLibrary`, and `MovementCommitSystem`.
- `ui/`: battle-specific windows (deployment, inspect, panel).

### `ai/`

- `EnemyAI`: enemy movement and attack planning/execution.

### `renderer/`

- `BattleRenderer`: battle draw pipeline, overlays, and map/unit presentation.
  Culling and the backdrop use the live camera viewport (the play field), not a
  fixed design size.
- `BattleRendererContext`: read-only render input contract.
- `BattleLayout`: framed battle layout — computes the permanent side-rail width,
  the centred `playRect`, and the integer `artScale` (device px per source
  texel) from the native output size. Rebuilt on resize by `BattleState`.
- `BattleWorldProjection`: shared tile→screen anchor math (Camera projection +
  elevation) for world-anchored VFX and floating damage text.

### `data/`

- `SettingsManager`: load/save/apply graphics/audio settings.
- `UnitLoader`: unit JSON to runtime data.
- `SkillLoader`: skill JSON to runtime data and directory bulk load.
- `SaveManager`: campaign saves (format v4: gold, roster, equipment, inventory, EXP, skill progression, quest log) and save-slot listing.
- `QuestData` / `QuestLoader`: quest JSON to runtime data.
- `ShopStock`: shop stock table (`assets/shop/stock.json`).
- `TavernData`: recruit candidates and lore (`assets/tavern/*.json`).

### `events/`

- `BattleEventSystem`: trigger-to-action dispatcher (`OnTurnStart`, `OnUnitDeath`, `OnVictory`, etc.).

### `config/`

- `BattleCatalog`: static battle definitions and per-map rules/events.
- `GameConstants`: game-wide constants for viewport and related values.

### `ui/`

- `UIManager`: single stack owner for windows and input/render blocking behavior.
- shared widgets/helpers: cursor, damage preview, floating text, theme, scaling, key repeat, unit portrait, confirm dialogs.
- `ui/windows/`: `ButtonMenuWindow`, `ConfirmWindow`, `DialogWindow`, `InventoryWindow`, `PartyWindow`, `ShopWindow`, `SkillProgressionWindow`, `UnitDetailWindow`.

### `settings/`

- `SettingsState`, `GraphicsSettings`, `AudioSettings`, `SettingsRowWindow`: menu flows for user configuration.

### `systems/`

- `RosterSystem`: recruitable/player-owned units, persistent equipped ItemIds,
  and atomic equip/unequip transactions.
- `PartySystem`: active party subset used in world/battle flows.
- `PartyContext`: campaign-lifetime owner of roster, party, gold, shared Inventory,
  and GearCatalog.
- `QuestLog`: active/completed quest state, persisted in the campaign save.

### `inventory/`

- `Inventory`: shared player item quantities and capacity rules.
- `GearCatalog`: stable campaign-owned `ItemId` to Gear-definition resolver.
- `EquippedGearIds`: persistent per-roster slot references; resolved into
  temporary Gear-pointer loadouts for UI and battle Units.

### `world/`

- `WorldGraph`, `WorldPathfinding`: node graph representation and shortest-path traversal.

## Battle Flow Ownership

1. `BattleState` enters deployment mode.
2. `DeploymentPhaseController` validates placement and starts combat.
3. `BattleSession` drives active unit and queue state.
4. `HumanTurnController` or `EnemyAI` chooses actions.
5. `AttackResolutionController` resolves hit sequence and state transitions.
6. `BattleEventSystem` emits side effects (dialogue, rewards, spawns, end battle).

## UI and Input Contract

- `UIManager` is the only authority for blocking/stacked windows.
- Controllers and states should react to drained `UIEvent`s, not poll window internals.
- If `UIManager::hasBlockingWindow()` is true, tactical input should not progress combat state.

## Data Assets

Primary content assets live under `game/assets`:

- maps (`.tmj`, `.tmx`)
- unit templates (`assets/units/*.json`)
- skills (`assets/skills/*.json`)
- quests (`assets/quests/*.json`), shop (`assets/shop/`), tavern (`assets/tavern/`), town talk (`assets/town/`), intro script (`assets/intro/`)
- visuals/audio/fonts/sprites

See `DATA_SCHEMAS.md` for map/unit/skill structures.

## Render Pipeline (coordinate spaces)

Battle rendering uses two coordinate spaces, deliberately kept separate:

- **World / play field** — the battlefield renders into a `Renderer` **world
  buffer** sized to the framed layout's `playRect` (native pixels) and is
  blitted **1:1**, so the art stays pixel-crisp. The `Camera` viewport is the
  play rect and its render scale is an **integer** device-px-per-texel
  (`BattleLayout::artScale`). All world draws (tiles, units, cursor, VFX,
  floating text, debug) go through the camera into this pass via
  `beginLogicalPass()` / `endLogicalPass()`.
- **Design / UI** — menus, dialogs, HUD text and screen-anchored effects are
  laid out in the 1280×720 (`GameConstants::VIEW_*`) design space and mapped to
  the window by the renderer's letterbox transform, exactly as before.
- **Rails / backdrop** — the map's sky is drawn across the **whole window** in
  raw native pixels, between `endWorldPass()` and `blitWorldBuffer()`, so the
  world composites over a continuous themed backdrop (the island floating on
  the sky). The permanent side rails are then a **vignette + hairline** layered
  over that sky via `beginNativeOverlay()` / `endNativeOverlay()` — not an
  opaque panel. Tier 2 parallax layers slot between the sky and the world blit.

`BattleLayout` (game) owns the rail/play-rect/art-scale math; the engine exposes
only the primitives (`Renderer::getOutputSize`, `setWorldBufferSize`,
`begin/endNativeOverlay`, `Camera::getViewportSize`). See
`FRAMED_LAYOUT_PLAN.md` for the staged rollout (M1 shipped: rails + crisp play
field; M2+ move the HUD content into the rails and the UI to a native layout).

## Current Gaps and Follow-Ups

1. Full smoke and regression test pass needs manual execution (see "Manual verify" items in `TODO.md`).
2. Combat VFX uses placeholder visuals (`CombatAnimationSystem` + `CombatVfxLibrary`); real sprite art is pending.
3. Enemy AI uses basic attacks only (no skill casting).
4. No audio backend exists in the engine; only a persisted music-volume setting.
