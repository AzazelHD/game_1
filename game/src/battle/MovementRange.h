#pragma once

#include "Unit.h"
#include "engine/math/Vec2.h"

#include <unordered_set>
#include <queue>

class Grid;

// Hash functor for Vec2i — required by std::unordered_set and std::unordered_map.
struct Vec2iHash
{
    std::size_t operator()(Vec2i v) const noexcept
    {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 16);
    }
};

// MovementRange computes which tiles are reachable from a start position
// using BFS flood-fill with a movement budget.
//
// [x]: Implement:
//   static std::unordered_set<Vec2i, Vec2iHash> compute(const Grid& grid, Vec2i start, int movementPoints)
//     BFS from start:
//       - Simple queue of {tile, remainingMoves}.
//       - Expand 4-directional neighbors (up/down/left/right).
//       - Skip tiles where tile.moveCost > remainingMoves.
//       - Add reachable tiles to the result set.
//     Return the set of reachable Vec2i positions (does NOT include the starting tile).
class MovementRange
{
public:
    static std::unordered_set<Vec2i, Vec2iHash> compute(
        const Grid &grid,
        Vec2i start,
        int movementPoints,
        int team,
        const std::vector<Unit *> &allUnits,
        int jump);
};