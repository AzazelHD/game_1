# TRPG Game (game_1) — Development Plan

## Mission

Build a tactical RPG (inspired by Final Fantasy Tactics Advance / Tactics Ogre) on top of a reusable engine. Focus on architecture correctness, maintainability, and a stable playable vertical slice.

---

## Current Status

The project has a complete playable tactical battle loop:

- Boot → Main Menu → World Map → Battle flow
- Deployment phase
- Turn-based combat (turn queue, move/attack/wait)
- Combat resolution (hit/crit/damage/KO)
- Enemy AI (heuristic movement + targeting)
- Tactical UI (action menu, skill menu, unit panel, damage preview, inspect)
- JSON content loading (maps, units, skill skeleton)
- Persistent settings (audio, graphics, window mode)
- Battle renderer (camera tracking, borderless/windowed support)
- World/UI render pass separation (crisp text, native‑coordinate UI) – committed, pending final integration
- World map skeleton

---

## Immediate Priorities

1. **Stabilise the vertical slice**
   - Fix remaining bugs (movement cost, cursor after combat, borderless settings glitch)
   - Complete the render‑pass integration in `BattleState::render()`
   - Run full smoke test and camera regression

2. **Finish dialog system**
   - DialogSystem wrapper, input blocking, sample pre‑battle dialogue

3. **Complete skill data pipeline**
   - SkillLoader, skill JSON files, feed skills into combat

4. **Documentation**
   - JSON schemas (map, unit, skill)

5. **Polish (visual / audio)**
   - Sprites, animations, SFX, BGM, screen shake

All new systems (unit progression, recruitment UI, etc.) are **deferred** until the vertical slice is rock‑solid and explicitly requested.

---

## Architecture Rules

### Engine / Game Separation

- Game code only consumes **public engine headers**.
- Gameplay logic never belongs inside the engine.
- Engine never depends on game types.
- `main.cpp` is the translation point.

Examples:

| Engine                 | Game                               |
| ---------------------- | ---------------------------------- |
| Renderer, Input, Audio | BattleState, Unit, Skills, AI      |
| Window, Scene stack    | Party, SettingsManager, WindowMode |

### Folder Organisation

Folders are grouped by **kind**, not feature.  
Feature‑specific UI gets its own folder under the feature directory (e.g. `battle/ui/`).  
Generic reusable windows stay in
ui/windows/
game/src/
ai/ EnemyAI
battle/ BattleState, controllers, systems, combat, map, unit, ui
config/ BattleCatalog, GameConstants
data/ SettingsManager, SkillLoader, UnitLoader
events/ BattleEventSystem
renderer/ BattleRenderer, BattleRendererContext
scenes/ BootState, MainMenuState, ResultState, WorldMapState
settings/ SettingsState, AudioSettings, GraphicsSettings, ui/
systems/ PartyContext, PartySystem, RosterSystem
ui/ Cursor, DamagePreview, FloatingTextSystem, UIManager, UIScale, UITheme, ...
world/ WorldGraph, WorldPathfinding

### Include Rules

- All includes are rooted from `game/src/`.
- Format: `#include "battle/BattleState.h"`
- Never use `"game_1/src/..."`

### Development Workflow

- Keep one map fully playable at all times.
- Compile after every change.
- Small commits.
- Zero warnings where practical.

---

## Ownership Model

- **BattleState** orchestrates the battle scene, not a god class.
- **BattleSession** owns runtime battle state (units, queue).
- **Controllers** (AttackResolution, BattleMenu, Deployment, HumanTurn) each operate on a context provided by BattleState.
- **UIManager** is the single owner of every window. No caching layer, no separate BattleUIManager. Controllers create/destroy windows directly.
- Input blocking uses only `UIManager::hasBlockingWindow()`.

---

## Deferred Features

These are intentionally postponed; do **not** start unless explicitly chosen:

- Unit progression / class system redesign (see `UNIT_DESIGN.md`)
- Portrait emotion system
- Battle sprite emotions
- Turn queue UI widget
- Recruitment / dismissal UI
- World‑map cities, nodes, NPCs
- Multi‑cast skills
- Real animations (blocked on assets)
