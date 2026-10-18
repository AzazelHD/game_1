#include "engine/core/App.h"
#include "engine/effects/ScreenTransition.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/FontManager.h"
#include "engine/renderer/Renderer.h"
#include "engine/math/MathUtils.h"
#include "config/GameConstants.h"
#include "battle/unit/Unit.h"
#include "battle/unit/UnitData.h"
#include "battle/unit/UnitProgression.h"
#include "data/SaveManager.h"
#include "data/ShopStock.h"
#include "data/TavernData.h"
#include "data/UnitLoader.h"
#include "scenes/BattleState.h"
#include "settings/SettingsState.h"
#include "ui/ConfirmDialogs.h"
#include "ui/windows/ButtonMenuWindow.h"
#include "ui/windows/ConfirmWindow.h"
#include "ui/windows/DialogWindow.h"
#include "ui/windows/InventoryWindow.h"
#include "ui/windows/ShopWindow.h"
#include "ui/windows/PartyWindow.h"
#include "ui/windows/SkillProgressionWindow.h"
#include "ui/windows/UnitDetailWindow.h"
#include "ui/UIUtils.h"
#include "systems/PartyContext.h"
#include "scenes/WorldMapState.h"
#include "world/WorldPathfinding.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <unordered_set>

namespace
{
    constexpr Color kWorldBackground = Color{28, 30, 36, 255};
    constexpr Color kEdgeColor = Color{96, 104, 122, 255};
    constexpr Color kNodeColor = Color{208, 212, 224, 255};
    constexpr Color kNodeLabelColor = Color{168, 176, 192, 220};
    constexpr Color kTownNodeColor = Color{120, 200, 140, 255};
    constexpr Color kHoverColor = Color{255, 220, 100, 255};
    constexpr Color kCursorColor = Color{255, 240, 170, 220};
    constexpr Color kPlayerColor = Color{96, 168, 255, 255};
    constexpr Color kObjectiveHaloColor = Color{236, 182, 66, 110};

    constexpr float kNodeRadius = 8.0f;
    constexpr float kHoverRadius = 13.0f;
    constexpr float kCursorRadius = 6.0f;
    constexpr float kPlayerRadius = 10.0f;

    constexpr float kGridStep = 180.0f;

    float distanceSquared(Vec2f a, Vec2f b)
    {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        return dx * dx + dy * dy;
    }
}

WorldMapState::WorldMapState(StateMachine<Scene> &sm, Renderer *renderer, int startNodeId, int wonNodeId)
    : m_sm(sm), m_renderer(renderer), m_playerNodeId(startNodeId), m_wonNodeId(wonNodeId)
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

    // One-shot controls tutorial (once per campaign): shown the first time
    // the world map opens, before the player has to move anywhere. Showing it
    // after the first tavern quest was too late — reaching the tavern already
    // required the very controls being explained.
    PartyContext &ctx = PartyContext::instance();
    ctx.ensureInitialized();
    if (!ctx.controlsTutorialShown())
    {
        ctx.markControlsTutorialShown();
        auto *dialog = m_uiManager.push<DialogWindow>(WindowId::WorldMapNodeInfo);
        dialog->setFont(FontManager::instance().get(FontRole::Body));
        dialog->start({
            DialogWindow::Line{.speaker = "Controls", .text = "Move cursor: WASD / Arrow keys.  Travel: Accept (Enter).  Details: Tab.  Journal: Quests.  Back: Esc."},
            DialogWindow::Line{.speaker = "Tip", .text = "Amber halos mark your quest objectives on the map. Your active quests are always in the journal."},
        });
    }

    // Returning from a battle just won: satisfy victory quests first so the
    // completion dialog shows on top of the fresh world map.
    if (m_wonNodeId >= 0)
    {
        completeQuestObjectives(m_wonNodeId, /*victorious=*/true);
        m_wonNodeId = -1;
    }

    updateHoverNode();
    updateCamera();

    m_transition.start({
        .transition = ScreenTransitions::FadeIn,
        .duration = 0.5f,
        .easing = easeInOut,
    });
}

void WorldMapState::onExit()
{
    m_uiManager.clear();
}

void WorldMapState::handleInput()
{
    const Input &input = Input::instance();

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

    if (input.isKeyPressed(KeyCode::Back, false))
    {
        m_worldMapMenu = m_uiManager.push<ButtonMenuWindow>(WindowId::WorldMap);
        m_worldMapMenu->setFont(FontManager::instance().get(FontRole::Heading));
        m_worldMapMenu->setWrappable(false);
        m_worldMapMenu->setScrollable(false);
        refreshWorldMapMenuItems();
        return;
    }

    if (!m_isMoving && input.isKeyPressed(KeyCode::Details, false))
    {
        const int nodeId = (m_hoveredNodeId >= 0) ? m_hoveredNodeId : m_playerNodeId;
        showNodeInfo(nodeId);
        return;
    }

    if (input.isKeyPressed(KeyCode::Journal, false))
    {
        openJournal();
        return;
    }

    // Fading in from the previous scene — accept input only once the map is
    // fully visible.
    if (m_transition.isActive())
        return;

    if (m_isMoving)
        return;

    if (input.isKeyPressed(KeyCode::Accept, false) && m_hoveredNodeId >= 0)
        movePlayerToNode(m_hoveredNodeId);
}

void WorldMapState::update(float dt)
{
    if (m_transition.isActive())
    {
        m_transition.update(dt);
        return;
    }

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

    // Fade layer last so it dims the whole screen, UI included.
    m_transition.render(m_renderer, GameConstants::VIEW_W, GameConstants::VIEW_H);
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

    m_nodeMeta[1] = NodeMeta{.type = NodeType::Town,
                             .description = "Riverwatch — a frontier town. Shelter, supplies, and work for those who can pay."};
    m_nodeMeta[2] = NodeMeta{.type = NodeType::Battle,
                             .battleMapPath = "assets/maps/1v1_test.tmj",
                             .description = "The old watchtower — an enemy picket sweeps the road below. Drive them off to open the way inland."};
    m_nodeMeta[6] = NodeMeta{.type = NodeType::Battle,
                             .battleMapPath = "assets/maps/test_map.tmj",
                             .description = "The enemy main camp — tents, banners, and the core of the line, entrenched and waiting. Break it."};
    m_nodeMeta[8] = NodeMeta{.type = NodeType::Battle,
                             .battleMapPath = "assets/maps/boss_arena.tmj",
                             .description = "A red scar on the ridge — the bandit chief's lair."};
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

    // Quest gating: locked regions cannot be entered until their unlocking
    // quest is complete. Also do not path over a locked node.
    PartyContext &context = PartyContext::instance();
    context.ensureInitialized();
    if (!context.questLog().isNodeUnlocked(nodeId))
    {
        m_uiManager.popById(WindowId::WorldMapNodeInfo);
        auto *dialog = m_uiManager.push<DialogWindow>(WindowId::WorldMapNodeInfo);
        dialog->setFont(FontManager::instance().get(FontRole::Body));
        dialog->start({DialogWindow::Line{.speaker = "Field", .text = "The way is blocked. Complete the main quest to open it."}});
        return;
    }

    if (nodeId == m_playerNodeId)
    {
        onArriveAtNode(nodeId);
        return;
    }

    m_path = world::shortestPath(m_graph, m_playerNodeId, nodeId);
    if (m_path.size() < 2)
        return;

    // Refuse routes that cross a locked node.
    for (int id : m_path)
    {
        if (!context.questLog().isNodeUnlocked(id))
        {
            m_path.clear();
            return;
        }
    }

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
    if (it != m_nodeMeta.end() && it->second.type == NodeType::Town)
    {
        // Town node: arrival satisfies reach-type objectives, then the
        // settlement hub (shop / tavern / leave) opens.
        completeQuestObjectives(nodeId, /*victorious=*/false);
        openTownHub();
        return;
    }

    // FFTA-style node events: every active quest whose objective sits on this
    // node is an interaction, and battle nodes additionally offer one even
    // when no quest points at them. Any event opens the chooser so the player
    // sees what is available; Enter on the entry then runs it (battle confirm
    // or quest dialog). Nodes with no events stay silent.
    collectNodeEvents(nodeId);
    if (!m_nodeEvents.empty())
        presentNodeEvents();

    // Pushed after the pending event so a completion toast stacks above it.
    completeQuestObjectives(nodeId, /*victorious=*/false);
}

