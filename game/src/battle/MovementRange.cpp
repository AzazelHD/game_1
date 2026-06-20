#include "MovementRange.h"
#include "Grid.h"

#include <queue>

std::unordered_set<Vec2i, Vec2iHash> MovementRange::compute(
    const Grid &grid,
    Vec2i start,
    int movementPoints)
{
    std::unordered_set<Vec2i, Vec2iHash> result;

    struct Node
    {
        Vec2i pos;
        int remaining;
    };

    std::queue<Node> q;

    q.push({start, movementPoints});

    static const Vec2i dirs[4] =
        {
            {1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    auto canEnter = [&](Vec2i from, Vec2i to, int remaining) -> bool
    {
        if (!grid.isValid(to))
            return false;

        const int cost = grid.getMoveCost(from, to);

        if (cost > remaining)
            return false;

        return true;
    };

    while (!q.empty())
    {
        Node current = q.front();
        q.pop();

        for (const Vec2i &d : dirs)
        {
            Vec2i next = {current.pos.x + d.x, current.pos.y + d.y};

            if (!canEnter(current.pos, next, current.remaining))
                continue;

            const int cost = grid.getMoveCost(current.pos, next);
            const int nextRemaining = current.remaining - cost;

            if (result.find(next) != result.end())
                continue;

            result.insert(next);
            q.push({next, nextRemaining});
        }
    }

    result.erase(start);
    return result;
}