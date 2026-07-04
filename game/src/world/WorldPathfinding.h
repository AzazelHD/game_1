#pragma once

#include "world/WorldGraph.h"

#include <algorithm>
#include <limits>
#include <queue>
#include <unordered_map>
#include <vector>

namespace world
{
    inline std::vector<WorldGraph::NodeId> shortestPath(const WorldGraph &graph,
                                                        WorldGraph::NodeId start,
                                                        WorldGraph::NodeId goal)
    {
        if (!graph.contains(start) || !graph.contains(goal))
            return {};
        if (start == goal)
            return {start};

        using NodeId = WorldGraph::NodeId;

        struct QueueItem
        {
            NodeId node = -1;
            int cost = std::numeric_limits<int>::max();

            bool operator>(const QueueItem &other) const
            {
                return cost > other.cost;
            }
        };

        std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<QueueItem>> frontier;
        std::unordered_map<NodeId, int> distance;
        std::unordered_map<NodeId, NodeId> parent;

        distance[start] = 0;
        frontier.push(QueueItem{start, 0});

        while (!frontier.empty())
        {
            QueueItem current = frontier.top();
            frontier.pop();

            if (current.node == goal)
                break;

            const WorldGraph::Node *node = graph.getNode(current.node);
            if (!node)
                continue;

            const int knownCost = distance[current.node];
            if (current.cost > knownCost)
                continue;

            for (const WorldGraph::Edge &edge : node->edges)
            {
                const int nextCost = knownCost + std::max(1, edge.cost);
                auto it = distance.find(edge.to);
                if (it == distance.end() || nextCost < it->second)
                {
                    distance[edge.to] = nextCost;
                    parent[edge.to] = current.node;
                    frontier.push(QueueItem{edge.to, nextCost});
                }
            }
        }

        if (parent.find(goal) == parent.end())
            return {};

        std::vector<NodeId> path;
        path.push_back(goal);
        NodeId cursor = goal;
        while (cursor != start)
        {
            cursor = parent[cursor];
            path.push_back(cursor);
        }
        std::reverse(path.begin(), path.end());
        return path;
    }
} // namespace world
