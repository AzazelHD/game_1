#include "engine/core/App.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/FontManager.h"
#include "engine/renderer/Renderer.h"
#include "states/WorldMapState.h"
#include "config/GameConstants.h"
#include "battle/UnitData.h"
#include "data/UnitLoader.h"
#include "world/WorldPathfinding.h"
#include "states/BattleState.h"
#include "states/SettingsState.h"
#include "ui/windows/ButtonMenuWindow.h"
#include "ui/windows/ConfirmWindow.h"
#include "ui/windows/PartyWindow.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr Color kWorldBackground = Color{28, 30, 36, 255};
    constexpr Color kEdgeColor = Color{96, 104, 122, 255};
    constexpr Color kNodeColor = Color{208, 212, 224, 255};
    constexpr Color kBattleNodeColor = Color{228, 136, 120, 255};
    constexpr Color kHoverColor = Color{255, 220, 100, 255};
    constexpr Color kCursorColor = Color{255, 240, 170, 220};
    constexpr Color kPlayerColor = Color{96, 168, 255, 255};

    constexpr float kNodeRadius = 8.0f;
    constexpr float kHoverRadius = 13.0f;
    constexpr float kCursorRadius = 6.0f;
    constexpr float kPlayerRadius = 10.0f;

    constexpr float kGridStep = 180.0f;

    Vec2f lerp(Vec2f a, Vec2f b, float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);
        return Vec2f{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
    }

    float distanceSquared(Vec2f a, Vec2f b)
    {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        return dx * dx + dy * dy;
    }

    void drawCircle(Renderer *renderer, Vec2f center, float radius, Color color, int segments = 20)
    {
        if (!renderer)
            return;

        const FColor fColor = {
            static_cast<float>(color.r) / 255.0f,
            static_cast<float>(color.g) / 255.0f,
            static_cast<float>(color.b) / 255.0f,
            static_cast<float>(color.a) / 255.0f,
        };

        std::vector<Renderer::Vertex> verts;
        std::vector<int> indices;
        verts.reserve(static_cast<std::size_t>(segments + 2));
        indices.reserve(static_cast<std::size_t>(segments * 3));

        verts.push_back(Renderer::Vertex{center, fColor});

        constexpr float pi = 3.1415926535f;
        for (int i = 0; i <= segments; ++i)
        {
            const float angle = 2.0f * pi * static_cast<float>(i) / static_cast<float>(segments);
            verts.push_back(Renderer::Vertex{
                Vec2f{center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius},
                fColor});

            if (i > 0)
            {
                indices.push_back(0);
                indices.push_back(i);
                indices.push_back(i + 1);
            }
        }

        renderer->drawGeometry(verts, indices);
    }
}

WorldMapState::WorldMapState(StateMachine<Scene> &sm, Renderer *renderer, int startNodeId)
    : m_sm(sm), m_renderer(renderer), m_playerNodeId(startNodeId)
{
}

void WorldMapState::onEnter()
{
    if (!m_renderer)
        m_renderer = App::getRenderer();

    buildDefaultWorld();

    if (!m_graph.contains(m_playerNodeId))
        m_playerNodeId = 4;

    const world::WorldGraph::Node *playerNode = m_graph.getNode(m_playerNodeId);
    if (playerNode)
        m_cursorPos = playerNode->position;

    m_hoveredNodeId = -1;
    m_isMoving = false;
    m_path.clear();
    m_pathSegment = 0;
    m_segmentProgress = 0.0f;

    m_uiManager.clear();
    m_pendingBattle = PendingBattle{};

    updateHoverNode();
    updateCamera();
}

void WorldMapState::onExit()
{
    m_uiManager.clear();
}

void WorldMapState::handleInput()
{
    const Input &input = Input::instance();

    if (input.isKeyPressed(KeyCode::Back, false))
    {
        if (m_uiManager.hasWindow(WindowId::WorldMapEncounterConfirm))
        {
            m_uiManager.handleInput(input);
            processUIEvents();
            return;
        }

        if (!m_uiManager.empty())
            m_uiManager.popTop();
        else
        {
            auto *menu = m_uiManager.push<ButtonMenuWindow>(WindowId::WorldMap);
            menu->setFont(FontManager::instance().get(FontRole::Heading));
            menu->setItems({
                ButtonMenuWindow::Item{.id = ActionId::OpenPartyMenu, .label = "Party", .enabled = true},
                ButtonMenuWindow::Item{.id = ActionId::OpenSettings, .label = "Options", .enabled = true},
                ButtonMenuWindow::Item{.id = ActionId::QuitGame, .label = "Quit", .enabled = true},
            });
        }
        return;
    }

    if (!m_uiManager.empty())
    {
        if (m_uiManager.hasWindow(WindowId::WorldMapEncounterConfirm))
        {
            if (input.isKeyPressed(KeyCode::Accept, false) ||
                input.isKeyPressed(KeyCode::Back, false) ||
                input.isKeyPressed(KeyCode::Left, false) ||
                input.isKeyPressed(KeyCode::Right, false) ||
                input.isKeyPressed(KeyCode::Up, false) ||
                input.isKeyPressed(KeyCode::Down, false) ||
                input.isKeyPressed(KeyCode::A, false) ||
                input.isKeyPressed(KeyCode::D, false) ||
                input.isKeyPressed(KeyCode::W, false) ||
                input.isKeyPressed(KeyCode::S, false))
                m_uiManager.handleInput(input);
        }
        else
        {
            m_uiManager.handleInput(input);
        }
        processUIEvents();
        return;
    }

    if (m_isMoving)
        return;

    if (input.isKeyPressed(KeyCode::Accept, false) && m_hoveredNodeId >= 0)
        movePlayerToNode(m_hoveredNodeId);
}

