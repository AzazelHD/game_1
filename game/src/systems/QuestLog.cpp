#include "systems/QuestLog.h"

#include "data/QuestLoader.h"

#include <algorithm>
#include <stdexcept>

void QuestLog::ensureLoaded()
{
    if (!m_catalog.empty())
        return;
    m_catalog = QuestLoader::loadAll("assets/quests");
}

const QuestDefinition *QuestLog::find(const std::string &id) const
{
    for (const QuestDefinition &quest : m_catalog)
    {
        if (quest.id == id)
            return &quest;
    }
    return nullptr;
}

bool QuestLog::isActive(const std::string &id) const
{
    const auto it = m_active.find(id);
    return it != m_active.end() && it->second;
}

bool QuestLog::isCompleted(const std::string &id) const
{
    const auto it = m_completed.find(id);
    return it != m_completed.end() && it->second;
}

void QuestLog::acceptAll()
{
    ensureLoaded();
    for (const QuestDefinition &quest : m_catalog)
        m_active[quest.id] = true;
}

void QuestLog::acceptAuto()
{
    ensureLoaded();
    for (const QuestDefinition &quest : m_catalog)
    {
        if (quest.offeredAtTavern)
            continue;
        m_active[quest.id] = true;
    }
}

bool QuestLog::accept(const std::string &id)
{
    ensureLoaded();
    if (isActive(id) || isCompleted(id))
        return false;
    if (find(id) == nullptr)
        return false;
    m_active[id] = true;
    return true;
}

std::vector<QuestDefinition> QuestLog::availableQuests() const
{
    std::vector<QuestDefinition> available;
    for (const QuestDefinition &quest : m_catalog)
    {
        if (!quest.offeredAtTavern)
            continue;
        if (isActive(quest.id) || isCompleted(quest.id))
            continue;
        available.push_back(quest);
    }
    return available;
}

bool QuestLog::hasActiveTavernQuest()
{
    ensureLoaded();
    for (const QuestDefinition &quest : m_catalog)
    {
        if (quest.offeredAtTavern && isActive(quest.id))
            return true;
    }
    return false;
}

std::vector<std::string> QuestLog::completeByNode(int nodeId, bool victorious)
{
    ensureLoaded();

    std::vector<std::string> completed;
    for (const QuestDefinition &quest : m_catalog)
    {
        if (!isActive(quest.id) || isCompleted(quest.id))
            continue;
        if (quest.objective.nodeId != nodeId)
            continue;

        const bool matches =
            (quest.objective.type == QuestObjectiveType::VictoryAtNode && victorious) ||
            (quest.objective.type == QuestObjectiveType::ReachNode);

        if (matches)
        {
            markCompleted(quest.id);
            completed.push_back(quest.id);
        }
    }
    return completed;
}

bool QuestLog::isNodeUnlocked(int nodeId) const
{
    if (m_catalog.empty())
        return true; // no quest data → nothing gates world nodes
    for (const QuestDefinition &quest : m_catalog)
    {
        const bool alreadyCompleted = isCompleted(quest.id);
        const bool gatesThisNode =
            std::find(quest.unlocksNodeIds.begin(), quest.unlocksNodeIds.end(), nodeId) !=
            quest.unlocksNodeIds.end();
        if (gatesThisNode && !alreadyCompleted)
            return false;
    }
    return true;
}

nlohmann::json QuestLog::toJson() const
{
    nlohmann::json j = nlohmann::json::object();

    nlohmann::json active = nlohmann::json::array();
    for (const auto &pair : m_active)
    {
        if (pair.second)
            active.push_back(pair.first);
    }
    j["active"] = std::move(active);

    nlohmann::json completed = nlohmann::json::array();
    for (const auto &pair : m_completed)
    {
        if (pair.second)
            completed.push_back(pair.first);
    }
    j["completed"] = std::move(completed);
    return j;
}

bool QuestLog::fromJson(const nlohmann::json &j)
{
    ensureLoaded();

    m_active.clear();
    m_completed.clear();

    if (j.contains("active") && j["active"].is_array())
    {
        for (const auto &entry : j["active"])
        {
            if (entry.is_string())
                m_active[entry.get<std::string>()] = true;
        }
    }
    if (j.contains("completed") && j["completed"].is_array())
    {
        for (const auto &entry : j["completed"])
        {
            if (entry.is_string())
                m_completed[entry.get<std::string>()] = true;
        }
    }
    return true;
}

void QuestLog::markCompleted(const std::string &id)
{
    m_completed[id] = true;
    m_active[id] = false;
}