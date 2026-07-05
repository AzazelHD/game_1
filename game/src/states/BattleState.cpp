#include "config/GameConstants.h"
#include "engine/math/Vec2.h"
#include "engine/math/MathUtils.h"
#include "engine/data/TiledJsonLoader.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/core/App.h"
#include "engine/core/Log.h"
#include "engine/core/Window.h"
#include "engine/effects/ScreenTransition.h"
#include "engine/renderer/Camera.h"
#include "engine/renderer/Texture.h"
#include "engine/renderer/Renderer.h"
#include "engine/renderer/DebugRenderer.h"
#include "config/BattleCatalog.h"
#include "states/BattleState.h"
#include "states/BattleLoader.h"
#include "states/MainMenuState.h"
#include "battle/Unit.h"
#include "battle/UnitFactory.h"
#include "battle/MovementRange.h"
#include "battle/CombatSystem.h"
#include "systems/PartyContext.h"
#include "systems/BattleParticipantsBuilder.h"
#include "renderer/BattleRendererContext.h"
#include "ai/EnemyAI.h"
#include "ui/Cursor.h"
#include "ui/windows/ActionMenuWindow.h"
#include "ui/windows/ConfirmWindow.h"
#include "ui/windows/DialogWindow.h"
#include "ui/windows/DeploymentWindow.h"
#include "ui/windows/InspectWindow.h"
#include "data/SkillLoader.h"
#include "data/UnitLoader.h"
#include "ui/UITheme.h"
#include "ui/UIScale.h"

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
    m_units.clear();
    m_pendingRewardXp = 0;
    m_flowPhase = BattleFlowPhase::Deployment;
    m_showDefeatOverlay = false;
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
    window->setResizable(true);
    window->setAspectRatio(16.0f / 9.0f, 16.0f / 9.0f);

    m_renderer->setLogicalPresentation(
        static_cast<int>(GameConstants::VIEW_W),
        static_cast<int>(GameConstants::VIEW_H),
        Renderer::PresentationMode::Letterbox);

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
    m_camera.setOffset(origin);
    m_camera.setZoom(m_battleDefinition->defaultZoom);
    m_previousCamera = m_camera;

    // ── 7. Set up debug renderer ──
    m_debugRenderer = &DebugRenderer::instance();
    m_debugRenderer->setEnabled(true);

    // ── 8. Load default UI font (once) & wire to UnitPanel ──
    if (!App::getDefaultFont())
    {

        const char *fontPath = R"(assets/fonts/PixeloidSans.ttf)";
        Font *font = m_renderer->loadFont(fontPath, 16);
        if (!font)
            LOG_ERROR("Battle", "Failed to load font: %s", fontPath);
        App::setDefaultFont(font);
    }

    // ── 8.1. Load skill database ─────────────────────────────────────────────
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
    m_unitPanelWindow = m_uiManager.push<UnitPanelWindow>("battle.unitpanel");
    m_unitPanelWindow->setFont(App::getDefaultFont());
    m_deploymentWindow = m_uiManager.push<DeploymentWindow>("battle.deployment");
    m_deploymentWindow->setFont(App::getDefaultFont());

    m_hud.clear();

    initializeDeploymentPhase();

    m_eventSystem.initialize(*m_battleDefinition, BattleEventSystem::Callbacks{
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

void BattleState::showBattleMenu(bool canMove, bool canAttack, bool canWait)
{
    std::vector<BattleMenuItem> items;
    items.push_back(BattleMenuItem{
        .label = "Move",
        .enabled = canMove,
        .onSelect = [this]()
        {
            Unit *active = m_turnQueue.getCurrentUnit();
            if (active)
                m_reachableTiles = MovementRange::compute(m_grid, m_battleMap,
                                                          active->getPosition(),
                                                          active->getMoveRangeLeft(),
                                                          active->getTeam(), m_units,
                                                          active->getJump());

            m_humanTurnPhase = HumanTurnPhase::MoveTarget;
            m_hud.clear();
        }});

    items.push_back(BattleMenuItem{
        .label = "Attack",
        .enabled = canAttack,
        .onSelect = [this]()
        {
            m_currentAttackRange = 1;
            m_selectedSkillId.clear();
            computeAttackRangeTiles();
            m_humanTurnPhase = HumanTurnPhase::AttackTarget;
            m_hud.clear();
        }});

    Unit *active = m_turnQueue.getCurrentUnit();
    if (active && !active->getSkillIds().empty())
    {
        items.push_back(BattleMenuItem{
            .label = "Skills",
            .enabled = canAttack,
            .onSelect = [this]()
            { showSkillMenu(); }});
    }

    items.push_back(BattleMenuItem{
        .label = "Defend",
        .enabled = canAttack,
        .onSelect = [this]()
        {
            Unit *active = m_turnQueue.getCurrentUnit();
            if (active)
            {
                active->setMajorAction(MajorAction::Defend);
            }
            m_hud.clear();
            advanceToNextUnit();
            m_turnState = TurnState::Idle;
        }});

    items.push_back(BattleMenuItem{
        .label = "Wait",
        .enabled = canWait,
        .onSelect = [this]()
        {
            m_hud.clear();
            advanceToNextUnit();
            m_turnState = TurnState::Idle;
        }});

    m_hud.setItems(std::move(items), m_flowPhase == BattleFlowPhase::Combat);
}

void BattleState::showSkillMenu()
{
    Unit *active = m_turnQueue.getCurrentUnit();
    if (!active)
        return;

    LOG_INFO("Battle", "showSkillMenu: unit %s has %zu skill IDs",
             active->getName().c_str(), active->getSkillIds().size());

    std::vector<BattleMenuItem> items;

    for (const std::string &skillId : active->getSkillIds())
    {
        LOG_INFO("Battle", "  -> %s", skillId.c_str());

        auto it = m_skillDB.find(skillId);
        if (it == m_skillDB.end())
            continue;

        const SkillData &skill = it->second;
        std::string label = skill.name + " (MP:" + std::to_string(skill.mpCost) + ")";
        bool canUse = (active->getCurrentMp() >= skill.mpCost);

        items.push_back(BattleMenuItem{
            .label = label,
            .enabled = canUse,
            .onSelect = [this, skillId]()
            {
                auto it = m_skillDB.find(skillId);
                if (it != m_skillDB.end())
                {
                    m_currentAttackRange = it->second.range;
                    m_selectedSkillId = skillId;
                    computeAttackRangeTiles();
                }
                m_humanTurnPhase = HumanTurnPhase::AttackTarget;
                m_hud.clear();
            }});
    }

    items.push_back(BattleMenuItem{
        .label = "Back",
        .enabled = true,
        .onSelect = [this]()
        {
            Unit *active = m_turnQueue.getCurrentUnit();
            if (active)
                showBattleMenu(!active->hasMoved(), !active->hasActed(), true);
            else
                m_hud.clear();
        }});

    m_hud.setItems(std::move(items), m_flowPhase == BattleFlowPhase::Combat);
}

void BattleState::showSystemMenu()
{
    const bool deploying = (m_flowPhase == BattleFlowPhase::Deployment);

    BattleMenuItem firstItem;
    if (deploying)
    {
        firstItem = BattleMenuItem{
            .label = "Start Battle",
            .enabled = m_deployment.canStartBattle(),
            .onSelect = [this]()
            {
                m_uiManager.popById("battle.deployment.confirm");
                auto *confirm = m_uiManager.push<ConfirmWindow>("battle.deployment.confirm");
                confirm->setFont(App::getDefaultFont());
                confirm->setPrompt("Start Battle?");
            }};
    }
    else
    {
        firstItem = BattleMenuItem{
            .label = "Resume",
            .enabled = true,
            .onSelect = [this]()
            { m_hud.clear(); }};
    }

    BattleMenuItem quitItem{
        .label = "Quit",
        .enabled = true,
        .onSelect = [this]()
        { m_sm.replace(std::make_unique<MainMenuState>(m_sm, m_renderer)); }};

    m_hud.setItems({std::move(firstItem), std::move(quitItem)}, false);
}

void BattleState::showStatusMenu(Unit *unit)
{
    if (!unit)
        return;
    showInspectWindow(unit);
}

void BattleState::showInspectWindow(Unit *unit)
{
    if (!unit)
        return;

    m_uiManager.popById("battle.inspect");
    auto *inspect = m_uiManager.push<InspectWindow>("battle.inspect");
    inspect->setFont(App::getDefaultFont());
    inspect->setTitle("Unit Inspect");
    inspect->setLines(InspectWindow::buildLines(unit->getData()));
    m_inspectWindow = inspect;
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

bool BattleState::canActiveUnitMove() const
{
    const Unit *active = m_turnQueue.getCurrentUnit();
    if (!active)
        return false;
    return active->getMoveRangeLeft() > 0 && (!active->hasMoved() || active->hasActed());
}

void BattleState::processUIEvents(Unit *active)
{
    auto events = m_uiManager.drainEvents();
    for (const UIEvent &event : events)
    {
        if (event.windowId == "battle.dialog" && event.type == UIEventType::DialogFinished)
        {
            m_uiManager.popById("battle.dialog");
            continue;
        }

        if (event.windowId == "battle.deployment" && event.type == UIEventType::ActionSelected)
        {
            if (event.actionId == "cycle_prev")
            {
                const Vec2i cursorPos = m_cursor.getPosition();
                if (!m_deployment.hasGrabbedUnit() && !m_deployment.isOccupied(cursorPos))
                    m_deployment.cycleSelection(-1);
                refreshDeploymentWindow();
                continue;
            }
            if (event.actionId == "cycle_next")
            {
                const Vec2i cursorPos = m_cursor.getPosition();
                if (!m_deployment.hasGrabbedUnit() && !m_deployment.isOccupied(cursorPos))
                    m_deployment.cycleSelection(1);
                refreshDeploymentWindow();
                continue;
            }
            if (event.actionId == "accept")
            {
                const Vec2i cursorPos = m_cursor.getPosition();

                if (m_deployment.hasGrabbedUnit())
                {
                    if (!m_deployment.isSpawnTile(cursorPos) || m_deployment.isOccupied(cursorPos))
                    {
                        // TODO: play "impossible action" sound (or similar) here
                        continue;
                    }

                    if (m_deployment.placeGrabbed(cursorPos))
                    {
                        syncDeploymentPreviewUnits();
                        refreshDeploymentWindow();
                    }
                    continue;
                }

                // Not grabbing → hovering a unit opens the one-item Inspect submenu.
                if (m_hoveredUnit)
                {
                    m_inspectTargetUnit = m_hoveredUnit;
                    m_uiManager.popById("battle.deploy.inspectmenu");
                    auto *menu = m_uiManager.push<ActionMenuWindow>("battle.deploy.inspectmenu");
                    menu->setFont(App::getDefaultFont());
                    menu->setItems({
                        ActionMenuWindow::Item{.id = "inspect", .label = "Inspect", .enabled = true},
                    });
                    continue;
                }

                const DeploymentEntry *selected = m_deployment.selectedEntry();
                if (!selected)
                    continue;

                if (m_deployment.isUnitPlaced(selected->unitId))
                {
                    if (m_deployment.unplaceUnit(selected->unitId) && m_deployment.grabUnit(selected->unitId))
                    {
                        syncDeploymentPreviewUnits();
                        refreshDeploymentWindow();
                    }
                    continue;
                }

                if (!m_deployment.isOccupied(cursorPos))
                {
                    if (m_deployment.placedCount() >= m_deployment.maxUnits())
                    {
                        // TODO: play "impossible action" sound (or similar) here
                        continue;
                    }

                    if (m_deployment.grabSelected())
                        refreshDeploymentWindow();
                }
                else
                {
                    refreshDeploymentWindow();
                }
                continue;
            }
            if (event.actionId == "back")
            {
                if (m_deployment.hasGrabbedUnit())
                {
                    m_deployment.releaseGrabbed();
                    refreshDeploymentWindow();
                    continue;
                }
                showSystemMenu();
                continue;
            }
            if (event.actionId == "start")
            {
                if (!m_deployment.canStartBattle())
                    continue;

                m_uiManager.popById("battle.deployment.confirm");
                auto *confirm = m_uiManager.push<ConfirmWindow>("battle.deployment.confirm");
                confirm->setFont(App::getDefaultFont());
                confirm->setPrompt("Start Battle?");
                continue;
            }
        }

        if (m_flowPhase == BattleFlowPhase::Deployment && event.windowId == "battle.deploy.inspectmenu")
        {
            if (event.type == UIEventType::ActionSelected && event.actionId == "inspect")
            {
                m_uiManager.popById("battle.deploy.inspectmenu");
                if (m_inspectTargetUnit && !m_inspectTargetUnit->isDead())
                    showInspectWindow(m_inspectTargetUnit);
                continue;
            }

            if (event.type == UIEventType::ActionCanceled)
            {
                m_uiManager.popById("battle.deploy.inspectmenu");
                m_inspectTargetUnit = nullptr;
                continue;
            }
        }

        if (m_flowPhase == BattleFlowPhase::Deployment && event.windowId == "battle.actionmenu")
        {
            if (event.type == UIEventType::ActionSelected)
            {
                const int index = event.index;
                if (index < 0 || index >= static_cast<int>(m_hud.items().size()))
                    continue;

                BattleMenuItem item = m_hud.items()[index];
                m_uiManager.popById("battle.actionmenu");
                if (item.enabled && item.onSelect)
                    item.onSelect();
                continue;
            }

            if (event.type == UIEventType::ActionCanceled)
            {
                m_hud.clear();
                continue;
            }
        }

        if (event.windowId == "battle.inspect" && event.type == UIEventType::ActionCanceled)
        {
            m_uiManager.popById("battle.inspect");
            m_inspectWindow = nullptr;
            continue;
        }

        if (event.windowId == "battle.deployment.confirm" && event.type == UIEventType::ConfirmResult)
        {
            m_uiManager.popById("battle.deployment.confirm");
            if (event.confirmed)
                startCombatPhase();
            continue;
        }

        if (m_flowPhase != BattleFlowPhase::Combat)
            continue;

        if (event.windowId == "battle.actionmenu" && event.type == UIEventType::ActionSelected)
        {
            const int index = event.index;
            if (index < 0 || index >= static_cast<int>(m_hud.items().size()))
                continue;

            BattleMenuItem item = m_hud.items()[index];
            m_uiManager.popById("battle.actionmenu");
            if (item.enabled && item.onSelect)
                item.onSelect();
            continue;
        }

        if (event.windowId == "battle.actionmenu" && event.type == UIEventType::ActionCanceled)
        {
            if (m_humanTurnPhase == HumanTurnPhase::AttackConfirm)
            {
                cancelPendingAttack();
                continue;
            }

            if (!active)
            {
                m_hud.clear();
                continue;
            }

            if (m_canUndoLastMove)
            {
                Vec2i currentPos = active->getPosition();
                m_grid.getTile(currentPos).occupied = false;
                active->setPosition(m_moveStartPos);
                m_grid.getTile(m_moveStartPos).occupied = true;
                active->refundMovePoints(m_moveStartPointsLeft - active->getMoveRangeLeft());
                m_canUndoLastMove = false;
                m_cursor.setPosition(m_moveStartPos);

                m_humanTurnPhase = HumanTurnPhase::ActionMenu;
                openBattleMenu(canActiveUnitMove(), !active->hasActed(), true, KeyCode::Back);
                LOG_INFO("Battle", "Move undone for %s", active->getName().c_str());
            }
            else if (!active->hasMoved() && !active->hasActed())
            {
                m_hud.clear();
                m_humanTurnPhase = HumanTurnPhase::FreeCursor;
                m_cursor.setPosition(active->getPosition());
            }
            continue;
        }

        if (event.windowId == "battle.confirm" && event.type == UIEventType::NavigatePrevious)
        {
            cyclePendingAttackTarget(-1);
            updatePendingAttackPreview(active);
            continue;
        }

        if (event.windowId == "battle.confirm" && event.type == UIEventType::NavigateNext)
        {
            cyclePendingAttackTarget(1);
            updatePendingAttackPreview(active);
            continue;
        }

        if (event.windowId == "battle.confirm" && event.type == UIEventType::ConfirmResult)
        {
            m_uiManager.popById("battle.confirm");

            if (!event.confirmed)
            {
                cancelPendingAttack();
                continue;
            }

            const SkillData *skill = nullptr;
            if (!m_pendingSkillId.empty())
            {
                auto it = m_skillDB.find(m_pendingSkillId);
                if (it != m_skillDB.end())
                    skill = &it->second;
            }

            // TODO: display skill animation
            m_pendingActionLabel = skill ? skill->name : "Attack";
            m_topBattleText = m_pendingActionLabel;
            m_pendingActor = active;
            m_pendingTarget = m_pendingAttack.focusedTarget();
            m_pendingResolution = PendingResolution::PlayerConfirmedAttack;
            m_turnTimer = 0.5f;
            m_turnState = TurnState::WaitingForAnimation;
            continue;
        }
    }
}

void BattleState::preparePendingAttack(Unit *active, Vec2i targetPos, Unit *directTarget, const SkillData *skill)
{
    m_pendingAttack.begin(active, targetPos, directTarget, skill, m_units);
    updatePendingAttackPreview(active);
}

void BattleState::cyclePendingAttackTarget(int delta)
{
    m_pendingAttack.cycleFocus(delta);
}

void BattleState::updatePendingAttackPreview(Unit *active)
{
    Unit *focus = m_pendingAttack.focusedTarget();
    if (!active || !focus)
    {
        m_damagePreview.hide();
        m_topBattleText.clear();
        return;
    }

    m_hoveredUnit = focus;

    const SkillData *previewSkill = nullptr;
    if (!m_selectedSkillId.empty())
    {
        auto it = m_skillDB.find(m_selectedSkillId);
        if (it != m_skillDB.end())
            previewSkill = &it->second;
    }
    m_damagePreview.show(*active, *focus, previewSkill);

    HitContext ctx = makeHitContext(active, focus, previewSkill);

    const CombatResult preview = CombatSystem::preview(ctx);
    char textBuf[96];
    std::snprintf(textBuf, sizeof(textBuf), "DMG %d   ACC %d%%", preview.damage, static_cast<int>(preview.hitChance + 0.5f));
    m_topBattleText = textBuf;
}

void BattleState::applyPendingAttack(Unit *active, const SkillData *skill)
{
    if (!active)
        return;

    bool anyDied = false;

    for (Unit *u : m_pendingAttack.targets())
    {
        if (!u || u->isDead())
            continue;

        HitContext ctx = makeHitContext(active, u, skill);

        CombatResult result = CombatSystem::resolve(ctx);
        if (result.hit)
        {
            u->takeDamage(result.damage);
            if (u->isDead())
                anyDied = true;
            LOG_INFO("Battle", "%s uses %s on %s for %d damage!",
                     active->getName().c_str(),
                     skill ? skill->name.c_str() : "Attack",
                     u->getName().c_str(), result.damage);
        }
        else
        {
            LOG_INFO("Battle", "%s misses %s", active->getName().c_str(), u->getName().c_str());
        }
    }

    if (skill)
        active->setCurrentMp(active->getCurrentMp() - skill->mpCost);

    active->setMajorAction(skill ? MajorAction::Skill : MajorAction::Attack);
    m_canUndoLastMove = false;
    m_selectedSkillId.clear();
    m_pendingSkillId.clear();

    m_damagePreview.hide();
    m_pendingAttack.clear();
    m_topBattleText.clear();
    m_hud.clear();
    m_humanTurnPhase = HumanTurnPhase::ActionMenu;
    openBattleMenu(canActiveUnitMove(), false, true, KeyCode::Accept);

    if (anyDied)
        m_eventSystem.emit(BattleTriggerType::OnUnitDeath);
}

void BattleState::cancelPendingAttack()
{
    m_pendingAttack.clear();
    m_pendingSkillId.clear();
    m_damagePreview.hide();
    m_topBattleText.clear();
    m_hud.clear();
    m_humanTurnPhase = HumanTurnPhase::AttackTarget;
}

void BattleState::initializeDeploymentPhase()
{
    PartyContext &partyCtx = PartyContext::instance();
    partyCtx.ensureInitialized();

    std::unordered_set<Vec2i, Vec2iHash> spawnTiles;
    for (GameTile *tile : m_battleMap.playerSpawns)
    {
        if (tile)
            spawnTiles.insert(Vec2i{tile->col, tile->row});
    }

    m_deployment.initialize(
        m_battleDefinition ? m_battleDefinition->maxUnits : 1,
        std::move(spawnTiles),
        partyCtx.party().memberIds(),
        partyCtx.roster());

    if (!m_battleMap.playerSpawns.empty() && m_battleMap.playerSpawns.front())
    {
        m_cursor.setPosition(Vec2i{m_battleMap.playerSpawns.front()->col, m_battleMap.playerSpawns.front()->row});
    }

    syncDeploymentPreviewUnits();
    refreshDeploymentWindow();
}

void BattleState::syncDeploymentPreviewUnits()
{
    m_hoveredUnit = nullptr;
    m_inspectTargetUnit = nullptr;
    m_uiManager.popById("battle.deploy.inspectmenu");

    for (Unit *u : m_units)
        delete u;
    m_units.clear();

    for (int y = 0; y < m_grid.getHeight(); ++y)
    {
        for (int x = 0; x < m_grid.getWidth(); ++x)
            m_grid.getTile(Vec2i{x, y}).occupied = false;
    }

    for (const DeploymentEntry &placed : m_deployment.deployed())
    {
        Unit *u = UnitFactory::createUnitFromJson(placed.templatePath, placed.position, 0);
        if (!u)
            continue;

        m_units.push_back(u);
        if (m_grid.isValid(u->getPosition()))
            m_grid.getTile(u->getPosition()).occupied = true;
    }

    if (!m_battleDefinition)
        return;

    std::vector<GameTile *> enemySpawns;
    for (const auto &pair : m_battleMap.enemySpawnsByTeam)
    {
        for (GameTile *tile : pair.second)
            enemySpawns.push_back(tile);
    }

    std::size_t spawnIndex = 0;
    for (const EnemyDefinition &enemy : m_battleDefinition->enemies)
    {
        if (enemySpawns.empty())
            break;

        GameTile *tile = enemySpawns[spawnIndex % enemySpawns.size()];
        ++spawnIndex;
        if (!tile)
            continue;

        Vec2i pos{tile->col, tile->row};
        if (!m_grid.isValid(pos) || m_grid.getTile(pos).occupied)
            continue;

        Unit *u = UnitFactory::createUnitFromJson(enemy.templatePath, pos, enemy.team);
        if (!u)
            continue;

        m_units.push_back(u);
        m_grid.getTile(pos).occupied = true;
    }
}

void BattleState::refreshDeploymentWindow()
{
    if (!m_deploymentWindow)
        return;

    const DeploymentEntry *selected = m_deployment.selectedEntry();
    const DeploymentEntry *grabbed = m_deployment.grabbedEntry();
    m_deploymentWindow->setSelectedUnitLabel(selected ? selected->unitId : std::string());
    m_deploymentWindow->setGrabbedState(grabbed != nullptr, grabbed ? grabbed->unitId : std::string());
    m_deploymentWindow->setDeploymentStatus(m_deployment.placedCount(), m_deployment.maxUnits(), m_deployment.canStartBattle());
}

void BattleState::startCombatPhase()
{
    if (m_flowPhase != BattleFlowPhase::Deployment || !m_battleDefinition)
        return;

    m_hoveredUnit = nullptr;
    m_inspectTargetUnit = nullptr;
    m_uiManager.popById("battle.deploy.inspectmenu");

    for (Unit *u : m_units)
        delete u;
    m_units.clear();

    for (int y = 0; y < m_grid.getHeight(); ++y)
    {
        for (int x = 0; x < m_grid.getWidth(); ++x)
            m_grid.getTile(Vec2i{x, y}).occupied = false;
    }

    BattleParticipants participants = BattleParticipantsBuilder::build(*m_battleDefinition, m_battleMap, m_deployment);
    if (participants.playerUnits.empty())
        return;

    for (Unit *u : participants.playerUnits)
    {
        if (!u)
            continue;
        m_units.push_back(u);
        if (m_grid.isValid(u->getPosition()))
            m_grid.getTile(u->getPosition()).occupied = true;
    }

    for (Unit *u : participants.enemyUnits)
    {
        if (!u)
            continue;
        m_units.push_back(u);
        if (m_grid.isValid(u->getPosition()))
            m_grid.getTile(u->getPosition()).occupied = true;
    }

    m_turnQueue.init(m_units);
    if (Unit *first = m_turnQueue.getCurrentUnit(); first)
        m_cursor.setPosition(first->getPosition());

    m_uiManager.popById("battle.deployment");
    m_uiManager.popById("battle.deployment.confirm");
    m_deploymentWindow = nullptr;
    if (m_unitPanelWindow)
        m_unitPanelWindow->clearPreview();

    m_flowPhase = BattleFlowPhase::Combat;
    m_pendingResolution = PendingResolution::None;
    m_pendingActor = nullptr;
    m_pendingTarget = nullptr;
    m_pendingActionLabel.clear();
    m_topBattleText.clear();
    m_turnState = TurnState::ProcessingTurn;
    processCurrentTurn();

    m_eventSystem.emit(BattleTriggerType::OnBattleStart);
}

void BattleState::showDialogueFromEvent(const std::string &text)
{
    m_uiManager.popById("battle.dialog");
    auto *dialog = m_uiManager.push<DialogWindow>("battle.dialog");
    dialog->setFont(App::getDefaultFont());
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

    Unit *unit = UnitFactory::createUnitFromJson(templatePath, spawnPos, 2);
    if (!unit)
        return;

    m_units.push_back(unit);
    m_grid.getTile(spawnPos).occupied = true;
    m_turnQueue.insert(unit, TurnQueue::BASE_WAIT_COST / static_cast<float>(std::max(1, unit->getSpeed())));
}

void BattleState::onExit()
{
    for (Unit *u : m_units)
        delete u;
    m_units.clear();

    m_uiManager.clear();
    m_unitPanelWindow = nullptr;
    m_deploymentWindow = nullptr;
    m_inspectWindow = nullptr;
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

    if (m_showDefeatOverlay)
    {
        if (input.isKeyPressed(KeyCode::Accept, false) ||
            input.isKeyPressed(KeyCode::Back, false) ||
            input.isKeyPressed(KeyCode::Advance, false))
        {
            if (m_onBattleFinished)
                m_onBattleFinished(false);
            else
                m_sm.replace(std::make_unique<MainMenuState>(m_sm, m_renderer, true));
        }
        return;
    }

    if (m_transition.isActive())
        return;

    if (input.isKeyPressed(KeyCode::Back, false))
    {
        if (m_flowPhase == BattleFlowPhase::Deployment)
        {
            if (m_deployment.hasGrabbedUnit())
            {
                m_deployment.releaseGrabbed();
                refreshDeploymentWindow();
                return;
            }

            if (m_uiManager.hasWindow("battle.deploy.inspectmenu"))
            {
                m_uiManager.popById("battle.deploy.inspectmenu");
                m_inspectTargetUnit = nullptr;
                return;
            }
            if (m_uiManager.hasWindow("battle.inspect"))
            {
                m_uiManager.popById("battle.inspect");
                m_inspectWindow = nullptr;
                return;
            }
            if (m_uiManager.hasWindow("battle.deployment.confirm"))
            {
                m_uiManager.popById("battle.deployment.confirm");
                return;
            }
            if (m_uiManager.hasWindow("battle.actionmenu"))
            {
                m_uiManager.popById("battle.actionmenu");
                return;
            }

            showSystemMenu();
            return;
        }

        if (m_uiManager.hasWindow("battle.inspect"))
        {
            m_uiManager.popById("battle.inspect");
            m_inspectWindow = nullptr;
            return;
        }
        if (m_uiManager.hasWindow("battle.dialog"))
        {
            m_uiManager.popById("battle.dialog");
            return;
        }
        if (m_uiManager.hasWindow("battle.confirm"))
        {
            m_uiManager.popById("battle.confirm");
            cancelPendingAttack();
            return;
        }
        if (m_uiManager.hasWindow("battle.actionmenu"))
        {
            Unit *active = (!m_units.empty()) ? m_turnQueue.getCurrentUnit() : nullptr;
            if (active && active->hasActed() && !m_canUndoLastMove)
                return;

            m_uiManager.handleInput(input);
            processUIEvents(active);
            return;
        }

        // Mid-action (targeting/moving): ESC cancels the action, not open the menu.
        if (m_humanTurnPhase == HumanTurnPhase::MoveTarget ||
            m_humanTurnPhase == HumanTurnPhase::AttackTarget)
        {
            cancelPendingAttack();
            m_humanTurnPhase = HumanTurnPhase::ActionMenu;
            openBattleMenu(canActiveUnitMove(), true, true, KeyCode::Back);
            return;
        }

        // Only in free-cursor mode does ESC open the system menu.
        if (m_humanTurnPhase == HumanTurnPhase::FreeCursor)
            showSystemMenu();
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
            Unit *active = m_turnQueue.getCurrentUnit();
            if (active && active->getTeam() == 0 && !active->isDead())
            {
                m_hud.clear();
                m_humanTurnPhase = HumanTurnPhase::ActionMenu;
                EnemyAI::takeTurn(*active, m_grid, m_battleMap, m_units);
                m_cursor.setPosition(active->getPosition());
                m_turnTimer = 0.15f;
                m_turnState = TurnState::WaitingForAnimation;
            }
        }
    }

    Unit *active = nullptr;
    if (m_flowPhase == BattleFlowPhase::Combat && !m_units.empty())
        active = m_turnQueue.getCurrentUnit();

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

    if (m_transition.isActive())
    {
        m_transition.update(dt);
        return;
    }

    if (m_flowPhase == BattleFlowPhase::Deployment)
    {
        const bool menuOpen =
            m_uiManager.hasWindow("battle.actionmenu") ||
            m_uiManager.hasWindow("battle.deployment.confirm") ||
            m_uiManager.hasWindow("battle.inspect") ||
            m_uiManager.hasWindow("battle.deploy.inspectmenu") ||
            m_uiManager.hasWindow("battle.dialog");

        if (!menuOpen)
            m_cursor.update(m_battleMap.cols(), m_battleMap.rows(), dt);

        Vec2i cursorPos = m_cursor.getPosition();
        m_hoveredUnit = unitAt(cursorPos);

        refreshDeploymentWindow();

        if (m_unitPanelWindow)
        {
            if (m_deployment.hasGrabbedUnit())
            {
                setUnitPanelPreviewFromEntry(m_deployment.grabbedEntry());
            }
            else if (m_hoveredUnit)
            {
                m_unitPanelWindow->setSingle(m_hoveredUnit, m_hoveredUnit->getTeam() != 0);
            }
            else
            {
                setUnitPanelPreviewFromEntry(m_deployment.selectedEntry());
            }
        }
        return;
    }

    if (m_showDefeatOverlay)
        return;

#ifdef _DEBUG
    if (m_autoPlayPhase == AutoPlayPhase::ShowMenu)
    {
        Unit *active = m_turnQueue.getCurrentUnit();
        if (active)
        {
            int action = m_autoPlayActionIndex; // 0=Move, 1=Attack, 2=Wait
            if (action == 0 || action == 1)     // Move or Attack → let the AI do its thing
            {
                EnemyAI::takeTurn(*active, m_grid, m_battleMap, m_units);
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
    m_hoveredUnit = unitAt(cursorPos);

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
            if (m_pendingResolution == PendingResolution::PlayerConfirmedAttack)
            {
                const SkillData *skill = nullptr;
                if (!m_pendingSkillId.empty())
                {
                    auto it = m_skillDB.find(m_pendingSkillId);
                    if (it != m_skillDB.end())
                        skill = &it->second;
                }

                Unit *active = m_pendingActor ? m_pendingActor : m_turnQueue.getCurrentUnit();
                applyPendingAttack(active, skill);

                m_pendingResolution = PendingResolution::None;
                m_pendingActor = nullptr;
                m_pendingTarget = nullptr;
                m_pendingActionLabel.clear();
                m_topBattleText.clear();
                m_turnState = TurnState::ProcessingTurn;
                break;
            }

            if (m_pendingResolution == PendingResolution::EnemyAction)
            {
                Unit *active = m_pendingActor ? m_pendingActor : m_turnQueue.getCurrentUnit();
                if (active && !active->isDead())
                    EnemyAI::takeTurn(*active, m_grid, m_battleMap, m_units);

                int playersAliveAfter = 0;
                for (const Unit *u : m_units)
                {
                    if (u && !u->isDead() && u->getTeam() == 0)
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

    if (!playerWon)
    {
        m_playerWon = false;
        m_showDefeatOverlay = true;
        m_hud.clear();
        m_uiManager.popById("battle.confirm");
        m_uiManager.popById("battle.dialog");
        return;
    }

    m_playerWon = playerWon;

    if (playerWon)
        m_eventSystem.emit(BattleTriggerType::OnVictory);
    else
        m_eventSystem.emit(BattleTriggerType::OnDefeat);

    m_transition.start({.transition = ScreenTransitions::FadeOut,
                        .duration = 0.5f,
                        .easing = easeInOut,
                        .onComplete = [this]()
                        {
                            if (m_onBattleFinished)
                                m_onBattleFinished(m_playerWon);
                            else
                                m_sm.replace(std::make_unique<MainMenuState>(m_sm, m_renderer, true));
                        }});
}

bool BattleState::checkVictory() const
{
    int enemyCount = countAliveEnemies();
    LOG_INFO("Battle", "checkVictory: %d enemies alive", enemyCount);
    return enemyCount == 0;
}

bool BattleState::checkDefeat() const
{
    int playerCount = countAlivePlayers();
    LOG_INFO("Battle", "checkDefeat: %d players alive", playerCount);
    return playerCount == 0;
}

int BattleState::countAliveEnemies() const
{
    int n = 0;
    for (const Unit *u : m_units)
        if (u && !u->isDead() && u->getTeam() >= 2)
            n++;
    return n;
}

int BattleState::countAlivePlayers() const
{
    int n = 0;
    for (const Unit *u : m_units)
        if (u && !u->isDead() && u->getTeam() == 0)
            n++;
    return n;
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
        m_pendingTarget = EnemyAI::chooseTarget(*active, m_units);
        m_pendingActionLabel = "Attack";
        m_topBattleText = m_pendingActionLabel;

        m_pendingEnemyAliveBefore = 0;
        for (const Unit *u : m_units)
        {
            if (u && !u->isDead() && u->getTeam() == 0)
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
    m_eventSystem.emit(BattleTriggerType::OnTurnEnd);

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

    m_canUndoLastMove = false; // turn is over — nothing left to undo

    // Turn banner — no round number, just the unit name
    Unit *nextUnit = m_turnQueue.getCurrentUnit();
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
    m_battleRenderer->drawBackground(m_bgTop, m_bgBottom);

    if (m_tileset && !m_mapData.isEmpty())
    {
        BattleOverlayMode overlayMode = BattleOverlayMode::None;
        const std::unordered_set<Vec2i, Vec2iHash> *overlayTiles = nullptr;
        if (m_flowPhase == BattleFlowPhase::Deployment)
        {
            overlayMode = BattleOverlayMode::MoveRange;
            overlayTiles = &m_deployment.spawnTiles();
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
            overlayTiles = &m_pendingAttack.tiles();
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
            .units = m_units,
            .debugRenderer = m_debugRenderer,
            .showSpawnOverlays = (m_flowPhase == BattleFlowPhase::Deployment),
            .overlayMode = overlayMode,
            .overlayTiles = overlayTiles,
        };

        m_battleRenderer->drawScene(renderCtx);
    }

    if (m_flowPhase == BattleFlowPhase::Deployment && m_deployment.hasGrabbedUnit() && m_debugRenderer)
    {
        const Vec2i cursorPos = m_cursor.getPosition();
        if (m_battleMap.isValid(cursorPos.x, cursorPos.y))
        {
            const float s = static_cast<float>(m_scale) * renderCam.getZoom();
            const float halfTW = static_cast<float>(m_mapData.tileWidth) * s * 0.5f;
            const float halfTH = static_cast<float>(m_mapData.tileHeight) * s * 0.5f;
            const float elevStep = static_cast<float>(m_mapData.tileHeight) * 0.5f * s;
            const Vec2f iso = tileToIso(cursorPos, m_mapData.tileWidth, m_mapData.tileHeight);
            const float ax = renderCam.getOffset().x + iso.x * s;
            const float ay = renderCam.getOffset().y + iso.y * s;
            const GameTile &gt = m_battleMap.at(cursorPos.x, cursorPos.y);
            const float elev = static_cast<float>(gt.height) * elevStep;

            const float cx = ax;
            const float cy = ay - elev - halfTH - 6.0f * s;
            m_debugRenderer->addScreenCircle(Vec2f{cx, cy}, halfTW * 0.6f, Color{64, 128, 255, 120}, true);
        }
    }

    if (m_unitPanelWindow)
    {
        if (m_flowPhase != BattleFlowPhase::Deployment)
        {
            Unit *active = nullptr;
            if (m_flowPhase == BattleFlowPhase::Combat && !m_units.empty())
                active = m_turnQueue.getCurrentUnit();
            m_unitPanelWindow->setTurnInfo(active, 0);

            if (m_pendingResolution != PendingResolution::None && m_pendingActor)
            {
                if (m_pendingTarget && !m_pendingTarget->isDead())
                    m_unitPanelWindow->setDuel(m_pendingActor, m_pendingTarget);
                else
                    m_unitPanelWindow->setSingle(m_pendingActor, m_pendingActor->getTeam() != 0);
            }
            else if (m_playerControlMode == PlayerControlMode::Human &&
                     (m_humanTurnPhase == HumanTurnPhase::AttackTarget ||
                      m_humanTurnPhase == HumanTurnPhase::AttackConfirm))
            {
                Unit *target = nullptr;
                if (m_humanTurnPhase == HumanTurnPhase::AttackConfirm && !m_pendingAttack.targets().empty())
                    target = m_pendingAttack.focusedTarget();
                else
                    target = m_hoveredUnit;

                if (active && !active->isDead() && target && !target->isDead())
                    m_unitPanelWindow->setDuel(active, target);
                else if (active && !active->isDead())
                    m_unitPanelWindow->setSingle(active, active->getTeam() != 0);
                else
                    m_unitPanelWindow->clearPanels();
            }
            else if (m_hoveredUnit)
            {
                m_unitPanelWindow->setSingle(m_hoveredUnit, m_hoveredUnit->getTeam() != 0);
            }
            else
            {
                m_unitPanelWindow->clearPanels();
            }
        }
    }

    if (m_flowPhase == BattleFlowPhase::Deployment)
    {
        const Font *font = App::getDefaultFont();
        if (font)
        {
            const std::string deploymentLabel = "Deployment Phase";
            m_renderer->renderTextInRect(font,
                                         deploymentLabel,
                                         Rectf{0.0f, 12.0f, GameConstants::VIEW_W, 24.0f},
                                         UITheme::SelectedText,
                                         Renderer::HorizontalAlign::Center,
                                         Renderer::VerticalAlign::Middle,
                                         false,
                                         false,
                                         false);

            char countBuf[32];
            std::snprintf(countBuf, sizeof(countBuf), "%d/%d", m_deployment.placedCount(), m_deployment.maxUnits());
            const std::string countLine = countBuf;
            m_renderer->renderTextInRect(font,
                                         countLine,
                                         Rectf{0.0f, 34.0f, GameConstants::VIEW_W, 24.0f},
                                         UITheme::Text,
                                         Renderer::HorizontalAlign::Center,
                                         Renderer::VerticalAlign::Middle,
                                         false,
                                         false,
                                         false);

            m_renderer->renderTextInRect(font,
                                         "Q/E: select roster  Enter: grab/place  Esc: cancel/menu",
                                         Rectf{0.0f, 56.0f, GameConstants::VIEW_W, 24.0f},
                                         UITheme::Text,
                                         Renderer::HorizontalAlign::Center,
                                         Renderer::VerticalAlign::Middle,
                                         false,
                                         false,
                                         false);
        }
    }

    if (!m_topBattleText.empty())
    {
        if (const Font *font = App::getDefaultFont())
        {
            m_renderer->renderTextInRect(font,
                                         m_topBattleText,
                                         Rectf{0.0f, 72.0f, GameConstants::VIEW_W, 24.0f},
                                         UITheme::SelectedText,
                                         Renderer::HorizontalAlign::Center,
                                         Renderer::VerticalAlign::Middle,
                                         false,
                                         false,
                                         false);
        }
    }

    m_uiManager.render(m_renderer);

    // ── Damage preview (always drawn if visible) ──
    m_damagePreview.render(m_renderer, App::getDefaultFont());

    m_combatAnimations.render(m_renderer);

    // ── Debug overlay ──
    if (m_debugRenderer)
        m_debugRenderer->flush(m_renderer, renderCam);

    // ── Screen transition (fade) ──
    m_transition.render(m_renderer, GameConstants::VIEW_W, GameConstants::VIEW_H);

    if (m_showDefeatOverlay)
    {
        m_renderer->setBlendMode(Renderer::BlendMode::Blend);
        m_renderer->setDrawColor(Color{92, 22, 28, 228});
        m_renderer->fillRect(Rectf{GameConstants::VIEW_W * 0.5f - 260.0f, GameConstants::VIEW_H * 0.5f - 90.0f, 520.0f, 180.0f});
        m_renderer->setDrawColor(Color{255, 120, 120, 255});
        m_renderer->drawRect(Rectf{GameConstants::VIEW_W * 0.5f - 260.0f, GameConstants::VIEW_H * 0.5f - 90.0f, 520.0f, 180.0f});

        if (const Font *font = App::getDefaultFont())
        {
            const std::string defeat = "DEFEAT";
            const float defeatW = static_cast<float>(defeat.size()) * 8.0f;
            m_renderer->renderText(font,
                                   defeat,
                                   Vec2f{(GameConstants::VIEW_W - defeatW) * 0.5f, GameConstants::VIEW_H * 0.5f - 26.0f},
                                   Color{255, 210, 210, 255},
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

    // exclude the tile the unit is standing on
    m_attackRangeTiles.erase(pos);
}

Unit *BattleState::unitAt(Vec2i pos) const
{
    for (Unit *u : m_units)
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

void BattleState::setUnitPanelPreviewFromEntry(const DeploymentEntry *entry)
{
    if (!m_unitPanelWindow)
        return;
    if (!entry)
    {
        m_unitPanelWindow->clearPreview();
        return;
    }
    try
    {
        const UnitData unitData = UnitLoader::load(entry->templatePath);
        m_unitPanelWindow->setPreview(unitData.name, unitData.level, unitData.maxHp, unitData.maxMp, false);
    }
    catch (...)
    {
        m_unitPanelWindow->setPreview(entry->unitId, 1, 1, 0, false);
    }
}
