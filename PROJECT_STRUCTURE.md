# TRPG Project Structure Index

This file is a practical map of where concrete logic currently lives.
It documents current structure only (no implied rename/reorg).

## Workspace Roots

- `engine/`: reusable engine/framework modules.
- `game_1/`: game-specific code, assets, states, battle systems.

## Engine (`engine/`)

### Build and project config

- `engine/CMakeLists.txt`: engine target definition and link setup.
- `engine/CMakePresets.json`: CMake configure/build presets.
- `engine/vcpkg.json`: dependency manifest.
- `engine/tools/build_debug.bat`, `engine/tools/build_release.bat`: build scripts.
- `engine/tools/vsenv.bat`: native tools wrapper.

### Core app/window/runtime

- `engine/include/engine/core/App.h`, `engine/src/engine/core/App.cpp`: app lifetime, global service access (renderer/window/default font).
- `engine/include/engine/core/Window.h`, `engine/src/engine/core/Window.cpp`: OS window control (size/fullscreen/borderless/aspect/vsync/title).
- `engine/include/engine/core/Log.h`, `engine/src/engine/core/Log.cpp`: logging.

### Input and key mapping

- `engine/include/engine/input/Input.h`, `engine/src/engine/input/Input.cpp`: keyboard input state and polling.
- `engine/include/engine/input/KeyCode.h`: game action key mapping enum.

### Rendering and text layout

- `engine/include/engine/renderer/Renderer.h`, `engine/src/engine/renderer/Renderer.cpp`: drawing primitives, text render/measure, in-rect text alignment helpers.
- `engine/include/engine/renderer/Camera.h`, `engine/src/engine/renderer/Camera.cpp`: camera transforms.
- `engine/include/engine/renderer/Texture.h`, `engine/src/engine/renderer/Texture.cpp`: texture resource wrapper.
- `engine/include/engine/renderer/Font.h`: font handle abstraction.

### Generic UI primitives

- `engine/include/engine/ui/Button.h`, `engine/src/engine/ui/Button.cpp`: generic button with padding/alignment.
- `engine/include/engine/ui/Slider.h`, `engine/src/engine/ui/Slider.cpp`: slider input/render behavior.
- `engine/include/engine/ui/MenuPanel.h`, `engine/src/engine/ui/MenuPanel.cpp`: button panel container/navigation.
- `engine/include/engine/ui/TextLabel.h`, `engine/src/engine/ui/TextLabel.cpp`: standalone text label widget.

## Game (`game_1/`)

### Build and project config

- `game_1/CMakeLists.txt`: game executable and links.
- `game_1/CMakePresets.json`: game CMake presets.
- `game_1/vcpkg.json`: game dependency manifest.
- `game_1/tools/build_debug.bat`, `game_1/tools/build_release.bat`: build scripts.

### Entry point

- `game_1/game/src/main.cpp`: game startup and initial state wiring.

### States / screen flow

- `game_1/game/src/states/MainMenuState.h/.cpp`: title screen, menu options, transitions.
- `game_1/game/src/states/WorldMapState.h/.cpp`: world graph cursor movement, node selection, battle entry.
- `game_1/game/src/states/BattleState.h/.cpp`: deployment, combat phases, turn flow, tactical input, battle UI orchestration.
- `game_1/game/src/states/SettingsState.h/.cpp`: settings menu logic and graphics/audio controls.
- `game_1/game/src/states/PauseState.h/.cpp`: in-battle pause menu.

### Battle rules and tactical systems

- `game_1/game/src/battle/CombatSystem.h/.cpp`: hit/damage resolution and combat outcome.
- `game_1/game/src/battle/TurnQueue.h/.cpp`: turn scheduling and advancement.
- `game_1/game/src/battle/MovementRange.h/.cpp`: reachable tiles/path range logic.
- `game_1/game/src/battle/Grid.h/.cpp`: logical occupancy grid.
- `game_1/game/src/battle/BattleMap.h/.cpp`: gameplay map representation.
- `game_1/game/src/battle/Unit.h/.cpp`: unit runtime state/actions/stats.
- `game_1/game/src/battle/UnitFactory.h/.cpp`: runtime unit creation from data templates.

### Deployment / roster / participants

- `game_1/game/src/systems/DeploymentSystem.h/.cpp`: placement state, selected/grabbed unit, spawn legality, deployed roster.
- `game_1/game/src/systems/PartyContext.h/.cpp`: persistent party and roster source for deployment.
- `game_1/game/src/systems/BattleParticipantsBuilder.h/.cpp`: builds player/enemy runtime units for combat start.