void WorldMapState::collectNodeEvents(int nodeId)
{
    m_nodeEvents.clear();

    auto it = m_nodeMeta.find(nodeId);
    const bool battleNode = (it != m_nodeMeta.end() && it->second.type == NodeType::Battle &&
                             !it->second.battleMapPath.empty());

    PartyContext &context = PartyContext::instance();
    context.ensureInitialized();
    const QuestLog &log = context.questLog();

    int victoryQuestCount = 0;
    for (const QuestDefinition &quest : log.all())
    {
        if (quest.objective.nodeId != nodeId || !log.isActive(quest.id))
            continue;

        if (quest.objective.type == QuestObjectiveType::VictoryAtNode)
        {
            if (!battleNode)
                continue;
            ++victoryQuestCount;
            m_nodeEvents.push_back(NodeEvent{.label = quest.title, .questId = quest.id, .startBattle = true});
        }
        else
        {
            m_nodeEvents.push_back(NodeEvent{.label = quest.title, .questId = quest.id, .startBattle = false});
        }
    }

    if (battleNode && victoryQuestCount == 0)
        m_nodeEvents.push_back(NodeEvent{.label = "Battle", .questId = "", .startBattle = true});
}

void WorldMapState::presentNodeEvents()
{
    std::vector<ButtonMenuWindow::Item> items;
    items.reserve(m_nodeEvents.size());
    for (const NodeEvent &event : m_nodeEvents)
        items.push_back(ButtonMenuWindow::Item{.id = ActionId::PlayNodeEvent, .label = event.label, .enabled = true});

    m_nodeEventWindow = m_uiManager.push<ButtonMenuWindow>(WindowId::WorldMapNodeEvents);
    m_nodeEventWindow->setFont(FontManager::instance().get(FontRole::Heading));
    m_nodeEventWindow->setPanelPosition(Vec2f{0.0f, GameConstants::VIEW_CY - 40.0f});
    m_nodeEventWindow->centerHorizontally(true);
    m_nodeEventWindow->setItems(std::move(items));
}

void WorldMapState::playNodeEvent(int index)
{
    if (index < 0 || index >= static_cast<int>(m_nodeEvents.size()))
        return;

    const NodeEvent &event = m_nodeEvents[static_cast<std::size_t>(index)];

    if (event.startBattle)
    {
        m_nodeEvents.clear();
        requestBattleAtNode(m_playerNodeId);
        return;
    }

    PartyContext &context = PartyContext::instance();
    context.ensureInitialized();
    const QuestDefinition *quest = context.questLog().find(event.questId);
    const std::string body = (quest && !quest->description.empty()) ? quest->description
                             : (quest ? quest->brief : event.label);
    m_nodeEvents.clear();

    auto *dialog = m_uiManager.push<DialogWindow>(WindowId::WorldMapNodeInfo);
    dialog->setFont(FontManager::instance().get(FontRole::Body));
    dialog->start({DialogWindow::Line{.speaker = quest ? quest->title : "Node", .text = body}});
}

void WorldMapState::requestBattleAtNode(int nodeId)
{
    auto it = m_nodeMeta.find(nodeId);
    if (it == m_nodeMeta.end() || it->second.type != NodeType::Battle || it->second.battleMapPath.empty())
        return;

    m_pendingBattle = PendingBattle{.nodeId = nodeId, .mapPath = it->second.battleMapPath};

    auto *confirm = m_uiManager.push<ConfirmWindow>(WindowId::WorldMapEncounterConfirm);
    confirm->setFont(FontManager::instance().get(FontRole::Heading));
    confirm->setButtonFont(FontManager::instance().get(FontRole::Body));
    confirm->setPrompt("Start Battle?");
}

void WorldMapState::completeQuestObjectives(int nodeId, bool victorious)
{
    PartyContext &context = PartyContext::instance();
    context.ensureInitialized();

    const std::vector<QuestDefinition> completed = context.completeQuestsForNode(nodeId, victorious);
    if (completed.empty())
        return;

    std::vector<DialogWindow::Line> lines;
    for (const QuestDefinition &quest : completed)
    {
        std::string text = quest.title + " — complete.";
        if (quest.reward.gold > 0)
            text += " Received " + std::to_string(quest.reward.gold) + " gold.";
        if (quest.reward.xp > 0)
            text += " Party gained " + std::to_string(quest.reward.xp) + " XP.";
        lines.push_back(DialogWindow::Line{.speaker = "Quest", .text = std::move(text)});
    }

    m_uiManager.popById(WindowId::WorldMapQuestComplete);
    auto *dialog = m_uiManager.push<DialogWindow>(WindowId::WorldMapQuestComplete);
    dialog->setFont(FontManager::instance().get(FontRole::Body));
    dialog->start(std::move(lines));
}

void WorldMapState::leaveTownHub()
{
    m_uiManager.popById(WindowId::WorldMapNodeMenu);
    m_townHubMenu = nullptr;
}

void WorldMapState::openShopWindow()
{
    m_uiManager.popById(WindowId::WorldMapNodeMenu);
    m_townHubMenu = nullptr;

    m_shopHubMenu = m_uiManager.push<ButtonMenuWindow>(WindowId::Shop);
    m_shopHubMenu->setFont(FontManager::instance().get(FontRole::Heading));
    m_shopHubMenu->setItems({
        ButtonMenuWindow::Item{.id = ActionId::BuyItem, .label = "Buy", .enabled = true},
        ButtonMenuWindow::Item{.id = ActionId::SellItem, .label = "Sell", .enabled = true},
        ButtonMenuWindow::Item{.id = ActionId::TradeItem, .label = "Trade", .enabled = false},
        ButtonMenuWindow::Item{.id = ActionId::Close, .label = "Leave", .enabled = true},
    });
}

void WorldMapState::openShopList(ShopWindow::Mode mode)
{
    PartyContext &ctx = PartyContext::instance();
    ctx.ensureInitialized();

    const std::vector<std::string> stockKeys = ShopStock::load("assets/shop/stock.json");
    std::vector<const Gear *> stock;
    stock.reserve(stockKeys.size());
    for (const std::string &key : stockKeys)
    {
        const Gear *gear = ctx.gearCatalog().findByKey(key);
        if (gear)
            stock.push_back(gear);
    }

    if (m_shopHubMenu)
        m_shopHubMenu->setVisible(false);

    // Equipped-state provider: how many copies of an item the party is
    // currently wearing (WorldMap is the only place with roster access).
    const auto equippedProvider = [&ctx](ItemId itemId) -> std::uint32_t
    {
        std::uint32_t count = 0;
        for (const RosterUnit &unit : ctx.roster().units())
        {
            for (const std::optional<ItemId> &id : unit.equippedGear.slots)
                if (id.has_value() && *id == itemId)
                    ++count;
            for (const std::optional<ItemId> &id : unit.equippedGear.accessories)
                if (id.has_value() && *id == itemId)
                    ++count;
        }
        return count;
    };

    const WindowId listId = (mode == ShopWindow::Mode::Buy) ? WindowId::ShopBuy : WindowId::ShopSell;
    m_shopWindow = m_uiManager.push<ShopWindow>(
        listId, mode, ctx.gearCatalog(), ctx.inventory(),
        [&ctx]() -> int { return ctx.gold(); }, stock, equippedProvider);
    m_shopWindow->setFont(FontManager::instance().get(FontRole::Body));
}

