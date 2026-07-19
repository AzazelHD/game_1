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
    Inspect,

    // Main menu
    OpenSettings,
    QuitGame,
    StartGame,

    // Settings
    AdjustLeft,
    AdjustRight,
    ApplyChanges,
    DiscardChanges,
    OpenAudioSettings,
    OpenGraphicSettings,

    // Shop
    BuyItem,
    CraftItem,
    SellItem,
    TradeItem,

    // Tavern
    AcceptMission,
    Talk,
    Whispers,

    // World map — node hub (city-type nodes)
    OpenInn,
    OpenPartyMenu,
    OpenPrison,
    OpenShop,
    OpenTavern,

    // World map — node interaction
    EnterBattle,
    InteractWithNode,
    MoveToNode,
    ViewMissionDetails,
};