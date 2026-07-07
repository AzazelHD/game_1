#pragma once

#include "battle/MovementRange.h"

#include <string>
#include <unordered_set>
#include <vector>

class RosterSystem;

struct DeploymentEntry
{
    std::string unitId;
    std::string templatePath;
    Vec2i position{0, 0};
};

class DeploymentSystem
{
public:
    void initialize(int maxUnits,
                    std::unordered_set<Vec2i, Vec2iHash> spawnTiles,
                    std::vector<std::string> partyUnitIds,
                    const RosterSystem &roster);

    bool isInitialized() const { return m_initialized; }
    int maxUnits() const { return m_maxUnits; }
    int placedCount() const { return static_cast<int>(m_deployed.size()); }
    bool canStartBattle() const { return !m_deployed.empty(); }
    bool hasGrabbedUnit() const { return !m_grabbedUnitId.empty(); }

    bool isSpawnTile(Vec2i pos) const;
    bool isOccupied(Vec2i pos) const;
    bool isUnitPlaced(const std::string &unitId) const;

    bool grabSelected();
    bool grabUnit(const std::string &unitId);
    void releaseGrabbed();
    bool placeGrabbed(Vec2i pos);
    bool unplaceUnit(const std::string &unitId);

    void cycleSelection(int delta);
    const DeploymentEntry *selectedEntry() const;

    // True when the roster selection points at a unit that's already on the
    // field and nothing is currently grabbed — in that state the cursor is
    // meant to stay pinned to that unit's tile rather than move freely
    // (there's nothing to "browse" for; Accept re-grabs it to relocate).
    bool isSelectionLocked() const
    {
        if (hasGrabbedUnit())
            return false;
        const DeploymentEntry *entry = selectedEntry();
        return entry && isUnitPlaced(entry->unitId);
    }
    const DeploymentEntry *grabbedEntry() const;
    const std::vector<DeploymentEntry> &partyEntries() const { return m_partyEntries; }

    const std::unordered_set<Vec2i, Vec2iHash> &spawnTiles() const { return m_spawnTiles; }
    const std::vector<DeploymentEntry> &deployed() const { return m_deployed; }

private:
    bool m_initialized = false;
    int m_maxUnits = 0;
    std::unordered_set<Vec2i, Vec2iHash> m_spawnTiles;
    std::vector<DeploymentEntry> m_partyEntries;
    std::vector<DeploymentEntry> m_deployed;
    std::size_t m_selectedIndex = 0;
    std::string m_grabbedUnitId;
};