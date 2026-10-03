#pragma once

#include "engine/math/Vec2.h"
#include "engine/renderer/Color.h"

#include <string>
#include <vector>
#include <unordered_map>

enum class BattleTriggerType
{
    OnBattleStart,
    OnUnitDeath,
    OnTurnStart,
    OnTurnEnd,
    OnVictory,
    OnDefeat,
    OnTileEnter,
};

enum class BattleActionType
{
    SpawnUnit,
    ShowDialogue,
    GiveReward,
    PlayAnimation,
    ModifyVariable,
    StartCutscene,
    EndBattle,
};

struct BattleEventAction
{
    BattleActionType type = BattleActionType::ShowDialogue;
    std::string text;
    std::string unitTemplatePath;
    int value = 0;
    bool flag = false;
};

struct BattleEvent
{
    BattleTriggerType trigger = BattleTriggerType::OnBattleStart;
    // Wave scripting (M5): minTurn fires the event only once the battle's
    // turn counter (session.m_turnsElapsed) has reached this value — used
    // with OnTurnStart/OnTurnEnd to stage reinforcements mid-fight.
    int minTurn = 0;
    // Once-fired events execute a single time even if the trigger fires
    // repeatedly (e.g. a reinforcement that must spawn only once).
    bool once = false;
    std::vector<BattleEventAction> actions;
};

struct EnemyDefinition
{
    std::string id;
    std::string templatePath;
    int level = 1;
    int team = 2;
};

struct BattleObjective
{
    std::string type = "defeat_all";
};

struct BattleRewards
{
    // Per-unit XP granted on victory (on top of GameConstants::BATTLE_VICTORY_XP).
    int xp = 0;

    // Currency granted once to the campaign on victory.
    int gold = 0;

    // Gear keys (GearCatalog::findByKey) granted to the shared inventory on
    // victory, one copy per entry in this list.
    std::vector<std::string> items;
};

enum class VictoryConditionType
{
    KillAll,
    KillBoss,
    SurviveTurns,
};

enum class DefeatConditionType
{
    AllAlliesDead,
    CriticalUnitDied,
};

struct BattleVictoryRule
{
    VictoryConditionType type = VictoryConditionType::KillAll;
    int turnsToSurvive = 0; // used only when type == SurviveTurns
};

struct BattleDefeatRule
{
    DefeatConditionType type = DefeatConditionType::AllAlliesDead;
};

struct ForcedUnitRule
{
    std::string unitRef;
    Vec2i position{0, 0};
    bool isCritical = false;
};

struct BattleDefinition
{
    std::string id;
    std::string mapPath;
    FColor backgroundTop;
    FColor backgroundBottom;
    std::string musicId;
    float defaultZoom = 1.0f;
    int maxUnits = 1;
    std::vector<EnemyDefinition> enemies;
    std::vector<std::string> playerUnits;
    std::vector<ForcedUnitRule> forcedUnits;
    BattleObjective objectives;
    BattleRewards rewards;
    std::vector<BattleEvent> events;
    BattleVictoryRule victoryRule{};
    BattleDefeatRule defeatRule{};
};

const BattleDefinition &getBattleDefinition(const std::string &mapPath);