void WorldMapState::update(float dt)
{
    m_uiManager.update(dt);
    processUIEvents();

    if (!m_uiManager.empty())
    {
        updateCamera();
        return;
    }

    if (!m_isMoving)
    {
        updateCursor(dt);
        applyNodeMagnetism(dt);
        updateHoverNode();
    }

    if (m_isMoving)
        updatePlayerMovement(dt);

    updateCamera();
}

void WorldMapState::render(float /*alpha*/)
{
    if (!m_renderer)
        return;

    m_renderer->clear(kWorldBackground);
    drawWorld();
    m_uiManager.render(m_renderer);
}

void WorldMapState::buildDefaultWorld()
{
    if (m_graph.contains(0))
        return;

    int id = 0;
    const float gridX0 = GameConstants::VIEW_CX - kGridStep;
    const float gridY0 = GameConstants::VIEW_CY - kGridStep;
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 3; ++x)
        {
            const Vec2f pos{gridX0 + static_cast<float>(x) * kGridStep,
                            gridY0 + static_cast<float>(y) * kGridStep};
            m_graph.addNode(id, pos);
            m_nodeGridPos[id] = Vec2i{x, y};
            m_nodeMeta[id] = NodeMeta{};
            ++id;
        }
    }

    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 3; ++x)
        {
            const int from = y * 3 + x;
            if (x + 1 < 3)
                m_graph.addUndirectedEdge(from, y * 3 + (x + 1), 1);
            if (y + 1 < 3)
                m_graph.addUndirectedEdge(from, (y + 1) * 3 + x, 1);
        }
    }

    m_nodeMeta[2] = NodeMeta{.type = NodeType::Battle, .battleMapPath = "assets/maps/1v1_test.tmj"};
    m_nodeMeta[6] = NodeMeta{.type = NodeType::Battle, .battleMapPath = "assets/maps/test_map.tmj"};
}

void WorldMapState::updateCursor(float dt)
{
    const Input &input = Input::instance();

    Vec2f dir{0.0f, 0.0f};
    if (input.isKeyDown(KeyCode::A) || input.isKeyDown(KeyCode::Left))
        dir.x -= 1.0f;
    if (input.isKeyDown(KeyCode::D) || input.isKeyDown(KeyCode::Right))
        dir.x += 1.0f;
    if (input.isKeyDown(KeyCode::W) || input.isKeyDown(KeyCode::Up))
        dir.y -= 1.0f;
    if (input.isKeyDown(KeyCode::S) || input.isKeyDown(KeyCode::Down))
        dir.y += 1.0f;

    if (dir.x == 0.0f && dir.y == 0.0f)
        return;

    const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len > 0.0f)
    {
        dir.x /= len;
        dir.y /= len;
    }

    m_cursorPos.x += dir.x * m_cursorSpeed * dt;
    m_cursorPos.y += dir.y * m_cursorSpeed * dt;
}

void WorldMapState::updateHoverNode()
{
    int nearestId = -1;
    float bestDistSq = std::numeric_limits<float>::max();

    for (int id : m_graph.nodeIds())
    {
        const world::WorldGraph::Node *node = m_graph.getNode(id);
        if (!node)
            continue;

        const float d2 = distanceSquared(m_cursorPos, node->position);
        if (d2 < bestDistSq)
        {
            bestDistSq = d2;
            nearestId = id;
        }
    }

    if (nearestId < 0)
    {
        m_hoveredNodeId = -1;
        return;
    }

    const float hitRadiusSq = m_nodeHitRadius * m_nodeHitRadius;
    m_hoveredNodeId = (bestDistSq <= hitRadiusSq) ? nearestId : -1;
}

