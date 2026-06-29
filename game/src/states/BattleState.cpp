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
#include "battle/CombatSystem.h"
#include "battle/MovementRange.h"
#include "ai/EnemyAI.h"
#include "ui/Cursor.h"
#include "data/SkillLoader.h"

#include <algorithm>
#include <memory>
#include <string>
#include <direct.h>

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
    m_selectedSkillId.clear();
    m_reachableTiles.clear();
    m_currentAttackRange = 1;

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

    // 6.5. Snap cursor to the first acting unit so it doesn't flash at (0,0)
    if (Unit *first = m_turnQueue.getCurrentUnit(); first)
        m_cursor.setPosition(first->getPosition());

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

        const char *fontPath = R"(assets/fonts/ARCADECLASSIC.TTF)";
        Font *font = m_renderer->loadFont(fontPath, 16);
        if (!font)
            LOG_ERROR("Battle", "Failed to load font: %s", fontPath);
        App::setDefaultFont(font);
    }

    // ── 10.1. Load skill database ─────────────────────────────────────────────
    try
    {
        auto skills = SkillLoader::loadAll("assets/skills");
        for (SkillData &s : skills)
            m_skillDB[s.id] = std::move(s);
        LOG_INFO("Battle", "Loaded %zu skills", m_skillDB.size());
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Battle", "Failed to load skills: %s", e.what());
        // Skills are optional – the game will fall back to basic attacks.
    }

    m_unitPanel.setFont(App::getDefaultFont());

    // ── 10.5. Configure the shared battle menu panel ─────────────────────
    Button::setDefaultFont(App::getDefaultFont());
    m_menuPanel.setVerticalLayout(VerticalLayoutConfig{
        .spacing = 4.0f,
        .alignment = HorizontalAlignment::Left});
    m_menuPanel.setPosition({GameConstants::VIEW_W - 270.0f, GameConstants::VIEW_H - 150.0f});
    m_menuPanel.setBackground(
        {GameConstants::VIEW_W - 270.0f, GameConstants::VIEW_H - 150.0f, 250.0f, 120.0f},
        {12, 12, 20, 230});

    // ── Escape (Undo) callback – used whenever the menu is open ─────────
    m_menuPanel.setOnCancel([this]()
                            {
        Unit *active = m_turnQueue.getCurrentUnit();
        if (!active) return;

        // Only undo the latest move if no action (attack/wait) followed it
        if (active->hasMoved() && !m_actedAfterMove)
        {
            Vec2i currentPos = active->getPosition();
            m_grid.getTile(currentPos).occupied = false;
            active->setPosition(m_moveStartPos);
            m_grid.getTile(m_moveStartPos).occupied = true;
            active->setHasMoved(false);
            m_cursor.setPosition(m_moveStartPos);

            m_humanTurnPhase = HumanTurnPhase::ActionMenu;
            openBattleMenu(true, !active->hasActed(), true, KeyCode::Back);
            LOG_INFO("Battle", "Move undone for %s", active->getName().c_str());
        }
        else
        {
            m_menuPanel.clearButtons();
            m_humanTurnPhase = HumanTurnPhase::FreeCursor;
            m_cursor.setPosition(active->getPosition());
        } });

    // ── 11. Initial turn banner (optional) ──
    if (!m_units.empty())
    {
        Unit *first = m_turnQueue.getCurrentUnit();
        m_unitPanel.setTurnInfo(1, first);
    }

    // ── 12. Start fade‑in transition ──
    m_transition.start({
        .transition = ScreenTransitions::FadeIn,
        .duration = 0.5f,
        .easing = easeInOut,
    });
}

