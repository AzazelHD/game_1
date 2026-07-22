#pragma once
#include "battle/MovementRange.h"
#include <string>
#include <unordered_set>
#include <vector>

class RosterSystem;
struct ForcedUnitRule;

struct DeploymentEntry
{
    int instanceId = -1;
    std::string templatePath;
    Vec2i position{0, 0};
    bool locked = false;   // forced by BattleDefinition — cannot be grabbed/unplaced/swapped out
    bool critical = false; // losing this unit is an instant defeat
};

class DeploymentSystem
{
public:
    void initialize(int maxUnits,
                    std::unordered_set<Vec2i, Vec2iHash> spawnTiles,
                    std::vector<int> partyInstanceIds,
                    const RosterSystem &roster,
                    const std::vector<ForcedUnitRule> &forcedUnits);

    bool isInitialized() const { return m_initialized; }
    int maxUnits() const { return m_maxUnits; }
    int placedCount() const { return static_cast<int>(m_deployed.size()); }
    bool canStartBattle() const { return !m_deployed.empty(); }

    bool hasGrabbedUnit() const { return m_grabbedInstanceId != -1; }

    bool isSpawnTile(Vec2i pos) const;
    bool isOccupied(Vec2i pos) const;
    bool isUnitPlaced(int instanceId) const;

    bool grabSelected();
    bool grabUnit(int instanceId);
    void releaseGrabbed();
    bool placeGrabbed(Vec2i pos);
    // Swaps the grabbed unit into the tile currently occupied by another
    // placed unit: the tile's occupant is unplaced and becomes the new
    // grabbed unit, while the previously-grabbed unit is placed onto that
    // tile. Returns false (no-op) if nothing is grabbed, or if pos isn't
    // occupied by one of YOUR placed units (deployedEntryAt only ever
    // returns your own entries — enemy/ally/neutral preview units live in
    // a separate list BattleState owns, never touched here).
    bool swapGrabbedWithPlacedAt(Vec2i pos);
    bool unplaceUnit(int instanceId);

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
        return entry && isUnitPlaced(entry->instanceId);
    }

    const DeploymentEntry *grabbedEntry() const;

    // Looks up the *live* placed position for instanceId (m_deployed), as
    // opposed to selectedEntry()/partyEntries(), whose .position field is
    // never updated after placement and is meaningless for placed units.
    const DeploymentEntry *deployedEntryFor(int instanceId) const;
    const DeploymentEntry *deployedEntryAt(Vec2i pos) const;

    const std::vector<DeploymentEntry> &partyEntries() const { return m_partyEntries; }
    const std::unordered_set<Vec2i, Vec2iHash> &spawnTiles() const { return m_spawnTiles; }
    std::unordered_set<Vec2i, Vec2iHash> visibleSpawnTiles() const;
    const std::vector<DeploymentEntry> &deployed() const { return m_deployed; }

private:
    bool m_initialized = false;
    int m_maxUnits = 0;
    std::unordered_set<Vec2i, Vec2iHash> m_spawnTiles;
    std::vector<DeploymentEntry> m_partyEntries;
    std::vector<DeploymentEntry> m_deployed;
    std::size_t m_selectedIndex = 0;
    int m_grabbedInstanceId = -1;
};
