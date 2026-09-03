# TRPG Game (game_1) - Development Plan

## Mission

Deliver a stable tactical RPG vertical slice on top of the shared engine while keeping gameplay systems modular and testable.

## Current Vertical Slice Status

The core path is implemented in code:

- App boot and scene flow (`BootState` -> `MainMenuState` -> `WorldMapState` -> `BattleState`)
- Deployment phase with roster selection and forced/critical unit handling
- Combat phase with turn queue, movement, attack/skill targeting, confirm, and resolution
- Event hooks (`BattleEventSystem`) for dialogue/reward/spawn/end-battle actions
- Tactical UI stack (action, skill, system, inspect, confirm, unit panel, damage preview)
- Settings flow (graphics/audio) with persisted settings
- Battle rendering split into world logical pass and native UI/effects pass

## Architecture Ownership

### Scene layer (`scenes/`)

- `BootState`: startup handoff and first scene transition
- `MainMenuState`: entry menu and navigation to world map/settings
- `WorldMapState`: node graph travel and battle request construction
- `BattleState`: battle orchestration shell (delegates behavior through context objects)
- `ResultState`: battle result presentation and return flow

### Battle domain (`battle/`)

- `BattleSession`: runtime unit ownership, turn queue integration, victory/defeat evaluation
- `controllers/`:
  - `HumanTurnController`: human input state machine for move/attack/confirm phases
  - `BattleMenuController`: action/skill/system/inspect menu flows
  - `AttackResolutionController`: pending-target set, preview, hit resolution sequencing
  - `DeploymentPhaseController`: deployment-specific interaction and transition into combat
  - `MovementAnimationController`: visual walk playback independent of logical move commit
  - `DialogueController`: dialogue sequencing bridge for battle cut-in lines
- `combat/`: hit formulas, turn queue, and area target selection support
- `map/`: movement range, pathfinding, occupancy, and tile metadata usage
- `systems/`: deployment model and combat animation queue hooks
- `ui/`: battle-specific windows (deployment, inspect, panel)

### Support systems

- `ai/EnemyAI`: enemy plan/act logic
- `config/BattleCatalog`: map->battle definition catalog and static event rules
- `data/`:
  - `SettingsManager`: settings load/save/apply
  - `UnitLoader`: unit template JSON parser
  - `SkillLoader`: skill JSON parser and bulk-load database source
- `events/BattleEventSystem`: trigger/action dispatcher abstraction
- `renderer/BattleRenderer`: battle drawing pipeline and overlays
- `ui/`: window stack manager, generic UI events, shared visual helpers
- `systems/PartySystem` and `systems/RosterSystem`: world/battle party composition data
- `world/`: graph and pathfinding for world navigation

## What Is Done vs Pending

### Done

1. Movement cost now uses real path cost from reachable cost map.
2. Cursor resets after combat resolution and action menu re-entry.
3. Graphics mode application path is implemented with borderless/windowed settings updates.
4. Render pass separation is implemented in `BattleState::render()`.
5. Skill loading and skill menu-to-combat wiring are implemented with multiple skill JSON files.

### Pending validation and completion

1. Full uninterrupted smoke run through all major scenes.
2. Camera and menu alignment regression checks across display modes.
3. Final visual verification for pass separation on all battle UI overlays.
4. Add explicit pre-battle scripted dialogue sequence usage.

## Engineering Rules

- Game code uses engine public headers only.
- Gameplay logic stays in `game/src/`, never in engine.
- `UIManager` is the single source of truth for modal/input-blocking UI behavior.
- New behavior should be introduced in focused controllers/systems before extending `BattleState` directly.
- Keep at least one map fully playable at all times.

## Documentation Map

- Engine architecture: `engine/ARCHITECTURE.md`
- Game architecture and ownership: `game_1/ARCHITECTURE.md`
- Data schema reference: `game_1/DATA_SCHEMAS.md`
- Task tracking: `game_1/TODO.md`

## Deferred Until Vertical Slice Is Fully Stable

1. Unit/class progression redesign (`UNIT_DESIGN.md`).
2. Expanded world simulation (cities, NPC interactions, recruitment UX).
3. Full combat animation and VFX package.
4. Advanced skill meta systems (multi-cast chains, status pipelines).
