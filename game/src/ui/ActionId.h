#pragma once

enum class ActionId
{
    None,

    // Generic UI actions
    Confirm,
    Cancel,
    Accept,
    Back,
    Close,
    Details,
    CyclePrev,
    CycleNext,
    Inspect,

    // Main menu
    StartGame,
    OpenSettings,
    QuitGame,

    // World map — node interaction
    MoveToNode,
    InteractWithNode,
    ViewMissionDetails,
    EnterBattle,

    // World map — node hub (city-type nodes)
    OpenPartyMenu,
    OpenTavern,
    OpenShop,
    OpenInn,
    OpenPrison,

    // Tavern
    AcceptMission,
    Talk,
    Whispers,

    // Shop
    BuyItem,
    SellItem,
    TradeItem,
    CraftItem,

    // Battle — flow
    StartCombat,

    // Battle — turn actions
    Move,
    Attack,
    OpenSkillsMenu,
    UseSkill,
    OpenItemsMenu,
    Defend,
    Wait,

    // Settings
    ApplyChanges,
    DiscardChanges,
    OpenGraphicSettings,
    OpenAudioSettings,
};