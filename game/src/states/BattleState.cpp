#include "config/GameConstants.h"
#include "engine/math/Vec2.h"
#include "engine/math/MathUtils.h"
#include "engine/data/TiledJsonLoader.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/core/App.h"
#include "engine/core/Log.h"
#include "engine/core/Window.h"
#include "engine/renderer/Camera.h"
#include "engine/renderer/Texture.h"
#include "engine/renderer/Renderer.h"
#include "engine/renderer/DebugRenderer.h"
#include "engine/renderer/FontManager.h"
#include "engine/effects/ScreenTransition.h"
#include "config/BattleCatalog.h"
#include "states/BattleState.h"
#include "states/BattleLoader.h"
#include "states/MainMenuState.h"
#include "battle/Unit.h"
#include "battle/UnitFactory.h"
#include "battle/UnitProgression.h"
#include "battle/MovementRange.h"
#include "battle/CombatSystem.h"
#include "systems/PartyContext.h"
#include "systems/BattleParticipantsBuilder.h"
#include "renderer/BattleRendererContext.h"
#include "ai/EnemyAI.h"
#include "ui/Cursor.h"
#include "ui/UITheme.h"
#include "ui/UIScale.h"
#include "ui/UnitPortrait.h"
#include "ui/windows/ActionMenuWindow.h"
#include "ui/windows/ConfirmWindow.h"
#include "ui/windows/DialogWindow.h"
#include "ui/windows/DeploymentWindow.h"
#include "ui/windows/InspectWindow.h"
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
    m_deploymentPreviewUnits.clear();
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
    m_deploymentWindow = m_uiManager.push<DeploymentWindow>(WindowId::BattleDeployment);
    m_deploymentWindow->setFont(FontManager::instance().get(FontRole::Body));

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
            Unit *active = m_session.getCurrentUnit();
            if (active)
            {
                m_reachableTiles = MovementRange::compute(m_grid, m_battleMap,
                                                          active->getPosition(),
                                                          active->getMoveRangeLeft(),
                                                          active->getTeam(),
                                                          m_session.getUnitPtrs(),
                                                          active->getJump());
                m_cursor.setPosition(active->getPosition());
            }
            m_humanTurnPhase = HumanTurnPhase::MoveTarget;
            m_hud.clear();
        }});

    items.push_back(BattleMenuItem{
        .label = "Attack",
        .enabled = canAttack,
        .onSelect = [this]()
        {
            Unit *active = m_session.getCurrentUnit();
            if (active)
            {
                m_currentAttackRange = 1;
                m_selectedSkillId.clear();
                computeAttackRangeTiles();
                m_cursor.setPosition(active->getPosition());
            }
            m_humanTurnPhase = HumanTurnPhase::AttackTarget;
            m_hud.clear();
        }});

    Unit *active = m_session.getCurrentUnit();
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
            Unit *active = m_session.getCurrentUnit();
            if (active)
                active->setMajorAction(MajorAction::Defend);
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
    Unit *active = m_session.getCurrentUnit();
    if (!active)
        return;

    m_skillMenuItems.clear();

    for (const std::string &skillId : active->getSkillIds())
    {
        auto it = m_skillDB.find(skillId);
        if (it == m_skillDB.end())
            continue;

        const SkillData &skill = it->second;
        std::string label = skill.name + " (MP:" + std::to_string(skill.mpCost) + ")";
        bool canUse = (active->getCurrentMp() >= skill.mpCost);

        m_skillMenuItems.push_back(BattleMenuItem{
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
                if (Unit *active = m_session.getCurrentUnit())
                    m_cursor.setPosition(active->getPosition());
                m_humanTurnPhase = HumanTurnPhase::AttackTarget;
                m_uiManager.popById(WindowId::BattleSkillMenu);
                m_hud.clear();
            }});
    }

    // A real push onto the UI stack — a second, distinct level above
    // battle.actionmenu, which stays untouched underneath. Back pops just
    // this window and battle.actionmenu is revealed exactly as it was, the
    // same way any other stacked window already behaves.
    m_uiManager.popById(WindowId::BattleSkillMenu);
    auto *menu = m_uiManager.push<ActionMenuWindow>(WindowId::BattleSkillMenu);
    menu->setFont(FontManager::instance().get(FontRole::Body));
    if (m_flowPhase == BattleFlowPhase::Combat)
    {
        UIScale::refresh();
        const float ui = UIScale::factor();
        menu->setPanelPosition(Vec2f{GameConstants::VIEW_W - 280.0f * ui, GameConstants::VIEW_H - 240.0f * ui});
    }

    std::vector<ActionMenuWindow::Item> uiItems;
    uiItems.reserve(m_skillMenuItems.size());
    for (int i = 0; i < static_cast<int>(m_skillMenuItems.size()); ++i)
    {
        uiItems.push_back(ActionMenuWindow::Item{
            .label = m_skillMenuItems[i].label,
            .enabled = m_skillMenuItems[i].enabled,
        });
    }
    menu->setItems(std::move(uiItems));
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
                m_uiManager.popById(WindowId::BattleDeploymentConfirm);
                auto *confirm = m_uiManager.push<ConfirmWindow>(WindowId::BattleDeploymentConfirm);
                confirm->setFont(FontManager::instance().get(FontRole::Body));
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

    m_uiManager.popById(WindowId::BattleInspect);
    auto *inspect = m_uiManager.push<InspectWindow>(WindowId::BattleInspect);
    inspect->setFont(FontManager::instance().get(FontRole::Body));
    inspect->setTitle("Inspect");
    inspect->setLines(InspectWindow::buildLines(unit->getData()));
    m_inspectWindow = inspect;
}

