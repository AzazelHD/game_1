#pragma once

#include "data/QuestData.h"

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>
#include <vector>

// QuestLog owns the loaded quest catalog (definitions) plus the runtime
// campaign state for each quest: accepted (active) and completed. It is a
// pure data/state container — reward application is left to the caller
// (PartyContext) so quest completion can drive UI (dialog/journal) in one
// place. Owned by PartyContext; serialized with the campaign save.
class QuestLog
{
public:
    // Loads the quest catalog (assets/quests). Safe to call repeatedly;
    // definitions loaded once.
    void ensureLoaded();

    // Definitions (loaded catalog).
    const QuestDefinition *find(const std::string &id) const;
    const std::vector<QuestDefinition> &all() const { return m_catalog; }

    // State queries.
    bool isActive(const std::string &id) const;
    bool isCompleted(const std::string &id) const;

    // Marks every catalog quest as active (used on New Game). Quest
    // definitions already completed stay completed.
    void acceptAll();

    // Marks quests that are not offered at the tavern as active (used on New
    // Game). Tavern-offered quests stay unaccepted until picked up from the
    // quest board, so the town can act as a quest-giver.
    void acceptAuto();

    // Accepts a single quest (tavern board pickup). Returns false when the id
    // is unknown, already active, or already completed.
    bool accept(const std::string &id);

    // Catalog quests that are offered at the tavern and not yet
    // accepted/completed — what the tavern quest board shows.
    std::vector<QuestDefinition> availableQuests() const;

    // True when any tavern-offered quest is currently active. Used to enforce
    // "one contracted task at a time" on the quest board.
    bool hasActiveTavernQuest();

    // Checks active quests whose objective is satisfied by the given node.
    //   completeByNode(id, /*victorious*/ true)  → VictoryAtNode objectives
    //   completeByNode(id, /*victorious*/ false) → ReachNode objectives
    // Returns the ids of quests that just became completed (already marked).
    std::vector<std::string> completeByNode(int nodeId, bool victorious);

    // A world-map node is traversable when no incomplete quest lists it in
    // unlocksNodeIds. Nodes referenced by no quest are always unlocked.
    bool isNodeUnlocked(int nodeId) const;

    // Serialization (campaign save format v3).
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json &j);

private:
    void markCompleted(const std::string &id);

    std::vector<QuestDefinition> m_catalog;
    std::unordered_map<std::string, bool> m_active;
    std::unordered_map<std::string, bool> m_completed;
};