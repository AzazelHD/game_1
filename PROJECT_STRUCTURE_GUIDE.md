# TRPG Project Structure Guide

This guide explains what each major folder/file is responsible for, so you can navigate and study the codebase faster.

## Workspace Roots

- `engine/`: reusable engine layer (windowing, rendering, input, UI primitives, data loaders).
- `game_1/`: game-specific implementation (states, battle logic, assets, game UI windows, data definitions).

---

## `engine/` (Framework Layer)

### Top-level

- `CMakeLists.txt`: engine build targets and compile/link setup.
- `CMakePresets.json`: configure/build presets for local workflows.
- `vcpkg.json`: engine dependency manifest (SDL3 stack, etc.).
- `tools/`: local build helpers and environment wrappers.

### Public headers

- `include/engine/`: engine API surface used by game code.

Key areas inside:

- `include/engine/core/`: app bootstrap, window lifecycle, logging.
- `include/engine/input/`: key mapping, input polling helpers.
- `include/engine/math/`: vector/rect utilities used by rendering and UI.
- `include/engine/renderer/`: renderer abstractions over SDL/SDL_ttf.
- `include/engine/ui/`: generic UI controls (Button, Slider, MenuPanel, TextLabel, focus/navigation).
- `include/engine/statemachine/`: scene/state stack primitives.
- `include/engine/data/`: tiled/JSON loaders and shared data adapters.

### Engine source

- `src/engine/`: implementation for all engine modules.

Notable files for alignment/layout:

- `src/engine/renderer/Renderer.cpp`: draw API, text rendering/measurement, in-rect alignment helpers.
- `src/engine/ui/Button.cpp`: button rendering with padding + text alignment.
- `src/engine/ui/MenuPanel.cpp`: button container and input navigation.
- `src/engine/ui/TextLabel.cpp`: free text rendering utility.

### Build outputs

- `build/`: generated CMake/MSBuild artifacts.
- `install/`: installed/exported engine package artifacts.

---

## `game_1/` (Game Layer)

### Top-level

- `CMakeLists.txt`: game executable target and links to engine.
- `CMakePresets.json`: game configure/build presets.
- `vcpkg.json`: game dependency manifest.
- `PLAN.md`, `TODO.md`: planning/tracking docs.
- `tools/`: game build helper scripts.

### Runtime/game content

- `game/assets/`: all runtime content (maps, units, sprites, skills, audio, fonts).

Examples:

- `game/assets/maps/`: tiled maps.
- `game/assets/units/`: unit JSON definitions.
- `game/assets/skills/`: skill behavior data.

### Game source

- `game/src/`: main gameplay code.

Important folders:

- `game/src/states/`: high-level scenes and screen flow.
  - `MainMenuState.cpp`: title screen/menu flow.
  - `WorldMapState.cpp`: world exploration node graph and transitions.
  - `BattleState.cpp`: deployment + combat loop, battle UI/event coordination.
  - `SettingsState.cpp`: options/settings UI and interaction.
- `game/src/battle/`: tactical battle domain logic.
  - units, combat math, movement range, grid state.
- `game/src/systems/`: reusable gameplay systems.
  - deployment, participants building, animations, party/roster context.
- `game/src/ui/` and `game/src/ui/windows/`: game-specific windows/widgets built on engine renderer/UI.
  - action menus, confirm windows, inspect/dialog/party/deployment windows.
- `game/src/config/`: data catalogs and static scenario definitions.
  - battle catalog (maps, encounters, events, rewards).
- `game/src/data/`: JSON loaders for skills/units.
- `game/src/renderer/`: battle scene renderer orchestration and render context.
- `game/src/events/`: battle event trigger/action pipeline.
- `game/src/ai/`: enemy behavior logic.

### Build outputs

- `build/debug/`: generated game build tree and executable.

---

## Practical Layering Rules

- Engine should stay generic:
  - no game-specific constants, unit names, map assumptions, or scenario logic.
- Game layer composes systems:
  - states orchestrate input, transitions, and which windows/systems are active.
- Rendering split:
  - engine renderer provides primitives and layout helpers.
  - game windows/states choose what to draw and where.

---

## Where to edit for common tasks

- Text alignment behavior globally:
  - `engine/src/engine/renderer/Renderer.cpp`
  - `engine/include/engine/renderer/Renderer.h`
- Button/menu visual alignment:
  - `engine/src/engine/ui/Button.cpp`
  - `game/src/ui/windows/ActionMenuWindow.cpp`
- Main menu/title positioning:
  - `game/src/states/MainMenuState.cpp`
- Deployment portrait/selection behavior:
  - `game/src/states/BattleState.cpp`
  - `game/src/ui/windows/UnitPanelWindow.cpp`

---

## Build order reminder

When engine public API changes (headers/signatures), rebuild both:

1. engine debug build
2. game debug build

This prevents linker mismatches from stale engine libs.