void BattleState::openUnitInspectMenu(Unit *unit)
{
    if (!unit)
        return;

    m_inspectTargetUnit = unit;
    m_uiManager.popById(WindowId::BattleInspectMenu);
    auto *menu = m_uiManager.push<ActionMenuWindow>(WindowId::BattleInspectMenu);
    menu->setFont(FontManager::instance().get(FontRole::Body));
    menu->setAnchorBottomRight();
    menu->setItems({
        ActionMenuWindow::Item{.id = ActionId::Inspect, .label = "Inspect", .enabled = true},
    });
}

void BattleState::showInspectWindowFromTemplate(const std::string &templatePath)
{
    // For a roster unit that hasn't been placed yet there's no live Unit
    // object to inspect — build the same effective stats a live Unit would
    // have (race/gender bonuses applied) from its template instead.
    try
    {
        const UnitData templateData = UnitLoader::load(templatePath);
        const RaceData &raceData = getRaceData(templateData.race);
        const GenderData &genderData = getGenderData(templateData.gender);
        const Unit previewUnit(templateData, raceData, genderData, Vec2i{0, 0});

        m_uiManager.popById(WindowId::BattleInspect);
        auto *inspect = m_uiManager.push<InspectWindow>(WindowId::BattleInspect);
        inspect->setFont(FontManager::instance().get(FontRole::Body));
        inspect->setTitle("Unit Inspect");
        inspect->setLines(InspectWindow::buildLines(previewUnit.getData()));
        m_inspectWindow = inspect;
    }
    catch (...)
    {
        // Template failed to load — nothing sensible to show.
    }
}

