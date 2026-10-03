#include "data/QuestLoader.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <unordered_set>

using json = nlohmann::json;

static QuestObjectiveType parseObjectiveType(const std::string &s)
{
    if (s == "reach")
        return QuestObjectiveType::ReachNode;
    return QuestObjectiveType::VictoryAtNode; // default: "victory"
}

QuestDefinition QuestLoader::load(const std::string &filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        throw std::runtime_error("Could not open quest file: " + filePath);

    const json j = json::parse(file);

    QuestDefinition quest;
    quest.id = j.at("id").get<std::string>();
    quest.title = j.at("title").get<std::string>();
    quest.brief = j.value("brief", "");
    quest.description = j.value("description", "");
    quest.isMain = j.value("isMain", false);
    quest.offeredAtTavern = j.value("offeredAtTavern", false);
    quest.client = j.value("client", "");
    quest.cost = j.value("cost", 0);

    const json &objectiveJson = j.at("objective");
    quest.objective.type = parseObjectiveType(objectiveJson.value("type", std::string("victory")));
    quest.objective.nodeId = objectiveJson.value("nodeId", -1);

    if (j.contains("reward"))
    {
        const json &rewardJson = j.at("reward");
        quest.reward.gold = rewardJson.value("gold", 0);
        quest.reward.xp = rewardJson.value("xp", 0);
        if (rewardJson.contains("items") && rewardJson["items"].is_array())
        {
            for (const auto &item : rewardJson["items"])
            {
                if (item.is_string())
                    quest.reward.items.push_back(item.get<std::string>());
            }
        }
    }

    if (j.contains("unlocks") && j["unlocks"].is_array())
    {
        for (const auto &node : j["unlocks"])
        {
            if (node.is_number_integer())
                quest.unlocksNodeIds.push_back(node.get<int>());
        }
    }

    return quest;
}

std::vector<QuestDefinition> QuestLoader::loadAll(const std::string &directory)
{
    std::vector<QuestDefinition> quests;
    std::unordered_set<std::string> seenIds;

    for (const auto &entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.path().extension() == ".json")
        {
            QuestDefinition quest = load(entry.path().string());
            if (seenIds.count(quest.id))
            {
                throw std::runtime_error(
                    "Duplicate quest ID '" + quest.id + "' in " + entry.path().string());
            }
            seenIds.insert(quest.id);
            quests.push_back(std::move(quest));
        }
    }
    return quests;
}