void WorldMapState::openTavernHub()
{
    if (!m_tavernDataLoaded)
    {
        m_tavernCandidates = TavernData::loadCandidates("assets/tavern/candidates.json");
        m_tavernLore = TavernData::loadLore("assets/tavern/lore.json");
        m_tavernDataLoaded = true;
    }

    m_uiManager.popById(WindowId::WorldMapNodeMenu);
    m_townHubMenu = nullptr;

    m_tavernHub = m_uiManager.push<ButtonMenuWindow>(WindowId::Tavern);
    m_tavernHub->setFont(FontManager::instance().get(FontRole::Heading));
    m_tavernHub->setItems({
        ButtonMenuWindow::Item{.id = ActionId::AcceptMission, .label = "Quest Board", .enabled = true},
        ButtonMenuWindow::Item{.id = ActionId::Recruit, .label = "Recruit", .enabled = true},
        ButtonMenuWindow::Item{.id = ActionId::Talk, .label = "Talk", .enabled = true},
        ButtonMenuWindow::Item{.id = ActionId::Close, .label = "Leave", .enabled = true},
    });
}

void WorldMapState::handleTavernEvent(const UIEvent &event)
{
    if (event.windowId == WindowId::Tavern)
    {
        if (event.type == UIEventType::ActionCanceled)
        {
            m_tavernHub = nullptr;
            m_uiManager.popById(WindowId::Tavern);
            openTownHub();
            return;
        }
        if (event.type != UIEventType::ActionSelected)
            return;

        if (event.actionId == ActionId::AcceptMission)
            openTavernQuestBoard();
        else if (event.actionId == ActionId::Recruit)
            openTavernRecruitList();
        else if (event.actionId == ActionId::Talk)
            openTavernTalk();
        else if (event.actionId == ActionId::Close)
        {
            m_tavernHub = nullptr;
            m_uiManager.popById(WindowId::Tavern);
        }
        return;
    }

    if (event.windowId == WindowId::TavernQuestBoard)
    {
        if (event.type == UIEventType::ActionCanceled)
        {
            m_tavernBoardWindow = nullptr;
            m_uiManager.popById(WindowId::TavernQuestBoard);
            if (m_tavernHub)
                m_tavernHub->setVisible(true);
        }
        else if (event.type == UIEventType::ActionSelected && event.index >= 0 &&
                 event.index < static_cast<int>(m_tavernQuestIds.size()))
        {
            // Selecting a quest opens its detail window first; buying (accept)
            // is confirmed from there.
            showTavernQuestDetail(event.index);
        }
        return;
    }

    if (event.windowId == WindowId::TavernQuestDetail && event.type == UIEventType::DialogFinished)
    {
        m_uiManager.popById(WindowId::TavernQuestDetail);

        PartyContext &ctx = PartyContext::instance();
        ctx.ensureInitialized();
        const QuestDefinition *quest = ctx.questLog().find(m_pendingQuestId);
        std::string prompt = "Accept this quest?";
        if (quest && quest->cost > 0)
            prompt += " (" + std::to_string(quest->cost) + " gold)";

        auto *confirm = m_uiManager.push<ConfirmWindow>(WindowId::TavernQuestAccept);
        confirm->setFont(FontManager::instance().get(FontRole::Body));
        confirm->setPrompt(prompt);
        return;
    }

    if (event.windowId == WindowId::TavernQuestAccept && event.type == UIEventType::ConfirmResult)
    {
        m_uiManager.popById(WindowId::TavernQuestAccept);

        PartyContext &ctx = PartyContext::instance();
        ctx.ensureInitialized();

        const std::string questId = m_pendingQuestId;
        m_pendingQuestId.clear();

        if (event.confirmed)
        {
            const QuestDefinition *quest = ctx.questLog().find(questId);
            if (quest && quest->cost > 0 && ctx.gold() < quest->cost)
            {
                if (m_tavernBoardWindow)
                    m_tavernBoardWindow->setVisible(true);
                auto *dialog = m_uiManager.push<DialogWindow>(WindowId::WorldMapNodeInfo);
                dialog->setFont(FontManager::instance().get(FontRole::Body));
                dialog->start({DialogWindow::Line{.speaker = "Quest Board", .text = "Not enough gold to take " + quest->title + " (" + std::to_string(quest->cost) + "g)."}});
            }
            else if (quest && quest->offeredAtTavern && ctx.questLog().hasActiveTavernQuest())
            {
                if (m_tavernBoardWindow)
                    m_tavernBoardWindow->setVisible(true);
                auto *dialog = m_uiManager.push<DialogWindow>(WindowId::WorldMapNodeInfo);
                dialog->setFont(FontManager::instance().get(FontRole::Body));
                dialog->start({DialogWindow::Line{.speaker = "Quest Board", .text = "You can only carry one contracted task at a time. Finish " + quest->title + " before taking another."}});
            }
            else if (quest && ctx.questLog().accept(questId))
            {
                if (quest->cost > 0)
                    ctx.spendGold(quest->cost);
                refreshTavernQuestBoard();
                if (m_tavernBoardWindow)
                    m_tavernBoardWindow->setVisible(true);
                std::string acceptedText = "Accepted: " + quest->title;
                if (quest->cost > 0)
                    acceptedText += " (" + std::to_string(quest->cost) + " gold paid)";
                std::string objectiveText =
                    "Head to the marked node ";
                if (quest->objective.type == QuestObjectiveType::VictoryAtNode)
                    objectiveText += "and win the battle to clear the objective.";
                else if (quest->objective.type == QuestObjectiveType::ReachNode)
                    objectiveText += "and reach it to clear the objective.";
                std::vector<DialogWindow::Line> dialogLines;
                dialogLines.push_back(DialogWindow::Line{.speaker = "Quest Board", .text = std::move(acceptedText)});
                dialogLines.push_back(DialogWindow::Line{.speaker = "Objective", .text = std::move(objectiveText)});
                auto *dialog = m_uiManager.push<DialogWindow>(WindowId::WorldMapNodeInfo);
                dialog->setFont(FontManager::instance().get(FontRole::Body));
                dialog->start(std::move(dialogLines));
            }
            else if (m_tavernBoardWindow)
            {
                m_tavernBoardWindow->setVisible(true);
            }
        }
        else if (m_tavernBoardWindow)
        {
            m_tavernBoardWindow->setVisible(true);
        }
        return;
    }

    if (event.windowId == WindowId::TavernRecruitList)
    {
        if (event.type == UIEventType::ActionCanceled)
        {
            m_tavernRecruitWindow = nullptr;
            m_uiManager.popById(WindowId::TavernRecruitList);
            if (m_tavernHub)
                m_tavernHub->setVisible(true);
        }
        else if (event.type == UIEventType::ActionSelected && event.index >= 0 &&
                 event.index < static_cast<int>(m_tavernCandidateIndexes.size()))
        {
            const std::size_t candidateIndex =
                m_tavernCandidateIndexes[static_cast<std::size_t>(event.index)];
            m_pendingRecruitIndex = candidateIndex;
            const TavernCandidate &candidate = m_tavernCandidates[candidateIndex];

            PartyContext &ctx = PartyContext::instance();
            ctx.ensureInitialized();

            if (ctx.gold() < candidate.cost)
            {
                auto *dialog = m_uiManager.push<DialogWindow>(WindowId::WorldMapNodeInfo);
                dialog->setFont(FontManager::instance().get(FontRole::Body));
                dialog->start({DialogWindow::Line{.speaker = "Tavern", .text = "Not enough gold to hire " + candidate.customName + " (" + std::to_string(candidate.cost) + "g)."}});
            }
            else if (static_cast<int>(ctx.party().memberIds().size()) >= 6)
            {
                auto *dialog = m_uiManager.push<DialogWindow>(WindowId::WorldMapNodeInfo);
                dialog->setFont(FontManager::instance().get(FontRole::Body));
                dialog->start({DialogWindow::Line{.speaker = "Tavern", .text = "Your party is full."}});
            }
            else
            {
                auto *confirm = m_uiManager.push<ConfirmWindow>(WindowId::TavernRecruitConfirm);
                confirm->setFont(FontManager::instance().get(FontRole::Body));
                confirm->setPrompt("Hire " + candidate.customName + " for " + std::to_string(candidate.cost) + " gold?");
            }
        }
        return;
    }

    if (event.windowId == WindowId::TavernRecruitConfirm && event.type == UIEventType::ConfirmResult)
    {
        m_uiManager.popById(WindowId::TavernRecruitConfirm);
        if (event.confirmed && m_pendingRecruitIndex < m_tavernCandidates.size())
        {
            const TavernCandidate &candidate = m_tavernCandidates[m_pendingRecruitIndex];

            PartyContext &ctx = PartyContext::instance();
            ctx.ensureInitialized();
            if (candidate.cost > 0 && !ctx.spendGold(candidate.cost))
            {
                auto *dialog = m_uiManager.push<DialogWindow>(WindowId::WorldMapNodeInfo);
                dialog->setFont(FontManager::instance().get(FontRole::Body));
                dialog->start({DialogWindow::Line{.speaker = "Tavern", .text = "Not enough gold."}});
            }
            else
            {
                performRecruit(m_pendingRecruitIndex);
                refreshTavernRecruitList();
                auto *dialog = m_uiManager.push<DialogWindow>(WindowId::WorldMapNodeInfo);
                dialog->setFont(FontManager::instance().get(FontRole::Body));
                dialog->start({DialogWindow::Line{.speaker = "Tavern", .text = candidate.customName + " joined the party."}});
            }
        }
        return;
    }

    if (event.windowId == WindowId::TavernTalk && event.type == UIEventType::DialogFinished)
    {
        m_uiManager.popById(WindowId::TavernTalk);
        if (m_tavernHub)
            m_tavernHub->setVisible(true);
        return;
    }
}

