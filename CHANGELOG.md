# Changelog

All notable completed milestones for the TRPG game project.

---

## [Unreleased] – refactor/renderer-passes (not yet merged)

### Added

- World / native render pass split: Renderer now owns a logical target texture, replacing SDL’s automatic logical presentation.
- `beginWorldPass()` / `endWorldPass()` to manually control letterbox stretching with configurable scale mode.
- Native‑pass coordinate conversion (`toNativeRect`, `toNativePos`) so all UI windows draw in physical pixels without code changes.
- Crisp text: `renderText` temporarily scales font point size to match native pixel density, rasterising at real resolution.
- Move‑only `Renderer` with proper Rule‑of‑5.

### Fixed

- Blurry text after resolution change (no more linear‑filtering stretch of a low‑res canvas).

---

## v0.x – Core Vertical Slice

### Bootstrap & Integration

- TRPG executable target linked to engine.
- `vcpkg.json` aligned; Debug/Release builds clean.

### State Flow

- BootState, MainMenuState, BattleState, SettingsState, ResultState fully wired.
- Menu → Battle → Result → return path stable.

### Grid & Navigation

- Grid data structure with tile metadata.
- Tiled map loading via `TiledJsonLoader` with gameplay properties.
- Cursor movement, pathfinding (A\*), movement range (BFS).

### Units, Turns, Combat

- UnitData, runtime Unit, UnitLoader from JSON.
- TurnQueue, combat resolution (hit/crit/damage/KO).
- Win/lose conditions.

### Enemy AI

- `EnemyAI::takeTurn()` heuristic movement + targeting.
- AI always performs a valid action or wait.

### Tactical UI

- UnitPanelWindow, ActionMenu (Move/Attack/Wait), DamagePreview.
- Turn/round indicators, UnitInspectWindow (name, job, stats).
- DeploymentWindow, PartyWindow, ConfirmWindow, DialogWindow, ButtonMenuWindow.
- FloatingTextSystem, UITheme, UIScale, UIUtils, UIWindow framework.

### Camera & Rendering

- Camera tracking during deployment and combat.
- Borderless mode with Stretch presentation, windowed mode with Letterbox.
- Aspect‑ratio fixes, clamp fixes, render‑scale corrections.

### Data & Settings

- UnitLoader, skill skeleton, BattleCatalog.
- SettingsManager: audio, graphics, window mode with persistence.
- Settings applied during boot.

### World Map

- Skeleton world map scene with WorldGraph and WorldPathfinding.

### Architecture & Clean‑up

- BattleState SRP split into controllers.
- BattleUIManager removed; UIManager is sole window owner.
- Folder restructure by kind, not feature.
- Engine/game boundary strict separation.
- UI ownership, input blocking, button menu config centralised.
- Settings feature extraction.

---

## Notes

- Unit progression / class system design exists in `UNIT_DESIGN.md` but is explicitly deferred.