### Targeting, previews, battle feedback

- `game_1/game/src/ui/DamagePreview.h/.cpp`: damage preview panel logic.
- `game_1/game/src/systems/CombatAnimationSystem.h/.cpp`: queued battle animation/event display hooks.
- `game_1/game/src/events/BattleEventSystem.h/.cpp`: battle triggers/events and callbacks.

### AI turn logic

- `game_1/game/src/ai/EnemyAI.h/.cpp`: enemy decision-making and action execution.

### Data definitions and loaders

- `game_1/game/src/data/UnitLoader.h/.cpp`: unit template JSON loading.
- `game_1/game/src/data/SkillLoader.h/.cpp`: skill JSON loading.
- `game_1/game/src/config/BattleCatalog.h/.cpp`: map/scenario/battle definitions.
- `game_1/game/src/config/GameConstants.h`: shared gameplay/view constants.

### UI windows (game-specific)

- `game_1/game/src/ui/UIManager.h/.cpp`: modal/window stack and UI event routing.
- `game_1/game/src/ui/UIWindow.h/.cpp`: base class for game windows.
- `game_1/game/src/ui/UITheme.h/.cpp`: shared UI colors/theme constants.
- `game_1/game/src/ui/windows/ActionMenuWindow.h/.cpp`: action list popup windows.
- `game_1/game/src/ui/windows/ConfirmWindow.h/.cpp`: yes/no prompt popup.
- `game_1/game/src/ui/windows/DialogWindow.h/.cpp`: dialogue text window.
- `game_1/game/src/ui/windows/DeploymentWindow.h/.cpp`: deployment roster/status input window.
- `game_1/game/src/ui/windows/InspectWindow.h/.cpp`: unit inspect/details popup.
- `game_1/game/src/ui/windows/PartyWindow.h/.cpp`: party summary window.
- `game_1/game/src/ui/windows/UnitPanelWindow.h/.cpp`: bottom portraits/turn panels (single, duel, deployment preview).
- `game_1/game/src/ui/unit_portrait.h/.cpp`: shared concrete unit portrait renderer used by deployment and in-battle action portraits.
- `game_1/game/src/ui/UIScale.h/.cpp`: shared UI scale factor utility used by scalable UI components.

### Battle rendering pipeline

- `game_1/game/src/renderer/BattleRenderer.h/.cpp`: map, overlays, units, cursor render orchestration.
- `game_1/game/src/renderer/BattleRendererContext.h`: render data bundle passed into battle renderer.

### Assets (runtime content)

- `game_1/game/assets/maps/`: map JSON files.
- `game_1/game/assets/units/`: per-unit data templates.
- `game_1/game/assets/skills/`: skill definitions.
- `game_1/game/assets/sprites/`: sprite sheets and unit portrait textures.
- `game_1/game/assets/fonts/`: UI fonts.
- `game_1/game/assets/audio/`: audio content.

## Fast "Where do I edit X?" Map

- Unit portraits UI: `game_1/game/src/ui/windows/UnitPanelWindow.cpp`
- Deployment interaction flow: `game_1/game/src/states/BattleState.cpp`, `game_1/game/src/systems/DeploymentSystem.cpp`, `game_1/game/src/ui/windows/DeploymentWindow.cpp`
- Placement legality and max placeable behavior: `game_1/game/src/systems/DeploymentSystem.cpp`
- Turn order and turn transitions: `game_1/game/src/battle/TurnQueue.cpp`, `game_1/game/src/states/BattleState.cpp`
- Player targeting/confirm flow: `game_1/game/src/states/BattleState.cpp`
- Enemy AI action selection: `game_1/game/src/ai/EnemyAI.cpp`
- Action/confirm popups: `game_1/game/src/ui/windows/ActionMenuWindow.cpp`, `game_1/game/src/ui/windows/ConfirmWindow.cpp`
- Graphics settings menu: `game_1/game/src/states/SettingsState.cpp`
- UI scaling mechanism: `game_1/game/src/ui/UIScale.cpp`
- Window resize/fullscreen/borderless behavior: `engine/src/engine/core/Window.cpp`
- Shared text alignment behavior: `engine/src/engine/renderer/Renderer.cpp`

## Runtime settings persistence

- `settings.json` (runtime-generated in executable working directory): persisted graphics/audio settings written by `SettingsState`.
