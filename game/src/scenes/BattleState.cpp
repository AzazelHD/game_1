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
#include "battle/controllers/EnemyTurnController.h"
#include "battle/map/MovementRange.h"
#include "battle/map/AttackRange.h"
#include "battle/map/Pathfinder.h"
#include "battle/combat/CombatSystem.h"
#include "battle/combat/SkillEffects.h"
#include "battle/systems/MovementCommitSystem.h"
#include "renderer/BattleRendererContext.h"
#include "renderer/BattleWorldProjection.h"
#include "ai/EnemyAI.h"
#include "systems/PartyContext.h"
#include "ui/Cursor.h"
#include "ui/UITheme.h"
#include "ui/UIScale.h"
#include "ui/UnitPortrait.h"
#include "ui/windows/ButtonMenuWindow.h"
#include "ui/windows/ConfirmWindow.h"
#include "ui/windows/DialogWindow.h"
#include "data/SkillLoader.h"
#include "data/UnitLoader.h"
#include "engine/ui/TextBlock.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

// Sprite sheet dimensions for 0.5H_IsoTiles.png.
//   Sprite size  : 48 × 36 px
//   Map tile size: 48 × 24 px
// Hardcoded because back-calculating from max-GID is fragile — the
// sheet format is fixed for this asset.

namespace
{
    constexpr float SPRITE_H = 36.0f;

    // Invalid-target feedback: a deliberately faint, quick hint that the press
    // did not work — enough to register, not enough to read as damage.
    constexpr float INVALID_FLASH_DURATION = 0.18f;
    // Peak opacity right at the screen border. Kept low so the flash is a soft
    // edge glow rather than a red wash.
    constexpr float INVALID_FLASH_ALPHA = 0.30f;
    // Border thickness as a fraction of the screen's short edge.
    constexpr float INVALID_FLASH_BAND = 0.05f;
    // Strips per border. SDL only interpolates linearly between vertices, so a
    // squared falloff needs a few levels to bend through rather than one ramp.
    constexpr int INVALID_FLASH_STRIPS = 3;
    constexpr Color INVALID_FLASH_COLOR = {220, 60, 60, 255};

    // Framed-layout backdrop + rail frame (see FRAMED_LAYOUT_PLAN.md).
    //
    // The map's own sky gradient is drawn across the WHOLE window (rails
    // included) so the battlefield reads as an island floating on the backdrop
    // rather than a panel between two bars. The rails are then a vignette +
    // hairline layered over that continuous sky — not an opaque panel — so the
    // frame derives from the theme instead of replacing it.
    //
    // Tier 2 parallax (drifting clouds/embers/snow) drops into renderBackdrop()
    // between the gradient and the world blit; nothing else has to move.
    const FColor kRailVignette = {0.0f, 0.0f, 0.0f, 0.55f};     // screen edge
    const FColor kRailVignetteSoft = {0.0f, 0.0f, 0.0f, 0.30f};  // top/bottom
    const FColor kRailVignetteNone = {0.0f, 0.0f, 0.0f, 0.0f};   // field edge
    constexpr float kRailTopBand = 96.0f;
    constexpr float kRailBottomBand = 120.0f;
    constexpr float kRailAccentW = 2.0f;

    void drawHorizontalGradient(Renderer *r, float x0, float x1, float y0, float y1,
                                const FColor &c0, const FColor &c1)
    {
        const std::vector<Renderer::Vertex> verts = {
            {{x0, y0}, c0}, {{x1, y0}, c1}, {{x1, y1}, c1}, {{x0, y1}, c0}};
        const std::vector<int> indices = {0, 1, 2, 0, 2, 3};
        r->drawGeometry(verts, indices);
    }

    void drawVerticalGradient(Renderer *r, float x0, float x1, float y0, float y1,
                              const FColor &c0, const FColor &c1)
    {
        const std::vector<Renderer::Vertex> verts = {
            {{x0, y0}, c0}, {{x1, y0}, c0}, {{x1, y1}, c1}, {{x0, y1}, c1}};
        const std::vector<int> indices = {0, 1, 2, 0, 2, 3};
        r->drawGeometry(verts, indices);
    }

    FColor mixColor(const FColor &a, const FColor &b, float t)
    {
        return FColor{a.r + (b.r - a.r) * t,
                      a.g + (b.g - a.g) * t,
                      a.b + (b.b - a.b) * t,
                      a.a + (b.a - a.a) * t};
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
    m_currentRangeRule = RangeRule{};
    m_session.init({});
    m_floatingText.clear();
    m_pendingRewardXp = 0;
    m_rewardXpAwarded = false;
    m_rewardGold = 0;
    m_rewardXpEach = 0;
    m_rewardDropNames.clear();
    m_rewardDropCounts.clear();
    m_rewardLevelUps.clear();
    m_flowPhase = BattleFlowPhase::Deployment;
    m_battleTutorialPending = true;
    m_battleEndPending = false;
    m_battleEndPendingWin = false;
    m_battleEndFading = false;
    m_battleLeaveTutorialRetry = false;
    m_battleLeaveWin = false;
    m_tutorialQuitRequested = false;
    m_tutorialDeployTipShown = false;
    m_tipActionMenuShown = false;
    m_tipMoveShown = false;
    m_tipAttackShown = false;
    m_tipDefendShown = false;
    m_tipWaitShown = false;
    m_tipSkillShown = false;
    m_openActionMenuAfterTip = false;
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
        Renderer::PresentationMode::Letterbox);

    // Framed layout: permanent side rails + a crisp play field. The design
    // canvas above stays 1280x720 for the UI/design transform, but the world
    // renders into a native-sized buffer at an integer art scale, so it stays
    // crisp at any window size. The rails are drawn natively on top of the
    // leftover. See FRAMED_LAYOUT_PLAN.md.
    const Vec2i outputSize = m_renderer->getOutputSize();
    m_layout = computeBattleLayout(outputSize.x, outputSize.y);
    m_scale = m_layout.artScale;
    m_renderer->setWorldBufferSize(m_layout.playRect.w, m_layout.playRect.h);
    const Vec2f playSize{static_cast<float>(m_layout.playRect.w),
                         static_cast<float>(m_layout.playRect.h)};

    // ── 3-6. Load map, build grid, load tileset, compute origin ──
    BattleLoadResult loaded = BattleLoader{m_renderer}.load(m_request.mapPath.c_str(), m_scale, SPRITE_H, playSize);
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

    // Built-in combat VFX (the skull today, and whatever the library grows
    // next). Loaded once here (the animation system only borrows the pointers)
    // and released in onExit(). Skills reference these by id from their
    // `visualOnHit`, so this is the only place combat animations get wired up.
    if (m_renderer)
        m_combatVfx.load(*m_renderer, m_combatAnimations);

    Vec2f origin = loaded.mapOrigin;

    m_camera.setTileSize(m_mapData.tileWidth, m_mapData.tileHeight);
    // The map's art reaches above its top anchor row (raised tiles + each tile's
    // own surface offset), and the height layers are all y-offsets: without this
    // the clamp treats the anchor box as the map and the top rows end up outside
    // the play field, while a fitting map is centred on its anchors instead of on
    // what is drawn.
    m_camera.setMapBoundsTopExtra(loaded.mapTopExtra);
    m_camera.setMapSize(m_mapData.width, m_mapData.height);
    m_camera.setViewportSize(playSize);
    m_camera.setRenderScale(static_cast<float>(m_scale));
    m_camera.setOffset(origin);
    m_worldZoom = m_battleDefinition->defaultZoom;
    applyWorldZoom();
    // Sprite overhang above tiles: unit sprites stand ~36px tall at up to
    // ~2.6x effective scale (~94px on screen), plus damage floaters. The
    // margin is screen pixels, so it holds at any zoom.
    m_camera.setMapBoundsMargin(112.0f);

    // Cast-beat glides read smoother with a touch more follow time than the
    // stock cursor chase; per-target snaps bypass easing entirely.
    m_camera.setFollowDuration(0.2f);

    // origin is a screen-space placement value, not a valid iso-space camera
    // offset — clamp immediately so the very first rendered frame already
    // sits inside valid bounds, instead of snapping there on the first
    // trackTarget()/clampToBounds() call later (visible as a hard "jump").
    // Frame: a map that fits the play field is centred on its art (so nothing
    // drawn above the top row is cut off); one that does not is clamped with the
    // sprite margin, exactly like the controller's edge-scroll expects.
    m_camera.clampToBounds(Camera::SmallMapFit::Frame);
    m_previousCamera = m_camera;

    // ── 7. Set up debug renderer ──
    m_debugRenderer = &DebugRenderer::instance();
    m_debugRenderer->setEnabled(true);

    // ── 8. Load skill database ─────────────────────────────────────────────
    // The basic attack is seeded in code first so the "Attack" button always
    // has a real skill to select; a basic_attack.json, if present, overrides it.
    m_skillDB[SkillData::makeBasicAttack().id] = SkillData::makeBasicAttack();
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