void BattleState::showBattleMenu(bool canMove, bool canAttack, bool canWait)
{
    m_menuPanel.clearButtons();

    auto addBtn = [&](const char *label, std::function<void()> onClick, bool enabled)
    {
        Button btn{Rectf{0.f, 0.f, 250.f, 36.f}, label};
        btn.setOnClick(std::move(onClick));
        btn.setEnabled(enabled);
        m_menuPanel.addButton(std::move(btn));
    };

    // ── 1. Move ──────────────────────────────────────────────────────────
    addBtn("Move", [this]
           {
        Unit *active = m_turnQueue.getCurrentUnit();
        if (active)
            m_reachableTiles = MovementRange::compute(m_grid, active->getPosition(),
                               active->getMoveRange(), active->getTeam(), m_units, 
                               active->getJump());

        m_humanTurnPhase = HumanTurnPhase::MoveTarget;

        
                m_menuPanel.clearButtons(); }, canMove);

    // ── 2. Attack ────────────────────────────────────────────────────────
    addBtn("Attack", [this]
           {
        m_currentAttackRange = 1;
        m_selectedSkillId.clear();
        computeAttackRangeTiles();
        m_humanTurnPhase = HumanTurnPhase::AttackTarget;
        m_menuPanel.clearButtons(); }, canAttack);

    // ── 3. Skills ────────────────────────────────────────────────────────
    Unit *active = m_turnQueue.getCurrentUnit();
    if (active && !active->getSkillIds().empty())
    {
        addBtn("Skills", [this]
               { showSkillMenu(); }, canAttack);
    }

    // ── 4. Defend ────────────────────────────────────────────────────────
    addBtn("Defend", [this]
           {
        Unit *active = m_turnQueue.getCurrentUnit();
        if (active) {
            active->setHasMoved(true);
            active->setHasActed(true);
            m_actedAfterMove = true;
            // TODO: apply defend status buff
        }
        m_menuPanel.clearButtons();
        advanceToNextUnit();
        m_turnState = TurnState::Idle; }, canAttack); // uses the same condition as Attack/Wait (unit hasn't acted yet)

    // ── 5. Wait ──────────────────────────────────────────────────────────
    addBtn("Wait", [this]
           {
        Unit *active = m_turnQueue.getCurrentUnit();
        if (active) {
            active->setHasMoved(true);
            active->setHasActed(true);
            m_actedAfterMove = true;
        }
        m_menuPanel.clearButtons();
        advanceToNextUnit();
        m_turnState = TurnState::Idle; }, canWait);

    // ── Dynamic panel sizing (grows upward from bottom) ──────────────────
    int buttonCount = static_cast<int>(m_menuPanel.getButtons().size());
    if (buttonCount > 0)
    {
        constexpr float ITEM_HEIGHT = 36.0f;
        constexpr float ITEM_SPACING = 4.0f;
        constexpr float PANEL_PADDING = 10.0f; // top & bottom padding inside background
        constexpr float BOTTOM_MARGIN = 10.0f; // space between panel bottom and screen edge

        float totalHeight = buttonCount * ITEM_HEIGHT + (buttonCount - 1) * ITEM_SPACING + PANEL_PADDING * 2;
        float panelY = GameConstants::VIEW_H - totalHeight - BOTTOM_MARGIN;

        m_menuPanel.setPosition({GameConstants::VIEW_W - 270.0f, panelY});
        m_menuPanel.setBackground(
            {GameConstants::VIEW_W - 270.0f, panelY, 250.0f, totalHeight},
            {12, 12, 20, 230});
    }
}

void BattleState::showSkillMenu()
{
    Unit *active = m_turnQueue.getCurrentUnit();
    if (!active)
        return;

    m_menuPanel.clearButtons();

    LOG_INFO("Battle", "showSkillMenu: unit %s has %zu skill IDs",
             active->getName().c_str(), active->getSkillIds().size());

    for (const std::string &skillId : active->getSkillIds())
    {
        LOG_INFO("Battle", "  -> %s", skillId.c_str());

        auto it = m_skillDB.find(skillId);
        if (it == m_skillDB.end())
            continue;

        const SkillData &skill = it->second;
        std::string label = skill.name + " (MP:" + std::to_string(skill.mpCost) + ")";
        bool canUse = (active->getCurrentMp() >= skill.mpCost);

        Button btn{Rectf{0.f, 0.f, 250.f, 36.f}, label};
        btn.setOnClick([this, skillId]
                       {
            auto it = m_skillDB.find(skillId);
            if (it != m_skillDB.end()) {
                m_currentAttackRange = it->second.range;
                m_selectedSkillId = skillId;
                computeAttackRangeTiles();
            }
            m_humanTurnPhase = HumanTurnPhase::AttackTarget;
            m_menuPanel.clearButtons(); });
        btn.setEnabled(canUse);
        m_menuPanel.addButton(std::move(btn));
    }

    Button backBtn{Rectf{0.f, 0.f, 250.f, 36.f}, "Back"};
    backBtn.setOnClick([this]
                       {
        Unit *active = m_turnQueue.getCurrentUnit();
        if (active)
            showBattleMenu(!active->hasMoved(), !active->hasActed(), true);
        else
            m_menuPanel.clearButtons(); });
    m_menuPanel.addButton(std::move(backBtn));
}

