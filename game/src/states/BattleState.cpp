#include "engine/core/App.h"
#include "engine/math/Vec2.h"
#include "engine/math/MathUtils.h"
#include "engine/data/TiledJsonLoader.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/core/Log.h"
#include "engine/core/Window.h"
#include "engine/effects/ScreenTransition.h"
#include "engine/renderer/Camera.h"
#include "config/GameConstants.h"
#include "engine/renderer/Renderer.h"
#include "states/BattleState.h"
#include "states/PauseState.h"
#include "states/ResultState.h"
#include "battle/Unit.h"
#include "battle/UnitFactory.h"
#include "ai/EnemyAI.h"
#include "ui/Cursor.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <direct.h>

namespace
{
    // Sprite sheet dimensions for 0.5H_IsoTiles.png.
    //   Sprite size  : 48 × 36 px
    //   Map tile size: 48 × 24 px
    // Hardcoded because back-calculating from max-GID is fragile — the
    // sheet format is fixed for this asset.
    namespace
    {
        constexpr float SPRITE_H = 36.0f;

        // Spawn-overlay colours.
        constexpr FColor COL_PLAYER_SPAWN = {0.18f, 0.52f, 0.89f, 0.55f}; // blue
        constexpr FColor COL_ENEMY_SPAWN = {0.89f, 0.18f, 0.18f, 0.55f};  // red

