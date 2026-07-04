#pragma once

#include "engine/scene/Scene.h"
#include "engine/statemachine/StateMachine.h"
#include "world/WorldGraph.h"
#include "ui/UIManager.h"

#include "engine/math/Vec2.h"

#include <string>
#include <unordered_map>
#include <vector>

class Renderer;

class WorldMapState final : public Scene
{
public:
    WorldMapState(StateMachine<Scene> &sm, Renderer *renderer, int startNodeId = 4);

    void onEnter() override;
    void onExit() override;
    void handleInput() override;
    void update(float dt) override;
    void render(float alpha) override;

private:
    enum class NodeType
    {
        Battle,
        Empty,
    };

    struct NodeMeta
    {
        NodeType type = NodeType::Empty;
        std::string battleMapPath;
    };

    struct PendingBattle
    {
        int nodeId = -1;
        std::string mapPath;
    };

private:
    void buildDefaultWorld();
    void updateCursor(float dt);
    void updateHoverNode();
    void applyNodeMagnetism(float dt);
    void updateCamera();
    void movePlayerToNode(int nodeId);
    void updatePlayerMovement(float dt);
    void onArriveAtNode(int nodeId);
    void processUIEvents();

    void startBattle(PendingBattle battle);

    void drawWorld() const;

private:
    StateMachine<Scene> &m_sm;
    Renderer *m_renderer = nullptr;

    world::WorldGraph m_graph;
    std::unordered_map<int, Vec2i> m_nodeGridPos;
    std::unordered_map<int, NodeMeta> m_nodeMeta;

    int m_playerNodeId = 4;
    int m_hoveredNodeId = -1;
    Vec2f m_cursorPos{0.0f, 0.0f};
    Vec2f m_cameraPos{0.0f, 0.0f};

    float m_cursorSpeed = 380.0f;
    float m_nodeHitRadius = 24.0f;
    float m_magnetRadius = 28.0f;
    float m_magnetStrength = 6.0f;
    float m_worldMargin = 120.0f;

    bool m_isMoving = false;
    std::vector<int> m_path;
    std::size_t m_pathSegment = 0;
    float m_segmentProgress = 0.0f;
    float m_moveSpeed = 3.5f;

    PendingBattle m_pendingBattle;
    UIManager m_uiManager;
};
