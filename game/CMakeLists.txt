# This file is the single source of truth for the game executable target.
# Keep all game source/header registration here to avoid drift with the root
# CMakeLists.

set(GAME_SOURCES
    src/main.cpp

    src/states/BootState.cpp
    src/states/MainMenuState.cpp
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

    src/data/UnitLoader.cpp
    src/data/SkillLoader.cpp

    src/ui/Cursor.cpp
    src/ui/ActionMenu.cpp
    src/ui/UnitPanel.cpp
    src/ui/DamagePreview.cpp
    src/ui/DialogBox.cpp
    src/ui/DialogSystem.cpp
)

set(GAME_HEADERS
    src/states/BootState.h
    src/states/MainMenuState.h
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

    src/data/UnitLoader.h
    src/data/SkillLoader.h

    src/ui/Cursor.h
    src/ui/ActionMenu.h
    src/ui/UnitPanel.h
    src/ui/DamagePreview.h
    src/ui/DialogBox.h
    src/ui/DialogSystem.h
)

add_executable(game
    ${GAME_SOURCES}
    ${GAME_HEADERS}
)

# Header layout policy:
# - src/ is the active implementation and header tree for the game target.
# - add include/ only when a real public game API is introduced.
target_include_directories(game PRIVATE
    src
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
