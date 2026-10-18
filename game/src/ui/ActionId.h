#pragma once

enum class ActionId
{
    None,

    // Battle — flow
    StartCombat,

    // Battle — turn actions
    Attack,
    Defend,
    Move,
    OpenItemsMenu,
    OpenSkillsMenu,
    UseSkill,
    Wait,

    // Generic UI actions
    Accept,
    Back,
    Cancel,
    Close,
    Confirm,
    CycleNext,
    CyclePrev,
    Details,
    Equip,
    Inspect,

    // Main menu
    StartGame,
    ContinueGame,
    LoadGame,
    DeleteSave,
    OpenSettings,
    QuitGame,

    // In-game (world map) — leave the campaign and go back to the title screen.
    // Quitting the application is the title screen's job.
    ReturnToTitle,

    // Settings
    OpenGraphicSettings,
    OpenAudioSettings,
    AdjustLeft,
    AdjustRight,

    // Shop
    BuyItem,
    CraftItem,
    SellItem,
    TradeItem,

    // Tavern
    AcceptMission,
    Recruit,
    Talk,
    Whispers,

    // World map — node hub (city-type nodes)
    OpenInn,
    OpenInventory,
    OpenJournal,
    OpenPartyMenu,
    OpenPrison,
    OpenShop,
    OpenTavern,
    SaveGame,

    // Party — unit detail
    OpenSkills,

    // World map — node interaction
    EnterBattle,
    InteractWithNode,
    MoveToNode,
    PlayNodeEvent,
    ViewMissionDetails,
};