void WorldMapState::openTavernQuestBoard()
{
    PartyContext &ctx = PartyContext::instance();
    ctx.ensureInitialized();

    if (m_tavernHub)
        m_tavernHub->setVisible(false);

    refreshTavernQuestBoard();
}

void WorldMapState::refreshTavernQuestBoard()
{
    PartyContext &ctx = PartyContext::instance();
    ctx.ensureInitialized();

    const std::vector<QuestDefinition> available = ctx.questLog().availableQuests();
    std::vector<ButtonMenuWindow::Item> items;
    m_tavernQuestIds.clear();
    m_tavernQuestIds.reserve(available.size());

    for (const QuestDefinition &quest : available)
    {
        items.push_back(ButtonMenuWindow::Item{.id = ActionId::AcceptMission, .label = quest.title, .enabled = true});
        m_tavernQuestIds.push_back(quest.id);
    }

    if (items.empty())
    {
        m_tavernBoardWindow = nullptr;
        m_uiManager.popById(WindowId::TavernQuestBoard);
        if (m_tavernHub)
            m_tavernHub->setVisible(true);
        return;
    }

    if (!m_tavernBoardWindow)
    {
        m_tavernBoardWindow = m_uiManager.push<ButtonMenuWindow>(WindowId::TavernQuestBoard);
        m_tavernBoardWindow->setFont(FontManager::instance().get(FontRole::Heading));
        m_tavernBoardWindow->setPanelPosition(Vec2f{0.0f, GameConstants::VIEW_CY - 40.0f});
        m_tavernBoardWindow->centerHorizontally(true);
    }
    m_tavernBoardWindow->setItems(std::move(items));
}

void WorldMapState::showTavernQuestDetail(int index)
{
    if (index < 0 || index >= static_cast<int>(m_tavernQuestIds.size()))
        return;

    PartyContext &ctx = PartyContext::instance();
    ctx.ensureInitialized();

    const QuestDefinition *quest = ctx.questLog().find(m_tavernQuestIds[static_cast<std::size_t>(index)]);
    if (!quest)
        return;

    if (m_tavernBoardWindow)
        m_tavernBoardWindow->setVisible(false);

    m_pendingQuestId = quest->id;

    std::string body = quest->description.empty() ? quest->brief : quest->description;
    body += "\n\nObjective: ";
    if (quest->objective.type == QuestObjectiveType::VictoryAtNode)
        body += "win the battle at node " + nodeNumberLabel(quest->objective.nodeId) + ".";
    else
        body += "reach node " + nodeNumberLabel(quest->objective.nodeId) + ".";
    body += "\nCost: ";
    body += (quest->cost > 0) ? std::to_string(quest->cost) + " gold" : "none";
    body += "\nReward:";
    if (quest->reward.gold > 0)
        body += " " + std::to_string(quest->reward.gold) + " gold";
    if (quest->reward.xp > 0)
        body += (quest->reward.gold > 0 ? "," : "") + std::string(" ") + std::to_string(quest->reward.xp) + " XP";
    if (!quest->reward.items.empty())
    {
        for (const std::string &key : quest->reward.items)
        {
            const Gear *gear = ctx.gearCatalog().findByKey(key);
            body += "\n  - " + std::string(gear ? gear->displayName : key);
        }
    }
    else if (quest->reward.gold == 0 && quest->reward.xp == 0)
    {
        body += " none";
    }

    auto *dialog = m_uiManager.push<DialogWindow>(WindowId::TavernQuestDetail);
    dialog->setFont(FontManager::instance().get(FontRole::Body));
    dialog->start({DialogWindow::Line{.speaker = quest->client.empty() ? "Quest" : quest->client, .text = quest->title},
                   DialogWindow::Line{.speaker = "Details", .text = body}});
}

void WorldMapState::openTavernRecruitList()
{
    if (m_tavernHub)
        m_tavernHub->setVisible(false);

    refreshTavernRecruitList();
}

void WorldMapState::refreshTavernRecruitList()
{
    PartyContext &ctx = PartyContext::instance();
    ctx.ensureInitialized();

    std::vector<ButtonMenuWindow::Item> items;
    m_tavernCandidateIndexes.clear();

    for (std::size_t candidateIndex = 0; candidateIndex < m_tavernCandidates.size(); ++candidateIndex)
    {
        const TavernCandidate &candidate = m_tavernCandidates[candidateIndex];

        // Already hired — look for the same (templatePath, customName) pair.
        bool alreadyHired = false;
        for (const RosterUnit &unit : ctx.roster().units())
        {
            if (unit.templatePath == candidate.templatePath && unit.customName == candidate.customName)
            {
                alreadyHired = true;
                break;
            }
        }
        if (alreadyHired)
            continue;

        std::string label = candidate.customName + " — " + std::to_string(candidate.cost) + "g";
        items.push_back(ButtonMenuWindow::Item{.id = ActionId::Recruit, .label = std::move(label), .enabled = true});
        m_tavernCandidateIndexes.push_back(candidateIndex);
    }

    if (items.empty())
    {
        m_tavernRecruitWindow = nullptr;
        m_uiManager.popById(WindowId::TavernRecruitList);
        if (m_tavernHub)
            m_tavernHub->setVisible(true);
        return;
    }

    if (!m_tavernRecruitWindow)
    {
        m_tavernRecruitWindow = m_uiManager.push<ButtonMenuWindow>(WindowId::TavernRecruitList);
        m_tavernRecruitWindow->setFont(FontManager::instance().get(FontRole::Heading));
        m_tavernRecruitWindow->setPanelPosition(Vec2f{0.0f, GameConstants::VIEW_CY - 40.0f});
        m_tavernRecruitWindow->centerHorizontally(true);
    }
    m_tavernRecruitWindow->setItems(std::move(items));
}

