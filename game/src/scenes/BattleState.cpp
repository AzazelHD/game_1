#include "config/GameConstants.h"
#include "engine/math/Vec2.h"
#include "engine/math/MathUtils.h"
#include "engine/data/TiledJsonLoader.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/core/App.h"
#include "engine/core/Log.h"
#include "engine/core/Window.h"
#include "engine/renderer/Aligment.h"
#include "engine/renderer/Camera.h"
#include "engine/renderer/Texture.h"
#include "engine/renderer/Renderer.h"
#include "engine/renderer/DebugRenderer.h"
#include "engine/renderer/FontManager.h"
#include "engine/effects/ScreenTransition.h"
#include "config/BattleCatalog.h"
#include "data/SettingsManager.h"
#include "scenes/BattleState.h"
#include "scenes/BattleLoader.h"
#include "scenes/MainMenuState.h"
#include "battle/unit/Unit.h"
#include "battle/unit/UnitFactory.h"
#include "battle/unit/UnitProgression.h"
#include "battle/controllers/AttackResolutionController.h"
#include "battle/controllers/DeploymentPhaseController.h"
#include "battle/map/MovementRange.h"
#include "battle/combat/CombatSystem.h"
#include "renderer/BattleRendererContext.h"
#include "ai/EnemyAI.h"
#include "ui/Cursor.h"
#include "ui/UITheme.h"
#include "ui/UIScale.h"
#include "ui/UnitPortrait.h"
#include "ui/windows/ButtonMenuWindow.h"
#include "ui/windows/ConfirmWindow.h"
#include "ui/windows/DialogWindow.h"
#include "data/SkillLoader.h"
#include "data/UnitLoader.h"

#include <algorithm>
#include <memory>
#include <string>

// Sprite sheet dimensions for 0.5H_IsoTiles.png.
//   Sprite size  : 48 × 36 px
//   Map tile size: 48 × 24 px
// Hardcoded because back-calculating from max-GID is fragile — the
// sheet format is fixed for this asset.

namespace
{
    constexpr float SPRITE_H = 36.0f;

