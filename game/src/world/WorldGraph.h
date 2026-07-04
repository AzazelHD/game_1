#pragma once

#include "engine/math/Vec2.h"

#include <optional>
#include <unordered_map>
#include <vector>

namespace world
{
    class WorldGraph
    {
    public:
        using NodeId = int;

        struct Edge
        {
            NodeId to = -1;
            int cost = 1;
        };

        struct Node
        {
            NodeId id = -1;
            Vec2f position{0.0f, 0.0f};
            std::vector<Edge> edges;
        };

        void addNode(NodeId id, Vec2f position)
        {
            m_nodes[id] = Node{id, position, {}};
        }

        void addUndirectedEdge(NodeId a, NodeId b, int cost)
        {
            if (!contains(a) || !contains(b))
                return;
            addDirectedEdge(a, b, cost);
            addDirectedEdge(b, a, cost);
        }

        bool contains(NodeId id) const
        {
            return m_nodes.find(id) != m_nodes.end();
        }

        const Node *getNode(NodeId id) const
        {
            auto it = m_nodes.find(id);
            if (it == m_nodes.end())
                return nullptr;
            return &it->second;
        }

        std::vector<NodeId> nodeIds() const
        {
            std::vector<NodeId> out;
            out.reserve(m_nodes.size());
            for (const auto &pair : m_nodes)
                out.push_back(pair.first);
            return out;
        }

    private:
        void addDirectedEdge(NodeId from, NodeId to, int cost)
        {
            Node &node = m_nodes[from];
            for (const Edge &edge : node.edges)
            {
                if (edge.to == to)
                    return;
            }
            node.edges.push_back(Edge{to, cost});
        }

    private:
        std::unordered_map<NodeId, Node> m_nodes;
    };
} // namespace world
