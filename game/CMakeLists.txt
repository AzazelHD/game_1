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
    src/scenes/WorldMapState.cpp
    
    src/battle/map/Grid.cpp
    src/battle/unit/Unit.cpp
    src/battle/unit/UnitProgression.cpp
    src/battle/unit/SkillProgression.cpp
    src/battle/unit/RecruitmentRules.cpp
    src/inventory/Item.cpp
    src/inventory/Inventory.cpp
    src/inventory/Gear.cpp
    src/inventory/EquipRules.cpp
    src/inventory/GearCatalog.cpp
    src/battle/combat/TurnQueue.cpp
    src/battle/combat/CombatSystem.cpp
    src/battle/map/Pathfinder.cpp
    src/battle/map/MovementRange.cpp
    src/battle/map/BattleMap.cpp
    src/battle/unit/UnitFactory.cpp
    src/battle/controllers/AttackResolutionController.cpp
    src/battle/controllers/MovementAnimationController.cpp
    src/battle/combat/PendingAttackController.cpp
    src/battle/controllers/HumanTurnController.cpp
    src/battle/controllers/DialogueController.cpp
    src/battle/BattleSession.cpp
    src/battle/controllers/DeploymentPhaseController.cpp
    src/battle/controllers/BattleMenuController.cpp
    src/battle/controllers/DeploymentPhaseController.cpp
    src/battle/systems/DeploymentSystem.cpp
    src/battle/systems/BattleParticipantsBuilder.cpp
    src/battle/systems/CombatAnimationSystem.cpp
    src/battle/ui/UnitPanelWindow.cpp
    src/battle/ui/UnitInspectWindow.cpp
    src/battle/ui/DeploymentWindow.cpp
    
    src/ai/EnemyAI.cpp
    
    src/renderer/BattleRenderer.cpp
    
    src/data/UnitLoader.cpp
    src/data/SkillLoader.cpp
    src/data/SettingsManager.cpp
    
    src/config/BattleCatalog.cpp
    
    src/systems/RosterSystem.cpp
    src/systems/PartySystem.cpp
    src/systems/PartyContext.cpp
    
    src/events/BattleEventSystem.cpp

    src/settings/SettingsState.cpp
    src/settings/AudioSettings.cpp
    src/settings/GraphicsSettings.cpp
    src/settings/ui/SettingsRowWindow.cpp
    
    src/ui/Cursor.cpp
    src/ui/UIManager.cpp
    src/ui/DamagePreview.cpp
    src/ui/UIScale.cpp
    src/ui/UnitPortrait.cpp
    src/ui/FloatingTextSystem.cpp
    src/ui/UIKeyRepeat.cpp
    
    src/ui/windows/ButtonMenuWindow.cpp
    src/ui/windows/ConfirmWindow.cpp
    src/ui/windows/DialogWindow.cpp
    src/ui/windows/InventoryWindow.cpp
    src/ui/windows/PartyWindow.cpp
    src/ui/windows/UnitDetailWindow.cpp
    
    )
    
    set(GAME_HEADERS
    src/scenes/BootState.h
    src/scenes/MainMenuState.h
    src/scenes/BattleState.h
    src/scenes/ResultState.h
    src/scenes/WorldMapState.h
    
    src/battle/map/Grid.h
    src/battle/unit/Unit.h
    src/battle/unit/UnitData.h
    src/battle/unit/UnitProgression.h
    src/battle/unit/SkillProgression.h
    src/battle/unit/RecruitmentRules.h
    src/inventory/Item.h
    src/inventory/Inventory.h
    src/inventory/Gear.h
    src/inventory/EquipRules.h
    src/inventory/GearCatalog.h
    src/battle/combat/TurnQueue.h
    src/battle/combat/CombatSystem.h
    src/battle/map/Pathfinder.h
    src/battle/map/MovementRange.h
    src/battle/map/GameTile.h
    src/battle/map/BattleMap.h
    src/battle/unit/UnitFactory.h
    src/battle/controllers/AttackResolutionController.h
    src/battle/controllers/MovementAnimationController.h
    src/battle/combat/PendingAttackController.h
    src/battle/controllers/HumanTurnController.h
    src/battle/controllers/DialogueController.h
    src/battle/BattleSession.h
    src/battle/controllers/DeploymentPhaseController.h
    src/battle/controllers/BattleMenuController.h
    src/battle/controllers/DeploymentPhaseController.h
    src/battle/systems/DeploymentSystem.h
    src/battle/systems/BattleParticipantsBuilder.h
    src/battle/systems/CombatAnimationSystem.h
    src/battle/ui/UnitPanelWindow.h
    src/battle/ui/UnitInspectWindow.h
    src/battle/ui/DeploymentWindow.h
    
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
    
    src/settings/SettingsState.h
    src/settings/AudioSettings.h
    src/settings/GraphicsSettings.h
    src/settings/ui/SettingsRowWindow.h
    
    src/events/BattleEventSystem.h
    
    src/ui/Cursor.h
    src/ui/UITheme.h
    src/ui/UIEvent.h
    src/ui/UIWindow.h
    src/ui/UIManager.h
    src/ui/DamagePreview.h
    src/ui/UIScale.h
    src/ui/UnitPortrait.h
    src/ui/BattleMenuItem.h
    src/ui/FloatingTextSystem.h
    src/ui/UIUtils.h
    src/ui/UIKeyRepeat.h
    
    src/ui/windows/ButtonMenuWindow.h
    src/ui/windows/ConfirmWindow.h
    src/ui/windows/DialogWindow.h
    src/ui/windows/InventoryWindow.h
    src/ui/windows/PartyWindow.h
    src/ui/windows/UnitDetailWindow.h
    
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