    std::string loadUnitDisplayName(const std::string &templatePath)
    {
        try
        {
            const UnitData data = UnitLoader::load(templatePath);
            return data.name;
        }
        catch (...)
        {
            return std::string();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

BattleState::BattleState(StateMachine<Scene> &sm,
                         Renderer *renderer,
                         BattleRequest request,
                         std::function<void(bool)> onBattleFinished)
    : m_sm(sm),
      m_renderer(renderer),
      m_request(std::move(request)),
      m_onBattleFinished(std::move(onBattleFinished))
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
    m_session.init({});
    m_floatingText.clear();
    m_pendingRewardXp = 0;
    m_flowPhase = BattleFlowPhase::Deployment;
    m_showDefeatOverlay = false;
    m_showVictoryOverlay = false;
    m_pendingResolution = PendingResolution::None;
    m_pendingActor = nullptr;
    m_pendingTarget = nullptr;
    m_pendingActionLabel.clear();
    m_topBattleText.clear();

    m_battleDefinition = &getBattleDefinition(m_request.mapPath);
    m_bgTop = m_battleDefinition->backgroundTop;
    m_bgBottom = m_battleDefinition->backgroundBottom;

    // ── 1. Acquire engine dependencies ──
    m_renderer = App::getRenderer();
    if (!m_renderer)
        return;

    m_battleRenderer = std::make_unique<BattleRenderer>(m_renderer);

    Window *window = App::getWindow();
    if (!window)
        return;

    // ── 2. Configure window and renderer ──
    const bool borderless = SettingsManager::instance().data().windowMode == WindowMode::Borderless;
    window->setResizable(!borderless);
    if (!borderless)
        window->setAspectRatio(16.0f / 9.0f, 16.0f / 9.0f);

    m_renderer->setLogicalPresentation(
        static_cast<int>(GameConstants::VIEW_W),
        static_cast<int>(GameConstants::VIEW_H),
        borderless ? Renderer::PresentationMode::Stretch : Renderer::PresentationMode::Letterbox);

    // ── 3-6. Load map, build grid, load tileset, compute origin ──
    BattleLoadResult loaded = BattleLoader{m_renderer}.load(m_request.mapPath.c_str(), m_scale, SPRITE_H);
    if (!loaded.ok)
        return;

    m_mapData = std::move(loaded.mapData);
    m_battleMap = std::move(loaded.battleMap);
    m_grid = std::move(loaded.grid);
    m_tileset = loaded.tileset;
    m_texW = loaded.texW;
    m_texH = loaded.texH;
    m_tilesPerRow = loaded.tilesPerRow;
    m_spriteH = loaded.spriteH;

    Vec2f origin = loaded.mapOrigin;

    m_camera.setTileSize(m_mapData.tileWidth, m_mapData.tileHeight);
    m_camera.setMapSize(m_mapData.width, m_mapData.height);
    m_camera.setViewportSize(Vec2f{GameConstants::VIEW_W, GameConstants::VIEW_H});
    m_camera.setRenderScale(static_cast<float>(m_scale));
    m_camera.setOffset(origin);
    m_camera.setZoom(m_battleDefinition->defaultZoom);
    m_camera.setMapBoundsMargin(0.0f);

    // origin is a screen-space placement value, not a valid iso-space camera
    // offset — clamp immediately so the very first rendered frame already
    // sits inside valid bounds, instead of snapping there on the first
    // trackTarget()/clampToBounds() call later (visible as a hard "jump").
    m_camera.clampToBounds();
    m_previousCamera = m_camera;

    // ── 7. Set up debug renderer ──
    m_debugRenderer = &DebugRenderer::instance();
    m_debugRenderer->setEnabled(true);

    // ── 8. Load skill database ─────────────────────────────────────────────
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

    m_uiManager.clear();
    m_unitPanelWindow = m_uiManager.push<UnitPanelWindow>(WindowId::BattleUnitPanel);
    m_unitPanelWindow->setFont(FontManager::instance().get(FontRole::Body));

    auto *deploymentWindow = m_uiManager.push<DeploymentWindow>(WindowId::BattleDeployment);
    deploymentWindow->setFont(FontManager::instance().get(FontRole::Body));
    m_deploymentPhase.setDeploymentWindow(deploymentWindow);

    m_battleMenu.closeAllMenus();

    m_deploymentPhase.initializeDeploymentPhase();

    m_eventSystem.initialize(*m_battleDefinition,
                             BattleEventSystem::Callbacks{
                                 .showDialogue = [this](const std::string &text)
                                 { showDialogueFromEvent(text); },
                                 .spawnUnitByTemplate = [this](const std::string &templatePath)
                                 { spawnEnemyFromEvent(templatePath); },
                                 .giveRewardXp = [this](int xp)
                                 { m_pendingRewardXp += xp; },
                                 .playAnimation = [this](const std::string &name)
                                 { m_combatAnimations.enqueue(name); },
                                 .startCutscene = [](const std::string & /*id*/) {},
                                 .endBattle = [this](bool win)
                                 { startBattleEnd(win); },
                             });

    // ── 9. Start fade‑in transition ──
    m_transition.start({
        .transition = ScreenTransitions::FadeIn,
        .duration = 0.5f,
        .easing = easeInOut,
    });
}

void BattleState::openBattleMenu(bool canMove, bool canAttack, bool canWait, KeyCode trigger)
{
    m_battleMenu.showBattleMenu(canMove, canAttack, canWait);
    Input::instance().consumeKey(trigger);
}

BattleState::HumanTurnContext BattleState::makeHumanTurnContext()
{
    return HumanTurnContext{
        .controlMode = m_playerControlMode,
        .phase = m_humanTurnPhase,
        .cursor = m_cursor,
        .session = m_session,
        .reachableTiles = m_reachableTiles,
        .reachableCosts = m_reachableCosts,
        .moveStartPos = m_moveStartPos,
        .moveStartPointsLeft = m_moveStartPointsLeft,
        .grid = m_grid,
        .canUndoLastMove = m_canUndoLastMove,
        .eventSystem = m_eventSystem,
        .currentAttackRange = m_currentAttackRange,
        .selectedSkillId = m_selectedSkillId,
        .skillDB = m_skillDB,
        .damagePreview = m_damagePreview,
        .topBattleText = m_topBattleText,
        .pendingAttack = m_attackResolution.pendingAttack(),
        .pendingSkillId = m_attackResolution.pendingSkillId(),
        .uiManager = m_uiManager,
    };
}

BattleState::AttackResolutionContext BattleState::makeAttackResolutionContext()
{
    return AttackResolutionContext{
        .session = m_session,
        .eventSystem = m_eventSystem,
        .damagePreview = m_damagePreview,
        .floatingText = m_floatingText,
        .cursor = m_cursor,
        .skillDB = m_skillDB,
        .selectedSkillId = m_selectedSkillId,
        .topBattleText = m_topBattleText,
        .hoveredUnit = m_hoveredUnit,
        .humanTurnPhase = m_humanTurnPhase,
        .turnState = m_turnState,
        .pendingResolution = m_pendingResolution,
        .turnTimer = m_turnTimer,
        .canUndoLastMove = m_canUndoLastMove,
        .pendingActor = m_pendingActor,
        .pendingTarget = m_pendingTarget,
        .pendingActionLabel = m_pendingActionLabel,
    };
}

BattleState::DeploymentContext BattleState::makeDeploymentContext()
{
    return DeploymentContext{
        .session = m_session,
        .eventSystem = m_eventSystem,
        .uiManager = m_uiManager,
        .cursor = m_cursor,
        .grid = m_grid,
        .battleMap = m_battleMap,
        .battleDefinition = m_battleDefinition,
        .hoveredUnit = m_hoveredUnit,
        .inspectTargetUnit = m_inspectTargetUnit,
        .unitPanelWindow = m_unitPanelWindow,
        .flowPhase = m_flowPhase,
        .turnState = m_turnState,
        .pendingResolution = m_pendingResolution,
        .pendingActor = m_pendingActor,
        .pendingTarget = m_pendingTarget,
        .pendingActionLabel = m_pendingActionLabel,
        .topBattleText = m_topBattleText,
        .camera = m_camera,
        .mapData = m_mapData,
    };
}

BattleState::MenuContext BattleState::makeMenuContext()
{
    return MenuContext{
        .session = m_session,
        .grid = m_grid,
        .battleMap = m_battleMap,
        .cursor = m_cursor,
        .reachableTiles = m_reachableTiles,
        .reachableCosts = m_reachableCosts,
        .currentAttackRange = m_currentAttackRange,
        .uiManager = m_uiManager,
        .humanTurnPhase = m_humanTurnPhase,
        .flowPhase = m_flowPhase,
        .turnState = m_turnState,
        .skillDB = m_skillDB,
        .selectedSkillId = m_selectedSkillId,
        .moveStartPos = m_moveStartPos,
        .moveStartPointsLeft = m_moveStartPointsLeft,
        .canUndoLastMove = m_canUndoLastMove,
        .inspectTargetUnit = m_inspectTargetUnit,
        .attackResolution = m_attackResolution,
        .deploymentPhase = m_deploymentPhase,
        .sm = m_sm,
        .renderer = m_renderer,
    };
}

bool BattleState::canActiveUnitMove() const
{
    const Unit *active = m_session.getCurrentUnit();
    if (!active)
        return false;
    return active->getMoveRangeLeft() > 0 && (!active->hasMoved() || active->hasActed());
}

void BattleState::processUIEvents(Unit *active)
{
    auto events = m_uiManager.drainEvents();
    for (const UIEvent &event : events)
    {
        if (event.windowId == WindowId::BattleDialog && event.type == UIEventType::DialogFinished)
        {
            m_uiManager.popById(WindowId::BattleDialog);
            continue;
        }

        if (m_deploymentPhase.handleUIEvent(event))
            continue;

        if (m_battleMenu.handleUIEvent(event, active))
            continue;

        if (event.windowId == WindowId::BattleActionConfirm && event.type == UIEventType::NavigatePrevious)
        {
            m_attackResolution.cycleFocus(-1);
            m_attackResolution.updatePreview(active);
            if (Unit *focus = m_attackResolution.pendingAttack().focusedTarget())
                m_cursor.setPosition(focus->getPosition());
            continue;
        }

        if (event.windowId == WindowId::BattleActionConfirm && event.type == UIEventType::NavigateNext)
        {
            m_attackResolution.cycleFocus(1);
            m_attackResolution.updatePreview(active);
            if (Unit *focus = m_attackResolution.pendingAttack().focusedTarget())
                m_cursor.setPosition(focus->getPosition());
            continue;
        }

        if (event.windowId == WindowId::BattleActionConfirm && event.type == UIEventType::ConfirmResult)
        {
            m_uiManager.popById(WindowId::BattleActionConfirm);

            if (!event.confirmed)
            {
                m_attackResolution.cancel();
                continue;
            }

            const SkillData *skill = nullptr;
            if (!m_attackResolution.pendingSkillId().empty())
            {
                auto it = m_skillDB.find(m_attackResolution.pendingSkillId());
                if (it != m_skillDB.end())
                    skill = &it->second;
            }

            // TODO: display skill animation
            m_pendingActionLabel = skill ? skill->name : "Attack";
            m_topBattleText = m_pendingActionLabel;
            m_pendingActor = active;
            m_pendingTarget = m_attackResolution.pendingAttack().focusedTarget();
            m_pendingResolution = PendingResolution::PlayerConfirmedAttack;
            m_turnTimer = 0.5f;
            m_turnState = TurnState::WaitingForAnimation;
            continue;
        }
    }
}

void BattleState::preparePendingAttack(Unit *active, Vec2i targetPos, Unit *directTarget, const SkillData *skill)
{
    m_attackResolution.prepare(active, targetPos, directTarget, skill);
}

void BattleState::showDialogueFromEvent(const std::string &text)
{
    m_uiManager.popById(WindowId::BattleDialog);
    auto *dialog = m_uiManager.push<DialogWindow>(WindowId::BattleDialog);
    dialog->setFont(FontManager::instance().get(FontRole::Body));
    dialog->start({DialogWindow::Line{.speaker = "System", .text = text}});
}

void BattleState::spawnEnemyFromEvent(const std::string &templatePath)
{
    Vec2i spawnPos{0, 0};
    bool found = false;
    for (const auto &pair : m_battleMap.enemySpawnsByTeam)
    {
        for (GameTile *tile : pair.second)
        {
            if (!tile)
                continue;
            Vec2i pos{tile->col, tile->row};
            if (!m_grid.isValid(pos) || m_grid.getTile(pos).occupied)
                continue;

            spawnPos = pos;
            found = true;
            break;
        }
        if (found)
            break;
    }

    if (!found)
        return;

    m_grid.getTile(spawnPos).occupied = true;
    m_session.spawnUnit(UnitSpawn{
        .unitFilePath = templatePath,
        .startPos = spawnPos,
        .team = 2,
    });
}

void BattleState::onExit()
{
    m_deploymentPhase.resetOnExit();

    m_uiManager.clear();
    m_unitPanelWindow = nullptr;
    m_inspectTargetUnit = nullptr;
    m_combatAnimations.clear();

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

    if (m_showDefeatOverlay || m_showVictoryOverlay)
    {
        if (input.isKeyPressed(KeyCode::Accept, false) ||
            input.isKeyPressed(KeyCode::Back, false) ||
            input.isKeyPressed(KeyCode::Advance, false))
        {
            const bool won = m_showVictoryOverlay;
            if (m_onBattleFinished)
                m_onBattleFinished(won);
            else
                m_sm.replace(std::make_unique<MainMenuState>(m_sm, m_renderer, true));
        }
        return;
    }

    if (m_transition.isActive())
        return;

    if (input.isKeyPressed(KeyCode::Back, false))
    {
        // Anything open on the UI stack already handles its own Back key and
        // emits its own cancellation event (ActionCanceled / ConfirmResult
        // with confirmed=false / etc.) — processUIEvents() already knows how
        // to react to every one of these. Forwarding generically means Back
        // always does exactly what the topmost window itself defines Back to
        // mean (e.g. DialogWindow advances its text instead of being torn
        // down mid-line), instead of BattleState re-deriving "what's open"
        // by checking window IDs one by one.
        //
        // Checked BEFORE the "grabbed unit" fallback below: if the Inspect
        // window (or any other modal) is open while a unit is also grabbed,
        // the first Back should close that window, not release the grab.
        if (m_uiManager.hasBlockingWindow())
        {
            // getCurrentUnit() asserts on an empty timeline, which is exactly
            // the case during Deployment (the turn queue doesn't exist yet).
            // Only Combat has a "currently active unit" concept.
            Unit *active = nullptr;
            if (m_flowPhase == BattleFlowPhase::Combat)
                active = m_session.getCurrentUnit();

            // The action menu specifically should not be cancellable via
            // Back once the active unit has committed to its action and
            // there's no move left to undo — there's nothing meaningful to
            // go ActionId::Back to at that point.
            if (m_uiManager.hasWindow(WindowId::BattleActionMenu) &&
                active && active->hasActed() && !m_canUndoLastMove)
                return;

            m_uiManager.handleInput(input);
            processUIEvents(active);
            return;
        }

        // Not a UI window — deployment's own "unit currently grabbed" state.
        if (m_flowPhase == BattleFlowPhase::Deployment && m_deploymentPhase.hasGrabbedUnit())
        {
            m_deploymentPhase.releaseGrabbedUnit();
            return;
        }

        if (m_flowPhase == BattleFlowPhase::Deployment)
        {
            m_battleMenu.showSystemMenu();
            return;
        }

        // Mid-action (targeting/moving): ESC cancels the action, not open the menu.
        if (m_humanTurnPhase == HumanTurnPhase::MoveTarget ||
            m_humanTurnPhase == HumanTurnPhase::AttackTarget)
        {
            m_attackResolution.cancel();
            m_humanTurnPhase = HumanTurnPhase::ActionMenu;

            const Unit *active = m_session.getCurrentUnit();
            if (active)
                m_cursor.setPosition(active->getPosition());
            const bool canAttack = active && !active->hasActed();
            openBattleMenu(canActiveUnitMove(), canAttack, true, KeyCode::Back);
            return;
        }

        // Only in free-cursor mode does ESC open the system menu.
        if (m_humanTurnPhase == HumanTurnPhase::FreeCursor)
            m_battleMenu.showSystemMenu();
        return;
    }

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
            Unit *active = m_session.getCurrentUnit();
            if (active && active->getTeam() == 0 && !active->isDead())
            {
                m_battleMenu.closeAllMenus();
                m_humanTurnPhase = HumanTurnPhase::ActionMenu;
                EnemyAI::takeTurn(*active, m_grid, m_battleMap, m_session.getUnitPtrs());
                m_session.checkResult();
                m_cursor.setPosition(active->getPosition());
                m_turnTimer = 0.15f;
                m_turnState = TurnState::WaitingForAnimation;
            }
        }
    }

