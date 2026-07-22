#include "MovementRange.h"
#include "Grid.h"
#include "battle/BattleMap.h"
#include "battle/Unit.h"
#include <queue>
#include <unordered_map>
#include <cstdlib> // for std::abs

std::unordered_set<Vec2i, Vec2iHash> MovementRange::compute(
    const Grid &grid,
    const BattleMap &battleMap,
    Vec2i start,
    int movementPoints,
    int team,
    const std::vector<Unit *> &allUnits,
    int jump)
{
    std::unordered_set<Vec2i, Vec2iHash> result;
    std::unordered_set<Vec2i, Vec2iHash> visited;

    struct Node
    {
        Vec2i pos;
        int remaining;
    };
    std::queue<Node> q;
    q.push({start, movementPoints});
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

            // Add tile to result unless an ally occupies it (cannot end on ally)
            auto it = unitTeamAt.find(next);
            if (it == unitTeamAt.end() || it->second != team)
                result.insert(next);

            const int cost = grid.getMoveCost(cur.pos, next);
            q.push({next, cur.remaining - cost});
        }
    }

    // Remove the starting tile from the result set
    result.erase(start);
    return result;
}