        // Cursor colours.
        constexpr FColor COL_BLACK = {0.0f, 0.0f, 0.0f, 1.0f};
        constexpr FColor COL_RED = {1.0f, 0.0f, 0.0f, 1.0f};
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

BattleState::BattleState(StateMachine<Scene> &sm, Renderer *renderer)
    : m_sm(sm), m_renderer(renderer)
{
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

void BattleState::onEnter()
{
    // ── 1. Acquire engine dependencies ──
    m_renderer = App::getRenderer();
    if (!m_renderer)
        return;

    Window *window = App::getWindow();
    if (!window)
        return;

    // ── 2. Configure window and renderer ──
    window->setResizable(true);
    window->setAspectRatio(16.0f / 9.0f, 16.0f / 9.0f);

    m_renderer->setLogicalPresentation(
        static_cast<int>(GameConstants::VIEW_W),
        static_cast<int>(GameConstants::VIEW_H),
        Renderer::PresentationMode::Letterbox);

    // ── 3. Load map data (tile dimensions, layers, etc.) ──
    if (!loadMap())
        return;

    // ── 4. Build gameplay grid (terrain, occupancy) ──
    if (!buildGameplayGrid())
        return;

    // ── 5. Create units from spawn points ──
    auto playerUnits = UnitFactory::createUnitsFromSpawns(
        m_battleMap.playerSpawns,
        "assets/units/soldier.json", 0);
    for (Unit *u : playerUnits)
    {
        m_grid.getTile(u->getPosition()).occupied = true;
        m_units.push_back(u);
    }

    auto allyUnits = UnitFactory::createUnitsFromSpawns(
        m_battleMap.allySpawns,
        "assets/units/soldier.json", 1);
    for (Unit *u : allyUnits)
    {
        m_grid.getTile(u->getPosition()).occupied = true;
        m_units.push_back(u);
    }

    for (auto &pair : m_battleMap.enemySpawnsByTeam)
    {
        int team = pair.first;
        auto &spawns = pair.second;
        auto units = UnitFactory::createUnitsFromSpawns(spawns, "assets/units/soldier.json", team);
        for (Unit *u : units)
        {
            m_grid.getTile(u->getPosition()).occupied = true;
            m_units.push_back(u);
        }
    }

    // ── 6. Initialise turn queue with all units ──
    m_turnQueue.init(m_units);

    // ── 7. Load tileset texture (sets m_spriteH) ──
    if (!loadTileset())
        return;

    // ── 8. Compute map origin (depends on m_spriteH) and set up camera ──
    Vec2f origin = computeMapOrigin();
    m_camera.setTileSize(m_mapData.tileWidth, m_mapData.tileHeight);
    m_camera.setMapSize(m_mapData.width, m_mapData.height);
    m_camera.setOffset(origin);
    m_previousCamera = m_camera;

    // ── 9. Set up debug renderer ──
    m_debugRenderer = &DebugRenderer::instance();
    m_debugRenderer->setEnabled(true);

    // ── 10. Load default UI font (once) & wire to UnitPanel ──
    if (!App::getDefaultFont())
    {

        char cwd[1024];
        if (_getcwd(cwd, sizeof(cwd)))
            LOG_INFO("Battle", "Working directory: %s", cwd);
        const char *fontPath = R"(H:\Coding\Games\TRPG\game_1\game\assets\fonts\ARCADECLASSIC.TTF)";
        Font *font = m_renderer->loadFont(fontPath, 16);
        if (!font)
            LOG_ERROR("Battle", "Failed to load font: %s", fontPath);
        App::setDefaultFont(font);
    }

    m_unitPanel.setFont(App::getDefaultFont());

    // ── 11. Initial turn banner ──
    if (!m_units.empty())
    {
        Unit *first = m_turnQueue.getCurrentUnit();
        m_unitPanel.setTurnInfo(1, first);
        m_actionMenu.setOnConfirm([this](int index)
                                  {
    Unit *active = m_turnQueue.getCurrentUnit();
    if (!active)
        return;

    if (m_playerControlMode == PlayerControlMode::AI)
    {
        // AI mode – run full turn immediately.
        EnemyAI::takeTurn(*active, m_grid, m_units);
        m_cursor.setPosition(active->getPosition());
        m_turnTimer = 0.15f;
        m_turnState = TurnState::WaitingForAnimation;
        return;
    }

    // Human mode – step through phases.
    switch (m_humanTurnPhase)
    {
    case HumanTurnPhase::ActionMenu:
    {
        // The player just chose an action from the initial menu.
        int action = index;   // 0 = Move, 1 = Attack, 2 = Wait

        if (action == 0)   // Move
        {
            m_humanTurnPhase = HumanTurnPhase::MoveTarget;
            m_actionMenu.hide();   // menu goes away while moving
            // Stay in ProcessingTurn state so update() drives cursor movement.
        }
        else if (action == 1)   // Attack
        {
            m_humanTurnPhase = HumanTurnPhase::AttackTarget;
            m_actionMenu.hide();
        }
        else   // Wait
        {
            // Unit does nothing – end turn immediately.
            m_humanTurnPhase = HumanTurnPhase::TurnEnded;
            active->setHasMoved(true);  // prevent move / act later
            active->setHasActed(true);
            advanceToNextUnit();
            m_turnState = TurnState::Idle;
        }
        break;
    }
    case HumanTurnPhase::MoveTarget:
        // Not used – movement is confirmed by Accept in handleActiveTurn.
        break;
    case HumanTurnPhase::AttackTarget:
        // Not used – attack confirmed by Accept in handleActiveTurn.
        break;
    case HumanTurnPhase::TurnEnded:
        break;
    } });
        m_actionMenu.setOnCancel([this]()
                                 {
                                     // Cancel = back to unit selection (or do nothing for now)
                                     Unit *active = m_turnQueue.getCurrentUnit();
                                     if (active)
                                         LOG_INFO("Battle", "Player cancelled action for %s", active->getName().c_str());
                                     // Do not advance turn; just hide menu
                                 });
    }

    // ── 12. Start fade‑in transition ──
    m_transition.start({
        .transition = ScreenTransitions::FadeIn,
        .duration = 0.5f,
        .easing = easeInOut,
    });
}

void BattleState::onExit()
{
    delete m_tileset;
    m_tileset = nullptr;

    m_battleMap.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// Input Pass
// ─────────────────────────────────────────────────────────────────────────────

void BattleState::handleInput()
{
    const Input &input = Input::instance();

    // 1. Structural Global States (Instant execution)
    if (input.isKeyPressed(KeyCode::Pause))
    {
        m_sm.push(std::make_unique<PauseState>(m_sm, m_renderer));
        return;
    }

    // Safety check: Freeze human input interactions if scenes are fading
    if (m_transition.isActive())
        return;

    // 2. Debug Override Trigger
    if (input.isKeyPressed(KeyCode::Advance))
    {
        startBattleEnd(false);
        return;
    }

    // 2.5. Toggle player control mode (DebugToggle key)
    if (input.isKeyPressed(KeyCode::DebugToggle))
    {
        if (m_playerControlMode == PlayerControlMode::AI)
        {
            m_playerControlMode = PlayerControlMode::Human;
            LOG_INFO("Battle", "Switched to Human control");
        }
        else
        {
            m_playerControlMode = PlayerControlMode::AI;
            LOG_INFO("Battle", "Switched to AI control");
        }
    }

    // 3. Delegate to tactical gameplay controls
    handleActiveTurn(input);
}

void BattleState::handleActiveTurn(const Input &input)
{
    // If the action menu is visible, it handles its own input via m_actionMenu.update().
    if (m_actionMenu.isVisible())
        return;

    // Only process targeting inputs if it's a human player's turn.
    if (m_playerControlMode != PlayerControlMode::Human)
        return;

    Unit *active = m_turnQueue.getCurrentUnit();
    if (!active || active->getTeam() != 0)
        return;

    // Handle Confirm (Accept) and Cancel (Back) during targeting phases.
    if (input.isKeyPressed(KeyCode::Accept, false))
    {
        if (m_humanTurnPhase == HumanTurnPhase::MoveTarget)
        {
            // Confirm the current cursor position as the move destination.
            Vec2i dest = m_cursor.getPosition();

            // You should add a check later: is 'dest' within reachable tiles?
            // For now, just move the unit there.
            Vec2i start = active->getPosition();
            m_grid.getTile(start).occupied = false;
            active->setPosition(dest);
            m_grid.getTile(dest).occupied = true;
            active->setHasMoved(true);

            // After moving, re‑open the action menu (now only Attack / Wait).
            m_humanTurnPhase = HumanTurnPhase::ActionMenu;
            m_actionMenu.show({"Move", "Attack", "Wait"}, {false, true, true});
            LOG_INFO("Battle", "Menu opened after move, visible=%d", m_actionMenu.isVisible());
        }
        else if (m_humanTurnPhase == HumanTurnPhase::AttackTarget)
        {
            // Confirm the current cursor position as the attack target.
            Vec2i targetPos = m_cursor.getPosition();

            // Find the enemy unit at that position.
            Unit *target = nullptr;
            for (Unit *u : m_units)
            {
                if (u && !u->isDead() && u->getTeam() != 0 && u->getPosition() == targetPos)
                {
                    target = u;
                    break;
                }
            }

            if (target)
            {
                // Perform the attack (deterministic, for now).
                int damage = std::max(active->getAttack() - target->getDefense(), 1);
                target->takeDamage(damage);
                active->setHasActed(true);

                LOG_INFO("Battle", "%s attacks %s for %d damage!",
                         active->getName().c_str(), target->getName().c_str(), damage);
            }

            // After attacking, end the turn (only Wait remains).
            m_humanTurnPhase = HumanTurnPhase::TurnEnded;
            advanceToNextUnit();
            m_turnState = TurnState::Idle;
        }
    }
    else if (input.isKeyPressed(KeyCode::Back, false))
    {
        // Cancel during targeting – go back to the action menu.
        if (m_humanTurnPhase == HumanTurnPhase::MoveTarget ||
            m_humanTurnPhase == HumanTurnPhase::AttackTarget)
        {
            m_humanTurnPhase = HumanTurnPhase::ActionMenu;
            // Re‑show the initial menu. If the unit already moved, we should show only the remaining options.
            // For simplicity we always show all three for now; you can refine later.
            m_actionMenu.show({"Move", "Attack", "Wait"}, {false, true, true});
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Update
// ─────────────────────────────────────────────────────────────────────────────

void BattleState::update(float dt)
{
    // Snapshot old state for precise camera rendering interpolation frames
    m_previousCamera = m_camera;

    if (m_transition.isActive())
    {
        m_transition.update(dt);
        return;
    }

#ifdef _DEBUG
    switch (m_autoPlayPhase)
    {
    case AutoPlayPhase::ShowMenu:
        m_autoPlayTimer -= dt;
        if (m_autoPlayTimer <= 0.0f)
        {
            m_actionMenu.show({"Move", "Attack", "Wait"});
            m_actionMenu.setSelectedIndex(m_autoPlayActionIndex);
            m_autoPlayPhase = AutoPlayPhase::MenuOpen;
            m_autoPlayTimer = 0.5f;
        }
        break;

    case AutoPlayPhase::MenuOpen:
        m_autoPlayTimer -= dt;
        if (m_autoPlayTimer <= 0.0f)
        {
            m_actionMenu.simulateAccept();
            m_autoPlayPhase = AutoPlayPhase::ExecuteAction;
        }
        break;

    case AutoPlayPhase::ExecuteAction:
        m_autoPlayPhase = AutoPlayPhase::Idle;
        break;

    case AutoPlayPhase::Idle:
    default:
        break;
    }

    if (m_autoPlayPhase != AutoPlayPhase::Idle)
        return; // skip normal update while autoplay runs
#endif

    // ── Human control phases ────────────────────────────────────────────
    if (m_playerControlMode == PlayerControlMode::Human &&
        m_turnState == TurnState::ProcessingTurn)
    {
        if (m_humanTurnPhase == HumanTurnPhase::MoveTarget)
        {
            // The player is choosing a destination tile.
            // The cursor is already being updated by the normal cursor call below.
            // We just need to show reachable tiles (optional: draw debug overlay).
            // No additional per-frame logic needed here; Accept is handled in handleActiveTurn.
        }
        else if (m_humanTurnPhase == HumanTurnPhase::AttackTarget)
        {
            // The player is selecting an enemy to attack.
            // Again, cursor movement is handled normally.
        }
    }

    if (m_actionMenu.isVisible())
    {
        m_actionMenu.update(dt);
        // Do not process turn logic while menu is visible
        return;
    }

    // ── CURSOR UPDATE ────────────────────────────────────────────────────────
    // This internally reads the arrow keys/WASD, handles the Fire Emblem-style
    // hold-to-repeat delays, and clamps itself within the map bounds!
    m_cursor.update(m_battleMap.cols(), m_battleMap.rows(), dt);
    // ─────────────────────────────────────────────────────────────────────────

    // Turn state machine simulation loops
    switch (m_turnState)
    {
    case TurnState::Idle:
        if (!m_units.empty())
        {
            m_turnState = TurnState::ProcessingTurn;
            processCurrentTurn();
        }
        break;

    case TurnState::WaitingForAnimation:
        m_turnTimer -= dt;
        if (m_turnTimer <= 0.0f)
        {
            advanceToNextUnit();
            m_turnState = TurnState::Idle;
        }
        break;

    case TurnState::ProcessingTurn:
        break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Turn logic
// ─────────────────────────────────────────────────────────────────────────────

void BattleState::startBattleEnd(bool playerWon)
{
    if (m_transition.isActive())
        return;

    m_playerWon = playerWon;

    m_transition.start({.transition = ScreenTransitions::FadeOut,
                        .duration = 0.5f,
                        .easing = easeInOut,
                        .onComplete = [this]()
                        {
                            m_sm.replace(
                                std::make_unique<ResultState>(m_sm, m_renderer, m_playerWon));
                        }});
}

bool BattleState::checkVictory() const
{
    int enemyCount = 0;
    for (const Unit *u : m_units)
        if (u && !u->isDead() && u->getTeam() != 0)
            enemyCount++;

    LOG_INFO("Battle", "checkVictory: %d enemies alive", enemyCount);
    return enemyCount == 0;
}

bool BattleState::checkDefeat() const
{
    int playerCount = 0;
    for (const Unit *u : m_units)
        if (u && !u->isDead() && u->getTeam() == 0)
            playerCount++;

    LOG_INFO("Battle", "checkDefeat: %d players alive", playerCount);
    return playerCount == 0;
}

void BattleState::processCurrentTurn()
{
    Unit *active = m_turnQueue.getCurrentUnit();

    if (!active || active->isDead())
    {
        const float timeCost = TurnQueue::BASE_ACTION_COST /
                               (active ? static_cast<float>(active->getSpeed()) : 100.0f);
        m_turnQueue.advance(timeCost);
        m_turnState = TurnState::Idle;

        if (checkDefeat() || checkVictory())
            return;
        return;
    }

    active->resetTurn();
    LOG_INFO("Battle", "Active unit: %s, Team: %d, Position: (%d,%d)",
             active->getName().c_str(),
             active->getTeam(),
             active->getPosition().x,
             active->getPosition().y);

    if (active->getTeam() != 0) // Enemies
    {
        EnemyAI::takeTurn(*active, m_grid, m_units);
        m_turnTimer = 0.25f;
        m_turnState = TurnState::WaitingForAnimation;

        if (checkDefeat())
        {
            startBattleEnd(false);
            m_turnState = TurnState::Idle;
            return;
        }
        if (checkVictory())
        {
            startBattleEnd(true);
            m_turnState = TurnState::Idle;
            return;
        }
    }
    else // Player team (team 0)
    {
        m_humanTurnPhase = HumanTurnPhase::ActionMenu;
        m_cursor.setPosition(active->getPosition());

        if (m_playerControlMode == PlayerControlMode::AI)
        {
#ifdef _DEBUG
            int actionIndex = EnemyAI::chooseAction(*active, m_grid, m_units);
            m_autoPlayActionIndex = actionIndex;
            m_autoPlayPhase = AutoPlayPhase::ShowMenu;
            m_autoPlayTimer = 0.1f;
            m_turnState = TurnState::ProcessingTurn;
            LOG_INFO("Battle", "AI autoplay: %s will choose action %d", active->getName().c_str(), actionIndex);
#else
            m_actionMenu.show({"Move", "Attack", "Wait"});
            m_turnState = TurnState::ProcessingTurn;
#endif
        }
        else // Human
        {
            // Start human phased turn: show menu with all three options.
            m_actionMenu.show({"Move", "Attack", "Wait"});
            m_turnState = TurnState::ProcessingTurn;
            // Later we'll add state for move-targeting / attack-targeting.
        }
    }
}

void BattleState::advanceToNextUnit()
{
    Unit *active = m_turnQueue.getCurrentUnit();
    if (active)
    {
        float timeCost = 0.0f;
        if (active->hasMoved())
            timeCost += TurnQueue::BASE_MOVE_COST;
        if (active->hasActed())
            timeCost += TurnQueue::BASE_ACTION_COST;
        if (timeCost == 0.0f)
            timeCost = TurnQueue::BASE_WAIT_COST; // waited
        timeCost /= static_cast<float>(active->getSpeed());

        m_turnQueue.advance(timeCost);
    }

    // Turn banner — no round number, just the unit name
    Unit *nextUnit = m_turnQueue.getCurrentUnit();
    if (nextUnit)
        m_unitPanel.setTurnInfo(0, nextUnit); // 0 means “round-less”

    if (checkDefeat())
    {
        startBattleEnd(false);
        return;
    }
    if (checkVictory())
    {
        startBattleEnd(true);
        return;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Render
// ─────────────────────────────────────────────────────────────────────────────

void BattleState::render(float alpha)
{
    if (!m_renderer)
        return;

    // ── Interpolate camera first ──
    Vec2f interpOffset = m_previousCamera.getOffset() +
                         (m_camera.getOffset() - m_previousCamera.getOffset()) * alpha;

    float interpZoom = m_previousCamera.getZoom() +
                       (m_camera.getZoom() - m_previousCamera.getZoom()) * alpha;

    Camera renderCam = m_camera;
    renderCam.setOffset(interpOffset);
    renderCam.setZoom(interpZoom);

    // ── Draw everything using renderCam ──
    drawBackground();

    if (m_tileset && !m_mapData.isEmpty())
        drawScene(renderCam);

    if (m_actionMenu.isVisible())
        m_actionMenu.render(m_renderer, App::getDefaultFont());

    m_unitPanel.render(m_renderer);

    if (m_debugRenderer)
        m_debugRenderer->flush(m_renderer, renderCam);

    m_transition.render(m_renderer, GameConstants::VIEW_W, GameConstants::VIEW_H);
}

// ─────────────────────────────────────────────────────────────────────────────
// onEnter helpers
// ─────────────────────────────────────────────────────────────────────────────

bool BattleState::loadMap()
{
    TiledJsonLoader loader;
    std::string error;

    auto opt = loader.loadFromFile("assets/maps/1v1_test.tmj", error);
    if (!opt)
    {
        LOG_ERROR("Battle", "Map load FAILED: %s", error.c_str());
        return false;
    }

    m_mapData = std::move(*opt);

    LOG_INFO(
        "Battle",
        "Map loaded: %dx%d tiles, %d layers, tileW=%d tileH=%d",
        m_mapData.width,
        m_mapData.height,
        static_cast<int>(m_mapData.layers.size()),
        m_mapData.tileWidth,
        m_mapData.tileHeight);

    return true;
}

bool BattleState::buildGameplayGrid()
{
    if (!m_battleMap.buildFrom(m_mapData))
    {
        LOG_ERROR("Battle", "BattleMap build FAILED - battle cannot proceed");
        return false;
    }

    m_grid = Grid(m_mapData.width, m_mapData.height);
    return true;
}

bool BattleState::loadTileset()
{
    m_tileset = m_renderer->loadTexture("assets/sprites/0.5H_IsoTiles.png");
    if (!m_tileset)
    {
        LOG_ERROR("Battle", "Tileset load FAILED");
        return false;
    }

    if (m_mapData.tileWidth <= 0)
    {
        LOG_ERROR("Battle", "tileWidth is 0 - cannot compute sprite layout");
        delete m_tileset;
        m_tileset = nullptr;
        return false;
    }

    m_renderer->setTextureScaleMode(m_tileset, Renderer::ScaleMode::Nearest);

    m_texW = static_cast<float>(m_tileset->getWidth());
    m_texH = static_cast<float>(m_tileset->getHeight());

    m_tilesPerRow = std::max(1, static_cast<int>(m_texW / m_mapData.tileWidth));
    m_spriteH = SPRITE_H;

    LOG_INFO(
        "Battle",
        "Tileset: %.0fx%.0f px, tilesPerRow=%d, spriteH=%.0f (constant)",
        m_texW,
        m_texH,
        m_tilesPerRow,
        m_spriteH);

    return true;
}

Vec2f BattleState::computeMapOrigin()
{
    if (m_mapData.width <= 0 || m_mapData.height <= 0 ||
        m_mapData.tileWidth <= 0 || m_mapData.tileHeight <= 0)
    {
        return {GameConstants::VIEW_CX, GameConstants::VIEW_CY};
    }

    const Vec2f c00 = tileToIso(Vec2i{0, 0}, m_mapData.tileWidth, m_mapData.tileHeight);
    const Vec2f c10 = tileToIso(Vec2i{m_mapData.width - 1, 0}, m_mapData.tileWidth, m_mapData.tileHeight);
    const Vec2f c01 = tileToIso(Vec2i{0, m_mapData.height - 1}, m_mapData.tileWidth, m_mapData.tileHeight);
    const Vec2f c11 = tileToIso(Vec2i{m_mapData.width - 1, m_mapData.height - 1}, m_mapData.tileWidth, m_mapData.tileHeight);

    const float minIsoX = std::min({c00.x, c10.x, c01.x, c11.x});
    const float maxIsoX = std::max({c00.x, c10.x, c01.x, c11.x});
    const float minIsoY = std::min({c00.y, c10.y, c01.y, c11.y});
    const float maxIsoY = std::max({c00.y, c10.y, c01.y, c11.y});

    const float tileH = static_cast<float>(m_mapData.tileHeight);
    const float gridH = maxIsoY - minIsoY;
    const float centerIsoX = (minIsoX + maxIsoX) * 0.5f;

    float minLayerOffsetY = 0.0f;
    for (const auto &layer : m_mapData.layers)
        minLayerOffsetY = std::min(minLayerOffsetY, layer.offsetY);

    const float s = static_cast<float>(m_scale);

    float originX = GameConstants::VIEW_CX - centerIsoX * s;
    float originY = GameConstants::VIEW_CY +
                    s * (m_spriteH - 2.0f * tileH - minLayerOffsetY - gridH) / 2.0f;

    LOG_INFO(
        "Battle",
        "Map origin: (%.0f, %.0f), gridH=%.0f, minLayerOffsetY=%.0f",
        originX,
        originY,
        gridH,
        minLayerOffsetY);

    return {originX, originY};
}

// ─────────────────────────────────────────────────────────────────────────────
// Render helpers
// ─────────────────────────────────────────────────────────────────────────────

void BattleState::drawUnitAt(int row, int col, float ax, float ay, const IsoMetrics &m) const
{
    if (!m_debugRenderer)
        return;

    const Unit *unit = nullptr;
    for (const Unit *u : m_units)
    {
        if (u && !u->isDead() && u->getPosition().x == col && u->getPosition().y == row)
        {
            unit = u;
            break;
        }
    }

    if (!unit)
        return;

    Color color;
    switch (unit->getTeam())
    {
    case 0:
        color = {64, 128, 255, 220};
        break;
    case 1:
        color = {64, 255, 64, 220};
        break;
    default:
        color = {255, 64, 64, 220};
        break;
    }

    const GameTile &gt = m_battleMap.at(col, row);
    const float elev = static_cast<float>(gt.height) * m.elevStep;

    // Floating effect: 6px above tile
    const float floatOffset = 6.0f * m.s;
    const float cx = ax;
    const float cy = ay - elev - m.halfTH - floatOffset;

    // Circle radius: 60% of tile width
    float radius = m.halfTW * 0.6f;

    m_debugRenderer->addScreenCircle(Vec2f(cx, cy), radius, color, true);
}

void BattleState::drawBackground() const
{
    // Full-screen gradient: dark navy (top) → mid blue (bottom).
    const FColor top = {10 / 255.0f, 18 / 255.0f, 55 / 255.0f, 1.0f};
    const FColor bottom = {25 / 255.0f, 60 / 255.0f, 120 / 255.0f, 1.0f};

    const std::vector<Renderer::Vertex> verts = {
        {{0.0f, 0.0f}, top},
        {{GameConstants::VIEW_W, 0.0f}, top},
        {{GameConstants::VIEW_W, GameConstants::VIEW_H}, bottom},
        {{0.0f, GameConstants::VIEW_H}, bottom}};

    const std::vector<int> indices = {0, 1, 2, 0, 2, 3};
    m_renderer->drawGeometry(verts, indices);
}

// ─────────────────────────────────────────────────────────────────────────────
// drawScene — orchestrates the per-cell painter's-order loop
//
// For correct isometric occlusion: row by row, col by col.
// Within each cell: tile layers bottom-to-top, then overlays (spawn, cursor,
// units, effects).  Taller tiles in later rows naturally occlude earlier ones.
// ─────────────────────────────────────────────────────────────────────────────

void BattleState::drawScene(const Camera &camera) const
{
    // ── Get camera values ──
    Vec2f offset = camera.getOffset();
    float zoom = camera.getZoom();

    // ── Build iso metrics using camera zoom ──
    const IsoMetrics m = makeIsoMetrics(zoom); // we'll modify makeIsoMetrics

    const auto tileLayers = collectTileLayers();
    const auto spawnGrid = buildSpawnGrid();
    const std::size_t gridSize =
        static_cast<std::size_t>(m_mapData.width) *
        static_cast<std::size_t>(m_mapData.height);

    m_renderer->setBlendMode(Renderer::BlendMode::Blend);

    for (int row = 0; row < m_mapData.height; ++row)
    {
        for (int col = 0; col < m_mapData.width; ++col)
        {
            const Vec2f iso = tileToIso(Vec2i{col, row}, m_mapData.tileWidth, m_mapData.tileHeight);
            const float ax = offset.x + iso.x * m.s;
            const float ay = offset.y + iso.y * m.s;

            drawTileLayersAt(row, col, ax, ay, m, tileLayers);
            drawSpawnOverlayAt(row, col, ax, ay, m, spawnGrid, gridSize);
            drawCursorAt(row, col, ax, ay, m);
            drawUnitAt(row, col, ax, ay, m);
            // drawEffectAt(row, col, ax, ay, m);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// drawScene — data preparation
// ─────────────────────────────────────────────────────────────────────────────

IsoMetrics BattleState::makeIsoMetrics(float zoom) const
{
    IsoMetrics m;
    m.s = static_cast<float>(m_scale) * zoom;
    m.tw = static_cast<float>(m_mapData.tileWidth) * m.s;
    m.th = static_cast<float>(m_mapData.tileHeight) * m.s;
    m.halfTW = m.tw * 0.5f;
    m.halfTH = m.th * 0.5f;
    m.ntw = static_cast<float>(m_mapData.tileWidth);
    m.elevStep = static_cast<float>(m_mapData.tileHeight) * 0.5f * m.s;
    return m;
}

std::vector<TileLayerRef> BattleState::collectTileLayers() const
{
    std::vector<TileLayerRef> out;
    out.reserve(m_mapData.layers.size());

    for (const auto &layer : m_mapData.layers)
    {
        if (layer.type != LayerType::Tile || layer.tiles.empty())
            continue;

        const std::size_t expected =
            static_cast<std::size_t>(layer.width) *
            static_cast<std::size_t>(layer.height);

        if (layer.tiles.size() < expected)
            continue;

        out.push_back({&layer, layer.opacity, layer.offsetX, layer.offsetY});
    }

    return out;
}

std::vector<std::uint8_t> BattleState::buildSpawnGrid() const
{
    const std::size_t gridSize =
        static_cast<std::size_t>(m_mapData.width) *
        static_cast<std::size_t>(m_mapData.height);

    std::vector<std::uint8_t> grid(gridSize, 0u);

    for (const GameTile *t : m_battleMap.playerSpawns)
    {
        const std::size_t idx =
            static_cast<std::size_t>(t->row * m_mapData.width + t->col);
        if (idx < gridSize)
            grid[idx] = 1u;
    }

    for (const GameTile *t : m_battleMap.allySpawns)
    {
        const std::size_t idx =
            static_cast<std::size_t>(t->row * m_mapData.width + t->col);
        if (idx < gridSize)
            grid[idx] = 2u;
    }

    for (const auto &pair : m_battleMap.enemySpawnsByTeam)
    {
        for (const GameTile *t : pair.second)
        {
            const std::size_t idx =
                static_cast<std::size_t>(t->row * m_mapData.width + t->col);
            if (idx < gridSize)
                grid[idx] = 3u;
        }
    }

    return grid;
}

// ─────────────────────────────────────────────────────────────────────────────
// drawScene — per-cell draw calls
// ─────────────────────────────────────────────────────────────────────────────

void BattleState::drawTileLayersAt(int row, int col,
                                   float ax, float ay,
                                   const IsoMetrics &m,
                                   const std::vector<TileLayerRef> &layers) const
{
    for (const auto &ref : layers)
    {
        const auto gid =
            ref.layer->tiles[static_cast<std::size_t>(
                row * ref.layer->width + col)];

        if (gid == 0)
            continue;

        const int localId = static_cast<int>(gid) - 1;
        const int srcCol = localId % m_tilesPerRow;
        const int srcRow = localId / m_tilesPerRow;

        const Recti src =
            {
                static_cast<int>(static_cast<float>(srcCol) * m.ntw),
                static_cast<int>(static_cast<float>(srcRow) * m_spriteH),
                static_cast<int>(m.ntw),
                static_cast<int>(m_spriteH)};

        const Rectf dst =
            {
                ax - m.halfTW + ref.offsetX * m.s,
                ay - m_spriteH * m.s + m.th + ref.offsetY * m.s,
                m.tw,
                m_spriteH * m.s};

        m_renderer->setTextureAlphaMod(m_tileset, ref.opacity);
        m_renderer->drawTexture(m_tileset, src, dst);
    }
}

void BattleState::drawSpawnOverlayAt(int row, int col,
                                     float ax, float ay,
                                     const IsoMetrics &m,
                                     const std::vector<std::uint8_t> &spawnGrid,
                                     std::size_t gridSize) const
{
    const std::size_t idx =
        static_cast<std::size_t>(row * m_mapData.width + col);

    if (idx >= gridSize || spawnGrid[idx] == 0u)
        return;

    const GameTile &gt = m_battleMap.at(col, row);
    const float ox = ax;
    const float oy = ay - static_cast<float>(gt.height) * m.elevStep;
    const FColor &overlayColor =
        spawnGrid[idx] == 1u ? COL_PLAYER_SPAWN : COL_ENEMY_SPAWN;

    const std::vector<Renderer::Vertex> verts = {
        {{ox, oy - m.halfTH}, overlayColor},
        {{ox + m.halfTW, oy}, overlayColor},
        {{ox, oy + m.halfTH}, overlayColor},
        {{ox - m.halfTW, oy}, overlayColor}};

    const std::vector<int> indices = {0, 1, 2, 0, 2, 3};
    m_renderer->drawGeometry(verts, indices);
}

void BattleState::drawCursorAt(int row, int col,
                               float ax, float ay,
                               const IsoMetrics &m) const
{
    const Vec2i pos = m_cursor.getPosition();
    if (pos.x != col || pos.y != row)
        return;

    const GameTile &gt = m_battleMap.at(col, row);
    const float elev = static_cast<float>(gt.height) * m.elevStep;
    const float cx = ax;
    const float cy = ay - elev - m.halfTH - m_cursorHoverOffset;

    drawCursorTriangle(cx, cy, m);
    drawCursorTicks(ax, ay, gt.height, m);
}

void BattleState::drawCursorTriangle(float cx, float cy,
                                     const IsoMetrics &m) const
{
    const float triW = m_cursorTriW * m.s;
    const float triH = m_cursorTriH * m.s;
    const float border = 2.0f;
    const std::vector<int> indices = {0, 1, 2};

    const std::vector<Renderer::Vertex> borderVerts = {
        {{cx, cy + triH + border}, COL_BLACK},
        {{cx - triW - border, cy - border}, COL_BLACK},
        {{cx + triW + border, cy - border}, COL_BLACK}};
    m_renderer->drawGeometry(borderVerts, indices);

    const std::vector<Renderer::Vertex> fillVerts = {
        {{cx, cy + triH}, COL_RED},
        {{cx - triW, cy}, COL_RED},
        {{cx + triW, cy}, COL_RED}};
    m_renderer->drawGeometry(fillVerts, indices);
}

void BattleState::drawCursorTicks(float ax, float ay, int tileHeight,
                                  const IsoMetrics &m) const
{
    const float elev = static_cast<float>(tileHeight) * m.elevStep;
    const float tickLen = 4.0f * m.s;

    const Vec2f corners[4] =
        {
            {ax, ay - m.halfTH - elev},
            {ax + m.halfTW, ay - elev},
            {ax, ay + m.halfTH - elev},
            {ax - m.halfTW, ay - elev}};

    const Vec2f inward[4] =
        {
            {0.0f, 1.0f},  // top    → down
            {-1.0f, 0.0f}, // right  → left
            {0.0f, -1.0f}, // bottom → up
            {1.0f, 0.0f}   // left   → right
        };

    m_renderer->setDrawColor(Color{255, 255, 255, 255});

    for (int i = 0; i < 4; ++i)
    {
        m_renderer->drawLine(
            corners[i],
            Vec2f{corners[i].x + inward[i].x * tickLen, corners[i].y + inward[i].y * tickLen});
    }
}