    // m_units is populated exclusively by startCombatPhase(), in the same
    // call that flips m_flowPhase to Combat — the phase check alone is a
    // sufficient and correct guard; probing m_units.empty() alongside it
    // was redundant.
    Unit *active = nullptr;
    if (m_flowPhase == BattleFlowPhase::Combat)
        active = m_session.getCurrentUnit();

    m_uiManager.handleInput(input);
    processUIEvents(active);

    if (m_flowPhase == BattleFlowPhase::Deployment)
        return;

    if (m_uiManager.hasBlockingWindow())
        return;

    // 4. Tactical input
    m_humanTurn.handleActiveTurn(input);
}

// ─────────────────────────────────────────────────────────────────────────────
// Update
// ─────────────────────────────────────────────────────────────────────────────

void BattleState::update(float dt)
{
    // Snapshot old state for precise camera rendering interpolation frames
    m_previousCamera = m_camera;

    m_uiManager.update(dt);
    m_combatAnimations.update(dt);
    m_floatingText.update(dt);

    if (m_transition.isActive())
    {
        m_transition.update(dt);
        return;
    }

    if (m_flowPhase == BattleFlowPhase::Deployment)
    {
        m_deploymentPhase.update(dt);
        return;
    }

    if (m_showDefeatOverlay || m_showVictoryOverlay)
        return;

#ifdef _DEBUG
    if (m_autoPlayPhase == AutoPlayPhase::ShowMenu)
    {
        Unit *active = m_session.getCurrentUnit();
        if (active)
        {
            int action = m_autoPlayActionIndex;
            if (action == 0 || action == 1) // Move or Attack → let the AI do its thing
            {
                EnemyAI::takeTurn(*active, m_grid, m_battleMap, m_session.getUnitPtrs());
                m_session.checkResult(); // same reasoning as the EnemyAction branch above
                m_cursor.setPosition(active->getPosition());
                m_turnTimer = 0.15f;
                m_turnState = TurnState::WaitingForAnimation;
            }
            else // Wait — true Wait, does NOT touch movement points or major action
            {
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

    if (m_uiManager.hasBlockingWindow())
        return;

    // ── CURSOR UPDATE ────────────────────────────────────────────────────────
    // This internally reads the arrow keys/WASD, handles the Fire Emblem-style
    // hold-to-repeat delays, and clamps itself within the map bounds!
    m_cursor.update(m_battleMap.cols(), m_battleMap.rows(), dt);

    // Track unit under cursor
    Vec2i cursorPos = m_cursor.getPosition();
    LOG_INFO("Battle", "cursor=(%d,%d)", cursorPos.x, cursorPos.y);
    const Vec2f isoPos = tileToIso(cursorPos, m_mapData.tileWidth, m_mapData.tileHeight);
    LOG_INFO("Battle",
             "cursor=(%d,%d) iso=(%.1f %.1f)",
             cursorPos.x,
             cursorPos.y,
             isoPos.x,
             isoPos.y);
    m_camera.trackTarget(isoPos, Vec2f{GameConstants::VIEW_W, GameConstants::VIEW_H}, dt);
    m_camera.clampToBounds();
    m_hoveredUnit = unitAt(cursorPos);

    // The turn-state machine (and everything that calls
    // m_turnQueue.getCurrentUnit()) is only valid once the queue has been
    // initialised in startCombatPhase(). During Deployment m_units is already
    // populated with preview units, so without this guard the Idle case would
    // call processCurrentTurn() -> getCurrentUnit() and assert on the empty
    // timeline.
    if (m_flowPhase != BattleFlowPhase::Combat)
        return;

    // Turn state machine simulation loops
    switch (m_turnState)
    {
    case TurnState::Idle:
        // Reaching this line already implies m_flowPhase == Combat (guarded
        // above) and m_units populated (guaranteed by startCombatPhase()) —
        // no separate "is there anything to process" check needed. This was
        // the exact false "combat is ready" signal that caused the assert.
        m_turnState = TurnState::ProcessingTurn;
        processCurrentTurn();
        break;

    case TurnState::WaitingForAnimation:
        m_turnTimer -= dt;
        if (m_turnTimer <= 0.0f)
        {
            if (m_pendingResolution == PendingResolution::PlayerConfirmedAttack)
            {
                const SkillData *skill = nullptr;
                if (!m_attackResolution.pendingSkillId().empty())
                {
                    auto it = m_skillDB.find(m_attackResolution.pendingSkillId());
                    if (it != m_skillDB.end())
                        skill = &it->second;
                }

                Unit *active = m_pendingActor ? m_pendingActor : m_session.getCurrentUnit();
                m_attackResolution.beginResolution(active, skill);
                break;
            }

            if (m_pendingResolution == PendingResolution::ResolvingHits)
            {
                m_attackResolution.processNextResult();
                break;
            }

            if (m_pendingResolution == PendingResolution::EnemyAction)
            {
                Unit *active = m_pendingActor ? m_pendingActor : m_session.getCurrentUnit();
                if (active && !active->isDead())
                    EnemyAI::takeTurn(*active, m_grid, m_battleMap, m_session.getUnitPtrs());

                // EnemyAI applies damage directly via Unit::takeDamage(), not
                // through m_session.applyDamage() — it's deliberately
                // stateless and doesn't know about BattleSession. So the
                // cached BattleResult won't reflect an enemy's kill unless we
                // refresh it here, right after the AI's turn resolves.
                m_session.checkResult();

                int playersAliveAfter = 0;
                for (const Unit &u : m_session.getUnits())
                {
                    if (!u.isDead() && u.getTeam() == 0)
                        ++playersAliveAfter;
                }
                if (playersAliveAfter < m_pendingEnemyAliveBefore)
                    m_eventSystem.emit(BattleTriggerType::OnUnitDeath);

                m_pendingResolution = PendingResolution::None;
                m_pendingActor = nullptr;
                m_pendingTarget = nullptr;
                m_pendingActionLabel.clear();
                m_topBattleText.clear();

                if (checkDefeat())
                {
                    startBattleEnd(false);
                    m_turnState = TurnState::Idle;
                    break;
                }
                if (checkVictory())
                {
                    startBattleEnd(true);
                    m_turnState = TurnState::Idle;
                    break;
                }

                advanceToNextUnit();
                m_turnState = TurnState::Idle;
                break;
            }

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
    m_battleMenu.closeAllMenus();
    m_uiManager.popById(WindowId::BattleActionConfirm);
    m_uiManager.popById(WindowId::BattleDialog);

    if (playerWon)
    {
        m_showVictoryOverlay = true;
        m_eventSystem.emit(BattleTriggerType::OnVictory);
    }
    else
    {
        m_showDefeatOverlay = true;
        m_eventSystem.emit(BattleTriggerType::OnDefeat);
    }
}

bool BattleState::checkVictory() const
{
    // Single source of truth: BattleSession::checkResult() is refreshed
    // right after every point where a unit could die (player attacks via
    // finishAttackResolution(), enemy attacks via the EnemyAI call sites in
    // update()/handleInput()) — so the cached result here is always current.
    // This also means victory now respects whatever BattleVictoryRule the
    // BattleDefinition set (KillAll / KillBoss / SurviveTurns), not just a
    // hardcoded "all enemies dead".
    return m_session.getResult() == BattleResult::Victory;
}

bool BattleState::checkDefeat() const
{
    return m_session.getResult() == BattleResult::Defeat;
}

void BattleState::processCurrentTurn()
{
    Unit *active = m_session.getCurrentUnit();
    if (!active || active->isDead())
    {
        const float timeCost = TurnQueue::BASE_ACTION_COST /
                               (active ? static_cast<float>(active->getSpeed()) : 100.0f);
        m_session.advanceTurn(timeCost);
        m_turnState = TurnState::Idle;
        if (checkDefeat() || checkVictory())
            return;
        return;
    }

    active->resetTurn();
    m_canUndoLastMove = false; // fresh turn — nothing to undo yet
    m_eventSystem.emit(BattleTriggerType::OnTurnStart);

    LOG_INFO("Battle", "Active unit: %s, Team: %d, Position: (%d,%d)",
             active->getName().c_str(),
             active->getTeam(),
             active->getPosition().x,
             active->getPosition().y);

    if (active->getTeam() != 0) // Enemies
    {
        m_pendingResolution = PendingResolution::EnemyAction;
        m_pendingActor = active;
        m_pendingTarget = EnemyAI::chooseTarget(*active, m_session.getUnitPtrs());
        m_pendingActionLabel = "Attack";
        m_topBattleText = m_pendingActionLabel;

        m_pendingEnemyAliveBefore = 0;
        for (const Unit &u : m_session.getUnits())
        {
            if (!u.isDead() && u.getTeam() == 0)
                ++m_pendingEnemyAliveBefore;
        }

        m_turnTimer = 0.5f;
        m_turnState = TurnState::WaitingForAnimation;
        return;
    }
    else // Player team (team 0)
    {
        m_humanTurnPhase = HumanTurnPhase::FreeCursor;
        m_cursor.setPosition(active->getPosition());
        if (m_playerControlMode == PlayerControlMode::AI)
        {
#ifdef _DEBUG
            int actionIndex = EnemyAI::chooseAction(*active, m_grid, m_session.getUnitPtrs());
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
    m_eventSystem.emit(BattleTriggerType::OnTurnEnd);

    Unit *active = m_session.getCurrentUnit();
    if (active)
    {
        float timeCost;
        if (active->hasMoved() && active->hasActed())
            timeCost = TurnQueue::BASE_MOVE_AND_ACTION_COST;
        else if (active->hasActed())
            timeCost = TurnQueue::BASE_ACTION_COST;
        else if (active->hasMoved())
            timeCost = TurnQueue::BASE_MOVE_COST;
        else
            timeCost = TurnQueue::BASE_WAIT_COST;

        timeCost /= static_cast<float>(active->getSpeed());
        m_session.advanceTurn(timeCost);
    }

    m_canUndoLastMove = false; // turn is over — nothing left to undo

    // Turn banner — no round number, just the unit name
    Unit *nextUnit = m_session.getCurrentUnit();
    if (nextUnit && m_unitPanelWindow)
        m_unitPanelWindow->setTurnInfo(nextUnit, 0); // 0 means round-less

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

Camera BattleState::buildInterpolatedCamera(float alpha) const
{
    Vec2f interpOffset = m_previousCamera.getOffset() +
                         (m_camera.getOffset() - m_previousCamera.getOffset()) * alpha;

    float interpZoom = m_previousCamera.getZoom() +
                       (m_camera.getZoom() - m_previousCamera.getZoom()) * alpha;

    Camera renderCam = m_camera;
    renderCam.setOffset(interpOffset);
    renderCam.setZoom(interpZoom);
    return renderCam;
}

void BattleState::renderSceneAndOverlays(const Camera &renderCam)
{
    m_battleRenderer->drawBackground(m_bgTop, m_bgBottom);

    if (!m_tileset || m_mapData.isEmpty())
        return;

    BattleOverlayMode overlayMode = BattleOverlayMode::None;
    const std::unordered_set<Vec2i, Vec2iHash> *overlayTiles = nullptr;
    std::unordered_set<Vec2i, Vec2iHash> visibleSpawnTiles;
    if (m_flowPhase == BattleFlowPhase::Deployment)
    {
        visibleSpawnTiles = m_deploymentPhase.deployment().visibleSpawnTiles();
        overlayMode = BattleOverlayMode::MoveRange;
        overlayTiles = &visibleSpawnTiles;
    }
    else if (m_humanTurnPhase == HumanTurnPhase::MoveTarget)
    {
        overlayMode = BattleOverlayMode::MoveRange;
        overlayTiles = &m_reachableTiles;
    }
    else if (m_humanTurnPhase == HumanTurnPhase::AttackTarget)
    {
        overlayMode = BattleOverlayMode::AttackRange;
        overlayTiles = &m_attackRangeTiles;
    }
    else if (m_humanTurnPhase == HumanTurnPhase::AttackConfirm)
    {
        overlayMode = BattleOverlayMode::ConfirmTargets;
        overlayTiles = &m_attackResolution.pendingAttack().tiles();
    }

    BattleRendererContext renderCtx{
        .camera = renderCam,
        .mapData = m_mapData,
        .battleMap = m_battleMap,
        .tileset = m_tileset,
        .tilesPerRow = m_tilesPerRow,
        .spriteH = m_spriteH,
        .scale = m_scale,
        .cursor = m_cursor,
        .cursorHoverOffset = m_cursorHoverOffset,
        .cursorTriW = m_cursorTriW,
        .cursorTriH = m_cursorTriH,
        .units = (m_flowPhase == BattleFlowPhase::Deployment) ? m_deploymentPhase.previewUnits() : m_session.getUnitPtrs(),
        .debugRenderer = m_debugRenderer,
        .showSpawnOverlays = (m_flowPhase == BattleFlowPhase::Deployment),
        .overlayMode = overlayMode,
        .overlayTiles = overlayTiles,
    };

    m_battleRenderer->drawScene(renderCtx);
}

void BattleState::renderDeploymentGrabbedGhost(const Camera &renderCam)
{
    if (m_flowPhase != BattleFlowPhase::Deployment || !m_deploymentPhase.hasGrabbedUnit())
        return;

    const Vec2i cursorPos = m_cursor.getPosition();
    if (!m_battleMap.isValid(cursorPos.x, cursorPos.y))
        return;

    const float s = static_cast<float>(m_scale) * renderCam.getZoom();
    const float halfTW = static_cast<float>(m_mapData.tileWidth) * s * 0.5f;
    const float halfTH = static_cast<float>(m_mapData.tileHeight) * s * 0.5f;
    const float elevStep = static_cast<float>(m_mapData.tileHeight) * 0.5f * s;
    const Vec2f iso = tileToIso(cursorPos, m_mapData.tileWidth, m_mapData.tileHeight);
    const float ax = (iso.x - renderCam.getOffset().x) * s;
    const float ay = (iso.y - renderCam.getOffset().y) * s;
    const GameTile &gt = m_battleMap.at(cursorPos.x, cursorPos.y);
    const float elev = static_cast<float>(gt.height) * elevStep;

    const float cx = ax;
    const float cy = ay - elev - halfTH - 6.0f * s;

    const DeploymentEntry *grabbed = m_deploymentPhase.deployment().grabbedEntry();
    std::string letter;
    if (grabbed)
    {
        const std::string name = loadUnitDisplayName(grabbed->templatePath);
        if (!name.empty())
            letter = std::string(1, name[0]);
    }
    UnitPortrait::drawPlaceholderSprite(m_renderer, FontManager::instance().get(FontRole::Body), Vec2f{cx, cy}, halfTW * 1.2f, 0, letter, 120);
}

void BattleState::syncUnitPanelWindow()
{
    if (!m_unitPanelWindow || m_flowPhase == BattleFlowPhase::Deployment)
        return;

    Unit *active = nullptr;
    if (m_flowPhase == BattleFlowPhase::Combat)
        active = m_session.getCurrentUnit();
    m_unitPanelWindow->setTurnInfo(active, 0);

    if (m_pendingResolution != PendingResolution::None && m_pendingActor)
    {
        if (m_pendingTarget && !m_pendingTarget->isDead())
            m_unitPanelWindow->setDuel(m_pendingActor, m_pendingTarget, m_pendingTarget->getTeam() != 0);
        else
            m_unitPanelWindow->setSingle(m_pendingActor, m_pendingActor->getTeam());
        return;
    }

    if (m_playerControlMode == PlayerControlMode::Human &&
        (m_humanTurnPhase == HumanTurnPhase::AttackTarget ||
         m_humanTurnPhase == HumanTurnPhase::AttackConfirm))
    {
        Unit *target = nullptr;
        if (m_humanTurnPhase == HumanTurnPhase::AttackConfirm && !m_attackResolution.pendingAttack().targets().empty())
            target = m_attackResolution.pendingAttack().focusedTarget();
        else
            target = m_hoveredUnit;

        if (active && !active->isDead())
        {
            if (target && !target->isDead())
                m_unitPanelWindow->setDuel(active, target, target->getTeam() != 0);
            else
                m_unitPanelWindow->setSingle(active, active->getTeam());
        }
        else
            m_unitPanelWindow->clearPanels();
        return;
    }

    if (m_hoveredUnit)
        m_unitPanelWindow->setSingle(m_hoveredUnit, m_hoveredUnit->getTeam());
    else
        m_unitPanelWindow->clearPanels();
}

void BattleState::renderDeploymentHud()
{
    if (m_flowPhase != BattleFlowPhase::Deployment)
        return;

    const Font *font = FontManager::instance().get(FontRole::Body);
    if (!font)
        return;

    const std::string deploymentLabel = "Deployment Phase";
    m_renderer->renderTextInRect(font,
                                 deploymentLabel,
                                 Rectf{0.0f, 12.0f, GameConstants::VIEW_W, 24.0f},
                                 UITheme::SelectedText,
                                 HorizontalAlign::Center,
                                 VerticalAlign::Middle,
                                 false,
                                 false,
                                 false);

    char countBuf[32];
    std::snprintf(countBuf, sizeof(countBuf), "%d/%d", m_deploymentPhase.deployment().placedCount(), m_deploymentPhase.deployment().maxUnits());
    const std::string countLine = countBuf;
    m_renderer->renderTextInRect(font,
                                 countLine,
                                 Rectf{0.0f, 34.0f, GameConstants::VIEW_W, 24.0f},
                                 UITheme::Text,
                                 HorizontalAlign::Center,
                                 VerticalAlign::Middle,
                                 false,
                                 false,
                                 false);

    m_renderer->renderTextInRect(font,
                                 "Q/E: select roster  Enter: grab/place  Esc: cancel/menu",
                                 Rectf{0.0f, 56.0f, GameConstants::VIEW_W, 24.0f},
                                 UITheme::Text,
                                 HorizontalAlign::Center,
                                 VerticalAlign::Middle,
                                 false,
                                 false,
                                 false);
}

void BattleState::renderTopBattleText()
{
    if (m_topBattleText.empty())
        return;

    if (const Font *font = FontManager::instance().get(FontRole::Body))
    {
        m_renderer->renderTextInRect(font,
                                     m_topBattleText,
                                     Rectf{0.0f, 72.0f, GameConstants::VIEW_W, 24.0f},
                                     UITheme::SelectedText,
                                     HorizontalAlign::Center,
                                     VerticalAlign::Middle,
                                     false,
                                     false,
                                     false);
    }
}

void BattleState::renderWorldEffects(const Camera &renderCam)
{
    m_combatAnimations.render(m_renderer);

    if (m_debugRenderer)
        m_debugRenderer->flush(m_renderer, renderCam);
}

void BattleState::renderNativeEffects()
{
    m_damagePreview.render(m_renderer, FontManager::instance().get(FontRole::Body));
}

void BattleState::renderUIStack()
{
    // UI is drawn last, on top of the battle scene, animations, and debug
    // overlay — otherwise unit sprites drawn after this point would paint
    // over any open menu (e.g. the Inspect window).
    if (m_battleMenu.hasInspectWindowOpen())
    {
        // Dim everything behind the Inspect panel so it reads as a modal.
        m_renderer->setBlendMode(Renderer::BlendMode::Blend);
        m_renderer->setDrawColor(Color{0, 0, 0, 140});
        m_renderer->fillRect(Rectf{0.0f, 0.0f, GameConstants::VIEW_W, GameConstants::VIEW_H});
    }
    m_uiManager.render(m_renderer);
}

void BattleState::renderEndOverlay(bool victory)
{
    const Color bgColor = victory ? Color{22, 92, 40, 228} : Color{92, 22, 28, 228};
    const Color borderColor = victory ? Color{120, 255, 150, 255} : Color{255, 120, 120, 255};
    const Color labelColor = victory ? Color{210, 255, 210, 255} : Color{255, 210, 210, 255};
    const std::string label = victory ? "VICTORY" : "DEFEAT";

    const Rectf box{GameConstants::VIEW_W * 0.5f - 260.0f, GameConstants::VIEW_H * 0.5f - 90.0f, 520.0f, 180.0f};

    m_renderer->setBlendMode(Renderer::BlendMode::Blend);
    m_renderer->setDrawColor(bgColor);
    m_renderer->fillRect(box);
    m_renderer->setDrawColor(borderColor);
    m_renderer->drawRect(box);

    if (const Font *font = FontManager::instance().get(FontRole::Body))
    {
        const float labelW = static_cast<float>(label.size()) * 8.0f;
        m_renderer->renderText(font,
                               label,
                               Vec2f{(GameConstants::VIEW_W - labelW) * 0.5f, GameConstants::VIEW_H * 0.5f - 26.0f},
                               labelColor,
                               false,
                               false,
                               false);

        const std::string hint = "Press Enter/Space/Esc";
        const float hintW = static_cast<float>(hint.size()) * 8.0f;
        m_renderer->renderText(font,
                               hint,
                               Vec2f{(GameConstants::VIEW_W - hintW) * 0.5f, GameConstants::VIEW_H * 0.5f + 24.0f},
                               UITheme::Text,
                               false,
                               false,
                               false);
    }
}

void BattleState::render(float alpha)
{
    if (!m_renderer)
        return;

    const Camera renderCam = buildInterpolatedCamera(alpha);

    m_renderer->beginWorldPass();
    renderSceneAndOverlays(renderCam);
    renderDeploymentGrabbedGhost(renderCam);
    renderWorldEffects(renderCam);
    m_renderer->endWorldPass();

    syncUnitPanelWindow();
    renderDeploymentHud();
    renderTopBattleText();
    renderNativeEffects();
    renderUIStack();

    m_transition.render(m_renderer, GameConstants::VIEW_W, GameConstants::VIEW_H);

    if (m_showDefeatOverlay)
        renderEndOverlay(false);
    if (m_showVictoryOverlay)
        renderEndOverlay(true);
}

void BattleState::computeAttackRangeTiles()
{
    m_attackRangeTiles.clear();
    Unit *active = m_session.getCurrentUnit();
    if (!active)
        return;

    Vec2i pos = active->getPosition();
    int range = m_currentAttackRange;

    for (int r = 0; r < m_mapData.height; ++r)
        for (int c = 0; c < m_mapData.width; ++c)
            if (manhattanDistance(pos, Vec2i{c, r}) <= range)
                m_attackRangeTiles.insert({c, r});

    // exclude the tile the unit is standing on
    m_attackRangeTiles.erase(pos);
}

Unit *BattleState::unitAt(Vec2i pos) const
{
    // Only ever called from update()'s Combat-only branch — deployment
    // hover-testing is DeploymentPhaseController::previewUnitAt() now.
    for (Unit *u : m_session.getUnitPtrs())
        if (u && !u->isDead() && u->getPosition() == pos)
            return u;
    return nullptr;
}

HitContext BattleState::makeHitContext(Unit *attacker, Unit *target, const SkillData *skill) const
{
    HitContext ctx;
    ctx.attacker = attacker;
    ctx.target = target;
    if (skill)
    {
        ctx.basePower = skill->basePower;
        ctx.isMagical = skill->isMagical;
        ctx.element = skill->element;
        ctx.skillAccuracy = skill->skillAccuracy;
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
    return ctx;
}