    // The opening tutorial duel skips deployment: the party is placed for the
    // player and combat starts immediately, so the facing picker (and its
    // repurposed Esc) is never shown before facing has been taught. Real
    // battles still use the full deployment phase.
    if (m_request.tutorial)
        m_deploymentPhase.autoDeployForTutorial();

    // Frame the board *before* the fade-in. The camera drive lives in update(),
    // which returns early while the transition plays, so without this the battle
    // fades in on the clamped map origin and then visibly jumps to the cursor
    // once control starts (worst on a big map, where the two are far apart).
    // Snap to the same tile the free-control glide would frame: the acting unit
    // when there is one (a restored mid-combat save), the cursor otherwise
    // (deployment).
    {
        const BattleCameraController::Context camCtx = makeCameraContext();
        const Vec2i focusTile = camCtx.anchorTile.x >= 0 ? camCtx.anchorTile : camCtx.cursorTile;
        m_battleCamera.focusImmediately(m_camera, playSize, camCtx.tileSize, camCtx.mapSize,
                                        camCtx.mapTopExtra, focusTile);
        m_previousCamera = m_camera;
    }

    // ── 9. Start fade‑in transition ──
    m_transition.start({
        .transition = ScreenTransitions::FadeIn,
        .duration = 0.5f,
        .easing = easeInOut,
    });
}

void BattleState::openBattleMenu(bool canMove, bool canAttack, bool canWait, KeyCode trigger)
{
    Input::instance().consumeKey(trigger);

    // Tutorial duels teach the action menu *before* it appears: the tip is a
    // blocking dialog, so hold the menu back and let processUIEvents() open it
    // once the tip is dismissed. Otherwise the menu would sit visible behind
    // the tip and the player would be reading the two together.
    if (maybeShowTutorialTip(TutorialTip::ActionMenu))
    {
        m_openActionMenuAfterTip = true;
        m_tipMenuCanMove = canMove;
        m_tipMenuCanAttack = canAttack;
        m_tipMenuCanWait = canWait;
        return;
    }

    m_battleMenu.showBattleMenu(canMove, canAttack, canWait);
}

void BattleState::beginUnitWalk(Unit *unit, Vec2i dest, int pathCost, std::function<void()> onComplete)
{
    if (!unit)
        return;

    MovementCommitResult result =
        MovementCommitSystem::commitMovement(m_session, m_grid, m_battleMap, unit, dest);
    if (!result.valid)
        return;

    const Vec2i start = unit->getPosition();

    // Commit the logical move NOW, instantly (grid occupancy, points spent)
    // — only the VISUAL playback is deferred. Undo (Back) reads this same
    // committed state, unaffected.
    m_grid.getTile(start).occupied = false;
    unit->setPosition(dest);
    m_grid.getTile(dest).occupied = true;
    unit->spendMovePoints(pathCost);
    m_canUndoLastMove = true;
    m_eventSystem.emit(BattleTriggerType::OnTileEnter);

    // Face the direction of travel from the last committed path segment.
    if (result.path.size() >= 2)
    {
        const Vec2i prev = result.path[result.path.size() - 2];
        const Vec2i last = result.path.back();
        unit->setFacing(isoDirectionForTileDelta(last - prev));
    }

    m_humanTurnPhase = HumanTurnPhase::ActionMenu;

    if (result.teleport)
    {
        if (onComplete)
            onComplete();
        return;
    }

    std::vector<int> heights;
    heights.reserve(result.path.size());
    for (const Vec2i &tile : result.path)
        heights.push_back(m_battleMap.at(tile.x, tile.y).height);

    m_movementAnimation.begin(unit, std::move(result.path), std::move(heights), std::move(onComplete));
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
        .battleMap = m_battleMap,
        .canUndoLastMove = m_canUndoLastMove,
        .eventSystem = m_eventSystem,
        .currentRangeRule = m_currentRangeRule,
        .selectedSkillId = m_selectedSkillId,
        .skillDB = m_skillDB,
        .damagePreview = m_damagePreview,
        .topBattleText = m_topBattleText,
        .pendingAttack = m_attackResolution.pendingAttack(),
        .pendingSkillId = m_attackResolution.pendingSkillId(),
        .uiManager = m_uiManager,
        .turnState = m_turnState,
    };
}

BattleState::AttackResolutionContext BattleState::makeAttackResolutionContext()
{
    return AttackResolutionContext{
        .session = m_session,
        .eventSystem = m_eventSystem,
        .grid = m_grid,
        .battleMap = m_battleMap,
        .damagePreview = m_damagePreview,
        .floatingText = m_floatingText,
        .combatAnimations = m_combatAnimations,
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

void BattleState::updateCamera(float dt)
{
    // ── Camera phase machine ─────────────────────────────────────────────────
    // Entering a resolution starts the cast beat (glide to the caster); the
    // enemy's applied strike promotes it to the per-target track. Player hits
    // promote it inside the WaitingForAnimation switch below, right as the
    // first consequence's effect plays. Anything else returns to free follow.
    const bool resolving = m_pendingResolution == PendingResolution::ResolvingHits ||
                           m_pendingResolution == PendingResolution::EnemyAction;
    if (resolving)
    {
        if (m_cameraPhase == BattleCameraController::Phase::Free)
            m_cameraPhase = BattleCameraController::Phase::ResolutionCast;
        if (m_pendingResolution == PendingResolution::EnemyAction && m_enemyActionResolved &&
            m_cameraPhase == BattleCameraController::Phase::ResolutionCast)
            m_cameraPhase = BattleCameraController::Phase::ResolutionTrack;
    }
    else
    {
        m_cameraPhase = BattleCameraController::Phase::Free;
    }

    const bool cameraSnapped = m_battleCamera.update(
        dt, m_camera,
        m_camera.getViewportSize(),
        makeCameraContext());
    if (cameraSnapped)
        m_previousCamera = m_camera; // hard cut: no this-frame render lerp
}

BattleCameraController::Context BattleState::makeCameraContext()
{
    BattleCameraController::Context ctx;

    ctx.inCombat = m_flowPhase == BattleFlowPhase::Combat;
    ctx.phase = m_cameraPhase;
    ctx.dialogueActive = m_dialogueController.isActive();
    if (Unit *speaker = m_dialogueController.currentSpeaker())
        ctx.speakerTile = speaker->getPosition();
    ctx.cursorTile = m_cursor.getPosition();
    ctx.anchorTile = Vec2i{-1, -1};
    ctx.turnSerial = m_turnSerial;
    if (Unit *active = m_session.getCurrentUnit())
        ctx.anchorTile = active->getPosition();
    ctx.tileSize = Vec2f{static_cast<float>(m_mapData.tileWidth),
                         static_cast<float>(m_mapData.tileHeight)};
    ctx.mapSize = Vec2i{m_mapData.width, m_mapData.height};
    ctx.mapTopExtra = m_camera.getMapBoundsTopExtra();

    if (ctx.phase == BattleCameraController::Phase::ResolutionCast ||
        ctx.phase == BattleCameraController::Phase::ResolutionTrack)
    {
        if (Unit *caster = m_pendingActor ? m_pendingActor : m_session.getCurrentUnit())
            ctx.casterTile = caster->getPosition();

        if (ctx.phase == BattleCameraController::Phase::ResolutionTrack)
        {
            Vec2i focus = ctx.casterTile;
            if (m_pendingResolution == PendingResolution::ResolvingHits)
            {
                if (Unit *hit = m_attackResolution.currentHitTarget())
                    focus = hit->getPosition();
            }
            else if (m_pendingResolution == PendingResolution::EnemyAction &&
                     m_enemyActionResolved && m_pendingTarget)
            {
                focus = m_pendingTarget->getPosition();
            }
            ctx.focusTile = focus;

            const SkillData *skill = nullptr;
            if (!m_attackResolution.pendingSkillId().empty())
            {
                auto it = m_skillDB.find(m_attackResolution.pendingSkillId());
                if (it != m_skillDB.end())
                    skill = &it->second;
            }
            ctx.stayOnCaster = skill && skill->castOncePerArea;
        }
    }

    return ctx;
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
        .currentRangeRule = m_currentRangeRule,
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
        .topBattleText = m_topBattleText,
        .tutorial = m_request.tutorial,
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
            m_dialogueController.notifyDialogFinished();
            m_uiManager.popById(WindowId::BattleDialog);

            // The Accept/Back press that advanced the dialog past its last beat
            // is still "live" for the rest of this frame. Whatever the dialog
            // was covering can read that same edge — most visibly the Wait
            // tip, which opens on top of ChooseFacing: dismissing the tip would
            // otherwise also confirm the facing the tip just told us to choose.
            // Same reasoning as consuming Accept after a menu item fires.
            Input::instance().consumeKey(KeyCode::Accept);
            Input::instance().consumeKey(KeyCode::Back);

            // The action menu was held back so the tip could explain it first
            // (see openBattleMenu); now that the tip is gone, show it.
            if (m_openActionMenuAfterTip)
            {
                m_openActionMenuAfterTip = false;
                m_battleMenu.showBattleMenu(m_tipMenuCanMove, m_tipMenuCanAttack, m_tipMenuCanWait);
            }
            continue;
        }

        if (m_deploymentPhase.handleUIEvent(event))
            continue;

        if (m_battleMenu.handleUIEvent(event, active))
            continue;

        if (event.type == UIEventType::ActionCanceled)
        {
            if (event.windowId == WindowId::BattleInspect ||
                event.windowId == WindowId::Equipment ||
                event.windowId == WindowId::PartyDetail)
            {
                m_uiManager.popById(event.windowId);
                continue;
            }
        }
    }
}