void BattleState::showStatusMenu(Unit * /*unit*/)
{
    m_menuPanel.clearButtons();

    Button btn{Rectf{0.f, 0.f, 250.f, 36.f}, "Status"};
    btn.setOnClick([this]
                   {
        // TODO: full stat/inspect screen later
        m_menuPanel.clearButtons();
        m_humanTurnPhase = HumanTurnPhase::FreeCursor; });
    m_menuPanel.addButton(std::move(btn));
}

void BattleState::openBattleMenu(bool canMove, bool canAttack, bool canWait, KeyCode trigger)
{
    showBattleMenu(canMove, canAttack, canWait);
    Input::instance().consumeKey(trigger);
}

void BattleState::openStatusMenu(Unit *unit)
{
    showStatusMenu(unit);
    Input::instance().consumeKey(KeyCode::Accept);
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

void BattleState::handleActiveTurn(const Input &input)
{
    if (!m_menuPanel.getButtons().empty())
        return; // menu is open – ignore tactical input
    if (m_playerControlMode != PlayerControlMode::Human)
        return;

    Unit *active = m_turnQueue.getCurrentUnit();
    if (!active || active->getTeam() != 0)
        return;

    // ── FreeCursor phase ─────────────────────────────────────────────────
    if (m_humanTurnPhase == HumanTurnPhase::FreeCursor)
    {
        if (input.isKeyPressed(KeyCode::Accept, false))
        {
            Vec2i cursorPos = m_cursor.getPosition();

            if (cursorPos == active->getPosition())
            {
                m_humanTurnPhase = HumanTurnPhase::ActionMenu;
                openBattleMenu(!active->hasMoved(), !active->hasActed(), true, KeyCode::Accept);
            }
            else
            {
                Unit *hovered = nullptr;
                for (Unit *u : m_units)
                    if (u && !u->isDead() && u->getPosition() == cursorPos)
                    {
                        hovered = u;
                        break;
                    }

                if (hovered)
                    openStatusMenu(hovered);
                else
                    m_cursor.setPosition(active->getPosition());
            }
        }
    }

    // ── MoveTarget phase ─────────────────────────────────────────────────
    else if (m_humanTurnPhase == HumanTurnPhase::MoveTarget)
    {
        if (input.isKeyPressed(KeyCode::Accept, false))
        {
            Vec2i dest = m_cursor.getPosition();
            // Only allow moving to a reachable tile
            if (m_reachableTiles.find(dest) == m_reachableTiles.end())
                return; // ignore confirm if tile not reachable

            m_moveStartPos = active->getPosition();

            Vec2i start = active->getPosition();
            m_grid.getTile(start).occupied = false;
            active->setPosition(dest);
            m_grid.getTile(dest).occupied = true;
            active->setHasMoved(true);
            m_actedAfterMove = false;

            m_humanTurnPhase = HumanTurnPhase::ActionMenu;
            openBattleMenu(false, !active->hasActed(), true, KeyCode::Accept);
        }
        else if (input.isKeyPressed(KeyCode::Back, false))
        {
            m_humanTurnPhase = HumanTurnPhase::ActionMenu;
            openBattleMenu(!active->hasMoved(), !active->hasActed(), true, KeyCode::Back);
        }
    }

    // ── AttackTarget phase ───────────────────────────────────────────────
    else if (m_humanTurnPhase == HumanTurnPhase::AttackTarget)
    {
        Vec2i cursorPos = m_cursor.getPosition();
        Unit *hoveredEnemy = nullptr;
        for (Unit *u : m_units)
            if (u && !u->isDead() && u->getTeam() != 0 && u->getPosition() == cursorPos)
            {
                hoveredEnemy = u;
                break;
            }

        // Update damage preview: use selected skill if any, else basic attack
        if (hoveredEnemy)
        {
            int dist = manhattanDistance(active->getPosition(), hoveredEnemy->getPosition());
            if (dist <= m_currentAttackRange)
            {
                const SkillData *previewSkill = nullptr;
                if (!m_selectedSkillId.empty())
                {
                    auto it = m_skillDB.find(m_selectedSkillId);
                    if (it != m_skillDB.end())
                        previewSkill = &it->second;
                }
                m_damagePreview.show(*active, *hoveredEnemy, previewSkill);
            }
            else
                m_damagePreview.hide();
        }
        else
            m_damagePreview.hide();

        if (input.isKeyPressed(KeyCode::Accept, false))
        {
            Vec2i targetPos = m_cursor.getPosition();
            Unit *target = nullptr;
            for (Unit *u : m_units)
                if (u && !u->isDead() && u->getTeam() != 0 && u->getPosition() == targetPos)
                {
                    target = u;
                    break;
                }

            // Look up the selected skill (nullptr = basic attack)
            const SkillData *skill = nullptr;
            if (!m_selectedSkillId.empty())
            {
                auto it = m_skillDB.find(m_selectedSkillId);
                if (it != m_skillDB.end())
                    skill = &it->second;
            }

            // Decide if the tile is a legal attack target
            bool canAttack = (target != nullptr); // direct target

            // AOE skill on empty tile – legal if at least one enemy is within the splash area
            if (!canAttack && skill && skill->area > 0)
            {
                for (Unit *u : m_units)
                {
                    if (u && !u->isDead() && u->getTeam() != 0 &&
                        manhattanDistance(u->getPosition(), targetPos) <= skill->area)
                    {
                        canAttack = true;
                        break;
                    }
                }
            }

            if (!canAttack)
                return; // no valid target – ignore confirm

            // Check attack range
            int dist = manhattanDistance(active->getPosition(), targetPos);
            if (dist > m_currentAttackRange)
                return; // out of range – ignore confirm

            // Determine which skill to use (nullptr = basic attack)
            const SkillData *skillToUse = nullptr;
            if (!m_selectedSkillId.empty())
            {
                auto it = m_skillDB.find(m_selectedSkillId);
                if (it != m_skillDB.end())
                    skillToUse = &it->second;
            }

            // ── Gather all targets affected by this attack ──────────────
            std::vector<Unit *> targets;

            if (skillToUse && skillToUse->area > 0)
            {
                // AOE skill – collect every enemy within the area
                for (Unit *u : m_units)
                {
                    if (!u || u->isDead() || u->getTeam() == 0)
                        continue;
                    if (manhattanDistance(u->getPosition(), targetPos) <= skillToUse->area)
                        targets.push_back(u);
                }
            }
            else
            {
                // Single‑target (basic attack or single‑target skill)
                if (target) // guaranteed non‑null because canAttack was true
                    targets.push_back(target);
            }

            // ── Apply the skill to every valid target ──────────────────
            for (Unit *u : targets)
            {
                HitContext ctx;
                ctx.attacker = active;
                ctx.target = u;
                if (skillToUse)
                {
                    ctx.basePower = skillToUse->basePower;
                    ctx.isMagical = skillToUse->isMagical;
                    ctx.element = skillToUse->element;
                    ctx.skillAccuracy = skillToUse->skillAccuracy;
                }
                else
                {
                    ctx.basePower = 0;
                    ctx.isMagical = false;
                    ctx.element = Element::Neutral;
                    ctx.skillAccuracy = 95;
                }
                ctx.side = AttackSide::Side;
                ctx.tileEvasionBonus = 0;

                CombatResult result = CombatSystem::resolve(ctx);

                // ── Animation hook (per target) ────────────────────────
                // TODO: Play attack animation (active -> u) here.
                //       Once animation finishes, apply the hit/miss below.

                if (result.hit)
                {
                    // TODO: Play hit animation on 'u'
                    u->takeDamage(result.damage);
                    LOG_INFO("Battle", "%s uses %s on %s for %d damage!",
                             active->getName().c_str(),
                             skillToUse ? skillToUse->name.c_str() : "Attack",
                             u->getName().c_str(), result.damage);
                }
                else
                {
                    // TODO: Play miss/dodge animation on 'u'
                    LOG_INFO("Battle", "%s misses %s",
                             active->getName().c_str(), u->getName().c_str());
                }
            }

            // Deduct MP cost (once, after all targets are processed)
            if (skillToUse)
                active->setCurrentMp(active->getCurrentMp() - skillToUse->mpCost);

            active->setHasActed(true);
            m_actedAfterMove = true;
            m_selectedSkillId.clear();

            m_damagePreview.hide();
            m_humanTurnPhase = HumanTurnPhase::ActionMenu;
            openBattleMenu(true, false, true, KeyCode::Accept);
        }
        else if (input.isKeyPressed(KeyCode::Back, false))
        {
            m_damagePreview.hide();
            m_selectedSkillId.clear(); // cancel skill selection
            m_humanTurnPhase = HumanTurnPhase::ActionMenu;
            openBattleMenu(!active->hasMoved(), true, true, KeyCode::Back);
        }
    }
}

void BattleState::handleInput()
{
    const Input &input = Input::instance();

    // 1. Pause
    if (input.isKeyPressed(KeyCode::Pause))
    {
        m_sm.push(std::make_unique<PauseState>(m_sm, m_renderer));
        return;
    }

    if (m_transition.isActive())
        return;

    // 2. Toggle AI / Human control
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
            LOG_INFO("Battle", "Switched to AI control (immediate takeover)");
            Unit *active = m_turnQueue.getCurrentUnit();
            if (active && active->getTeam() == 0 && !active->isDead())
            {
                m_menuPanel.clearButtons();
                m_humanTurnPhase = HumanTurnPhase::ActionMenu;
                EnemyAI::takeTurn(*active, m_grid, m_units);
                m_cursor.setPosition(active->getPosition());
                m_turnTimer = 0.15f;
                m_turnState = TurnState::WaitingForAnimation;
            }
        }
    }

    // 3. Debug end battle
    if (input.isKeyPressed(KeyCode::Advance))
    {
        startBattleEnd(false);
        return;
    }

    // 4. Tactical input
    handleActiveTurn(input);
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
    if (m_autoPlayPhase == AutoPlayPhase::ShowMenu)
    {
        Unit *active = m_turnQueue.getCurrentUnit();
        if (active)
        {
            int action = m_autoPlayActionIndex; // 0=Move, 1=Attack, 2=Wait
            if (action == 0 || action == 1)     // Move or Attack → let the AI do its thing
            {
                EnemyAI::takeTurn(*active, m_grid, m_units);
                m_cursor.setPosition(active->getPosition());
                m_turnTimer = 0.15f;
                m_turnState = TurnState::WaitingForAnimation;
            }
            else // Wait
            {
                active->setHasMoved(true);
                active->setHasActed(true);
                m_actedAfterMove = true;
                advanceToNextUnit();
                m_turnState = TurnState::Idle;
            }
        }
        m_autoPlayPhase = AutoPlayPhase::Idle;
    }
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

    if (!m_menuPanel.getButtons().empty())
    {
        m_menuPanel.update();
        return;
    }

    // ── CURSOR UPDATE ────────────────────────────────────────────────────────
    // This internally reads the arrow keys/WASD, handles the Fire Emblem-style
    // hold-to-repeat delays, and clamps itself within the map bounds!
    m_cursor.update(m_battleMap.cols(), m_battleMap.rows(), dt);

    // Track unit under cursor
    m_hoveredUnit = nullptr;
    Vec2i cursorPos = m_cursor.getPosition();
    for (Unit *u : m_units)
    {
        if (u && !u->isDead() && u->getPosition() == cursorPos)
        {
            m_hoveredUnit = u;
            break;
        }
    }

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
        m_humanTurnPhase = HumanTurnPhase::FreeCursor;
        m_actedAfterMove = false;
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
            m_turnState = TurnState::ProcessingTurn;
