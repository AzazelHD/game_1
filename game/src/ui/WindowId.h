#pragma once

enum class WindowId
{
    None,

    // Battle windows
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
    PartySkills,
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
    WorldMapJournal,
    WorldMapNodeEvents,
    WorldMapNodeInfo,
    WorldMapNodeMenu,
    WorldMapQuestComplete,

    // Town — shop
    Shop,
    ShopBuy,
    ShopSell,
    ShopConfirm,

    // Town — tavern
    Tavern,
    TavernQuestBoard,
    TavernQuestAccept,
    TavernQuestDetail,
    TavernRecruitList,
    TavernRecruitConfirm,
    TavernTalk,

    // Town — hub talk
    TownTalk,
};