void BattleState::preparePendingAttack(Unit *active, Vec2i targetPos, Unit *directTarget, const SkillData *skill)
{
    m_attackResolution.prepare(active, targetPos, directTarget, skill);
}

void BattleState::flashInvalidTarget()
{
    m_invalidFlashTimer = INVALID_FLASH_DURATION;
}

void BattleState::maybeShowBattleControlsTutorial()
{
    PartyContext &ctx = PartyContext::instance();
    if (ctx.battleControlsTutorialShown())
        return;
    if (m_playerControlMode != PlayerControlMode::Human)
        return;

    ctx.markBattleControlsTutorialShown();

    // Tutorial duels don't dump a static block of controls at the player —
    // they teach contextually via maybeShowTutorialTip() as the player acts.
    if (m_request.tutorial)
    {
        maybeShowTutorialTip(TutorialTip::Welcome);
        return;
    }

    std::vector<DialogWindow::Line> lines;
    lines.push_back(DialogWindow::Line{.speaker = "Controls", .text = "Move the cursor: WASD / Arrow keys.  Open the action menu for the active unit: Accept (Enter / Space)."});
    lines.push_back(DialogWindow::Line{.speaker = "Controls", .text = "Menu options: Move, Attack, Skills, Defend, Wait.  Confirm choices: Accept.  Cancel / back out: Esc."});
    lines.push_back(DialogWindow::Line{.speaker = "Controls", .text = "Inspect any unit on the field with Details (Tab).  Defend or Wait ends your turn."});

    auto *dialog = m_uiManager.push<DialogWindow>(WindowId::BattleDialog);
    dialog->setFont(FontManager::instance().get(FontRole::Body));
    dialog->start(std::move(lines));
}

bool BattleState::maybeShowTutorialTip(TutorialTip tip)
{
    if (!m_request.tutorial)
        return false;

    const char *text = nullptr;
    switch (tip)
    {
    case TutorialTip::Deploy:
        if (m_tutorialDeployTipShown)
            return false;
        m_tutorialDeployTipShown = true;
        text = "Deployment — place your unit on a highlighted tile, then press Esc and choose Start Battle.";
        break;
    case TutorialTip::Welcome:
        // Deliberately does NOT say "press Enter on your unit" — discovering that
        // is the point of the first real turn. The enemy opens the duel, so by
        // the time control arrives the player already knows which unit is theirs.
        text = "Follow the tips — they appear as you act. Move the cursor with the arrow keys, and dismiss each tip with Enter or Space.";
        break;
    case TutorialTip::ActionMenu:
        if (m_tipActionMenuShown)
            return false;
        m_tipActionMenuShown = true;
        text = "Your unit's turn. Pick one: Move, Attack, Skills, Defend, Wait. Accept confirms, Esc backs out.";
        break;
    case TutorialTip::Move:
        if (m_tipMoveShown)
            return false;
        m_tipMoveShown = true;
        text = "Move — any highlighted tile is in range. Accept walks there; opening the menu undoes a move.";
        break;
    case TutorialTip::Attack:
        if (m_tipAttackShown)
            return false;
        m_tipAttackShown = true;
        text = "Attack — the highlighted area is your reach. Advance onto an enemy and confirm the strike.";
        break;
    case TutorialTip::Defend:
        if (m_tipDefendShown)
            return false;
        m_tipDefendShown = true;
        text = "Defend — your unit hunkers down and takes far less damage until its next turn.";
        break;
    case TutorialTip::Wait:
        if (m_tipWaitShown)
            return false;
        m_tipWaitShown = true;
        text = "Wait — end your turn. Choose a facing with the arrows, then confirm with Accept.";
        break;
    case TutorialTip::Skill:
        if (m_tipSkillShown)
            return false;
        m_tipSkillShown = true;
        text = "Skills — special moves that cost MP. Pick one, aim like an attack, and confirm.";
        break;
    }

    if (!text)
        return false;

    m_uiManager.popById(WindowId::BattleDialog);
    auto *dialog = m_uiManager.push<DialogWindow>(WindowId::BattleDialog);
    dialog->setFont(FontManager::instance().get(FontRole::Body));
    // A tutorial tip is the only thing on screen: nothing underneath it renders,
    // so a unit-detail window still open from deployment can't show a portrait
    // behind the text.
    dialog->setBlockRenderBelow(true);
    dialog->start({DialogWindow::Line{.speaker = "Tutorial", .text = text}});
    return true;
}