void BattleState::syncCursorToSelection()
{
    // Whenever the roster selection points at a unit that's already on the
    // field, snap the cursor to it — hovering and "being selected" become
    // the same thing for placed units, so there's exactly one way to land
    // on them instead of two (roster cycling vs. manually walking the
    // cursor over). The cursor only moves here; whether it's then allowed
    // to move freely again is decided by DeploymentSystem::isSelectionLocked()
    // at the point movement input is read.
    if (const DeploymentEntry *entry = m_deployment.selectedEntry())
    {
        if (const DeploymentEntry *placed = m_deployment.deployedEntryFor(entry->instanceId))
            m_cursor.setPosition(placed->position);
    }
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

BattleState::HumanTurnContext BattleState::makeHumanTurnContext()
{
    return HumanTurnContext{
        .controlMode = m_playerControlMode,
        .hudOpen = m_hud.isOpen(),
        .phase = m_humanTurnPhase,
        .cursor = m_cursor,
        .session = m_session,
        .reachableTiles = m_reachableTiles,
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
        .pendingAttack = m_pendingAttack,
        .pendingSkillId = m_pendingSkillId,
        .uiManager = m_uiManager,
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

        if (event.windowId == WindowId::BattleDeployment && event.type == UIEventType::ActionSelected)
        {
            const bool wasGrabbed = m_deployment.hasGrabbedUnit();
            if (event.actionId == ActionId::CyclePrev)
            {
                m_deployment.cycleSelection(-1);
                if (!wasGrabbed)
                    syncCursorToSelection();
                else
                    syncDeploymentPreviewUnits();
                refreshDeploymentWindow();
                continue;
            }
            if (event.actionId == ActionId::CycleNext)
            {
                m_deployment.cycleSelection(1);
                if (!wasGrabbed)
                    syncCursorToSelection();
                else
                    syncDeploymentPreviewUnits();
                refreshDeploymentWindow();
                continue;
            }
            if (event.actionId == ActionId::Accept)
            {
                const Vec2i cursorPos = m_cursor.getPosition();

                if (wasGrabbed)
                {
                    if (!m_deployment.isSpawnTile(cursorPos))
                    {
                        // TODO: play "impossible action" sound (or similar) here
                        continue;
                    }

                    // Occupied by one of YOUR OWN placed units: swap places
                    // instead of blocking — you end up holding whichever
                    // unit was there.
                    if (m_deployment.isOccupied(cursorPos))
                    {
                        if (m_deployment.swapGrabbedWithPlacedAt(cursorPos))
                        {
                            syncDeploymentPreviewUnits();
                            refreshDeploymentWindow();
                        }
                        continue;
                    }

                    if (m_deployment.placeGrabbed(cursorPos))
                    {
                        syncDeploymentPreviewUnits();
                        // Selection auto-advanced inside placeGrabbed(); snap
                        // the cursor if it now points at another placed unit.
                        syncCursorToSelection();
                        refreshDeploymentWindow();
                    }
                    continue;
                }

                // Not grabbing, hovering an enemy preview unit: open the
                // ActionId::Inspect submenu (kept as a submenu rather than instant,
                // since Accept on an enemy could grow more options later).
                if (m_hoveredUnit && m_hoveredUnit->getTeam() != 0)
                {
                    openUnitInspectMenu(m_hoveredUnit);
                    continue;
                }

                // A placed unit under the cursor — checked by TILE, not by
                // selectedEntry(), so walking the cursor onto a placed unit
                // (without QE-selecting it) behaves the same as QE landing on it.
                if (const DeploymentEntry *placedHere = m_deployment.deployedEntryAt(cursorPos))
                {
                    const int instanceId = placedHere->instanceId;
                    if (m_deployment.unplaceUnit(instanceId) &&
                        m_deployment.grabUnit(instanceId))
                    {
                        syncDeploymentPreviewUnits();
                        refreshDeploymentWindow();
                    }
                    continue;
                }

                const DeploymentEntry *selected = m_deployment.selectedEntry();
                if (!selected)
                    continue;

                // Already placed (the cursor is pinned to it, per
                // isSelectionLocked): pick it back up so it can be relocated.
                if (m_deployment.isUnitPlaced(selected->instanceId))
                {
                    if (m_deployment.unplaceUnit(selected->instanceId) && m_deployment.grabUnit(selected->instanceId))
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
            if (event.actionId == ActionId::Details)
            {
                // Details (Tab): inspect, in priority order —
                //   1. Whatever's grabbed in hand (if anything).
                //   2. Whatever's hovered under the cursor (ours or enemy preview).
                //   3. Whatever's QE-selected in the roster (even unplaced —
                //      stats read straight from its template).
                if (const DeploymentEntry *grabbed = m_deployment.grabbedEntry())
                {
                    showInspectWindowFromTemplate(grabbed->templatePath);
                    continue;
                }

                if (m_hoveredUnit && !m_hoveredUnit->isDead())
                {
                    showInspectWindow(m_hoveredUnit);
                    continue;
                }

                if (const DeploymentEntry *selected = m_deployment.selectedEntry())
                    showInspectWindowFromTemplate(selected->templatePath);
                continue;
            }
            if (event.actionId == ActionId::Back)
            {
                if (wasGrabbed)
                {
                    m_deployment.releaseGrabbed();
                    refreshDeploymentWindow();
                    continue;
                }
                showSystemMenu();
                continue;
            }
            if (event.actionId == ActionId::StartCombat)
            {
                if (!m_deployment.canStartBattle())
                    continue;

                m_uiManager.popById(WindowId::BattleDeploymentConfirm);
                auto *confirm = m_uiManager.push<ConfirmWindow>(WindowId::BattleDeploymentConfirm);
                confirm->setFont(FontManager::instance().get(FontRole::Body));
                confirm->setPrompt("Start Battle?");
                continue;
            }
        }

        if (event.windowId == WindowId::BattleInspectMenu)
        {
            if (event.type == UIEventType::ActionSelected && event.actionId == ActionId::Inspect)
            {
                m_uiManager.popById(WindowId::BattleInspectMenu);
                if (m_inspectTargetUnit && !m_inspectTargetUnit->isDead())
                    showInspectWindow(m_inspectTargetUnit);
                continue;
            }

            if (event.type == UIEventType::ActionCanceled)
            {
                m_uiManager.popById(WindowId::BattleInspectMenu);
                m_inspectTargetUnit = nullptr;
                continue;
            }
        }

        if (m_flowPhase == BattleFlowPhase::Deployment && event.windowId == WindowId::BattleActionMenu)
        {
            if (event.type == UIEventType::ActionSelected)
            {
                const int index = event.index;
                if (index < 0 || index >= static_cast<int>(m_hud.items().size()))
                    continue;

                BattleMenuItem item = m_hud.items()[index];
                // hideById, not popById: the action menu is a persistent
                // window owned by BattleHud (see BattleHud::m_menu) — popById
                // would destroy it out from under BattleHud's cached pointer.
                m_uiManager.hideById(WindowId::BattleActionMenu);
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

        if (event.windowId == WindowId::BattleInspect && event.type == UIEventType::ActionCanceled)
        {
            m_uiManager.popById(WindowId::BattleInspect);
            m_inspectWindow = nullptr;
            continue;
        }

        if (event.windowId == WindowId::BattleDeploymentConfirm && event.type == UIEventType::ConfirmResult)
        {
            m_uiManager.popById(WindowId::BattleDeploymentConfirm);
            if (event.confirmed)
                startCombatPhase();
            continue;
        }

        if (m_flowPhase != BattleFlowPhase::Combat)
            continue;

        if (event.windowId == WindowId::BattleActionMenu && event.type == UIEventType::ActionSelected)
        {
            const int index = event.index;
            if (index < 0 || index >= static_cast<int>(m_hud.items().size()))
                continue;

            BattleMenuItem item = m_hud.items()[index];
            // Same reasoning as the Deployment branch above: hide, don't pop.
            m_uiManager.hideById(WindowId::BattleActionMenu);
            if (item.enabled && item.onSelect)
                item.onSelect();
            // The Accept press that selected this menu item is still "live"
            // for the rest of this frame — without consuming it here, the
            // very next input pass (HumanTurnController::handleActiveTurn,
            // called right after this in the same handleInput() call) sees
            // the same edge and can immediately fire whatever new phase we
            // just entered (e.g. AoE targeting the cursor's current tile
            // before the player ever pressed Accept themselves).
            Input::instance().consumeKey(KeyCode::Accept);
            continue;
        }

        if (event.windowId == WindowId::BattleSkillMenu && event.type == UIEventType::ActionSelected)
        {
            const int index = event.index;
            if (index < 0 || index >= static_cast<int>(m_skillMenuItems.size()))
                continue;

            BattleMenuItem item = m_skillMenuItems[index];
            if (item.enabled && item.onSelect)
                item.onSelect();
            Input::instance().consumeKey(KeyCode::Accept);
            continue;
        }

        if (event.windowId == WindowId::BattleSkillMenu && event.type == UIEventType::ActionCanceled)
        {
            // Pop just this level — battle.actionmenu underneath was never
            // touched, so it's revealed exactly as it was. No flag needed:
            // this IS the stack working correctly.
            m_uiManager.popById(WindowId::BattleSkillMenu);
            continue;
        }

        if (event.windowId == WindowId::BattleActionMenu && event.type == UIEventType::ActionCanceled)
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

        if (event.windowId == WindowId::BattleActionConfirm && event.type == UIEventType::NavigatePrevious)
        {
            cyclePendingAttackTarget(-1);
            updatePendingAttackPreview(active);
            if (Unit *focus = m_pendingAttack.focusedTarget())
                m_cursor.setPosition(focus->getPosition());
            continue;
        }

        if (event.windowId == WindowId::BattleActionConfirm && event.type == UIEventType::NavigateNext)
        {
            cyclePendingAttackTarget(1);
            updatePendingAttackPreview(active);
            if (Unit *focus = m_pendingAttack.focusedTarget())
                m_cursor.setPosition(focus->getPosition());
            continue;
        }

        if (event.windowId == WindowId::BattleActionConfirm && event.type == UIEventType::ConfirmResult)
        {
            m_uiManager.popById(WindowId::BattleActionConfirm);

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
    m_pendingAttack.begin(active, targetPos, directTarget, skill, m_session.getUnitPtrs());
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

void BattleState::beginAttackResolution(Unit *active, const SkillData *skill)
{
    if (!active)
    {
        finishAttackResolution();
        return;
    }

    // Roll every target's hit/damage NOW, up front — deterministic for the
    // rest of the sequence, rather than re-rolling mid-animation later.
    m_pendingHitResults.clear();
    m_pendingHitIndex = 0;
    m_pendingAnyDied = false;

    for (Unit *u : m_pendingAttack.targets())
    {
        if (!u || u->isDead())
            continue;
        HitContext ctx = makeHitContext(active, u, skill);
        m_pendingHitResults.push_back(PendingHitResult{.target = u, .result = CombatSystem::resolve(ctx)});
    }

    if (skill && skill->castOncePerArea)
    {
        // TODO: play the skill's single cast animation once here (e.g. a
        // dragon's breath), covering the whole area — before any individual
        // hit/miss is shown. Something like:
        //   m_combatAnimations.enqueue(skill->id + "_cast_once", castDuration);
    }

    m_pendingResolution = PendingResolution::ResolvingHits;
    m_turnTimer = 0.4f; // TODO: tune per-hit pacing, maybe per-skill
}

void BattleState::processNextPendingResult()
{
    if (m_pendingHitIndex >= m_pendingHitResults.size())
    {
        finishAttackResolution();
        return;
    }

    PendingHitResult &entry = m_pendingHitResults[m_pendingHitIndex];
    m_session.applyDamage(*entry.target, entry.result);

    const SkillData *skill = nullptr;
    if (!m_pendingSkillId.empty())
    {
        auto it = m_skillDB.find(m_pendingSkillId);
        if (it != m_skillDB.end())
            skill = &it->second;
    }
    const bool castOnce = skill && skill->castOncePerArea;

    if (entry.result.hit)
    {
        if (entry.target->isDead())
            m_pendingAnyDied = true;

        m_floatingText.enqueue(std::to_string(entry.result.damage), entry.target->getPosition(),
                               Color{255, 90, 90, 255});

        // TODO: play per-target cast animation here if !castOnce (e.g. a
        // mage's fire bolt jumping target to target), plus a hit-impact
        // animation on entry.target regardless of castOnce:
        //   if (!castOnce) m_combatAnimations.enqueue(skill->id + "_cast", ...);
        //   m_combatAnimations.enqueue("hit_impact", ...);

        LOG_INFO("Battle", "Attack hits %s for %d damage!",
                 entry.target->getName().c_str(), entry.result.damage);
    }
    else
    {
        m_floatingText.enqueue("MISS", entry.target->getPosition(), Color{200, 200, 200, 255});

        // TODO: play a dodge/evade animation on entry.target:
        //   m_combatAnimations.enqueue("dodge", ...);

        LOG_INFO("Battle", "Attack misses %s", entry.target->getName().c_str());
    }

    ++m_pendingHitIndex;
    m_turnTimer = 0.4f; // TODO: tune per-hit pacing; stays in WaitingForAnimation
}

void BattleState::finishAttackResolution()
{
    Unit *active = m_pendingActor ? m_pendingActor : m_session.getCurrentUnit();

    const SkillData *skill = nullptr;
    if (!m_pendingSkillId.empty())
    {
        auto it = m_skillDB.find(m_pendingSkillId);
        if (it != m_skillDB.end())
            skill = &it->second;
    }

    if (active)
    {
        if (skill)
            active->setCurrentMp(active->getCurrentMp() - skill->mpCost);
        active->setMajorAction(skill ? MajorAction::Skill : MajorAction::Attack);
    }

    m_canUndoLastMove = false;
    m_selectedSkillId.clear();
    m_pendingSkillId.clear();

    m_damagePreview.hide();
    m_pendingAttack.clear();
    m_topBattleText.clear();
    m_pendingHitResults.clear();
    m_pendingHitIndex = 0;

    m_session.checkResult();

    if (m_pendingAnyDied)
        m_eventSystem.emit(BattleTriggerType::OnUnitDeath);
    m_pendingAnyDied = false;

    m_pendingResolution = PendingResolution::None;
    m_pendingActor = nullptr;
    m_pendingTarget = nullptr;
    m_pendingActionLabel.clear();
    m_turnState = TurnState::ProcessingTurn;

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

    m_hud.clear();
    m_humanTurnPhase = HumanTurnPhase::ActionMenu;
    if (active)
        openBattleMenu(canActiveUnitMove(), false, true, KeyCode::Accept);
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

    static const std::vector<ForcedUnitRule> kNoForcedUnits;
    m_deployment.initialize(
        m_battleDefinition ? m_battleDefinition->maxUnits : 1,
        std::move(spawnTiles),
        partyCtx.party().memberIds(),
        partyCtx.roster(),
        m_battleDefinition ? m_battleDefinition->forcedUnits : kNoForcedUnits);

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
    m_uiManager.popById(WindowId::BattleInspectMenu);

    for (Unit *u : m_deploymentPreviewUnits)
        delete u;
    m_deploymentPreviewUnits.clear();

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

        m_deploymentPreviewUnits.push_back(u);
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

        m_deploymentPreviewUnits.push_back(u);
        m_grid.getTile(pos).occupied = true;
    }
}

void BattleState::refreshDeploymentWindow()
{
    if (!m_deploymentWindow)
        return;

    const DeploymentEntry *selected = m_deployment.selectedEntry();
    const DeploymentEntry *grabbed = m_deployment.grabbedEntry();
    m_deploymentWindow->setSelectedUnitLabel(selected ? loadUnitDisplayName(selected->templatePath) : std::string());
    m_deploymentWindow->setGrabbedState(grabbed != nullptr, grabbed ? loadUnitDisplayName(grabbed->templatePath) : std::string());
    m_deploymentWindow->setDeploymentStatus(m_deployment.placedCount(), m_deployment.maxUnits(), m_deployment.canStartBattle());
}

void BattleState::startCombatPhase()
{
    if (m_flowPhase != BattleFlowPhase::Deployment || !m_battleDefinition)
        return;

    m_hoveredUnit = nullptr;
    m_inspectTargetUnit = nullptr;
    m_uiManager.popById(WindowId::BattleInspectMenu);

    // Tear down the preview list, not m_units — m_units is combat-only and
    // is about to be filled for the first time, right below, from
    // BattleParticipantsBuilder.
    for (int y = 0; y < m_grid.getHeight(); ++y)
    {
        for (int x = 0; x < m_grid.getWidth(); ++x)
            m_grid.getTile(Vec2i{x, y}).occupied = false;
    }

    std::vector<UnitSpawn> spawns = BattleParticipantsBuilder::build(*m_battleDefinition, m_battleMap, m_deployment);
    if (spawns.empty())
        return;

    m_session.init(spawns, m_battleDefinition->victoryRule, m_battleDefinition->defeatRule);

    for (Unit *u : m_session.getUnitPtrs())
    {
        if (u && m_grid.isValid(u->getPosition()))
            m_grid.getTile(u->getPosition()).occupied = true;
    }

    if (Unit *first = m_session.getCurrentUnit(); first)
        m_cursor.setPosition(first->getPosition());

    m_uiManager.popById(WindowId::BattleDeployment);
    m_uiManager.popById(WindowId::BattleDeploymentConfirm);
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
    // m_session owns its Units by value — no manual delete needed.
    for (Unit *u : m_deploymentPreviewUnits)
        delete u;
    m_deploymentPreviewUnits.clear();

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
        if (m_flowPhase == BattleFlowPhase::Deployment && m_deployment.hasGrabbedUnit())
        {
            m_deployment.releaseGrabbed();
            refreshDeploymentWindow();
            return;
        }

        if (m_flowPhase == BattleFlowPhase::Deployment)
        {
            showSystemMenu();
            return;
        }

        // Mid-action (targeting/moving): ESC cancels the action, not open the menu.
        if (m_humanTurnPhase == HumanTurnPhase::MoveTarget ||
            m_humanTurnPhase == HumanTurnPhase::AttackTarget)
        {
            cancelPendingAttack();
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
            Unit *active = m_session.getCurrentUnit();
            if (active && active->getTeam() == 0 && !active->isDead())
            {
                m_hud.clear();
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
        const bool menuOpen =
            m_uiManager.hasWindow(WindowId::BattleActionMenu) ||
            m_uiManager.hasWindow(WindowId::BattleDeploymentConfirm) ||
            m_uiManager.hasWindow(WindowId::BattleInspect) ||
            m_uiManager.hasWindow(WindowId::BattleInspectMenu) ||
            m_uiManager.hasWindow(WindowId::BattleDialog);

        // Cursor movement is locked while the roster selection is pinned to
        // an already-placed unit (see DeploymentSystem::isSelectionLocked) —
        // there's nothing to browse for until either that unit is grabbed
        // (freeing movement to relocate it) or a different, unplaced entry
        // is selected instead.
        if (!menuOpen && !m_deployment.isSelectionLocked())
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
                // If the hovered tile holds one of our placed units, route it
                // through the preview path so the red "X" placed-marker draws
                // (setSingle renders via UnitPortrait::render, which has no marker).
                if (const DeploymentEntry *placed = m_deployment.deployedEntryAt(cursorPos))
                    setUnitPanelPreviewFromEntry(placed);
                else
                    m_unitPanelWindow->setSingle(m_hoveredUnit, m_hoveredUnit->getTeam());
            }
            else
            {
                setUnitPanelPreviewFromEntry(m_deployment.selectedEntry());
            }
        }

        // Deployment has its own cursor/hover handling above (including the
        // isSelectionLocked() gate on movement) — it must not fall through
        // into the shared Combat code below, which calls m_cursor.update()
        // unconditionally and would advance the cursor a second time this
        // same frame, plus recompute m_hoveredUnit redundantly.
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
                if (!m_pendingSkillId.empty())
                {
                    auto it = m_skillDB.find(m_pendingSkillId);
                    if (it != m_skillDB.end())
                        skill = &it->second;
                }

                Unit *active = m_pendingActor ? m_pendingActor : m_session.getCurrentUnit();
                beginAttackResolution(active, skill);
                // beginAttackResolution sets m_pendingResolution = ResolvingHits
                // and its own m_turnTimer — stay in WaitingForAnimation, do
                // NOT fall through to ProcessingTurn yet.
                break;
            }

            if (m_pendingResolution == PendingResolution::ResolvingHits)
            {
                processNextPendingResult();
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
    m_hud.clear();
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
        std::unordered_set<Vec2i, Vec2iHash> visibleSpawnTiles;
        if (m_flowPhase == BattleFlowPhase::Deployment)
        {
            visibleSpawnTiles = m_deployment.visibleSpawnTiles();
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
            .units = (m_flowPhase == BattleFlowPhase::Deployment) ? m_deploymentPreviewUnits : m_session.getUnitPtrs(),
            .debugRenderer = m_debugRenderer,
            .showSpawnOverlays = (m_flowPhase == BattleFlowPhase::Deployment),
            .overlayMode = overlayMode,
            .overlayTiles = overlayTiles,
        };

        m_battleRenderer->drawScene(renderCtx);
    }

    if (m_flowPhase == BattleFlowPhase::Deployment && m_deployment.hasGrabbedUnit())
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

            const DeploymentEntry *grabbed = m_deployment.grabbedEntry();
            std::string letter;
            if (grabbed)
            {
                const std::string name = loadUnitDisplayName(grabbed->templatePath);
                if (!name.empty())
                    letter = std::string(1, name[0]);
            }
            UnitPortrait::drawPlaceholderSprite(m_renderer, FontManager::instance().get(FontRole::Body), Vec2f{cx, cy}, halfTW * 1.2f, 0, letter, 120);
        }
    }

    if (m_unitPanelWindow)
    {
        if (m_flowPhase != BattleFlowPhase::Deployment)
        {
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

                if (active && !active->isDead())
                {
                    if (target && !target->isDead())
                        m_unitPanelWindow->setDuel(active, target, target->getTeam() != 0);
                    else
                        m_unitPanelWindow->setSingle(active, active->getTeam());
                }
                else
                    m_unitPanelWindow->clearPanels();
            }
            else if (m_hoveredUnit)
                m_unitPanelWindow->setSingle(m_hoveredUnit, m_hoveredUnit->getTeam());
            else
                m_unitPanelWindow->clearPanels();
        }
    }

    if (m_flowPhase == BattleFlowPhase::Deployment)
    {
        const Font *font = FontManager::instance().get(FontRole::Body);
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
        if (const Font *font = FontManager::instance().get(FontRole::Body))
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

    // ── Damage preview (always drawn if visible) ──
    m_damagePreview.render(m_renderer, FontManager::instance().get(FontRole::Body));

    m_combatAnimations.render(m_renderer);

    // ── Debug overlay ──
    if (m_debugRenderer)
        m_debugRenderer->flush(m_renderer, renderCam);

    // UI is drawn last, on top of the battle scene, animations, and debug
    // overlay — otherwise unit sprites drawn after this point would paint
    // over any open menu (e.g. the Inspect window).
    if (m_inspectWindow)
    {
        // Dim everything behind the Inspect panel so it reads as a modal.
        m_renderer->setBlendMode(Renderer::BlendMode::Blend);
        m_renderer->setDrawColor(Color{0, 0, 0, 140});
        m_renderer->fillRect(Rectf{0.0f, 0.0f, GameConstants::VIEW_W, GameConstants::VIEW_H});
    }
    m_uiManager.render(m_renderer);

    // ── Screen transition (fade) ──
    m_transition.render(m_renderer, GameConstants::VIEW_W, GameConstants::VIEW_H);

    if (m_showDefeatOverlay)
    {
        m_renderer->setBlendMode(Renderer::BlendMode::Blend);
        m_renderer->setDrawColor(Color{92, 22, 28, 228});
        m_renderer->fillRect(Rectf{GameConstants::VIEW_W * 0.5f - 260.0f, GameConstants::VIEW_H * 0.5f - 90.0f, 520.0f, 180.0f});
        m_renderer->setDrawColor(Color{255, 120, 120, 255});
        m_renderer->drawRect(Rectf{GameConstants::VIEW_W * 0.5f - 260.0f, GameConstants::VIEW_H * 0.5f - 90.0f, 520.0f, 180.0f});

        if (const Font *font = FontManager::instance().get(FontRole::Body))
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

    if (m_showVictoryOverlay)
    {
        m_renderer->setBlendMode(Renderer::BlendMode::Blend);
        m_renderer->setDrawColor(Color{22, 92, 40, 228});
        m_renderer->fillRect(Rectf{GameConstants::VIEW_W * 0.5f - 260.0f, GameConstants::VIEW_H * 0.5f - 90.0f, 520.0f, 180.0f});
        m_renderer->setDrawColor(Color{120, 255, 150, 255});
        m_renderer->drawRect(Rectf{GameConstants::VIEW_W * 0.5f - 260.0f, GameConstants::VIEW_H * 0.5f - 90.0f, 520.0f, 180.0f});

        if (const Font *font = FontManager::instance().get(FontRole::Body))
        {
            const std::string victory = "VICTORY";
            const float victoryW = static_cast<float>(victory.size()) * 8.0f;
            m_renderer->renderText(font,
                                   victory,
                                   Vec2f{(GameConstants::VIEW_W - victoryW) * 0.5f, GameConstants::VIEW_H * 0.5f - 26.0f},
                                   Color{210, 255, 210, 255},
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
    const std::vector<Unit *> &units =
        (m_flowPhase == BattleFlowPhase::Deployment) ? m_deploymentPreviewUnits : m_session.getUnitPtrs();

    for (Unit *u : units)
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

    const bool isPlaced = m_deployment.isUnitPlaced(entry->instanceId);

    try
    {
        // Build the same effective stats a live Unit would have (race/gender
        // bonuses applied) instead of showing the raw template numbers —
        // otherwise the preview disagrees with what the unit actually has
        // once placed (e.g. a race's MP bonus wouldn't show up here).
        const UnitData templateData = UnitLoader::load(entry->templatePath);
        const RaceData &raceData = getRaceData(templateData.race);
        const GenderData &genderData = getGenderData(templateData.gender);
        const Unit previewUnit(templateData, raceData, genderData, Vec2i{0, 0});
        const UnitData &data = previewUnit.getData();
        m_unitPanelWindow->setPreview(data.name, data.level, data.maxHp, data.maxMp, false, isPlaced);
    }
    catch (...)
    {
        m_unitPanelWindow->setPreview("Unit " + std::to_string(entry->instanceId), 1, 1, 0, 0, isPlaced);
    }
}