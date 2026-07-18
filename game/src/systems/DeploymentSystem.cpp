#include "config/BattleCatalog.h"
#include "systems/DeploymentSystem.h"
#include "systems/RosterSystem.h"

#include <algorithm>

void DeploymentSystem::initialize(int maxUnits,
                                  std::unordered_set<Vec2i, Vec2iHash> spawnTiles,
                                  std::vector<int> partyInstanceIds,
                                  const RosterSystem &roster,
                                  const std::vector<ForcedUnitRule> &forcedUnits)
{
    m_initialized = true;
    m_maxUnits = std::max(1, maxUnits);
    m_spawnTiles = std::move(spawnTiles);
    m_partyEntries.clear();
    m_deployed.clear();
    m_selectedIndex = 0;
    m_grabbedInstanceId = -1;

    for (int id : partyInstanceIds)
    {
        const RosterUnit *unit = roster.findById(id);
        if (!unit)
            continue;

        m_partyEntries.push_back(DeploymentEntry{
            .instanceId = unit->instanceId,
            .templatePath = unit->templatePath,
            .position = Vec2i{0, 0},
        });
    }

    for (const ForcedUnitRule &rule : forcedUnits)
    {
        const RosterUnit *unit = roster.findByRef(rule.unitRef);
        if (!unit)
            continue;

        m_deployed.push_back(DeploymentEntry{
            .instanceId = unit->instanceId,
            .templatePath = unit->templatePath,
            .position = rule.position,
            .locked = true,
            .critical = rule.isCritical,
        });
    }
}

bool DeploymentSystem::isSpawnTile(Vec2i pos) const
{
    return m_spawnTiles.find(pos) != m_spawnTiles.end();
}

std::unordered_set<Vec2i, Vec2iHash> DeploymentSystem::visibleSpawnTiles() const
{
    std::unordered_set<Vec2i, Vec2iHash> result;
    for (const Vec2i &tile : m_spawnTiles)
    {
        bool lockedHere = false;
        for (const DeploymentEntry &e : m_deployed)
        {
            if (e.locked && e.position == tile)
            {
                lockedHere = true;
                break;
            }
        }
        if (!lockedHere)
            result.insert(tile);
    }
    return result;
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

bool DeploymentSystem::isUnitPlaced(int instanceId) const
{
    for (const DeploymentEntry &e : m_deployed)
    {
        if (e.instanceId == instanceId)
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

    if (isUnitPlaced(selected->instanceId))
        return false;

    return grabUnit(selected->instanceId);
}

bool DeploymentSystem::grabUnit(int instanceId)
{
    if (!m_initialized || instanceId < 0)
        return false;

    if (const DeploymentEntry *placed = deployedEntryFor(instanceId))
    {
        if (placed->locked)
            return false;
    }

    auto it = std::find_if(m_partyEntries.begin(), m_partyEntries.end(), [instanceId](const DeploymentEntry &entry)
                           { return entry.instanceId == instanceId; });
    if (it == m_partyEntries.end())
        return false;

    // Whatever you're holding IS the selection — grabbing a unit (fresh
    // from the roster, or picked back up from a placed tile) always moves
    // the roster selection to match it.
    m_selectedIndex = static_cast<std::size_t>(std::distance(m_partyEntries.begin(), it));
    m_grabbedInstanceId = instanceId;
    return true;
}

void DeploymentSystem::releaseGrabbed()
{
    m_grabbedInstanceId = -1;
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

    if (isUnitPlaced(grabbed->instanceId))
        return false;

    DeploymentEntry placed = *grabbed;
    placed.position = pos;
    m_deployed.push_back(std::move(placed));
    m_grabbedInstanceId = -1;

    // Land the selection on whichever unplaced unit is first in roster
    // order — same rule every time, regardless of whether this was a
    // fresh pick or a reposition. If everything is placed, selection just
    // stays put.
    for (std::size_t i = 0; i < m_partyEntries.size(); ++i)
    {
        if (!isUnitPlaced(m_partyEntries[i].instanceId))
        {
            m_selectedIndex = i;
            break;
        }
    }

    return true;
}

bool DeploymentSystem::swapGrabbedWithPlacedAt(Vec2i pos)
{
    if (!m_initialized || !hasGrabbedUnit())
        return false;

    if (!isSpawnTile(pos))
        return false;

    const DeploymentEntry *occupant = deployedEntryAt(pos);
    if (!occupant || occupant->locked)
        return false;

    const int occupantId = occupant->instanceId;
    const DeploymentEntry *grabbed = grabbedEntry();
    if (!grabbed)
        return false;

    const std::string templatePath = grabbed->templatePath;
    const int grabbedId = grabbed->instanceId;

    // Remove B from the field, place A in its slot.
    if (!unplaceUnit(occupantId))
        return false;

    DeploymentEntry placedA{
        .instanceId = grabbedId,
        .templatePath = templatePath,
        .position = pos,
    };
    m_deployed.push_back(std::move(placedA));

    // B is now the one in hand.
    m_grabbedInstanceId = -1;
    grabUnit(occupantId);

    return true;
}

bool DeploymentSystem::unplaceUnit(int instanceId)
{
    auto it = std::find_if(m_deployed.begin(), m_deployed.end(), [instanceId](const DeploymentEntry &entry)
                           { return entry.instanceId == instanceId; });
    if (it == m_deployed.end())
        return false;

    if (it->locked)
        return false;

    m_deployed.erase(it);
    return true;
}

void DeploymentSystem::cycleSelection(int delta)
{
    if (m_partyEntries.empty())
        return;

    const int count = static_cast<int>(m_partyEntries.size());
    const int next = (static_cast<int>(m_selectedIndex) + delta + count) % count;
    m_selectedIndex = static_cast<std::size_t>(next);

    // Cycling while holding a unit swaps what's in hand: drop whatever was
    // grabbed (it simply returns to the roster unplaced — it was never
    // placed on the field, so there's nothing to unplace), then grab the
    // newly-selected entry instead.
    if (hasGrabbedUnit())
    {
        m_grabbedInstanceId = -1;
        grabUnit(m_partyEntries[m_selectedIndex].instanceId);
    }
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
                           { return entry.instanceId == m_grabbedInstanceId; });
    return it == m_partyEntries.end() ? nullptr : &(*it);
}

const DeploymentEntry *DeploymentSystem::deployedEntryFor(int instanceId) const
{
    auto it = std::find_if(m_deployed.begin(), m_deployed.end(), [instanceId](const DeploymentEntry &entry)
                           { return entry.instanceId == instanceId; });
    return it == m_deployed.end() ? nullptr : &(*it);
}

const DeploymentEntry *DeploymentSystem::deployedEntryAt(Vec2i pos) const
{
    auto it = std::find_if(m_deployed.begin(), m_deployed.end(), [pos](const DeploymentEntry &entry)
                           { return entry.position == pos; });
    return it == m_deployed.end() ? nullptr : &(*it);
}