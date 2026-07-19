# TRPG Game (game_1) — Development Checklist

Mark tasks with `[x]` as you complete them.
Mark tasks with `[*]` if some details are still missing but the core work is complete.
This file tracks only game-side work in `game_1/`.

---

## JRPG Framework Refactor

- [x] Phase 1 UI Window System core landed:
  - `UIWindow` base + `UIManager` stack and top-input routing
  - `UITheme` palette constants
  - `ButtonMenuWindow`, `DialogWindow`, `UnitPanelWindow`, `ConfirmWindow`
  - `BattleState` UI interaction migrated to window events
  - `WorldMapState` battle prompt migrated to `ConfirmWindow`
  - `MainMenuState` selection migrated to `ButtonMenuWindow`
- [x] Phase 2 Party + Deployment system landed:
  - `RosterSystem`, `PartySystem`, `PartyContext`
  - `DeploymentSystem`, `BattleParticipantsBuilder`
  - `DeploymentWindow` and deployment-first battle flow in `BattleState`
  - map-specific deployment limits (`1v1_test.tmj` -> 1, `test_map.tmj` -> 6)
- [x] Phase 3 Event + Battle definition system landed:
  - expanded `BattleDefinition` data model (enemies, objectives, rewards, events)
  - declarative trigger/action event layer via `BattleEventSystem`
  - battle triggers wired (`OnBattleStart`, `OnTurnStart`, `OnTurnEnd`, `OnUnitDeath`, `OnVictory`, `OnDefeat`)

---

## G1 — Bootstrap & Integration

- [x] Define `TRPG` executable target in [CMakeLists.txt](game/CMakeLists.txt)
- [x] Link game target to `engine` target and required libs
- [x] Keep [vcpkg.json](vcpkg.json) aligned with embedded engine dependencies
- [x] Wire [main.cpp](game/src/main.cpp) / `App` bootstrap to the first game-owned Scene cleanly
- [x] Build Debug and Release without link errors

Checkpoint: executable launches and transitions into first state.

---

## G2 — State Flow

- [x] Implement `BootState` (load essentials, transition) — [BootState.h](game/src/states/BootState.h) / [BootState.cpp](game/src/states/BootState.cpp), [main.cpp](game/src/main.cpp)
- [x] Implement `MainMenuState` (input to start game) — [MainMenuState.h](game/src/states/MainMenuState.h) / [MainMenuState.cpp](game/src/states/MainMenuState.cpp)
- [x] Implement `BattleState` skeleton and ownership model — [BattleState.h](game/src/states/BattleState.h) / [BattleState.cpp](game/src/states/BattleState.cpp), [main.cpp](game/src/main.cpp)
- [x] Implement `SettingsState` (game-owned options menu) — [SettingsState.h](game/src/states/SettingsState.h) / [SettingsState.cpp](game/src/states/SettingsState.cpp)
- [x] Implement `ResultState` (victory/defeat + return path) — [ResultState.h](game/src/states/ResultState.h) / [ResultState.cpp](game/src/states/ResultState.cpp)

Fix instructions (TODO: description):

- [x]: main flow bootstrap
  description: In [main.cpp](game/src/main.cpp), start the app at `BootState` (not `BattleState`) so the intended flow can actually be exercised.

- [x]: menu to battle transition
  description: In [MainMenuState.cpp](game/src/states/MainMenuState.cpp), replace the current placeholder with a real transition to `BattleState`; avoid leaving `m_transitioning` true without transitioning.

- [x]: result return path
  description: In [ResultState.cpp](game/src/states/ResultState.cpp), implement Confirm -> return to `MainMenuState`; keep this as the canonical post-battle exit.

- [x]: settings exit wiring
  description: In [SettingsState.cpp](game/src/states/SettingsState.cpp), wire Back/Confirm to pop or replace via state machine; remove placeholder TODO path.

- [x]: settings constructor definition
  description: Add/verify `SettingsState` constructor definition in [SettingsState.cpp](game/src/states/SettingsState.cpp) to match [SettingsState.h](game/src/states/SettingsState.h) and avoid linker errors when instantiated.

