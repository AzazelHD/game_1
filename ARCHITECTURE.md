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
2. `MainMenuState`
3. `WorldMapState`
4. `BattleState`
5. `ResultState` (or return to world map depending on outcome flow)

## Module Responsibilities

### `scenes/`

- `BootState`: startup setup and first transition.
- `MainMenuState`: top-level navigation and settings entry.
- `WorldMapState`: node graph cursor/movement and battle launch requests.
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
- `unit/`: unit runtime model, progression, and template-driven factory construction.
- `systems/`: deployment and combat animation event queue support.
- `ui/`: battle-specific windows (deployment, inspect, panel).

### `ai/`

- `EnemyAI`: enemy movement and attack planning/execution.

### `renderer/`

- `BattleRenderer`: battle draw pipeline, overlays, and map/unit presentation.
- `BattleRendererContext`: read-only render input contract.

### `data/`

- `SettingsManager`: load/save/apply graphics/audio settings.
- `UnitLoader`: unit JSON to runtime data.
- `SkillLoader`: skill JSON to runtime data and directory bulk load.

### `events/`

- `BattleEventSystem`: trigger-to-action dispatcher (`OnTurnStart`, `OnUnitDeath`, `OnVictory`, etc.).

### `config/`

- `BattleCatalog`: static battle definitions and per-map rules/events.
- `GameConstants`: game-wide constants for viewport and related values.

### `ui/`

- `UIManager`: single stack owner for windows and input/render blocking behavior.
- shared widgets/helpers: cursor, damage preview, floating text, theme, scaling.

### `settings/`

- `SettingsState`, `GraphicsSettings`, `AudioSettings`, `SettingsRowWindow`: menu flows for user configuration.

### `systems/`

- `RosterSystem`: recruitable/player-owned units, persistent equipped ItemIds,
  and atomic equip/unequip transactions.
- `PartySystem`: active party subset used in world/battle flows.
- `PartyContext`: campaign-lifetime owner of roster, party, shared Inventory,
  and GearCatalog.

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
- visuals/audio/fonts/sprites

See `DATA_SCHEMAS.md` for map/unit/skill structures.

## Current Gaps and Follow-Ups

1. Full smoke and regression test pass needs manual execution.
2. Pre-battle scripted dialogue usage exists as API hooks but still needs an always-on sample sequence.
3. Combat animation system currently exposes structure and queueing, with placeholder rendering behavior.
4. Campaign save/load serialization for roster equipment, inventory, and EXP remains deferred.
