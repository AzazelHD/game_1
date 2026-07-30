#pragma once
#include <vector>
#include <functional>
#include "engine/math/Vec2.h"

// Pathfinder finds the shortest walkable path between two tiles using A*.
//
// [x]: Implement:
//   static std::vector<Vec2i> findPath(const Grid& grid, const PathRequest& request, const PathRules& rules)
//
//     Standard A* algorithm:
//       - Open list: priority queue sorted by f = g + h.
//       - g = movement cost accumulated so far (from rules.moveCost).
//       - h = Manhattan distance heuristic: abs(dx) + abs(dy).
//       - Closed set: already evaluated tiles.
//       - Expand neighbors using rules.getNeighbors(current).
//       - For each neighbor:
//           * Skip if not reachable via rules.getNeighbors.
//           * Compute tentative g using rules.moveCost(from, to).
//       - Track parent map for path reconstruction.
//
//     Path reconstruction:
//       - Rebuild path from goal -> start using parent pointers.
//       - Return reversed path from start to goal (inclusive).
//       - Return empty vector if goal is unreachable.
//
//     Rules responsibility:
//       - Grid does NOT define movement rules.
//       - Rules define walking, jumping, terrain penalties, blocking, etc.
//       - Pathfinder is fully agnostic to game mechanics.
//
// Why A* and not plain BFS?
//   - BFS assumes uniform cost edges.
//   - A* supports weighted movement costs and is more efficient with heuristics.
//   - Required for terrain-based movement (e.g. forests vs plains).

class Grid;

// Defines a single pathfinding request.
struct PathRequest
{
    Vec2i start;
    Vec2i goal;
};

// Full movement ruleset provided by the game layer.
// Pathfinder does NOT know about terrain, units, factions, or mechanics.
//
// It only receives:
//   - Which moves are possible from a tile
//   - How expensive each move is
struct PathRules
{
    // Returns all reachable destination tiles from a given tile.
    // This is where walking, jumping, cliffs, etc. are defined.
    std::function<std::vector<Vec2i>(Vec2i from)> getNeighbors;

    // Returns movement cost of moving from -> to.
    // Must be consistent with getNeighbors (only valid moves will be queried).
    std::function<int(Vec2i from, Vec2i to)> moveCost;
};

// A* pathfinding on a grid with fully externalized movement rules.
class Pathfinder
{
public:
    [[nodiscard]]
    static std::vector<Vec2i> findPath(
        const Grid &grid,
        const PathRequest &request,
        const PathRules &rules);

private:
    [[nodiscard]]
    static int heuristic(Vec2i a, Vec2i b);
};