- [x]: battle state transitions
  description: In [BattleState.cpp](game/src/states/BattleState.cpp), add minimal input/condition hooks in `update` to reach `ResultState` so G2 flow is testable end-to-end.

- [x]: status/comment alignment
  description: Align [TODO.md](TODO.md) status and state-header comments with actual implementation, especially where headers say done but `.cpp` still has placeholder TODOs.

Checkpoint: Boot -> Menu -> Battle -> Result flow is stable.

---

## G3 — Grid & Navigation

- [x] Implement `Grid` data structure and tile metadata — [Grid.h](game/src/battle/Grid.h) / [Grid.cpp](game/src/battle/Grid.cpp)
- [x] Use engine `TiledJsonLoader` as canonical map loader and extract gameplay map metadata (spawn points/object layers/properties) in battle flow — [BattleState.h](game/src/states/BattleState.h) / [BattleState.cpp](game/src/states/BattleState.cpp)
- [x] Create at least one playable map in [maps/](game/assets/maps/)
- [x] Implement cursor movement/selection on map — [Cursor.h](game/src/ui/Cursor.h) / [Cursor.cpp](game/src/ui/Cursor.cpp), [BattleState.h](game/src/states/BattleState.h) / [BattleState.cpp](game/src/states/BattleState.cpp)
- [x] Implement `Pathfinder` (A\*) — [Pathfinder.h](game/src/battle/Pathfinder.h) / [Pathfinder.cpp](game/src/battle/Pathfinder.cpp)
- [x] Implement `MovementRange` (BFS) — [MovementRange.h](game/src/battle/MovementRange.h) / [MovementRange.cpp](game/src/battle/MovementRange.cpp)

Checkpoint: map loads, cursor moves, paths/range are valid.

---

## G4 — Units, Turns, Combat

- [x] Define `UnitData` and runtime `Unit` state — [Unit.h](game/src/battle/Unit.h) / [Unit.cpp](game/src/battle/Unit.cpp)
- [x] Implement `UnitLoader` from JSON — [UnitLoader.h](game/src/data/UnitLoader.h) / [UnitLoader.cpp](game/src/data/UnitLoader.cpp)
- [x] Create 2+ unit entries in [units/](game/assets/units/)
- [x] Implement `TurnQueue` — [TurnQueue.h](game/src/battle/TurnQueue.h) / [TurnQueue.cpp](game/src/battle/TurnQueue.cpp)
- [x] Implement `CombatSystem::resolve()` (hit, crit, damage) — [CombatSystem.h](game/src/battle/CombatSystem.h) / [CombatSystem.cpp](game/src/battle/CombatSystem.cpp)
- [x] Handle KO and turn-queue removal — [TurnQueue.h](game/src/battle/TurnQueue.h) / [TurnQueue.cpp](game/src/battle/TurnQueue.cpp), [CombatSystem.h](game/src/battle/CombatSystem.h) / [CombatSystem.cpp](game/src/battle/CombatSystem.cpp)
- [x] Implement win/lose conditions — [BattleState.h](game/src/states/BattleState.h) / [BattleState.cpp](game/src/states/BattleState.cpp), [ResultState.h](game/src/states/ResultState.h) / [ResultState.cpp](game/src/states/ResultState.cpp)

Checkpoint: full tactical turn loop works for both teams.

---

## G5 — Enemy AI

- [x] Implement `EnemyAI::takeTurn()` movement + target heuristic — [EnemyAI.h](game/src/ai/EnemyAI.h) / [EnemyAI.cpp](game/src/ai/EnemyAI.cpp)
- [x] Integrate enemy turns into battle loop — [BattleState.h](game/src/states/BattleState.h) / [BattleState.cpp](game/src/states/BattleState.cpp)
- [x] Ensure AI always performs a valid action (or valid wait) — [EnemyAI.h](game/src/ai/EnemyAI.h) / [EnemyAI.cpp](game/src/ai/EnemyAI.cpp)

Checkpoint: enemy completes autonomous turns without breaking flow.

---

## G6 — Tactical UI + Dialog

