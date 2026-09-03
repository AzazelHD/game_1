#pragma once

#include "engine/math/Vec2.h"

#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Grid;
class BattleMap;
class Unit;

struct MovementRangeOptions
{
    // Used by GearSpecialEffect::TeleportMovement. Occupied tiles may be
    // traversed while searching, but never returned as landing destinations.
    bool ignoreUnitCollisionAlongPath = false;
};

struct Vec2iHash
{
    std::size_t operator()(Vec2i v) const noexcept
    {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 16);
    }
};

// MovementRange computes which tiles are reachable from a start position
// using BFS flood-fill with a movement budget.
class MovementRange
{
public:
    // reachable and costs are populated together from a single BFS pass —
    // costs[tile] is the ACTUAL accumulated path cost to reach that tile
    // (accounting for obstacles/detours), never a straight-line estimate.
    // Every key in reachable has a corresponding entry in costs, always.
    struct Result
    {
        std::unordered_set<Vec2i, Vec2iHash> reachable;
        std::unordered_map<Vec2i, int, Vec2iHash> costs;
    };

    static Result compute(
        const Grid &grid,
        const BattleMap &battleMap,
        Vec2i start,
        int movementPoints,
        int team,
        const std::vector<Unit *> &allUnits,
        int jump,
        MovementRangeOptions options = {});
};