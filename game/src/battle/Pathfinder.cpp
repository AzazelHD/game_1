#include "Pathfinder.h"

#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <limits>
#include <algorithm>

static int hashVec(const Vec2i &v)
{
    return (v.x * 73856093) ^ (v.y * 19349663);
}

static int manhattan(const Vec2i a, const Vec2i b)
{
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

std::vector<Vec2i> Pathfinder::findPath(
    const Grid &,
    const PathRequest &request,
    const PathRules &rules)
{
    const Vec2i start = request.start;
    const Vec2i goal = request.goal;

    struct Node
    {
        Vec2i pos;
        int g;
        int f;
    };

    struct Compare
    {
        bool operator()(const Node &a, const Node &b) const
        {
            return a.f > b.f;
        }
    };

    std::priority_queue<Node, std::vector<Node>, Compare> open;

    std::unordered_map<int, Vec2i> parent;
    std::unordered_map<int, int> gScore;
    std::unordered_set<int> closed;

    auto key = [](const Vec2i &v)
    {
        return hashVec(v);
    };

    gScore[key(start)] = 0;
    open.push({start, 0, manhattan(start, goal)});

    while (!open.empty())
    {
        Node current = open.top();
        open.pop();

        const int cKey = key(current.pos);

        if (closed.count(cKey))
            continue;

        const int currentG = gScore[cKey];

        if (current.pos == goal)
        {
            std::vector<Vec2i> path;
            Vec2i p = goal;

            path.push_back(p);

            while (p != start)
            {
                p = parent[key(p)];
                path.push_back(p);
            }

            std::reverse(path.begin(), path.end());
            return path;
        }

        closed.insert(cKey);

        for (const Vec2i &n : rules.getNeighbors(current.pos))
        {
            const int nKey = key(n);

            if (closed.count(nKey))
                continue;

            const int tentativeG =
                currentG + rules.moveCost(current.pos, n);

            auto it = gScore.find(nKey);
            if (it == gScore.end() || tentativeG < it->second)
            {
                gScore[nKey] = tentativeG;
                parent[nKey] = current.pos;

                const int f = tentativeG + manhattan(n, goal);

                open.push({n, tentativeG, f});
            }
        }
    }

    return {};
}