void WorldMapState::applyNodeMagnetism(float dt)
{
    int nearestId = -1;
    float bestDistSq = std::numeric_limits<float>::max();

    for (int id : m_graph.nodeIds())
    {
        const world::WorldGraph::Node *node = m_graph.getNode(id);
        if (!node)
            continue;

        const float d2 = distanceSquared(m_cursorPos, node->position);
        if (d2 < bestDistSq)
        {
            bestDistSq = d2;
            nearestId = id;
        }
    }

    if (nearestId < 0)
        return;

    const world::WorldGraph::Node *node = m_graph.getNode(nearestId);
    if (!node)
        return;

    const float dist = std::sqrt(bestDistSq);
    if (dist <= 0.001f || dist > m_magnetRadius)
        return;

    const float t = 1.0f - (dist / m_magnetRadius);
    const float pull = m_magnetStrength * t * dt;
    m_cursorPos.x += (node->position.x - m_cursorPos.x) * pull;
    m_cursorPos.y += (node->position.y - m_cursorPos.y) * pull;
}

void WorldMapState::updateCamera()
{
    if (m_graph.nodeIds().empty())
        return;

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    for (int id : m_graph.nodeIds())
    {
        const world::WorldGraph::Node *node = m_graph.getNode(id);
        if (!node)
            continue;

        minX = std::min(minX, node->position.x);
        minY = std::min(minY, node->position.y);
        maxX = std::max(maxX, node->position.x);
        maxY = std::max(maxY, node->position.y);
    }

    Vec2f target{
        m_cursorPos.x - GameConstants::VIEW_W * 0.5f,
        m_cursorPos.y - GameConstants::VIEW_H * 0.5f,
    };

    const float minCamX = minX - m_worldMargin;
    const float minCamY = minY - m_worldMargin;
    const float maxCamX = maxX + m_worldMargin - GameConstants::VIEW_W;
    const float maxCamY = maxY + m_worldMargin - GameConstants::VIEW_H;

    if (maxCamX > minCamX)
        target.x = std::clamp(target.x, minCamX, maxCamX);
    else
        target.x = ((minX + maxX) * 0.5f) - GameConstants::VIEW_W * 0.5f;

    if (maxCamY > minCamY)
        target.y = std::clamp(target.y, minCamY, maxCamY);
    else
        target.y = ((minY + maxY) * 0.5f) - GameConstants::VIEW_H * 0.5f;

    m_cameraPos = target;
}

void WorldMapState::movePlayerToNode(int nodeId)
{
    if (!m_graph.contains(nodeId))
        return;

    if (nodeId == m_playerNodeId)
    {
        onArriveAtNode(nodeId);
        return;
    }

    m_path = world::shortestPath(m_graph, m_playerNodeId, nodeId);
    if (m_path.size() < 2)
        return;

    m_isMoving = true;
    m_pathSegment = 0;
    m_segmentProgress = 0.0f;
}

void WorldMapState::updatePlayerMovement(float dt)
{
    if (!m_isMoving || m_path.size() < 2)
        return;

    m_segmentProgress += dt * m_moveSpeed;

    while (m_segmentProgress >= 1.0f)
    {
        m_segmentProgress -= 1.0f;
        ++m_pathSegment;

        if (m_pathSegment >= m_path.size() - 1)
        {
            m_playerNodeId = m_path.back();
            m_isMoving = false;
            m_path.clear();
            m_pathSegment = 0;
            m_segmentProgress = 0.0f;
            onArriveAtNode(m_playerNodeId);
            return;
        }

        m_playerNodeId = m_path[m_pathSegment];
    }
}

void WorldMapState::onArriveAtNode(int nodeId)
{
    auto it = m_nodeMeta.find(nodeId);
    if (it == m_nodeMeta.end() || it->second.type != NodeType::Battle || it->second.battleMapPath.empty())
        return;

    m_pendingBattle = PendingBattle{.nodeId = nodeId, .mapPath = it->second.battleMapPath};

    auto *confirm = m_uiManager.push<ConfirmWindow>(WindowId::WorldMapEncounterConfirm);
    confirm->setFont(FontManager::instance().get(FontRole::Body));
    confirm->setPrompt("Start Battle?");
}

void WorldMapState::startBattle(PendingBattle battle)
{
    if (battle.nodeId < 0 || battle.mapPath.empty())
        return;

    m_pendingBattle = PendingBattle{};

    StateMachine<Scene> &sm = m_sm;
    Renderer *renderer = m_renderer;
    auto onFinished = [nodeId = battle.nodeId, &sm, renderer](bool /*playerWon*/)
    {
        sm.replace(std::make_unique<WorldMapState>(sm, renderer, nodeId));
    };

    BattleRequest request{
        .mapPath = battle.mapPath,
        .returnWorldNodeId = battle.nodeId,
    };
    m_sm.replace(std::make_unique<BattleState>(m_sm, m_renderer, request, std::move(onFinished)));
}