#endif
        }
        else // Human
        {
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

    // ── Action menu (always drawn if visible) ──
    if (!m_menuPanel.getButtons().empty())
        m_menuPanel.render(m_renderer);

    // ── Hover-based unit panel ──
    // Show a coloured panel for the unit under the cursor (or during attack, show both)
    if (m_playerControlMode == PlayerControlMode::Human &&
        m_humanTurnPhase == HumanTurnPhase::AttackTarget)
    {
        // ── Attack targeting: show player (left) and target (right) ──
        Unit *active = m_turnQueue.getCurrentUnit();
        if (active && !active->isDead())
        {
            // Player panel (blue, left side)
            m_unitPanel.show(active);
            m_unitPanel.setTeamColor(Color{64, 128, 255, 255}); // blue
            m_unitPanel.setPosition({16.0f, GameConstants::VIEW_H - 116.0f - 16.0f});
            m_unitPanel.render(m_renderer);

            // Target panel (red, right side) if hovering an enemy
            if (m_hoveredUnit && m_hoveredUnit->getTeam() != 0)
            {
                m_targetPanel.show(m_hoveredUnit);
                m_targetPanel.setFont(App::getDefaultFont());
                m_targetPanel.setTeamColor(Color{255, 64, 64, 255}); // red
                m_targetPanel.setPosition({GameConstants::VIEW_W - 316.0f, GameConstants::VIEW_H - 116.0f - 16.0f});
                m_targetPanel.render(m_renderer);
            }
            else
            {
                m_targetPanel.hide();
            }
        }
    }
    else
    {
        // ── Normal hover: show single panel for unit under cursor ──
        if (m_hoveredUnit)
        {
            m_unitPanel.show(m_hoveredUnit);
            m_unitPanel.setTeamColor(m_hoveredUnit->getTeam() == 0 ? Color{64, 128, 255, 255}
                                                                   : Color{255, 64, 64, 255});
            m_unitPanel.setPosition({16.0f, GameConstants::VIEW_H - 116.0f - 16.0f});
            m_unitPanel.render(m_renderer);
        }
        else
        {
            // Optionally show the turn banner when no unit is hovered
            // If you want the turn banner always visible, leave it; otherwise hide the panel.
            // For now, just hide the stat panel.
            m_unitPanel.hide();
        }
    }

    // ── Damage preview (always drawn if visible) ──
    m_damagePreview.render(m_renderer, App::getDefaultFont());

    // ── Debug overlay ──
    if (m_debugRenderer)
        m_debugRenderer->flush(m_renderer, renderCam);

    // ── Screen transition (fade) ──
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

            // ── Range overlays ─────────────────────────────────────────
            if (m_humanTurnPhase == HumanTurnPhase::MoveTarget &&
                m_reachableTiles.count({col, row}))
            {
                drawRangeOverlayAt(row, col, ax, ay, m, Color{0, 100, 255, 120}); // blue for move
            }
            else if (m_humanTurnPhase == HumanTurnPhase::AttackTarget &&
                     m_attackRangeTiles.count({col, row}))
            {
                drawRangeOverlayAt(row, col, ax, ay, m, Color{255, 50, 50, 120}); // red for attack
            }

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

void BattleState::computeAttackRangeTiles()
{
    m_attackRangeTiles.clear();
    Unit *active = m_turnQueue.getCurrentUnit();
    if (!active)
        return;

    Vec2i pos = active->getPosition();
    int range = m_currentAttackRange;

    for (int r = 0; r < m_mapData.height; ++r)
        for (int c = 0; c < m_mapData.width; ++c)
            if (manhattanDistance(pos, Vec2i{c, r}) <= range)
                m_attackRangeTiles.insert({c, r});
}

void BattleState::drawRangeOverlayAt(int row, int col, float ax, float ay,
                                     const IsoMetrics &m, Color color) const
{
    const GameTile &gt = m_battleMap.at(col, row);
    const float ox = ax;
    const float oy = ay - static_cast<float>(gt.height) * m.elevStep;

    const std::vector<Renderer::Vertex> verts = {
        Renderer::Vertex{{ox, oy - m.halfTH}, color},
        Renderer::Vertex{{ox + m.halfTW, oy}, color},
        Renderer::Vertex{{ox, oy + m.halfTH}, color},
        Renderer::Vertex{{ox - m.halfTW, oy}, color}};

    const std::vector<int> indices = {0, 1, 2, 0, 2, 3};
    m_renderer->drawGeometry(verts, indices);
}