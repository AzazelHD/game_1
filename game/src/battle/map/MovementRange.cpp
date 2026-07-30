#include "MovementRange.h"
#include "Grid.h"
#include "battle/map/BattleMap.h"
#include "battle/unit/Unit.h"
#include <queue>
#include <unordered_map>
#include <cstdlib> // for std::abs

MovementRange::Result MovementRange::compute(
    const Grid &grid,
    const BattleMap &battleMap,
    Vec2i start,
    int movementPoints,
    int team,
    const std::vector<Unit *> &allUnits,
    int jump)
{
    Result result;
    std::unordered_set<Vec2i, Vec2iHash> visited;

    struct Node
    {
        Vec2i pos;
        int remaining;
        int costSoFar;
    };
    std::queue<Node> q;
    q.push({start, movementPoints, 0});
    visited.insert(start);

    // Quick lookup: which unit (by team) is on each tile
    std::unordered_map<Vec2i, int, Vec2iHash> unitTeamAt; // position -> team
    for (const Unit *u : allUnits)
    {
        if (u && !u->isDead())
            unitTeamAt[u->getPosition()] = u->getTeam();
    }

    static const Vec2i dirs[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    auto canEnter = [&](Vec2i from, Vec2i to, int remaining) -> bool
    {
        if (!grid.isValid(to))
            return false;

        // Height restriction – uses unit's jump stat
        if (std::abs(battleMap.at(to.x, to.y).height - battleMap.at(from.x, from.y).height) > jump)
            return false;

        // Occupancy check
        auto it = unitTeamAt.find(to);
        if (it != unitTeamAt.end())
        {
            int occTeam = it->second;
            if (occTeam != team) // enemy unit blocks completely
                return false;
            // Ally unit – can pass through but not stop (handled later)
        }

        // Movement cost
        const int cost = grid.getMoveCost(from, to);
        return cost <= remaining;
    };

    while (!q.empty())
    {
        Node cur = q.front();
        q.pop();

        for (const Vec2i &d : dirs)
        {
            Vec2i next = {cur.pos.x + d.x, cur.pos.y + d.y};

            if (!canEnter(cur.pos, next, cur.remaining))
                continue;
            if (visited.count(next))
                continue;

            visited.insert(next);

            const int stepCost = grid.getMoveCost(cur.pos, next);
            const int totalCost = cur.costSoFar + stepCost;

            // Add tile to result unless an ally occupies it (cannot end on ally)
            auto it = unitTeamAt.find(next);
            if (it == unitTeamAt.end() || it->second != team)
            {
                result.reachable.insert(next);
                result.costs[next] = totalCost;
            }

            q.push({next, cur.remaining - stepCost, totalCost});
        }
    }

    // Remove the starting tile from the result set
    result.reachable.erase(start);
    result.costs.erase(start);
    return result;
}
