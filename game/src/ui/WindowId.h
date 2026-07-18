#pragma once

enum class WindowId
{
    None,

    // Main game windows
    MainMenu,
    SettingsMenu,

    // World map
    WorldMap,
    WorldMapEncounterConfirm,
    WorldMapNodeMenu, // hub menu for city-type nodes: Tavern/Shop/Inn/Prison/etc.

    // Party
    PartyMenu,

    // Battle windows
    BattleUnitPanel,
    BattleDeployment,
    BattleDeploymentConfirm,

    BattleActionMenu,
    BattleSkillMenu,
    BattleActionConfirm,

    BattleInspect,
    BattleInspectMenu,

    BattleDialog,
};