- [x] Implement `UnitPanel` (hover/selected stats) — [UnitPanel.h](game/src/ui/UnitPanel.h) / [UnitPanel.cpp](game/src/ui/UnitPanel.cpp), [BattleState.h](game/src/states/BattleState.h) / [BattleState.cpp](game/src/states/BattleState.cpp)
- [x] Implement `ActionMenu` (Move/Attack/Wait) — [ActionMenu.h](game/src/ui/ActionMenu.h) / [ActionMenu.cpp](game/src/ui/ActionMenu.cpp), [BattleState.h](game/src/states/BattleState.h) / [BattleState.cpp](game/src/states/BattleState.cpp)
- [x] Implement `DamagePreview` — [DamagePreview.h](game/src/ui/DamagePreview.h) / [DamagePreview.cpp](game/src/ui/DamagePreview.cpp), [BattleState.h](game/src/states/BattleState.h) / [BattleState.cpp](game/src/states/BattleState.cpp)
- [x] Add turn and round indicators — [UnitPanel.h](game/src/ui/UnitPanel.h) / [UnitPanel.cpp](game/src/ui/UnitPanel.cpp), [BattleState.h](game/src/states/BattleState.h) / [BattleState.cpp](game/src/states/BattleState.cpp)
- [x] Own `DialogBox` widget presentation in the game layer — [DialogBox.h](game/src/ui/DialogBox.h) / [DialogBox.cpp](game/src/ui/DialogBox.cpp)
- [ ] Implement `DialogSystem` wrapper (`start/update/render/isActive`) — [DialogSystem.h](game/src/ui/DialogSystem.h) / [DialogSystem.cpp](game/src/ui/DialogSystem.cpp)
- [ ] Gate player input while dialog is active — [BattleState.h](game/src/states/BattleState.h) / [BattleState.cpp](game/src/states/BattleState.cpp), [DialogSystem.h](game/src/ui/DialogSystem.h) / [DialogSystem.cpp](game/src/ui/DialogSystem.cpp)
- [ ] Add sample pre-battle dialog sequence — [BattleState.h](game/src/states/BattleState.h) / [BattleState.cpp](game/src/states/BattleState.cpp), [assets/](game/assets/)

Checkpoint: one complete playable vertical slice with usable UI and dialog.

---

## G7 — Data Pipeline

- [ ] Define JSON schema docs for maps, units, and skills — [assets/](game/assets/)
- [ ] Implement `SkillLoader` — [SkillLoader.h](game/src/data/SkillLoader.h) / [SkillLoader.cpp](game/src/data/SkillLoader.cpp)
- [ ] Add at least 2 skill JSON files in [skills/](game/assets/skills/)
- [ ] Feed selected skill into combat resolution — [CombatSystem.h](game/src/battle/CombatSystem.h) / [CombatSystem.cpp](game/src/battle/CombatSystem.cpp), [SkillLoader.h](game/src/data/SkillLoader.h) / [SkillLoader.cpp](game/src/data/SkillLoader.cpp)
- [ ] Optional: Debug hot-reload for changed JSON files — [BattleState.h](game/src/states/BattleState.h) / [BattleState.cpp](game/src/states/BattleState.cpp)

Checkpoint: content-driven setup works from JSON files only.

---

## G8 — Polish

- [ ] Replace placeholder unit visuals with sprites/animations — [BattleState.h](game/src/states/BattleState.h) / [BattleState.cpp](game/src/states/BattleState.cpp), [sprites/](game/assets/sprites/)
- [ ] Add combat/menu SFX and BGM — [BattleState.h](game/src/states/BattleState.h) / [BattleState.cpp](game/src/states/BattleState.cpp), [audio/](game/assets/audio/)
- [ ] Add impact polish (shake, highlights, small FX) — [BattleState.h](game/src/states/BattleState.h) / [BattleState.cpp](game/src/states/BattleState.cpp), [ui/](game/src/ui/)

Checkpoint: game has coherent presentation and feel.

---

## Ongoing Rules

- [x] Keep gameplay logic, concrete scenes, and dialog presentation out of engine
- [ ] Keep one map fully playable at all times — [maps/](game/assets/maps/), [BattleState.h](game/src/states/BattleState.h) / [BattleState.cpp](game/src/states/BattleState.cpp)
- [ ] Maintain zero-warning target where feasible — all [src/](game/src/) files
