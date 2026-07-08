#include "systems/DeploymentSystem.h"

#include "systems/RosterSystem.h"

#include <algorithm>

void DeploymentSystem::initialize(int maxUnits,
                                  std::unordered_set<Vec2i, Vec2iHash> spawnTiles,
                                  std::vector<std::string> partyUnitIds,
                                  const RosterSystem &roster)
{
    m_initialized = true;
    m_maxUnits = std::max(1, maxUnits);
    m_spawnTiles = std::move(spawnTiles);
    m_partyEntries.clear();
    m_deployed.clear();
    m_selectedIndex = 0;
    m_grabbedUnitId.clear();

    for (const std::string &id : partyUnitIds)
    {
        const RosterUnit *unit = roster.findById(id);
        if (!unit)
            continue;

        m_partyEntries.push_back(DeploymentEntry{
            .unitId = unit->unitId,
            .templatePath = unit->templatePath,
            .position = Vec2i{0, 0},
        });
    }
}

bool DeploymentSystem::isSpawnTile(Vec2i pos) const
{
    return m_spawnTiles.find(pos) != m_spawnTiles.end();
}

bool DeploymentSystem::isOccupied(Vec2i pos) const
{
    for (const DeploymentEntry &e : m_deployed)
    {
        if (e.position == pos)
            return true;
    }
    return false;
}

bool DeploymentSystem::isUnitPlaced(const std::string &unitId) const
{
    for (const DeploymentEntry &e : m_deployed)
    {
        if (e.unitId == unitId)
            return true;
    }
    return false;
}

bool DeploymentSystem::grabSelected()
{
    if (!m_initialized || m_partyEntries.empty())
        return false;

    const DeploymentEntry *selected = selectedEntry();
    if (!selected)
        return false;

    if (isUnitPlaced(selected->unitId))
        return false;

    return grabUnit(selected->unitId);
}

bool DeploymentSystem::grabUnit(const std::string &unitId)
{
    if (!m_initialized || unitId.empty())
        return false;

    auto it = std::find_if(m_partyEntries.begin(), m_partyEntries.end(), [&unitId](const DeploymentEntry &entry)
                           { return entry.unitId == unitId; });
    if (it == m_partyEntries.end())
        return false;

    m_grabbedUnitId = unitId;
    return true;
}

void DeploymentSystem::releaseGrabbed()
{
    m_grabbedUnitId.clear();
}

bool DeploymentSystem::placeGrabbed(Vec2i pos)
{
    if (!m_initialized || !hasGrabbedUnit())
        return false;

    if (!isSpawnTile(pos) || isOccupied(pos))
        return false;

    if (static_cast<int>(m_deployed.size()) >= m_maxUnits)
        return false;

    const DeploymentEntry *grabbed = grabbedEntry();
    if (!grabbed)
        return false;

    if (isUnitPlaced(grabbed->unitId))
        return false;

    DeploymentEntry placed = *grabbed;
    placed.position = pos;
    m_deployed.push_back(std::move(placed));
    m_grabbedUnitId.clear();

    // Move the roster selection to the next entry so the player doesn't have
    // to manually cycle past the unit they just placed.
    if (!m_partyEntries.empty())
        m_selectedIndex = (m_selectedIndex + 1) % m_partyEntries.size();

    return true;
}

bool DeploymentSystem::unplaceUnit(const std::string &unitId)
{
    auto it = std::find_if(m_deployed.begin(), m_deployed.end(), [&unitId](const DeploymentEntry &entry)
                           { return entry.unitId == unitId; });
    if (it == m_deployed.end())
        return false;

    m_deployed.erase(it);
    return true;
}

void DeploymentSystem::cycleSelection(int delta)
{
    if (m_partyEntries.empty() || hasGrabbedUnit())
        return;

    const int count = static_cast<int>(m_partyEntries.size());
    const int next = (static_cast<int>(m_selectedIndex) + delta + count) % count;
    m_selectedIndex = static_cast<std::size_t>(next);
}

const DeploymentEntry *DeploymentSystem::selectedEntry() const
{
    if (m_partyEntries.empty() || m_selectedIndex >= m_partyEntries.size())
        return nullptr;
    return &m_partyEntries[m_selectedIndex];
}

const DeploymentEntry *DeploymentSystem::grabbedEntry() const
{
    if (!hasGrabbedUnit())
        return nullptr;

    auto it = std::find_if(m_partyEntries.begin(), m_partyEntries.end(), [this](const DeploymentEntry &entry)
                           { return entry.unitId == m_grabbedUnitId; });
    return it == m_partyEntries.end() ? nullptr : &(*it);
}

const DeploymentEntry *DeploymentSystem::deployedEntryFor(const std::string &unitId) const
{
    auto it = std::find_if(m_deployed.begin(), m_deployed.end(), [&unitId](const DeploymentEntry &entry)
                           { return entry.unitId == unitId; });
    return it == m_deployed.end() ? nullptr : &(*it);
}

const DeploymentEntry *DeploymentSystem::deployedEntryAt(Vec2i pos) const
{
    auto it = std::find_if(m_deployed.begin(), m_deployed.end(), [pos](const DeploymentEntry &entry)
                           { return entry.position == pos; });
    return it == m_deployed.end() ? nullptr : &(*it);
}