void WorldMapState::openTavernTalk()
{
    if (!m_tavernDataLoaded)
    {
        m_tavernCandidates = TavernData::loadCandidates("assets/tavern/candidates.json");
        m_tavernLore = TavernData::loadLore("assets/tavern/lore.json");
        m_tavernDataLoaded = true;
    }

    m_uiManager.popById(WindowId::TavernTalk);
    if (m_tavernHub)
        m_tavernHub->setVisible(false);

    auto *dialog = m_uiManager.push<DialogWindow>(WindowId::TavernTalk);
    dialog->setFont(FontManager::instance().get(FontRole::Body));

    std::vector<DialogWindow::Line> lines;
    lines.reserve(m_tavernLore.size());
    for (const TavernLine &line : m_tavernLore)
        lines.push_back(DialogWindow::Line{.speaker = line.speaker, .text = line.text});
    dialog->start(std::move(lines));
}

void WorldMapState::performRecruit(std::size_t candidateIndex)
{
    if (candidateIndex >= m_tavernCandidates.size())
        return;

    const TavernCandidate &candidate = m_tavernCandidates[candidateIndex];

    PartyContext &ctx = PartyContext::instance();
    RosterUnit *unit = ctx.roster().recruit(candidate.templatePath, candidate.customName);
    if (!unit)
        return;

    std::vector<int> members = ctx.party().memberIds();
    if (static_cast<int>(members.size()) < 6)
    {
        members.push_back(unit->instanceId);
        ctx.party().setMembers(std::move(members));
    }
}

void WorldMapState::handleShopEvent(const UIEvent &event)
{
    // Shop hub scene: pick which side of the counter to visit.
    if (event.windowId == WindowId::Shop)
    {
        if (event.type == UIEventType::ActionCanceled)
        {
            m_shopHubMenu = nullptr;
            m_uiManager.popById(WindowId::Shop);
            openTownHub();
            return;
        }

        if (event.type != UIEventType::ActionSelected)
            return;

        if (event.actionId == ActionId::BuyItem)
        {
            openShopList(ShopWindow::Mode::Buy);
        }
        else if (event.actionId == ActionId::SellItem)
        {
            openShopList(ShopWindow::Mode::Sell);
        }
        else if (event.actionId == ActionId::Close)
        {
            m_shopHubMenu = nullptr;
            m_uiManager.popById(WindowId::Shop);
        }
        return;
    }

    // A buy/sell list window: back returns to the shop hub scene.
    if (event.type == UIEventType::ActionCanceled)
    {
        m_shopWindow = nullptr;
        m_uiManager.popById(event.windowId);
        m_pendingShopIndex = -1;
        m_pendingShopBuy = false;
        if (m_shopHubMenu)
            m_shopHubMenu->setVisible(true);
        return;
    }

    if (event.type != UIEventType::ActionSelected || !m_shopWindow || event.index < 0)
        return;

    const std::size_t index = static_cast<std::size_t>(event.index);
    std::string prompt;

    if (event.actionId == ActionId::BuyItem)
    {
        if (index >= m_shopWindow->buyCount())
            return;
        const ShopWindow::BuyRow &row = m_shopWindow->buyRow(index);
        m_pendingShopIndex = event.index;
        m_pendingShopBuy = true;
        prompt = row.gear ? ("Buy " + row.gear->displayName + " for " + std::to_string(row.price) + " gold?")
                          : std::string("Buy?");
    }
    else if (event.actionId == ActionId::SellItem)
    {
        if (index >= m_shopWindow->sellCount())
            return;
        const ShopWindow::SellRow &row = m_shopWindow->sellRow(index);
        if (row.equippedCount > 0)
            return; // equipped copies cannot be sold
        m_pendingShopIndex = event.index;
        m_pendingShopBuy = false;
        prompt = "Sell " + row.name + " for " + std::to_string(row.price) + " gold?";
    }
    else
    {
        return;
    }

    auto *confirm = m_uiManager.push<ConfirmWindow>(WindowId::ShopConfirm);
    confirm->setFont(FontManager::instance().get(FontRole::Body));
    confirm->setPrompt(prompt);
}

void WorldMapState::openTownHub()
{
    m_uiManager.popById(WindowId::WorldMapNodeMenu);
    m_townHubMenu = m_uiManager.push<ButtonMenuWindow>(WindowId::WorldMapNodeMenu);
    m_townHubMenu->setFont(FontManager::instance().get(FontRole::Heading));
    m_townHubMenu->setItems({
        ButtonMenuWindow::Item{.id = ActionId::OpenTavern, .label = "Tavern", .enabled = true},
        ButtonMenuWindow::Item{.id = ActionId::OpenShop, .label = "Shop", .enabled = true},
        ButtonMenuWindow::Item{.id = ActionId::Talk, .label = "Talk", .enabled = true},
        ButtonMenuWindow::Item{.id = ActionId::Close, .label = "Quit", .enabled = true},
    });
}

void WorldMapState::openTownTalk()
{
    if (!m_townTalkLoaded)
    {
        m_townTalk = TavernData::loadLore("assets/town/talk.json");
        m_townTalkLoaded = true;
    }

    if (m_townHubMenu)
        m_townHubMenu->setVisible(false);

    auto *dialog = m_uiManager.push<DialogWindow>(WindowId::TownTalk);
    dialog->setFont(FontManager::instance().get(FontRole::Body));

    std::vector<DialogWindow::Line> lines;
    lines.reserve(m_townTalk.size());
    for (const TavernLine &line : m_townTalk)
        lines.push_back(DialogWindow::Line{.speaker = line.speaker, .text = line.text});
    dialog->start(std::move(lines));
}

void WorldMapState::showNodeInfo(int nodeId)
{
    auto it = m_nodeMeta.find(nodeId);
    const std::string description = (it != m_nodeMeta.end()) ? it->second.description : std::string("Empty Node");

    m_uiManager.popById(WindowId::WorldMapNodeInfo);
    auto *dialog = m_uiManager.push<DialogWindow>(WindowId::WorldMapNodeInfo);
    dialog->setFont(FontManager::instance().get(FontRole::Body));
    dialog->start({DialogWindow::Line{.speaker = "Node", .text = description}});
}

void WorldMapState::startBattle(PendingBattle battle)
{
    if (battle.nodeId < 0 || battle.mapPath.empty())
        return;
    if (m_transition.isActive())
        return;

    m_pendingBattle = PendingBattle{};

    StateMachine<Scene> &sm = m_sm;
    Renderer *renderer = m_renderer;
    auto onFinished = [nodeId = battle.nodeId, &sm, renderer](bool playerWon)
    {
        sm.replace(std::make_unique<WorldMapState>(sm, renderer, nodeId, playerWon ? nodeId : -1));
    };

    BattleRequest request{
        .mapPath = battle.mapPath,
        .returnWorldNodeId = battle.nodeId,
    };

    // Close every world-map window and fade to black first, then hand over to
    // the battle. BattleState fades back in on entry, so the two halves read as
    // one ~1s transition instead of a hard cut.
    m_uiManager.clear();
    m_nodeEventWindow = nullptr;
    m_worldMapMenu = nullptr;
    m_townHubMenu = nullptr;
    m_saveListWindow = nullptr;
    m_journalWindow = nullptr;
    m_shopHubMenu = nullptr;
    m_shopWindow = nullptr;
    m_tavernHub = nullptr;
    m_tavernBoardWindow = nullptr;
    m_tavernRecruitWindow = nullptr;

    m_transition.start({
        .transition = ScreenTransitions::FadeOut,
        .duration = 0.5f,
        .easing = easeInOut,
        .onComplete = [this, request = std::move(request), onFinished = std::move(onFinished)]()
        {
            m_sm.replace(std::make_unique<BattleState>(m_sm, m_renderer, request, std::move(onFinished)));
        }});
}

