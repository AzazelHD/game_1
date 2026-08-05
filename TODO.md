# TRPG Game — TODO

## Active Bugs

- [x] **Movement cost**  
       Movement points are spent based on Manhattan distance instead of actual path length from pathfinder.  
       _Files: `battle/map/MovementRange`, `battle/map/Pathfinder`_

- [x] **Cursor position after combat**  
       Cursor remains on target tile after attack/spell. Add cursor to `AttackResolutionContext` and reset to active unit when battle menu reopens.  
       _Files: `BattleState`, `AttackResolutionController`_

- [x] **Borderless graphics**  
       Intermittent graphics application when changing WindowMode. Investigate and fix if still reproducible.  
       _Files: `SettingsRowWindow`, `ValueControl`, input timing_

## Verification & Regression

- [ ] **Full smoke test** (uninterrupted run): Boot → Main Menu → World Map → Deployment → Combat → Attack → Victory → Return
- [ ] **Camera regression test**: windowed, borderless, deployment, combat, cursor at all map corners, clamping
- [ ] **Button menu verification**: marker positioning, width sizing, alignment across action/skill/system/inspect menus

## Render Pass Integration (refactor/renderer-passes branch)

- [x] Update `BattleState::render()` to use `beginLogicalPass()` / `endLogicalPass()`
- [ ] Split `renderEffects()`: `DamagePreview` → native pass, `CombatAnimationSystem::render` + `DebugRenderer::flush` → world pass
- [ ] Confirm no visual regressions (text crisp, correct coordinate spaces)

## Gameplay & Content

- [ ] **DialogSystem**  
       Implement wrapper (`start/update/render/isActive`), block input while active, add sample pre‑battle dialog
- [ ] **SkillLoader**  
       Complete loader, add at least 2 skill JSONs, feed selected skill into combat
- [ ] **JSON schema documentation**  
       Document map, unit, and skill JSON structures

## Polish

- [ ] Sprites and animations for units
- [ ] Menu / combat SFX and background music
- [ ] Impact effects (screen shake, highlights)

## Technical Goals

- [ ] Keep one map fully playable at all times
- [ ] Maintain zero-warning build where practical
- [ ] Compile after every architectural change