void BattleState::startStoryDialogue(std::vector<DialogueBeat> beats, std::function<void()> onComplete)
{
    m_uiManager.popById(WindowId::BattleDialog);
    auto *dialog = m_uiManager.push<DialogWindow>(WindowId::BattleDialog);
    dialog->setFont(FontManager::instance().get(FontRole::Body));
    m_dialogueController.begin(dialog, std::move(beats), std::move(onComplete));
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
    m_combatAnimations.clear();
    m_combatVfx.unload();

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
        // Fading out: input is locked; the onComplete handler swaps the scene.
        if (m_battleEndFading)
            return;

        if (input.isKeyPressed(KeyCode::Accept, false) ||
            input.isKeyPressed(KeyCode::Back, false) ||
            input.isKeyPressed(KeyCode::Advance, false))
        {
            const bool won = m_showVictoryOverlay;
            const bool tutorialRetry = !won && m_request.tutorial && !m_tutorialQuitRequested;
            m_battleLeaveWin = won;
            m_battleLeaveTutorialRetry = tutorialRetry;
            m_battleEndFading = true;

            // Take the panel down first so the fade-out dims the battlefield
            // itself, instead of leaving a bright VICTORY box sitting on top
            // of a darkening screen for the whole transition.
            m_showVictoryOverlay = false;
            m_showDefeatOverlay = false;
            m_transition.start({
                .transition = ScreenTransitions::FadeOut,
                .duration = 0.5f,
                .easing = easeInOut,
                .onComplete = [this]()
                {
                    m_battleEndFading = false;
                    if (m_battleLeaveTutorialRetry)
                    {
                        // Tutorial duels re-run until won so a fresh game can't
                        // strand the player at a loss. Use the system-menu Quit
                        // to skip back to the world map instead.
                        m_battleLeaveTutorialRetry = false;
                        m_sm.replace(std::make_unique<BattleState>(m_sm, m_renderer, m_request, m_onBattleFinished));
                    }
                    else if (m_onBattleFinished)
                        m_onBattleFinished(m_battleLeaveWin);
                    else
                        m_sm.replace(std::make_unique<MainMenuState>(m_sm, m_renderer, true));
                }});
        }
        return;
    }

    if (m_transition.isActive())
        return;

    // ── Cast confirmation ────────────────────────────────────────────────
    // The player has locked a tile: the splash is fixed and the preview box is
    // up. Q/E walk the focus through every unit caught in it so each one can
    // be read in turn, Accept casts, Back unlocks the tile again. The preview
    // box IS the confirmation — there is deliberately no confirm window here.
    if (m_flowPhase == BattleFlowPhase::Combat &&
        m_humanTurnPhase == HumanTurnPhase::AttackConfirm &&
        m_turnState == TurnState::ProcessingTurn)
    {
        Unit *actor = m_session.getCurrentUnit();

        if (input.isKeyPressed(KeyCode::Q, false) || input.isKeyPressed(KeyCode::E, false))
        {
            const int dir = input.isKeyPressed(KeyCode::E, false) ? 1 : -1;
            m_attackResolution.cycleFocus(dir);
            m_attackResolution.updatePreview(actor);
            Input::instance().consumeKey(KeyCode::Q);
            Input::instance().consumeKey(KeyCode::E);
            return;
        }

        if (input.isKeyPressed(KeyCode::Back, false))
        {
            // Undo the lock: put the cursor back on the blast centre the player
            // originally chose, so re-targeting starts from that tile instead
            // of wherever the last focused AoE target happened to be.
            const Vec2i center = m_attackResolution.pendingAttack().center();
            m_attackResolution.cancel();
            m_cursor.setPosition(center);
            m_damagePreview.hide();
            m_humanTurnPhase = HumanTurnPhase::AttackTarget;
            Input::instance().consumeKey(KeyCode::Back);
            return;
        }

        if (input.isKeyPressed(KeyCode::Accept, false))
        {
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
            m_pendingActor = actor;
            m_pendingTarget = m_attackResolution.pendingAttack().focusedTarget();
            m_pendingResolution = PendingResolution::PlayerConfirmedAttack;
            m_damagePreview.hide();
            m_turnTimer = 0.5f;
            m_turnState = TurnState::WaitingForAnimation;
            Input::instance().consumeKey(KeyCode::Accept);
            return;
        }
    }

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
            // nullptr before combat (empty queue) and after every unit is
            // gone — processUIEvents() treats "no active unit" as "no
            // tactical context", which is exactly right for those phases.
            Unit *active = m_session.getCurrentUnit();

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

        if (m_flowPhase == BattleFlowPhase::Deployment && m_deploymentPhase.isChoosingFacing())
        {
            // ESC during facing choice undoes the placement — no system menu.
            m_deploymentPhase.undoFacingChoice();
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
            const bool wasAttackTarget = (m_humanTurnPhase == HumanTurnPhase::AttackTarget);
            m_attackResolution.cancel();
            m_humanTurnPhase = HumanTurnPhase::ActionMenu;

            const Unit *active = m_session.getCurrentUnit();
            if (active)
                m_cursor.setPosition(active->getPosition());
            const bool canAttack = active && !active->hasActed();

            // "Regret" while aiming a learned skill: drop back to the skill
            // submenu. The basic attack is also aimed through the same phase,
            // but it has no submenu of its own, so it returns to the action
            // menu like it always did.
            const bool aimingLearnedSkill =
                wasAttackTarget && isLearnedSkillSelected();
            if (aimingLearnedSkill)
            {
                m_selectedSkillId.clear();
                m_battleMenu.showSkillMenu();
                Input::instance().consumeKey(KeyCode::Back);
            }
            else
            {
                openBattleMenu(canActiveUnitMove(), canAttack, true, KeyCode::Back);
            }
            return;
        }

        // Only in free-cursor mode does ESC open the system menu.
        if (m_humanTurnPhase == HumanTurnPhase::FreeCursor)
            m_battleMenu.showSystemMenu();
        // Wait's facing picker: Esc undoes Wait (back to the action menu)
        // while the unit still has something to do; with a spent turn there
        // is nothing to go back to, so Esc confirms instead. Handled here in
        // the Back block — alongside the Move/AttackTarget cancels — rather
        // than in tactical input, so the edge can't be eaten upstream.
        else if (m_humanTurnPhase == HumanTurnPhase::ChooseFacing)
        {
            Unit *pickerActive = m_session.getCurrentUnit();
            if (pickerActive && pickerActive->getTeam() == 0)
            {
                LOG_INFO("Battle", "ChooseFacing Back pressed (phase ok)");
                if (m_battleMenu.cancelWaitToMenu())
                    return;
                m_damagePreview.hide();
                m_topBattleText.clear();
                m_humanTurnPhase = HumanTurnPhase::ActionMenu;
                Input::instance().consumeKey(KeyCode::Back);
                advanceToNextUnit();
                m_turnState = TurnState::Idle;
            }
        }
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
                EnemyAI::takeTurn(*active, m_grid, m_battleMap, m_session.getUnitPtrs(),
                                  basicAttackSkill());
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
    // The window can be resized (or moved between window modes) mid-battle;
    // when the native output size changes, rebuild the framed layout so the
    // rails and the crisp integer world scaling stay correct.
    const Vec2i outputSize = m_renderer->getOutputSize();
    if (outputSize.x != m_layout.output.x || outputSize.y != m_layout.output.y)
        applyBattleLayout(computeBattleLayout(outputSize.x, outputSize.y));

    // Snapshot old state for precise camera rendering interpolation frames
    m_previousCamera = m_camera;

    m_uiManager.update(dt);
    m_movementAnimation.update(dt);
    m_combatAnimations.update(dt);
    m_floatingText.update(dt);

    // Invalid-target flash decay. Kept here rather than in the confirm path so
    // it plays out even while the cursor is being moved again.
    if (m_invalidFlashTimer > 0.0f)
        m_invalidFlashTimer = std::max(0.0f, m_invalidFlashTimer - dt);

    if (m_transition.isActive())
    {
        m_transition.update(dt);
        return;
    }

    // A won/lost battle only raises its overlay once the last damage number and
    // hit ring have finished. Runs before the overlay early-return so the flip
    // still happens on the frame the VFX drain.
    if (m_battleEndPending)
    {
        updatePendingBattleEnd();
        if (m_showVictoryOverlay || m_showDefeatOverlay)
            return;
    }

    // One-shot per battle (and once per new game via the PartyContext flag):
    // the first time combat really starts, explain the controls. The dialog
    // is a blocking window, so no tactical input runs until the player closes
    // it — see handleInput()'s hasBlockingWindow() guard.
    if (m_battleTutorialPending && m_flowPhase == BattleFlowPhase::Combat)
    {
        // Don't show unit panels before the tutorial dialog appears
        if (m_unitPanelWindow)
            m_unitPanelWindow->clearPanels();
        m_battleTutorialPending = false;
        maybeShowBattleControlsTutorial();
    }

    if (m_flowPhase == BattleFlowPhase::Deployment)
    {
        m_deploymentElapsed += dt;
        if (m_request.tutorial && !m_tutorialDeployTipShown)
            maybeShowTutorialTip(TutorialTip::Deploy);
        m_deploymentPhase.update(dt);
        // Deployment returns early, above the shared camera drive further down,
        // so it has to be driven here as well — otherwise nothing moves the
        // camera during placement and the cursor can walk off screen.
        updateCamera(dt);
        return;
    }

    if (m_showDefeatOverlay || m_showVictoryOverlay)
        return;

    // Still waiting on the finishing blow's VFX: freeze the turn machine and
    // input rather than letting the next unit start behind the pause.
    if (m_battleEndPending)
        return;