void WorldMapState::processUIEvents()
{
    auto events = m_uiManager.drainEvents();
    for (const UIEvent &event : events)
    {
        if (event.windowId == WindowId::WorldMapEncounterConfirm && event.type == UIEventType::ConfirmResult)
        {
            m_uiManager.popById(WindowId::WorldMapEncounterConfirm);
            if (event.confirmed)
                startBattle(m_pendingBattle);
            else
                m_pendingBattle = PendingBattle{};
            continue;
        }

        if (event.windowId == WindowId::WorldMap && event.type == UIEventType::ActionSelected)
        {
            if (event.actionId == ActionId::OpenPartyMenu)
            {
                auto *party = m_uiManager.push<PartyWindow>(WindowId::PartyMenu);
                party->setFont(FontManager::instance().get(FontRole::Body));

                std::vector<PartyWindow::Entry> entries;
                entries.reserve(2);

                bool ariaHasSprite = false;
                bool soldierHasSprite = false;
                try
                {
                    const UnitData aria = UnitLoader::load("assets/units/aria.json");
                    const UnitData soldier = UnitLoader::load("assets/units/soldier.json");
                    ariaHasSprite = !aria.spriteSetId.empty();
                    soldierHasSprite = !soldier.spriteSetId.empty();
                }
                catch (...)
                {
                }

                entries.push_back(PartyWindow::Entry{.name = "Aria", .hasSprite = ariaHasSprite});
                entries.push_back(PartyWindow::Entry{.name = "Soldier", .hasSprite = soldierHasSprite});
                party->setEntries(std::move(entries));
            }
            else if (event.actionId == ActionId::OpenSettings)
            {
                m_sm.push(std::make_unique<SettingsState>(m_sm, m_renderer));
            }
            else if (event.actionId == ActionId::QuitGame)
            {
                SDL_Event quitEvent{};
                quitEvent.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quitEvent);
            }
            continue;
        }

        if ((event.windowId == WindowId::WorldMap || event.windowId == WindowId::PartyMenu) && event.type == UIEventType::ActionCanceled)
        {
            m_uiManager.popById(event.windowId);
            continue;
        }
    }
}

void WorldMapState::drawWorld() const
{
    if (!m_renderer)
        return;

    m_renderer->setBlendMode(Renderer::BlendMode::Blend);

    const auto toScreen = [this](Vec2f worldPos)
    {
        return Vec2f{worldPos.x - m_cameraPos.x, worldPos.y - m_cameraPos.y};
    };

    for (int id : m_graph.nodeIds())
    {
        const world::WorldGraph::Node *node = m_graph.getNode(id);
        if (!node)
            continue;

        for (const world::WorldGraph::Edge &edge : node->edges)
        {
            if (edge.to < id)
                continue;
            const world::WorldGraph::Node *other = m_graph.getNode(edge.to);
            if (!other)
                continue;

            m_renderer->setDrawColor(kEdgeColor);
            m_renderer->drawLine(toScreen(node->position), toScreen(other->position));
        }
    }

    for (int id : m_graph.nodeIds())
    {
        const world::WorldGraph::Node *node = m_graph.getNode(id);
        if (!node)
            continue;

        Color color = kNodeColor;
        auto metaIt = m_nodeMeta.find(id);
        if (metaIt != m_nodeMeta.end())
        {
            if (metaIt->second.type == NodeType::Battle)
                color = kBattleNodeColor;
        }

        drawCircle(m_renderer, toScreen(node->position), kNodeRadius, color);
    }

    if (m_hoveredNodeId >= 0)
    {
        const world::WorldGraph::Node *hoveredNode = m_graph.getNode(m_hoveredNodeId);
        if (hoveredNode)
            drawCircle(m_renderer, toScreen(hoveredNode->position), kHoverRadius, kHoverColor, 28);
    }

    drawCircle(m_renderer, toScreen(m_cursorPos), kCursorRadius, kCursorColor, 16);

    Vec2f playerPos{0.0f, 0.0f};
    const world::WorldGraph::Node *playerNode = m_graph.getNode(m_playerNodeId);
    if (playerNode)
        playerPos = playerNode->position;

    if (m_isMoving && m_path.size() >= 2)
    {
        const world::WorldGraph::Node *fromNode = m_graph.getNode(m_path[m_pathSegment]);
        const world::WorldGraph::Node *toNode = m_graph.getNode(m_path[m_pathSegment + 1]);
        if (fromNode && toNode)
            playerPos = lerp(fromNode->position, toNode->position, m_segmentProgress);
    }

    drawCircle(m_renderer, toScreen(playerPos), kPlayerRadius, kPlayerColor, 24);
}