void WorldMapState::processUIEvents()
{
    auto events = m_uiManager.drainEvents();
    for (const UIEvent &event : events)
    {
        if (event.windowId == WindowId::WorldMapNodeMenu)
        {
            if (event.type == UIEventType::ActionCanceled)
            {
                leaveTownHub();
            }
            else if (event.type == UIEventType::ActionSelected)
            {
                if (event.actionId == ActionId::OpenShop)
                    openShopWindow();
                else if (event.actionId == ActionId::OpenTavern)
                    openTavernHub();
                else if (event.actionId == ActionId::Talk)
                    openTownTalk();
                else if (event.actionId == ActionId::Close)
                    leaveTownHub();
            }
            continue;
        }

        if (event.windowId == WindowId::TownTalk && event.type == UIEventType::DialogFinished)
        {
            m_uiManager.popById(WindowId::TownTalk);
            if (m_townHubMenu)
                m_townHubMenu->setVisible(true);
            continue;
        }

        if (event.windowId == WindowId::Shop || event.windowId == WindowId::ShopBuy ||
            event.windowId == WindowId::ShopSell)
        {
            handleShopEvent(event);
            continue;
        }

        if (event.windowId == WindowId::Tavern || event.windowId == WindowId::TavernQuestBoard ||
            event.windowId == WindowId::TavernQuestDetail ||
            event.windowId == WindowId::TavernQuestAccept ||
            event.windowId == WindowId::TavernRecruitList ||
            event.windowId == WindowId::TavernRecruitConfirm ||
            event.windowId == WindowId::TavernTalk)
        {
            handleTavernEvent(event);
            continue;
        }

        if (event.windowId == WindowId::ShopConfirm && event.type == UIEventType::ConfirmResult)
        {
            m_uiManager.popById(WindowId::ShopConfirm);
            if (event.confirmed && m_shopWindow && m_pendingShopIndex >= 0)
            {
                PartyContext &ctx = PartyContext::instance();
                ctx.ensureInitialized();
                std::string toast;
                if (m_pendingShopBuy)
                {
                    const std::size_t index = static_cast<std::size_t>(m_pendingShopIndex);
                    if (index < m_shopWindow->buyCount())
                    {
                        const ShopWindow::BuyRow &row = m_shopWindow->buyRow(index);
                        if (row.gear && row.price > 0 && ctx.spendGold(row.price))
                        {
                            ctx.inventory().add(*row.gear, 1);
                            m_shopWindow->refresh();
                            toast = "Purchased " + row.gear->displayName + " for " + std::to_string(row.price) + " gold.";
                        }
                        else
                        {
                            toast = "Not enough gold.";
                        }
                    }
                }
                else
                {
                    const std::size_t index = static_cast<std::size_t>(m_pendingShopIndex);
                    if (index < m_shopWindow->sellCount())
                    {
                        const ShopWindow::SellRow &row = m_shopWindow->sellRow(index);
                        if (row.equippedCount == 0 && row.price > 0 && ctx.inventory().remove(row.itemId, 1))
                        {
                            ctx.addGold(row.price);
                            m_shopWindow->refresh();
                            toast = "Sold " + row.name + " for " + std::to_string(row.price) + " gold.";
                        }
                    }
                }
                if (!toast.empty())
                {
                    auto *dialog = m_uiManager.push<DialogWindow>(WindowId::WorldMapNodeInfo);
                    dialog->setFont(FontManager::instance().get(FontRole::Body));
                    dialog->start({DialogWindow::Line{.speaker = "Shopkeeper", .text = toast}});
                }
            }
            m_pendingShopIndex = -1;
            m_pendingShopBuy = false;
            continue;
        }

        if (event.windowId == WindowId::WorldMapEncounterConfirm && event.type == UIEventType::ConfirmResult)
        {
            m_uiManager.popById(WindowId::WorldMapEncounterConfirm);
            if (event.confirmed)
                startBattle(m_pendingBattle);
            else
                m_pendingBattle = PendingBattle{};
            continue;
        }

        if (event.windowId == WindowId::WorldMapNodeEvents)
        {
            m_nodeEventWindow = nullptr;
            m_uiManager.popById(WindowId::WorldMapNodeEvents);
            if (event.type == UIEventType::ActionSelected && event.index >= 0)
                playNodeEvent(event.index);
            continue;
        }

        if (event.windowId == WindowId::WorldMapJournal)
        {
            if (event.type == UIEventType::ActionCanceled)
            {
                closeJournal();
            }
            else if (event.type == UIEventType::ActionSelected)
            {
                if (event.index >= 0 && event.index < static_cast<int>(m_journalQuestIndexes.size()))
                {
                    m_journalWindow->setVisible(false);
                    showQuestDetail(m_journalQuestIndexes[static_cast<std::size_t>(event.index)]);
                }
            }
            continue;
        }

        if (event.windowId == WindowId::WorldMapNodeInfo && event.type == UIEventType::DialogFinished)
        {
            m_uiManager.popById(WindowId::WorldMapNodeInfo);
            continue;
        }

        if (event.windowId == WindowId::WorldMapQuestComplete && event.type == UIEventType::DialogFinished)
        {
            m_uiManager.popById(WindowId::WorldMapQuestComplete);
            if (m_journalWindow)
            {
                m_journalWindow->setVisible(true);
                continue;
            }
            continue;
        }

        if (event.windowId == WindowId::SaveListDeleteConfirm && event.type == UIEventType::ConfirmResult)
        {
            m_uiManager.popById(WindowId::SaveListDeleteConfirm);
            if (event.confirmed && m_pendingDeleteSlotIndex >= 0)
            {
                SaveManager::instance().deleteSave(m_pendingDeleteSlotIndex);
                m_pendingDeleteSlotIndex = -1;
                if (m_saveListWindow)
                    refreshSaveList();
            }
            continue;
        }

        if (event.windowId == WindowId::SaveList)
        {
            if (event.type == UIEventType::ActionCanceled)
            {
                closeSaveList();
            }
            else if (event.type == UIEventType::ActionSelected)
            {
                const int idx = event.index;
                if (idx < 0 || idx >= static_cast<int>(m_saveListSlotIndexes.size()))
                    continue;

                if (event.actionId == ActionId::LoadGame)
                {
                    CampaignSave save;
                    const int slotIndex = m_saveListSlotIndexes[static_cast<std::size_t>(idx)];
                    if (SaveManager::instance().loadCampaign(slotIndex, save))
                    {
                        PartyContext &partyContext = PartyContext::instance();
                        partyContext.ensureInitialized();
                        if (partyContext.applyCampaign(save))
                        {
                            m_uiManager.popById(WindowId::SaveList);
                            m_sm.replace(std::make_unique<WorldMapState>(m_sm, m_renderer, save.worldNodeId));
                            return;
                        }
                    }
                }
                else if (event.actionId == ActionId::DeleteSave)
                {
                    m_pendingDeleteSlotIndex = m_saveListSlotIndexes[static_cast<std::size_t>(idx)];
                    pushDeleteSaveConfirm(m_uiManager);
                }
            }
            continue;
        }

        if (event.windowId == WindowId::WorldMap && event.type == UIEventType::ActionSelected)
        {
            if (event.actionId == ActionId::OpenPartyMenu)
            {
                PartyContext &partyContext = PartyContext::instance();
                partyContext.ensureInitialized();
                auto *party = m_uiManager.push<PartyWindow>(WindowId::PartyMenu);
                party->setFont(FontManager::instance().get(FontRole::Body));

                std::vector<PartyWindow::Entry> entries;
                const std::vector<int> &memberIds = partyContext.party().memberIds();
                entries.reserve(memberIds.size());
                m_partyEntryInstanceIds.clear();
                m_partyEntryInstanceIds.reserve(memberIds.size());

                for (int instanceId : memberIds)
                {
                    const RosterUnit *rosterUnit = partyContext.roster().findById(instanceId);
                    if (!rosterUnit)
                        continue;

                    const std::string &templatePath = rosterUnit->templatePath;
                    bool hasSprite = false;
                    std::string displayName = rosterUnit->customName;
                    try
                    {
                        const UnitData data = UnitLoader::load(templatePath);
                        hasSprite = !data.spriteSetId.empty();
                        if (displayName.empty())
                            displayName = data.name;
                    }
                    catch (...)
                    {
                    }

                    entries.push_back(PartyWindow::Entry{.name = displayName, .templatePath = templatePath, .hasSprite = hasSprite});
                    m_partyEntryInstanceIds.push_back(instanceId);
                }

                party->setEntries(std::move(entries));
            }
            else if (event.actionId == ActionId::OpenInventory)
            {
                PartyContext &partyContext = PartyContext::instance();
                partyContext.ensureInitialized();
                auto *inventoryWin = m_uiManager.push<InventoryWindow>(
                    WindowId::Inventory, partyContext.inventory(), partyContext.gearCatalog());
                inventoryWin->setFont(FontManager::instance().get(FontRole::Body));
            }
            else if (event.actionId == ActionId::OpenJournal)
            {
                openJournal();
            }
            else if (event.actionId == ActionId::SaveGame)
            {
                PartyContext &partyContext = PartyContext::instance();
                partyContext.ensureInitialized();
                const bool ok = SaveManager::instance().saveCampaign(
                    partyContext.roster(), partyContext.party(),
                    partyContext.inventory(), m_playerNodeId, partyContext.gold(),
                    partyContext.controlsTutorialShown(),
                    partyContext.battleControlsTutorialShown(),
                    partyContext.questLog().toJson());

                m_uiManager.popById(WindowId::WorldMap);
                m_worldMapMenu = nullptr;
                if (ok)
                {
                    auto *dialog = m_uiManager.push<DialogWindow>(WindowId::WorldMapNodeInfo);
                    dialog->setFont(FontManager::instance().get(FontRole::Body));
                    dialog->start({DialogWindow::Line{.speaker = "System", .text = "Game saved."}});
                }
            }
            else if (event.actionId == ActionId::LoadGame)
            {
                openSaveList();
            }
            else if (event.actionId == ActionId::OpenSettings)
            {
                m_sm.push(std::make_unique<SettingsState>(m_sm, m_renderer));
            }
            else if (event.actionId == ActionId::QuitGame)
            {
                App::requestQuit();
            }
            continue;
        }

        if (event.windowId == WindowId::Inventory && event.type == UIEventType::ActionCanceled)
        {
            m_uiManager.popById(WindowId::Inventory);
            continue;
        }

        if (event.windowId == WindowId::PartyMenu && event.type == UIEventType::ActionSelected && event.actionId == ActionId::Inspect)
        {
            if (event.index >= 0 && event.index < static_cast<int>(m_partyEntryInstanceIds.size()))
            {
                PartyContext &partyContext = PartyContext::instance();
                partyContext.ensureInitialized();
                const int instanceId = m_partyEntryInstanceIds[static_cast<std::size_t>(event.index)];
                if (const RosterUnit *rosterUnit = partyContext.roster().findById(instanceId))
                {
                    m_activePartyDetailInstanceId = instanceId;
                    auto *details = m_uiManager.push<UnitDetailWindow>(
                        WindowId::PartyDetail, *rosterUnit, partyContext.roster(),
                        partyContext.inventory(), partyContext.gearCatalog());
                    details->setFont(FontManager::instance().get(FontRole::Body));
                }
            }
            continue;
        }

        if (event.windowId == WindowId::PartyDetail && event.type == UIEventType::ActionCanceled)
        {
            m_activePartyDetailInstanceId = -1;
            m_uiManager.popById(WindowId::PartyDetail);
            continue;
        }

        if (event.windowId == WindowId::PartyDetail && event.type == UIEventType::ActionSelected &&
            event.actionId == ActionId::OpenSkills)
        {
            PartyContext &partyContext = PartyContext::instance();
            partyContext.ensureInitialized();
            if (const RosterUnit *rosterUnit = partyContext.roster().findById(m_activePartyDetailInstanceId))
            {
                auto *skills = m_uiManager.push<SkillProgressionWindow>(
                    WindowId::PartySkills, *rosterUnit, partyContext.roster());
                skills->setFont(FontManager::instance().get(FontRole::Body));
            }
            continue;
        }

        if (event.windowId == WindowId::PartySkills && event.type == UIEventType::ActionCanceled)
        {
            m_uiManager.popById(WindowId::PartySkills);
            continue;
        }

        if ((event.windowId == WindowId::WorldMap || event.windowId == WindowId::PartyMenu) && event.type == UIEventType::ActionCanceled)
        {
            m_uiManager.popById(event.windowId);
            if (event.windowId == WindowId::WorldMap)
                m_worldMapMenu = nullptr;
            continue;
        }
    }
}