#ifdef _DEBUG
    if (m_autoPlayPhase == AutoPlayPhase::ShowMenu)
    {
        Unit *active = m_session.getCurrentUnit();
        if (active)
        {
            int action = m_autoPlayActionIndex;
            if (action == 0 || action == 1) // Move or Attack — let the AI do its thing
            {
                EnemyAI::takeTurn(*active, m_grid, m_battleMap, m_session.getUnitPtrs(),
                                  basicAttackSkill());
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
    // While choosing a facing the cursor is frozen (and hidden) so the arrow
    // keys exclusively drive the facing picker instead.
    // While a tile is locked in (cast confirmation) the cursor is frozen: the
    // splash is already built around that tile and Q/E move the preview focus
    // instead, so letting the arrow keys wander would only move a sprite that
    // no longer means anything.
    if (m_humanTurnPhase != HumanTurnPhase::ChooseFacing &&
        m_humanTurnPhase != HumanTurnPhase::AttackConfirm)
        m_cursor.update(m_battleMap.cols(), m_battleMap.rows(), dt);

    // Track unit under cursor
    Vec2i cursorPos = m_cursor.getPosition();

    // ── Camera ───────────────────────────────────────────────────────────────
    updateCamera(dt);

    m_hoveredUnit = unitAt(m_session.getUnitPtrs(), cursorPos);

    // The turn-state machine only applies to combat: before startCombatPhase()
    // there is no queue (getCurrentUnit() would return nullptr) and after it
    // the overlays below own the screen.
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
                // First processNextResult() call = the cast beat is over and
                // the first target's effect begins — from here the camera
                // snaps focus target-to-target.
                if (m_cameraPhase == BattleCameraController::Phase::ResolutionCast)
                    m_cameraPhase = BattleCameraController::Phase::ResolutionTrack;
                m_attackResolution.processNextResult();
                break;
            }

            if (m_pendingResolution == PendingResolution::EnemyAction)
            {
                Unit *active = m_pendingActor ? m_pendingActor : m_session.getCurrentUnit();
                if (!active || active->isDead())
                {
                    finishEnemyAction();
                    break;
                }

                // The rolled attack already played (damage + VFX + pause): now
                // just close the turn out.
                if (m_enemyActionResolved)
                {
                    m_enemyActionResolved = false;
                    finishEnemyAction();
                    break;
                }

                EnemyAI::EnemyTurnPlan plan = EnemyAI::planTurn(*active, m_grid, m_battleMap, m_session.getUnitPtrs());

                // Parked here (no-op case in the switch) until the walk (if
                // any) finishes and its onComplete callback below resumes
                // the turn — same pattern as the player's MoveTarget path.
                m_turnState = TurnState::ProcessingTurn;

                if (plan.wantsToMove)
                {
                    const int pathCost = active->getMoveRangeLeft(); // AI always exhausts its whole pool, same as before
                    beginUnitWalk(active, plan.destination, pathCost,
                                  [this, active, plan]()
                                  {
                                      resolveEnemyAttack(*active, plan.target);
                                  });
                }
                else
                {
                    active->exhaustMovement();
                    resolveEnemyAttack(*active, plan.target);
                }
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
    if (m_transition.isActive() || m_battleEndPending ||
        m_showVictoryOverlay || m_showDefeatOverlay)
        return;

    m_battleMenu.closeAllMenus();
    m_uiManager.popById(WindowId::BattleDialog);
    m_openActionMenuAfterTip = false;

    // Defer the overlay (and its rewards/event emission) until the queued
    // damage numbers and hit rings have played out — otherwise the victory
    // screen lands on top of the finishing blow's VFX and cuts them off.
    m_battleEndPending = true;
    m_battleEndPendingWin = playerWon;
    m_turnState = TurnState::Idle;
}

// Called every frame while a battle end is pending: flips the overlay on once
// no floating text / combat VFX are left in flight.
void BattleState::updatePendingBattleEnd()
{
    if (!m_battleEndPending)
        return;

    if (m_combatAnimations.isActive() || !m_floatingText.getEntries().empty())
        return;

    m_battleEndPending = false;

    if (m_battleEndPendingWin)
    {
        m_playerWon = true;
        m_showVictoryOverlay = true;
        applyVictoryRewards();
        m_eventSystem.emit(BattleTriggerType::OnVictory);
    }
    else
    {
        m_playerWon = false;
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

void BattleState::quitFromSystemMenu()
{
    // Marks a tutorial quit so the defeat-overlay dismiss skips the retry
    // loop and hands control back to the world map (see handleInput()).
    m_tutorialQuitRequested = true;
    startBattleEnd(false);
}

void BattleState::applyVictoryRewards()
{
    if (m_rewardXpAwarded)
        return;
    m_rewardXpAwarded = true;

    PartyContext &ctx = PartyContext::instance();
    ctx.ensureInitialized();

    // Currency — one grant per battle, added to the campaign purse.
    const int gold = m_battleDefinition ? m_battleDefinition->rewards.gold : 0;
    if (gold > 0)
        ctx.addGold(gold);
    m_rewardGold = gold;

    // Drops — one copy per declared gear key, resolved through the catalog.
    m_rewardDropNames.clear();
    m_rewardDropCounts.clear();
    if (m_battleDefinition)
    {
        for (const std::string &key : m_battleDefinition->rewards.items)
        {
            const Gear *gear = ctx.gearCatalog().findByKey(key);
            if (!gear)
            {
                LOG_WARN("Battle", "Reward gear key not in catalog, skipped: %s", key.c_str());
                continue;
            }
            if (!ctx.inventory().add(*gear, 1))
            {
                LOG_WARN("Battle", "Inventory full, dropped item lost: %s", key.c_str());
                continue;
            }

            bool counted = false;
            for (std::size_t i = 0; i < m_rewardDropNames.size(); ++i)
            {
                if (m_rewardDropNames[i] == gear->displayName)
                {
                    ++m_rewardDropCounts[i];
                    counted = true;
                    break;
                }
            }
            if (!counted)
            {
                m_rewardDropNames.push_back(gear->displayName);
                m_rewardDropCounts.push_back(1);
            }
        }
    }

    // XP + levels — only surviving player-team units.
    const int gained = GameConstants::BATTLE_VICTORY_XP + m_pendingRewardXp +
                       (m_battleDefinition ? m_battleDefinition->rewards.xp : 0);
    m_rewardXpEach = gained;
    m_rewardLevelUps.clear();
    for (const Unit &u : m_session.getUnits())
    {
        if (u.getTeam() != 0 || u.isDead())
            continue;
        const int rosterId = u.getRosterInstanceId();
        if (rosterId < 0)
            continue;
        if (RosterUnit *rosterUnit = ctx.roster().findById(rosterId))
        {
            // Progression rule: exp accumulates toward the next level on a
            // linear curve (100 * level). Crossing the threshold levels up
            // (surplus carries over) and grants one skill point (spent
            // permanently in the world-map Skills panel to unlock and equip
            // skills from the progression table).
            rosterUnit->exp += gained;
            const int maxLevel = u.getMaxLevel();
            if (rosterUnit->level < maxLevel)
            {
                while (rosterUnit->level < maxLevel &&
                       rosterUnit->exp >= expNeededForLevel(rosterUnit->level))
                {
                    rosterUnit->exp -= expNeededForLevel(rosterUnit->level);
                    ++rosterUnit->level;
                    ++rosterUnit->skillPoints;
                    m_rewardLevelUps.push_back(u.getName());
                }
                if (rosterUnit->level >= maxLevel)
                    rosterUnit->exp = 0;
            }
        }
    }
    m_pendingRewardXp = 0;
}

void BattleState::evaluateBattleResult()
{
    m_session.checkResult();
    if (checkDefeat())
        startBattleEnd(false);
    else if (checkVictory())
        startBattleEnd(true);
}

void BattleState::endCurrentUnitTurn(float timeCost)
{
    m_session.advanceTurn(timeCost);
    m_turnState = TurnState::Idle;

    evaluateBattleResult();
}

void BattleState::processCurrentTurn()
{
    // Turn-start evaluation. This is the trigger that resolves turn-based
    // objectives (e.g. SurviveTurns) which can be met without any unit dying.
    // Deaths already evaluated the result inside applyDamage(); re-checking is
    // idempotent and costs nothing.
    evaluateBattleResult();
    if (m_battleEndPending || m_showVictoryOverlay || m_showDefeatOverlay)
        return;

    Unit *active = m_session.getCurrentUnit();
    if (!active || active->isDead())
    {
        endCurrentUnitTurn(TurnQueue::scaledCost(
            TurnQueue::BASE_ACTION_COST,
            active ? active->getSpeed() : TurnQueue::kFallbackSpeed));
        return;
    }

    active->resetTurn();
    m_canUndoLastMove = false; // fresh turn — nothing to undo yet
    ++m_turnSerial;            // camera re-frames the acting unit once
    m_eventSystem.setTurn(m_session.m_turnsElapsed);
    m_eventSystem.emit(BattleTriggerType::OnTurnStart);

    LOG_INFO("Battle", "Active unit: %s, Team: %d, Position: (%d,%d)",
             active->getName().c_str(),
             active->getTeam(),
             active->getPosition().x,
             active->getPosition().y);

    // Status phase: DoT (poison/burn) ticks and turn locks (stun/sleep)
    // consume the unit's turn BEFORE it can act. A DoT kill or a skip both
    // end the turn through the same tail as a plain dead unit — the next
    // unit gets its turn, or the battle ends if that resolved it.
    const Unit::StatusTickResult statusTick = active->tickStatuses();
    if (statusTick.dotDamage > 0)
    {
        const StatusTickVisual visual = statusTickVisual(statusTick.dotType);
        m_floatingText.enqueue("-" + std::to_string(statusTick.dotDamage),
                               active->getPosition(), visual.floatColor);
        m_combatAnimations.enqueue(statusTick.dotType == StatusType::Burn ? "burn" : "poison",
                                   active->getPosition());
        LOG_INFO("Battle", "%s takes %d DoT damage from statuses",
                 active->getName().c_str(), statusTick.dotDamage);
    }

    if (active->isDead())
    {
        endCurrentUnitTurn(TurnQueue::scaledCost(TurnQueue::BASE_ACTION_COST,
                                                 active->getSpeed()));
        return;
    }

    if (statusTick.skippedTurn)
    {
        const char *label = statusTick.skipType == StatusType::Sleep ? "ASLEEP" : "STUNNED";
        m_floatingText.enqueue(label, active->getPosition(), Color{210, 180, 120, 255});
        m_combatAnimations.enqueue(statusTick.skipType == StatusType::Sleep ? "sleep" : "stun",
                                   active->getPosition());
        LOG_INFO("Battle", "%s is %s and loses its turn",
                 active->getName().c_str(), label);
        endCurrentUnitTurn(TurnQueue::scaledCost(TurnQueue::BASE_ACTION_COST,
                                                 active->getSpeed()));
        return;
    }

    if (active->getTeam() != 0) // Enemies
    {
        const EnemyDecision decision = EnemyTurnController::chooseAction(m_session, active);

        m_pendingResolution = PendingResolution::EnemyAction;
        m_pendingActor = active;
        m_pendingTarget = decision.target;
        m_pendingEnemyAliveBefore = decision.playersAliveBefore;
        m_enemyActionResolved = false;
        m_pendingActionLabel = "Attack";
        m_topBattleText = m_pendingActionLabel;

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
            int actionIndex = EnemyAI::chooseAction(*active, m_grid, m_battleMap, m_session.getUnitPtrs(),
                                                   basicAttackSkill());
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
            // The unit's menu opens immediately — no need to Enter on it
            // first. Esc from the fresh menu drops to the free cursor for
            // inspecting, Enter on the unit reopens it. Consuming Accept
            // swallows a press held over from the previous turn so it can't
            // instantly fire a menu row.
            m_cursor.setPosition(active->getPosition());
            m_humanTurnPhase = HumanTurnPhase::ActionMenu;
            openBattleMenu(canActiveUnitMove(), !active->hasActed(), true, KeyCode::Accept);
            m_turnState = TurnState::ProcessingTurn;
        }
    }
}

bool BattleState::isHumanTurn() const
{
    // Team 0 is the player's team (see BattleParticipantsBuilder::build).
    // An empty queue (pre-combat / post-battle) has no enemy turn to wait
    // for, so it reads as the player's own state and the cursor stays visible.
    const Unit *current = m_session.getCurrentUnit();
    return !current || current->getTeam() == 0;
}

void BattleState::advanceToNextUnit()
{
    m_eventSystem.setTurn(m_session.m_turnsElapsed);
    m_eventSystem.emit(BattleTriggerType::OnTurnEnd);

    Unit *active = m_session.getCurrentUnit();
    if (active)
        m_session.advanceTurn(TurnQueue::turnCostFor(*active));

    m_canUndoLastMove = false; // turn is over — nothing left to undo

    if (m_battleEndPending || m_showVictoryOverlay || m_showDefeatOverlay)
        return;
}

void BattleState::resolveEnemyAttack(Unit &unit, Unit *target)
{
    const EnemyAttackResult atk = EnemyAI::resolveAttack(unit, target, m_grid, m_battleMap, basicAttackSkill());

    m_pendingResolution = PendingResolution::EnemyAction;

    if (atk.attacked)
    {
        // Apply through the session so victory/defeat and death detection
        // stay in sync with the player's path.
        m_session.applyDamage(*atk.target, atk.result);

        if (atk.result.hit)
        {
            m_floatingText.enqueue(std::to_string(atk.result.damage), atk.target->getPosition(),
                                   Color{255, 90, 90, 255});
            m_combatAnimations.enqueue("hit_impact", atk.target->getPosition());
        }
        else
        {
            m_floatingText.enqueue("MISS", atk.target->getPosition(), Color{200, 200, 200, 255});
            m_combatAnimations.enqueue("dodge", atk.target->getPosition());
        }

        // Pause briefly so the damage number + ring are readable, then the
        // next WaitingForAnimation tick (with m_enemyActionResolved) closes
        // the turn.
        m_enemyActionResolved = true;
        m_turnTimer = 0.45f;
        m_turnState = TurnState::WaitingForAnimation;
        return;
    }

    // No legal target in range (or nothing to attack) — close the turn now.
    finishEnemyAction();
}

void BattleState::finishEnemyAction()
{
    const EnemyTurnResult result = EnemyTurnController::finishAction(m_session, m_pendingEnemyAliveBefore);

    if (result.unitDied)
        m_eventSystem.emit(BattleTriggerType::OnUnitDeath);

    m_pendingResolution = PendingResolution::None;
    m_pendingActor = nullptr;
    m_pendingTarget = nullptr;
    m_pendingActionLabel.clear();
    m_topBattleText.clear();

    if (result.outcome == EnemyTurnOutcome::PlayerDefeat)
    {
        startBattleEnd(false);
        m_turnState = TurnState::Idle;
        return;
    }
    if (result.outcome == EnemyTurnOutcome::PlayerVictory)
    {
        startBattleEnd(true);
        m_turnState = TurnState::Idle;
        return;
    }

    advanceToNextUnit();
    m_turnState = TurnState::Idle;
}

// ─────────────────────────────────────────────────────────────────────────────
// Render
// ─────────────────────────────────────────────────────────────────────────────

void BattleState::applyBattleLayout(const BattleLayout &layout)
{
    if (!m_renderer)
        return;

    m_layout = layout;
    m_scale = layout.artScale;

    // World buffer + camera viewport both follow the play field; artScale is
    // the integer world zoom (device px per source texel).
    m_renderer->setWorldBufferSize(m_layout.playRect.w, m_layout.playRect.h);

    const Vec2f playSize{static_cast<float>(m_layout.playRect.w),
                         static_cast<float>(m_layout.playRect.h)};
    m_camera.setViewportSize(playSize);
    m_camera.setRenderScale(static_cast<float>(m_scale));
    applyWorldZoom();
    m_camera.clampToBounds(Camera::SmallMapFit::Frame);

    // Fold the change into the render-interpolation snapshot so the resize
    // doesn't lerp across the old framing for one frame.
    m_previousCamera = m_camera;
}

void BattleState::applyWorldZoom()
{
    // artScale is floored to a whole number so the world buffer and the box
    // sizes baked into it stay integral; worldScale is the exact ratio the
    // window height implies. Folding worldScale/artScale into the camera zoom
    // keeps tiles the same size *relative to the window* on resolutions that sit
    // between art-scale steps (a 900p window is 1.25x a 720p one, so it draws
    // each tile 1.25x larger in device pixels instead of showing 25% more map).
    const float art = static_cast<float>(std::max(1, m_layout.artScale));
    m_camera.setZoom(m_worldZoom * m_layout.worldScale / art);
}

Camera BattleState::buildInterpolatedCamera(float alpha) const
{
    Vec2f interpOffset = m_previousCamera.getOffset() +
                         (m_camera.getOffset() - m_previousCamera.getOffset()) * alpha;

    float interpZoom = m_previousCamera.getZoom() +
                       (m_camera.getZoom() - m_previousCamera.getZoom()) * alpha;

    Camera renderCam = m_camera;
    renderCam.setOffset(interpOffset);
    renderCam.setZoom(interpZoom);

    // Pixel-perfect world pass. The battlefield is drawn into the fixed logical
    // canvas and sampled with NEAREST, so a fractional offset puts tile and
    // sprite edges between pixels — visible as seams between iso tiles and
    // jittering sprites as the camera moves. screen = (iso - offset) * z, and
    // iso coordinates are integers for this tile size, so snapping offset * z
    // to an integer makes every anchor land on a whole logical pixel.
    const float z = std::max(interpZoom, 0.0001f) * renderCam.getRenderScale();
    renderCam.setOffset(Vec2f{std::round(interpOffset.x * z) / z,
                              std::round(interpOffset.y * z) / z});
    return renderCam;
}

void BattleState::renderSceneAndOverlays(const Camera &renderCam)
{
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
        overlayTiles = &m_attackCenterTiles;
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
        .movementAnimation = &m_movementAnimation,
        .showSpawnOverlays = (m_flowPhase == BattleFlowPhase::Deployment),
        .overlayMode = overlayMode,
        .overlayTiles = overlayTiles,
        .facingChoiceUnit = nullptr,
        .facingChoiceSelection = IsoDirection::SouthWest,
        .hideCursor = !isHumanTurn() ||
                      (m_humanTurnPhase == HumanTurnPhase::ChooseFacing) ||
                      (m_flowPhase == BattleFlowPhase::Deployment && m_deploymentPhase.isChoosingFacing()),
    };

    if (m_flowPhase == BattleFlowPhase::Combat &&
        m_humanTurnPhase == HumanTurnPhase::ChooseFacing)
    {
        if (Unit *active = m_session.getCurrentUnit())
        {
            renderCtx.facingChoiceUnit = active;
            renderCtx.facingChoiceSelection = active->getFacing();
        }
    }
    else if (m_flowPhase == BattleFlowPhase::Deployment && m_deploymentPhase.isChoosingFacing())
    {
        renderCtx.facingChoiceUnit = m_deploymentPhase.facingChoiceUnit();
        renderCtx.facingChoiceSelection = m_deploymentPhase.facingChoiceSelection();
    }

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
    const Vec2f screenPos = renderCam.tileToScreen(cursorPos);
    const float ax = screenPos.x;
    const float ay = screenPos.y;
    const GameTile &gt = m_battleMap.at(cursorPos.x, cursorPos.y);
    const float elev = static_cast<float>(gt.height) * halfTH;

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
    const float breathe = 0.5f + 0.5f * std::sin(m_deploymentElapsed * 6.0f);
    const int alpha = 80 + static_cast<int>(breathe * 80.0f);
    UnitPortrait::drawPlaceholderSprite(m_renderer, FontManager::instance().get(FontRole::Body), Vec2f{cx, cy}, halfTW * 1.2f, 0, letter, alpha);
}

void BattleState::syncUnitPanelWindow()
{
    if (!m_unitPanelWindow || m_flowPhase == BattleFlowPhase::Deployment)
        return;

    // No unit panel behind a modal dialog/tip (tutorial welcome, controls
    // block, scripted dialogue) — it reads as a stray overlay floating over
    // the message. It reappears once the dialog is dismissed.
    if (m_uiManager.hasWindow(WindowId::BattleDialog))
    {
        m_unitPanelWindow->clearPanels();
        return;
    }

    // Combat has started but the first tutorial/controls dialog hasn't been
    // pushed yet (it goes up in update(), after the first render). Keep panels
    // hidden so no portraits flash for a frame before the message appears.
    if (m_battleTutorialPending && m_flowPhase == BattleFlowPhase::Combat)
    {
        m_unitPanelWindow->clearPanels();
        return;
    }

    Unit *active = nullptr;
    if (m_flowPhase == BattleFlowPhase::Combat)
        active = m_session.getCurrentUnit();

    // Enemy action: duel panels only once the strike has actually landed
    // (damage applied, pause playing). While the enemy is still walking into
    // position — or when no strike happened at all — show only the mover, so
    // no stray ally portrait appears mid-walk.
    if (m_pendingResolution == PendingResolution::EnemyAction && m_pendingActor)
    {
        if (m_enemyActionResolved && m_pendingTarget && !m_pendingTarget->isDead())
            m_unitPanelWindow->setDuel(m_pendingActor, m_pendingTarget, m_pendingTarget->getTeam() != 0);
        else
            m_unitPanelWindow->setSingle(m_pendingActor, m_pendingActor->getTeam());
        return;
    }

    if (m_pendingResolution != PendingResolution::None && m_pendingActor)
    {
        if (m_pendingTarget && !m_pendingTarget->isDead())
            m_unitPanelWindow->setDuel(m_pendingActor, m_pendingTarget, m_pendingTarget->getTeam() != 0);
        else
            m_unitPanelWindow->setSingle(m_pendingActor, m_pendingActor->getTeam());
        return;
    }

    if (m_playerControlMode == PlayerControlMode::Human &&
        m_humanTurnPhase == HumanTurnPhase::ActionMenu)
    {
        if (active && !active->isDead())
            m_unitPanelWindow->setSingle(active, active->getTeam());
        else
            m_unitPanelWindow->clearPanels();
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

    // Enemy turn: show only the acting enemy. The hovered-unit readout below
    // is for player turns; during the enemy's move it would show a stray ally
    // portrait alongside it. Caster+target duels are handled above.
    if (active && active->getTeam() != 0 && !active->isDead())
    {
        m_unitPanelWindow->setSingle(active, active->getTeam());
        return;
    }

    if (m_hoveredUnit)
        m_unitPanelWindow->setSingle(m_hoveredUnit, m_hoveredUnit->getTeam());
    else
        m_unitPanelWindow->clearPanels();
}

void BattleState::renderRails()
{
    const Recti &play = m_layout.playRect;
    const float outW = static_cast<float>(m_layout.output.x);
    const float outH = static_cast<float>(m_layout.output.y);
    const float railW = static_cast<float>(m_layout.railW);
    if (railW <= 0.0f || outH <= 0.0f)
        return;

    const float fieldL = static_cast<float>(play.x);
    const float fieldR = static_cast<float>(play.x + play.w);

    m_renderer->beginNativeOverlay();
    m_renderer->setBlendMode(Renderer::BlendMode::Blend);

    // Outer vignette over the continuous sky: darkest at the screen edge, clear
    // at the field boundary, so the island sits in the light.
    drawHorizontalGradient(m_renderer, 0.0f, fieldL, 0.0f, outH, kRailVignette, kRailVignetteNone);
    drawHorizontalGradient(m_renderer, outW, fieldR, 0.0f, outH, kRailVignette, kRailVignetteNone);
    // Gentle top/bottom falloff so the frame reads all the way around.
    drawVerticalGradient(m_renderer, fieldL, fieldR, 0.0f, kRailTopBand, kRailVignetteSoft, kRailVignetteNone);
    drawVerticalGradient(m_renderer, fieldL, fieldR, outH - kRailBottomBand, outH, kRailVignetteNone, kRailVignetteSoft);

    // Hairline where the rails meet the play field: a tinted version of the
    // theme's lower sky so the framed window reads as deliberate.
    FColor accent = mixColor(m_bgBottom, FColor{1.0f, 1.0f, 1.0f, 1.0f}, 0.5f);
    accent.a = 0.55f;
    m_renderer->setDrawColor(Color{static_cast<uint8_t>(accent.r * 255.0f),
                                   static_cast<uint8_t>(accent.g * 255.0f),
                                   static_cast<uint8_t>(accent.b * 255.0f),
                                   static_cast<uint8_t>(accent.a * 255.0f)});
    m_renderer->fillRect(Rectf{fieldL - kRailAccentW, 0.0f, kRailAccentW, outH});
    m_renderer->fillRect(Rectf{fieldR, 0.0f, kRailAccentW, outH});

    m_renderer->endNativeOverlay();
}

void BattleState::renderBackdrop()
{
    if (!m_battleRenderer || !m_renderer)
        return;

    const Vec2f window{static_cast<float>(m_layout.output.x),
                       static_cast<float>(m_layout.output.y)};
    if (window.x <= 0.0f || window.y <= 0.0f)
        return;

    // The map's sky spans the whole window, rails included, drawn natively
    // between endWorldPass() and blitWorldBuffer() so the world composites on
    // top of it. Tier 2 parallax layers go here, after the gradient.
    m_renderer->beginNativeOverlay();
    m_renderer->setBlendMode(Renderer::BlendMode::None);
    m_battleRenderer->drawBackground(m_bgTop, m_bgBottom, window);
    m_renderer->endNativeOverlay();
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

    const Font *font = FontManager::instance().get(FontRole::Body);
    if (!font)
        return;

    // Skill descriptions are long, so the top band wraps instead of running off
    // the screen. Wrapping and growing downward from the same y keeps the single
    // short preview lines (status floater banners) exactly where they were.
    // Sits below the confirm cascade (which is pinned to the top edge) so the
    // two never overlap while a target is being confirmed.
    constexpr float kTop = 72.0f;
    constexpr float kMaxW = static_cast<float>(GameConstants::VIEW_W) - 64.0f;
    constexpr float kLineSpacing = 2.0f;

    const TextBlock::Layout layout =
        TextBlock::measure(*m_renderer, {TextBlock::Line{font, m_topBattleText, UITheme::SelectedText}},
                           kMaxW, kLineSpacing);

    TextBlock::render(*m_renderer,
                      {TextBlock::Line{font, m_topBattleText, UITheme::SelectedText}},
                      Rectf{32.0f, kTop, kMaxW, layout.height()},
                      TextBlock::Anchor::TopLeft,
                      kMaxW,
                      kLineSpacing,
                      HorizontalAlign::Center);
}

void BattleState::renderWorldEffects(const Camera &renderCam)
{
    const battleworld::AnchorContext effectsCtx{
        .camera = renderCam,
        .mapData = m_mapData,
        .battleMap = m_battleMap,
        .scale = m_scale,
    };
    m_combatAnimations.render(m_renderer, effectsCtx);
    m_floatingText.render(m_renderer, FontManager::instance().get(FontRole::Body), effectsCtx);

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
    if (m_battleMenu.hasInspectWindowOpen() || m_uiManager.hasWindow(WindowId::BattleSystemMenu))
    {
        // Dim everything behind the open modal (inspect or system/pause menu)
        // so it reads as a modal overlay.
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

    const Font *font = FontManager::instance().get(FontRole::Body);
    if (!font)
        return;

    m_renderer->setBlendMode(Renderer::BlendMode::Blend);

    if (!victory)
    {
        const Rectf box{GameConstants::VIEW_W * 0.5f - 260.0f, GameConstants::VIEW_H * 0.5f - 90.0f, 520.0f, 180.0f};
        m_renderer->setDrawColor(bgColor);
        m_renderer->fillRect(box);
        m_renderer->setDrawColor(borderColor);
        m_renderer->drawRect(box);

        const float labelW = m_renderer->measureText(font, label).x;
        m_renderer->renderText(font,
                               label,
                               Vec2f{(GameConstants::VIEW_W - labelW) * 0.5f, box.y + 34.0f},
                               labelColor,
                               false,
                               false,
                               false);

        const std::string hint = "Press Enter/Space/Esc";
        const float hintW = m_renderer->measureText(font, hint).x;
        m_renderer->renderText(font,
                               hint,
                               Vec2f{(GameConstants::VIEW_W - hintW) * 0.5f, box.y + box.h - 40.0f},
                               UITheme::Text,
                               false,
                               false,
                               false);
        return;
    }

    // Victory: summarize what the battle paid out. Rows are built from the
    // reward snapshot captured in applyVictoryRewards().
    std::vector<std::string> rows;
    rows.push_back("Gold: +" + std::to_string(m_rewardGold));
    rows.push_back("XP: +" + std::to_string(m_rewardXpEach) + " each");
    for (const std::string &name : m_rewardLevelUps)
        rows.push_back("Lv up: " + name);
    if (!m_rewardLevelUps.empty())
        rows.push_back("(level-ups grant +1 skill point)");
    for (std::size_t i = 0; i < m_rewardDropNames.size(); ++i)
        rows.push_back("Drop: " + m_rewardDropNames[i] + " x" + std::to_string(m_rewardDropCounts[i]));

    constexpr float kRowH = 30.0f;
    constexpr float kPadV = 16.0f;
    constexpr float kTitleH = 40.0f;
    constexpr float kHintH = 40.0f;
    constexpr float kBoxMinH = 220.0f;
    constexpr float kBoxW = 520.0f;

    const float boxH = std::max(
        kBoxMinH, kPadV + kTitleH + static_cast<float>(rows.size()) * kRowH + kHintH + kPadV);
    const Rectf box{(GameConstants::VIEW_W - kBoxW) * 0.5f,
                    (GameConstants::VIEW_H - boxH) * 0.5f,
                    kBoxW,
                    boxH};

    m_renderer->setDrawColor(bgColor);
    m_renderer->fillRect(box);
    m_renderer->setDrawColor(borderColor);
    m_renderer->drawRect(box);

    const float titleW = m_renderer->measureText(font, label).x;
    m_renderer->renderText(font,
                           label,
                           Vec2f{(GameConstants::VIEW_W - titleW) * 0.5f, box.y + kPadV},
                           labelColor,
                           false,
                           false,
                           false);

    float rowY = box.y + kPadV + kTitleH;
    for (const std::string &row : rows)
    {
        m_renderer->renderText(font, row, Vec2f{box.x + 24.0f, rowY}, UITheme::Text, false, false, false);
        rowY += kRowH;
    }

    const std::string hint = "Press Enter/Space/Esc";
    const float hintW = m_renderer->measureText(font, hint).x;
    m_renderer->renderText(font,
                           hint,
                           Vec2f{(GameConstants::VIEW_W - hintW) * 0.5f, box.y + boxH - kHintH},
                           UITheme::Text,
                           false,
                           false,
                           false);
}

void BattleState::render(float alpha)
{
    if (!m_renderer)
        return;

    const Camera renderCam = buildInterpolatedCamera(alpha);

    m_renderer->beginLogicalPass();
    renderSceneAndOverlays(renderCam);
    renderDeploymentGrabbedGhost(renderCam);
    renderWorldEffects(renderCam);
    m_renderer->endWorldPass();

    // Full-window themed sky behind everything, then the world composited on
    // top of it, then the rail frame — the FFTA "island floating on the
    // backdrop" look (see FRAMED_LAYOUT_PLAN.md).
    renderBackdrop();
    m_renderer->blitWorldBuffer();
    renderRails();

    syncUnitPanelWindow();
    renderDeploymentHud();
    renderTopBattleText();
    renderNativeEffects();
    renderUIStack();

    if (m_showDefeatOverlay)
        renderEndOverlay(false);
    if (m_showVictoryOverlay)
        renderEndOverlay(true);

    // Fade layer draws last so it covers the whole screen — battlefield, UI
    // stack and the end overlay alike.
    m_transition.render(m_renderer, GameConstants::VIEW_W, GameConstants::VIEW_H);

    // Invalid-target flash sits above the fade: it is input feedback, not part
    // of the battle picture.
    renderInvalidFlashVignette();
}

void BattleState::renderInvalidFlashVignette() const
{
    if (m_invalidFlashTimer <= 0.0f || !m_renderer)
        return;

    const float w = static_cast<float>(GameConstants::VIEW_W);
    const float h = static_cast<float>(GameConstants::VIEW_H);

    // Band thickness tracks the short edge so it reads the same at any aspect
    // ratio; past it the screen is completely untouched.
    const float band = std::min(w, h) * INVALID_FLASH_BAND;

    const float t = m_invalidFlashTimer / INVALID_FLASH_DURATION;
    const float peak = INVALID_FLASH_ALPHA * t;
    const FColor rgb{INVALID_FLASH_COLOR.r / 255.0f,
                     INVALID_FLASH_COLOR.g / 255.0f,
                     INVALID_FLASH_COLOR.b / 255.0f,
                     1.0f};

    // One gradient band per screen border: full red at the edge, transparent
    // `band` inward, with a squared falloff so the colour hugs the frame
    // instead of washing toward the middle. SDL interpolates per-vertex alpha
    // across each triangle, so this needs no texture, shader or mask asset.
    //
    // Each band is origin + direction-along-the-edge + direction-inward. The
    // bands overlap in the corners and their alphas blend there, so corners end
    // up a touch darker — which is what a vignette should look like anyway.
    struct Band
    {
        Vec2f origin, along, inward;
    };
    const Band bands[4] = {
        {{0.0f, 0.0f}, {w, 0.0f}, {0.0f, band}}, // top
        {{w, 0.0f}, {0.0f, h}, {-band, 0.0f}},   // right
        {{w, h}, {-w, 0.0f}, {0.0f, -band}},     // bottom
        {{0.0f, h}, {0.0f, -h}, {band, 0.0f}},   // left
    };

    const int rows = INVALID_FLASH_STRIPS + 1;

    std::vector<Renderer::Vertex> verts;
    std::vector<int> indices;
    verts.reserve(static_cast<std::size_t>(4 * rows * 2));
    indices.reserve(static_cast<std::size_t>(4 * INVALID_FLASH_STRIPS * 6));

    for (const Band &b : bands)
    {
        const int base = static_cast<int>(verts.size());
        for (int s = 0; s < rows; ++s)
        {
            const float f = static_cast<float>(s) / static_cast<float>(INVALID_FLASH_STRIPS);
            const float alpha = peak * (1.0f - f) * (1.0f - f);
            const FColor c{rgb.r, rgb.g, rgb.b, alpha};
            const float ox = b.inward.x * f, oy = b.inward.y * f;
            verts.push_back(Renderer::Vertex{{b.origin.x + ox, b.origin.y + oy}, c});
            verts.push_back(Renderer::Vertex{{b.origin.x + b.along.x + ox,
                                              b.origin.y + b.along.y + oy}, c});
        }
        for (int s = 0; s < INVALID_FLASH_STRIPS; ++s)
        {
            const int a0 = base + s * 2, a1 = a0 + 1;
            const int b0 = a0 + 2, b1 = a0 + 3;
            indices.push_back(a0);
            indices.push_back(b0);
            indices.push_back(b1);
            indices.push_back(a0);
            indices.push_back(b1);
            indices.push_back(a1);
        }
    }

    const Renderer::BlendMode previous = m_renderer->getBlendMode();
    m_renderer->setBlendMode(Renderer::BlendMode::Blend);
    m_renderer->drawGeometry(verts, indices);
    m_renderer->setBlendMode(previous);
}

void BattleState::computeAttackRangeTiles()
{

    m_attackCenterTiles.clear();
    Unit *active = m_session.getCurrentUnit();
    if (!active)
        return;

    Vec2i pos = active->getPosition();

    AttackRange::Result rng = AttackRange::compute(m_grid, m_battleMap, pos, m_currentRangeRule);
    m_attackCenterTiles = std::move(rng.tiles);

    // Supportive skills (heal/buff) may be cast on the caster's own tile, so
    // keep `pos` highlighted for them; every other range keeps it excluded.
    bool supportive = false;
    if (!m_selectedSkillId.empty())
    {
        auto it = m_skillDB.find(m_selectedSkillId);
        if (it != m_skillDB.end())
            supportive = skill_effects::isSupportive(it->second);
    }
    if (!supportive)
        m_attackCenterTiles.erase(pos);
}

bool BattleState::isLearnedSkillSelected() const
{
    if (m_selectedSkillId.empty())
        return false;
    auto it = m_skillDB.find(m_selectedSkillId);
    return it != m_skillDB.end() && !it->second.isBasicAttack;
}

SkillData BattleState::basicAttackSkill() const
{
    auto it = m_skillDB.find(kBasicAttackSkillId);
    return it != m_skillDB.end() ? it->second : SkillData::makeBasicAttack();
}

HitContext BattleState::makeHitContext(Unit *attacker, Unit *target, const SkillData *skill) const
{
    // Every attack — including the basic attack, which is a real skill — goes
    // through the same skill -> HitContext builder the AI uses.
    HitContext ctx = skill ? CombatSystem::hitContextFor(*skill, *attacker, *target)
                           : CombatSystem::basicAttackContext(*attacker, *target);
    // Terrain: attacker's ground (grass_atk/grass_mag) and defender's ground
    // (grass_eva) both feed the fight. Shared by player, AI and preview.
    CombatSystem::applyTileBonuses(ctx, m_battleMap);
    return ctx;
}
