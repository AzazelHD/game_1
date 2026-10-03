#pragma once

#include <string>
#include <vector>

// Objective a quest asks the player to achieve. The world "spine" uses
// node-based objectives; future objective types (collect, hunt, survive)
// can be added without restructuring the loader.
enum class QuestObjectiveType
{
    VictoryAtNode, // win the battle at nodeId
    ReachNode,     // arrive at nodeId without a battle requirement
};

struct QuestObjective
{
    QuestObjectiveType type = QuestObjectiveType::VictoryAtNode;
    int nodeId = -1;
};

struct QuestReward
{
    int gold = 0;
    int xp = 0;
    std::vector<std::string> items; // gear catalog keys granted on completion
};

// QuestDefinition: a quest in the campaign spine. Loaded from JSON via
// QuestLoader; consumed by QuestLog (runtime state) and the journal UI.
struct QuestDefinition
{
    std::string id;             // unique key (e.g. "first_watch")
    std::string title;          // display name
    std::string brief;          // journal one-liner
    std::string description;    // journal full-text flavour (optional)
    bool isMain = false;        // main quest vs side quest

    // Who offers the quest (shown on the quest board info window).
    std::string client;         // display-only flavour (optional)

    // Gold fee charged when the player accepts the quest from the town quest
    // board. 0 = free. Main/auto-accepted quests never pay this.
    int cost = 0;

    QuestObjective objective;
    QuestReward reward;

    // True when this quest is picked up from the town tavern quest board
    // rather than auto-accepted on New Game. Defaults to false.
    bool offeredAtTavern = false;

    // World-map nodes unlocked by completing this quest. Gating is
    // one-way: completing the quest makes these nodes traversable.
    std::vector<int> unlocksNodeIds;
};