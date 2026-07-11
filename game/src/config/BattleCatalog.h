#pragma once

#include "engine/renderer/Color.h"

#include <string>
#include <unordered_map>
#include <vector>

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
    int xp = 0;
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
    BattleObjective objectives;
    BattleRewards rewards;
    std::vector<BattleEvent> events;
    BattleVictoryRule victoryRule{};
    BattleDefeatRule defeatRule{};
};

const BattleDefinition &getBattleDefinition(const std::string &mapPath);
