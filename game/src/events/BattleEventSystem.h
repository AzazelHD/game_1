#pragma once

#include "config/BattleCatalog.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class BattleEventSystem
{
public:
    struct Callbacks
    {
        std::function<void(const std::string &)> showDialogue;
        std::function<void(const std::string &)> spawnUnitByTemplate;
        std::function<void(int)> giveRewardXp;
        std::function<void(const std::string &)> playAnimation;
        std::function<void(const std::string &)> startCutscene;
        std::function<void(bool)> endBattle;
    };

    void initialize(const BattleDefinition &definition, Callbacks callbacks);

    void emit(BattleTriggerType trigger);

    // Feeds the battle's turn counter; events with `minTurn > 0` fire only
    // once m_turn has reached it (see BattleEvent). Call before emit() of
    // turn-gated triggers.
    void setTurn(int turn) { m_turn = turn; }

private:
    void executeAction(const BattleEventAction &action);

    std::vector<BattleEvent> m_events;
    std::unordered_map<std::string, int> m_variables;
    std::vector<int> m_firedOnce; // indices of `once` events already executed
    int m_turn = 0;
    Callbacks m_callbacks;
};