void WorldMapState::openSaveList()
{
    SaveManager::instance().loadIndex();

    std::vector<ButtonMenuWindow::Item> items;
    std::vector<int> slotIndexes;
    for (const SaveSlotInfo &info : SaveManager::instance().slots())
    {
        if (info.kind != SaveKind::Campaign)
            continue;
        items.push_back(ButtonMenuWindow::Item{.id = ActionId::LoadGame, .label = info.label, .enabled = true});
        slotIndexes.push_back(info.slotIndex);
    }
    if (items.empty())
        return;

    m_saveListSlotIndexes = std::move(slotIndexes);
    if (m_worldMapMenu)
        m_worldMapMenu->setVisible(false);

    m_saveListWindow = m_uiManager.push<ButtonMenuWindow>(WindowId::SaveList);
    m_saveListWindow->setFont(FontManager::instance().get(FontRole::Heading));
    m_saveListWindow->setPanelPosition(Vec2f{0.0f, GameConstants::VIEW_CY - 40.0f});
    m_saveListWindow->centerHorizontally(true);
    m_saveListWindow->setItems(std::move(items));
    m_saveListWindow->setDeleteMode(ActionId::DeleteSave, "X: Delete save file");
}

void WorldMapState::refreshSaveList()
{
    if (!m_saveListWindow)
        return;

    std::vector<ButtonMenuWindow::Item> items;
    std::vector<int> slotIndexes;
    for (const SaveSlotInfo &info : SaveManager::instance().slots())
    {
        if (info.kind != SaveKind::Campaign)
            continue;
        items.push_back(ButtonMenuWindow::Item{.id = ActionId::LoadGame, .label = info.label, .enabled = true});
        slotIndexes.push_back(info.slotIndex);
    }
    m_saveListSlotIndexes = std::move(slotIndexes);
    if (items.empty())
    {
        closeSaveList();
        return;
    }
    m_saveListWindow->setItems(std::move(items));
}

void WorldMapState::closeSaveList()
{
    m_uiManager.popById(WindowId::SaveList);
    m_saveListWindow = nullptr;
    m_saveListSlotIndexes.clear();
    m_pendingDeleteSlotIndex = -1;
    if (m_worldMapMenu)
    {
        m_worldMapMenu->setVisible(true);
        refreshWorldMapMenuItems();
    }
}

void WorldMapState::refreshWorldMapMenuItems()
{
    if (!m_worldMapMenu)
        return;
    m_worldMapMenu->setItems({
        ButtonMenuWindow::Item{.id = ActionId::OpenPartyMenu, .label = "Party", .enabled = true},
        ButtonMenuWindow::Item{.id = ActionId::OpenJournal, .label = "Quests", .enabled = true},
        ButtonMenuWindow::Item{.id = ActionId::OpenInventory, .label = "Inventory", .enabled = true},
        ButtonMenuWindow::Item{.id = ActionId::SaveGame, .label = "Save", .enabled = true},
        ButtonMenuWindow::Item{.id = ActionId::LoadGame, .label = "Load", .enabled = SaveManager::instance().latestCampaignSlot() >= 0},
        ButtonMenuWindow::Item{.id = ActionId::OpenSettings, .label = "Options", .enabled = true},
        ButtonMenuWindow::Item{.id = ActionId::QuitGame, .label = "Quit", .enabled = true},
    });
}

