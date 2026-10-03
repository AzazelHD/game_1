#pragma once

#include "data/QuestData.h"

#include <string>
#include <vector>

// QuestLoader reads quest definitions from JSON.
//
// Expected JSON schema:
// {
//   "id": "first_watch",
//   "title": "Claim the First Watch",
//   "brief": "Win the battle at the watchtower.",
//   "description": "A longer flavour line (optional).",
//   "isMain": true,
//   "objective": { "type": "victory", "nodeId": 2 },
//   "reward": { "gold": 80, "xp": 20, "items": [] },
//   "unlocks": [6]
// }
class QuestLoader
{
public:
    static QuestDefinition load(const std::string &filePath);
    static std::vector<QuestDefinition> loadAll(const std::string &directory);
};