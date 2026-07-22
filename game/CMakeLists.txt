# This file is the single source of truth for the game executable target.
# Keep all game source/header registration here to avoid drift with the root
# CMakeLists.

set(GAME_SOURCES
    src/main.cpp

    src/scenes/BootState.cpp
    src/scenes/MainMenuState.cpp
    src/scenes/BattleState.cpp
    src/scenes/ResultState.cpp
    src/scenes/BattleLoader.cpp   
    src/scenes/SettingsState.cpp
    src/scenes/AudioSettings.cpp
    src/scenes/GraphicsSettings.cpp
    src/scenes/WorldMapState.cpp
    
    src/battle/Grid.cpp
    src/battle/Unit.cpp
    src/battle/UnitProgression.cpp
    src/battle/TurnQueue.cpp
    src/battle/CombatSystem.cpp
    src/battle/Pathfinder.cpp
    src/battle/MovementRange.cpp
    src/battle/BattleMap.cpp
    src/battle/UnitFactory.cpp
    src/battle/AttackResolutionController.cpp
    src/battle/PendingAttackController.cpp
    src/battle/HumanTurnController.cpp
    src/battle/BattleSession.cpp
    
    src/ai/EnemyAI.cpp
    
    src/renderer/BattleRenderer.cpp
    
    src/data/UnitLoader.cpp
    src/data/SkillLoader.cpp
    src/data/SettingsManager.cpp
    
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
    src/ui/UIScale.cpp
    src/ui/UnitPortrait.cpp
    src/ui/BattleHud.cpp  
    src/ui/FloatingTextSystem.cpp
    
    src/ui/windows/ButtonMenuWindow.cpp
    src/ui/windows/ConfirmWindow.cpp
    src/ui/windows/DialogWindow.cpp
    src/ui/windows/DeploymentWindow.cpp
    src/ui/windows/InspectWindow.cpp
    src/ui/windows/PartyWindow.cpp
    src/ui/windows/SettingsRowWindow.cpp
    src/ui/windows/UnitPanelWindow.cpp
    src/ui/windows/ButtonMenuWindow.cpp
    src/ui/windows/UnitInspectWindow.cpp
    
    )
    
    set(GAME_HEADERS
    src/scenes/BootState.h
    src/scenes/MainMenuState.h
    src/scenes/BattleState.h
    src/scenes/ResultState.h
    src/scenes/SettingsState.h
    src/scenes/AudioSettings.h
    src/scenes/GraphicsSettings.h
    src/scenes/WorldMapState.h
    
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
    src/battle/AttackResolutionController.h
    src/battle/PendingAttackController.h
    src/battle/HumanTurnController.h
    src/battle/BattleSession.h
    
    src/ai/EnemyAI.h
    
    src/renderer/BattleRenderer.h
    src/renderer/BattleRendererContext.h
    
    src/data/UnitLoader.h
    src/data/SkillLoader.h
    src/data/SettingsManager.h

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
    src/ui/UIScale.h
    src/ui/UnitPortrait.h
    src/ui/BattleHud.h  
    src/ui/BattleMenuItem.h
    src/ui/FloatingTextSystem.h
    src/ui/UIUtils.h
    
    src/ui/windows/ButtonMenuWindow.h
    src/ui/windows/ConfirmWindow.h
    src/ui/windows/DialogWindow.h
    src/ui/windows/DeploymentWindow.h
    src/ui/windows/InspectWindow.h
    src/ui/windows/PartyWindow.h
    src/ui/windows/SettingsRowWindow.h
    src/ui/windows/UnitPanelWindow.h
    src/ui/windows/ButtonMenuWindow.h
    src/ui/windows/UnitInspectWindow.h
    
    src/world/WorldGraph.h
    src/world/WorldPathfinding.h
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