void WorldMapState::openJournal()
{
    PartyContext &ctx = PartyContext::instance();
    ctx.ensureInitialized();
    ctx.questLog().ensureLoaded();

    const std::vector<QuestDefinition> &allQuests = ctx.questLog().all();
    std::vector<ButtonMenuWindow::Item> items;
    m_journalQuestIndexes.clear();

    for (std::size_t i = 0; i < allQuests.size(); ++i)
    {
        const QuestDefinition &q = allQuests[i];
        const bool completed = ctx.questLog().isCompleted(q.id);
        const bool active = ctx.questLog().isActive(q.id);

        // Only quests the party knows about show in the journal: beyond
        // completed quests, un-accepted tavern quests stay hidden until
        // picked up from the quest board.
        if (!active && !completed)
            continue;

        std::string label;
        if (completed)
            label = "[done] ";
        else
            label = "[...] ";
        label += q.title;

        items.push_back(ButtonMenuWindow::Item{.id = ActionId::None, .label = std::move(label), .enabled = true});
        m_journalQuestIndexes.push_back(static_cast<int>(i));
    }

    if (items.empty())
    {
        m_uiManager.popById(WindowId::WorldMap);
        m_worldMapMenu = nullptr;
        return;
    }

    if (m_worldMapMenu)
        m_worldMapMenu->setVisible(false);

    m_journalWindow = m_uiManager.push<ButtonMenuWindow>(WindowId::WorldMapJournal);
    m_journalWindow->setFont(FontManager::instance().get(FontRole::Heading));
    m_journalWindow->setPanelPosition(Vec2f{0.0f, GameConstants::VIEW_CY - 40.0f});
    m_journalWindow->centerHorizontally(true);
    m_journalWindow->setItems(std::move(items));
}

void WorldMapState::showQuestDetail(int questIndex)
{
    PartyContext &ctx = PartyContext::instance();
    ctx.ensureInitialized();
    ctx.questLog().ensureLoaded();

    const std::vector<QuestDefinition> &allQuests = ctx.questLog().all();
    if (questIndex < 0 || questIndex >= static_cast<int>(allQuests.size()))
        return;
    const QuestDefinition &q = allQuests[static_cast<std::size_t>(questIndex)];

    std::string body = q.description.empty() ? q.brief : q.description;
    body += "\n\n";
    if (q.objective.type == QuestObjectiveType::VictoryAtNode)
        body += "Objective: win the battle at node " + nodeNumberLabel(q.objective.nodeId) + ".";
    else if (q.objective.type == QuestObjectiveType::ReachNode)
        body += "Objective: reach node " + nodeNumberLabel(q.objective.nodeId) + ".";
    body += "\nReward:";
    if (q.reward.gold > 0)
        body += " " + std::to_string(q.reward.gold) + " gold";
    if (q.reward.xp > 0)
        body += (q.reward.gold > 0 ? "," : "") + std::string(" ") + std::to_string(q.reward.xp) + " XP";
    if (q.reward.items.empty())
        body += " none";

    m_uiManager.popById(WindowId::WorldMapQuestComplete);
    auto *dialog = m_uiManager.push<DialogWindow>(WindowId::WorldMapQuestComplete);
    dialog->setFont(FontManager::instance().get(FontRole::Body));
    dialog->start({DialogWindow::Line{.speaker = q.isMain ? "Main" : "Side", .text = q.title},
                   DialogWindow::Line{.speaker = "Details", .text = std::move(body)}});
}

void WorldMapState::closeJournal()
{
    m_uiManager.popById(WindowId::WorldMapJournal);
    m_journalWindow = nullptr;
    m_journalQuestIndexes.clear();
    if (m_worldMapMenu)
    {
        m_worldMapMenu->setVisible(true);
        refreshWorldMapMenuItems();
    }
}

std::string WorldMapState::nodeNumberLabel(int id) const
{
    auto posIt = m_nodeGridPos.find(id);
    if (posIt == m_nodeGridPos.end())
        return std::to_string(id + 1);

    int gridWidth = 0;
    for (const auto &entry : m_nodeGridPos)
        gridWidth = std::max(gridWidth, entry.second.x + 1);

    const int rank = posIt->second.y * gridWidth + posIt->second.x + 1;
    return std::to_string(rank);
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

    PartyContext &ctx = PartyContext::instance();
    ctx.ensureInitialized();

    for (int id : m_graph.nodeIds())
    {
        const world::WorldGraph::Node *node = m_graph.getNode(id);
        if (!node)
            continue;

        Color color = kNodeColor;
        auto metaIt = m_nodeMeta.find(id);
        if (metaIt != m_nodeMeta.end() && metaIt->second.type == NodeType::Town)
            color = kTownNodeColor;

        if (!ctx.questLog().isNodeUnlocked(id))
            color = Color{88, 90, 100, 255}; // locked — neutral slate

        const Vec2f screenPos = toScreen(node->position);
        UIUtils::drawCircle(m_renderer, screenPos, kNodeRadius, color);

        // Numbered like a phone keypad (top row 1-2-3, middle 4-5-6, bottom
        // 7-8-9) so the quest journal can point at a node by number.
        if (const Font *font = FontManager::instance().get(FontRole::Body))
        {
            const std::string label = nodeNumberLabel(id);
            const Vec2f labelSize = m_renderer->measureText(font, label);
            m_renderer->renderText(font,
                                   label.c_str(),
                                   Vec2f{screenPos.x - labelSize.x * 0.5f, screenPos.y + kNodeRadius + 4.0f},
                                   kNodeLabelColor,
                                   false,
                                   false,
                                   false);
        }
    }

    ctx.questLog().ensureLoaded();
    std::unordered_set<int> objectiveNodes;
    for (const QuestDefinition &quest : ctx.questLog().all())
    {
        if (ctx.questLog().isActive(quest.id) && quest.objective.nodeId >= 0)
            objectiveNodes.insert(quest.objective.nodeId);
    }
    for (int id : objectiveNodes)
    {
        const world::WorldGraph::Node *goalNode = m_graph.getNode(id);
        if (goalNode)
            UIUtils::drawCircle(m_renderer, toScreen(goalNode->position), kNodeRadius + 5.0f, kObjectiveHaloColor, 24);
    }

    if (m_hoveredNodeId >= 0)
    {
        const world::WorldGraph::Node *hoveredNode = m_graph.getNode(m_hoveredNodeId);
        if (hoveredNode)
            UIUtils::drawCircle(m_renderer, toScreen(hoveredNode->position), kHoverRadius, kHoverColor, 28);
    }

    UIUtils::drawCircle(m_renderer, toScreen(m_cursorPos), kCursorRadius, kCursorColor, 16);

    Vec2f playerPos{0.0f, 0.0f};
    const world::WorldGraph::Node *playerNode = m_graph.getNode(m_playerNodeId);
    if (playerNode)
        playerPos = playerNode->position;

    if (m_isMoving && m_path.size() >= 2)
    {
        const world::WorldGraph::Node *fromNode = m_graph.getNode(m_path[m_pathSegment]);
        const world::WorldGraph::Node *toNode = m_graph.getNode(m_path[m_pathSegment + 1]);
        if (fromNode && toNode)
        {
            const float t = std::clamp(m_segmentProgress, 0.0f, 1.0f);
            playerPos = Vec2f{lerp(fromNode->position.x, toNode->position.x, t),
                              lerp(fromNode->position.y, toNode->position.y, t)};
        }
    }

    UIUtils::drawCircle(m_renderer, toScreen(playerPos), kPlayerRadius, kPlayerColor, 24);
}
