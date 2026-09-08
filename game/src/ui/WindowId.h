#pragma once

enum class WindowId
{
    None,

    // Battle windows
    BattleActionConfirm,
    BattleActionMenu,
    BattleSystemMenu,
    BattleDeployment,
    BattleDeploymentConfirm,
    BattleDialog,
    BattleInspect,
    BattleInspectMenu,
    BattleSkillMenu,
    BattleUnitPanel,

    // Main game windows
    MainMenu,
    SaveList,
    SaveListDeleteConfirm,

    // Party
    PartyMenu,
    PartyInspect,
    PartyDetail,
    Equipment,
    Inventory,

    // Settings
    SettingsAudio,
    SettingsExitConfirm,
    SettingsGraphics,
    SettingsMenu,

    // World map
    WorldMap,
    WorldMapEncounterConfirm,
    WorldMapNodeInfo,
    WorldMapNodeMenu,
};
