# This file is the single source of truth for the game executable target.
# Keep all game source/header registration here to avoid drift with the root
# CMakeLists.

set(GAME_SOURCES
    src/main.cpp

    src/states/BootState.cpp
    src/states/MainMenuState.cpp
    src/states/WorldMapState.cpp
    src/states/BattleState.cpp
    src/states/PauseState.cpp
    src/states/ResultState.cpp
    src/states/SettingsState.cpp

    src/battle/Grid.cpp
    src/battle/Unit.cpp
    src/battle/UnitProgression.cpp
    src/battle/TurnQueue.cpp
    src/battle/CombatSystem.cpp
    src/battle/Pathfinder.cpp
    src/battle/MovementRange.cpp
    src/battle/BattleMap.cpp
    src/battle/UnitFactory.cpp

    src/ai/EnemyAI.cpp

    src/renderer/BattleRenderer.cpp

    src/data/UnitLoader.cpp
    src/data/SkillLoader.cpp
    src/config/BattleCatalog.cpp
    src/systems/RosterSystem.cpp
    src/systems/PartySystem.cpp
    src/systems/PartyContext.cpp
    src/systems/DeploymentSystem.cpp
    src/systems/BattleParticipantsBuilder.cpp
    src/systems/CombatAnimationSystem.cpp
    src/events/BattleEventSystem.cpp

    src/ui/Cursor.cpp
    src/ui/UIManager.cpp
    src/ui/DamagePreview.cpp
    src/ui/DialogSystem.cpp
    src/ui/UIScale.cpp
    src/ui/unit_portrait.cpp
    src/ui/windows/ActionMenuWindow.cpp
    src/ui/windows/ConfirmWindow.cpp
    src/ui/windows/DialogWindow.cpp
    src/ui/windows/UnitPanelWindow.cpp
    src/ui/windows/DeploymentWindow.cpp
    src/ui/windows/InspectWindow.cpp
    src/ui/windows/PartyWindow.cpp
)

set(GAME_HEADERS
    src/states/BootState.h
    src/states/MainMenuState.h
    src/states/WorldMapState.h
    src/states/BattleState.h
    src/states/PauseState.h
    src/states/ResultState.h
    src/states/SettingsState.h

    src/battle/Grid.h
    src/battle/Unit.h
    src/battle/UnitData.h
    src/battle/UnitProgression.h
    src/battle/TurnQueue.h
    src/battle/CombatSystem.h
    src/battle/Pathfinder.h
    src/battle/MovementRange.h
    src/battle/GameTile.h
    src/battle/BattleMap.h
    src/battle/UnitFactory.h

    src/ai/EnemyAI.h

    src/renderer/BattleRenderer.h
    src/renderer/BattleRendererContext.h

    src/data/UnitLoader.h
    src/data/SkillLoader.h
    src/config/BattleCatalog.h
    src/systems/RosterSystem.h
    src/systems/PartySystem.h
    src/systems/PartyContext.h
    src/systems/DeploymentSystem.h
    src/systems/BattleParticipantsBuilder.h
    src/systems/CombatAnimationSystem.h
    src/events/BattleEventSystem.h

    src/ui/Cursor.h
    src/ui/UITheme.h
    src/ui/UIEvent.h
    src/ui/UIWindow.h
    src/ui/UIManager.h
    src/ui/DamagePreview.h
    src/ui/DialogSystem.h
    src/ui/UIScale.h
    src/ui/unit_portrait.h
    src/ui/windows/ActionMenuWindow.h
    src/ui/windows/ConfirmWindow.h
    src/ui/windows/DialogWindow.h
    src/ui/windows/UnitPanelWindow.h
    src/ui/windows/DeploymentWindow.h
    src/ui/windows/InspectWindow.h
    src/ui/windows/PartyWindow.h
)

add_executable(game
    ${GAME_SOURCES}
    ${GAME_HEADERS}
)

# Make the working directory match the folder where game.exe is placed
set_target_properties(game PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(OutDir)")

# Header layout policy:
# - src/ is the active implementation and header tree for the game target.
# - add include/ only when a real public game API is introduced.
target_include_directories(game PRIVATE
    src
    ../engine/include
)

target_link_libraries(game PRIVATE
    TRPG::engine
    SDL3::SDL3
)

# Automatically copy required runtime DLLs next to the executable on Windows.
if(WIN32)
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:game>
            $<TARGET_FILE_DIR:game>
        COMMAND_EXPAND_LISTS
    )
endif()

# Copy the assets folder next to the executable so the game can find them at runtime.
add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:game>/assets"
)
