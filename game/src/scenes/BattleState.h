#pragma once

#include "engine/scene/Scene.h"
#include "engine/effects/ScreenTransition.h"
#include "engine/data/TileMapData.h"
#include "engine/math/Rect.h"
#include "engine/renderer/Camera.h"
#include "engine/statemachine/StateMachine.h"
#include "engine/input/KeyCode.h"
#include "battle/controllers/HumanTurnController.h"
#include "battle/controllers/AttackResolutionController.h"
#include "battle/controllers/DeploymentPhaseController.h"
#include "battle/controllers/DialogueController.h"
#include "battle/controllers/MovementAnimationController.h"
#include "battle/combat/PendingAttackController.h"
#include "battle/controllers/BattleMenuController.h"
#include "battle/map/Grid.h"
#include "battle/map/BattleMap.h"
#include "battle/BattleSession.h"
#include "battle/map/MovementRange.h"
#include "battle/map/RangeRule.h"
#include "battle/map/Pathfinder.h"
#include "renderer/BattleRenderer.h"
#include "config/BattleCatalog.h"
#include "events/BattleEventSystem.h"
#include "battle/systems/CombatAnimationSystem.h"
#include "battle/systems/CombatVfxLibrary.h"
#include "ui/UIManager.h"
#include "ui/BattleMenuItem.h"
#include "ui/Cursor.h"
#include "ui/DamagePreview.h"
#include "ui/FloatingTextSystem.h"
#include "battle/ui/UnitPanelWindow.h"
#include "data/SkillLoader.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class Input;
class Renderer;
class DebugRenderer;
class Texture;
class Unit;
struct TileLayerData;
struct HitContext;
struct BattleMenuItem;

struct BattleRequest
{
    std::string mapPath;
    int returnWorldNodeId = 4;
    // Tutorial duel: launched straight after the narrator on a fresh game,
    // driven by contextual one-line tips and retried on defeat instead of
    // returning to the world map.
    bool tutorial = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// BattleState
// ─────────────────────────────────────────────────────────────────────────────

class BattleState : public Scene
{
public:
    // ── Construction & Scene interface ─────────────────────────────────────
    BattleState(StateMachine<Scene> &sm,
                Renderer *renderer,
                BattleRequest request,
                std::function<void(bool)> onBattleFinished);

    void onEnter() override;
    void onExit() override;
    void handleInput() override;
    void update(float dt) override;
    void render(float alpha) override;

    void processCurrentTurn();

    // Advances the timeline and hands control to the next unit, ending the
    // battle first if the advance produced a victory/defeat result. The single
    // "what happens when a unit's turn ends" tail used by every early-exit
    // path in processCurrentTurn().
    void endCurrentUnitTurn(float timeCost);
    void advanceToNextUnit();

    // ── Public enums ──────────────────────────────────────────────────────
    enum class PlayerControlMode
    {
        AI,
        Human
    };

    // Contextual one-line hints used by the fresh-game tutorial duel. Each is
    // shown once per battle, at the moment the player first does the thing it
    // explains (or right at the tutorial's start/deployment). Real battles
    // keep the static controls dialog instead.
    enum class TutorialTip
    {
        Deploy,
        Welcome,
        ActionMenu,
        Move,
        Attack,
        Defend,
        Wait,
        Skill,
    };

    enum class HumanTurnPhase
    {
        FreeCursor,
        ActionMenu,
        MoveTarget,
        AttackTarget,
        AttackConfirm,
        ChooseFacing,
        TurnEnded
    };

    // Public so AttackResolutionController/DeploymentPhaseController can
    // drive it via their Context structs — same reasoning as HumanTurnPhase.
    enum class TurnState
    {
        Idle,
        ProcessingTurn,
        WaitingForAnimation
    };

    // Public for the same reason as TurnState.
    enum class PendingResolution
    {
        None,
        PlayerConfirmedAttack,
        ResolvingHits,
        EnemyAction,
    };

    // Public so DeploymentPhaseController can flip this via DeploymentContext.
    enum class BattleFlowPhase
    {
        Deployment,
        Combat,
    };

    // ── Human turn controller interface ───────────────────────────────────
    //
    // TODO: Move this into HumanTurnController once BattleState no longer
    // exposes its internal state directly.
    struct HumanTurnContext
    {
        PlayerControlMode controlMode;
        HumanTurnPhase &phase;
        Cursor &cursor;
        BattleSession &session;
        std::unordered_set<Vec2i, Vec2iHash> &reachableTiles;
        std::unordered_map<Vec2i, int, Vec2iHash> &reachableCosts;
        Vec2i &moveStartPos;
        int &moveStartPointsLeft;
        Grid &grid;
        BattleMap &battleMap;
        bool &canUndoLastMove;
        BattleEventSystem &eventSystem;
        RangeRule currentRangeRule;
        std::string &selectedSkillId;
        const std::unordered_map<std::string, SkillData> &skillDB;
        DamagePreview &damagePreview;
        std::string &topBattleText;
        PendingAttackController &pendingAttack;
        std::string &pendingSkillId;
        UIManager &uiManager;
        TurnState &turnState;
    };

    HumanTurnContext makeHumanTurnContext();

    // ── Attack resolution controller interface ─────────────────────────────
    // Bundles the shared turn-flow state AttackResolutionController needs to
    // read/write. Extend this (not AttackResolutionController's own members)
    // when a future feature needs another piece of BattleState's shared
    // state during attack resolution.
    struct AttackResolutionContext
    {
        BattleSession &session;
        BattleEventSystem &eventSystem;
        Grid &grid;
        BattleMap &battleMap;
        DamagePreview &damagePreview;
        FloatingTextSystem &floatingText;
        CombatAnimationSystem &combatAnimations;
        Cursor &cursor;
        const std::unordered_map<std::string, SkillData> &skillDB;
        std::string &selectedSkillId;
        std::string &topBattleText;
        Unit *&hoveredUnit;
        HumanTurnPhase &humanTurnPhase;
        TurnState &turnState;
        PendingResolution &pendingResolution;
        float &turnTimer;
        bool &canUndoLastMove;
        Unit *&pendingActor;
        Unit *&pendingTarget;
        std::string &pendingActionLabel;
    };

    AttackResolutionContext makeAttackResolutionContext();

    // ── Deployment phase controller interface ──────────────────────────────
    // Mirrors HumanTurnContext/AttackResolutionContext — bundles the shared
    // state DeploymentPhaseController needs to read/write. Extend this (not
    // DeploymentPhaseController's own members) when a future feature needs
    // another piece of BattleState's shared state during deployment.
    struct DeploymentContext
    {
        BattleSession &session;
        BattleEventSystem &eventSystem;
        UIManager &uiManager;
        Cursor &cursor;
        Grid &grid;
        BattleMap &battleMap;
        const BattleDefinition *battleDefinition;
        Unit *&hoveredUnit;
        Unit *&inspectTargetUnit;
        UnitPanelWindow *unitPanelWindow;
        BattleFlowPhase &flowPhase;
        TurnState &turnState;
        PendingResolution &pendingResolution;
        Unit *&pendingActor;
        Unit *&pendingTarget;
        std::string &pendingActionLabel;
        std::string &topBattleText;
        Camera &camera;
        const TileMapData &mapData;
    };

    DeploymentContext makeDeploymentContext();

    // ── Battle menu controller interface ────────────────────────────────
    struct MenuContext
    {
        BattleSession &session;
        Grid &grid;
        BattleMap &battleMap;
        Cursor &cursor;
        std::unordered_set<Vec2i, Vec2iHash> &reachableTiles;
        std::unordered_map<Vec2i, int, Vec2iHash> &reachableCosts;
        RangeRule &currentRangeRule;
        UIManager &uiManager;
        HumanTurnPhase &humanTurnPhase;
        BattleFlowPhase &flowPhase;
        TurnState &turnState;
        const std::unordered_map<std::string, SkillData> &skillDB;
        std::string &selectedSkillId;
        Vec2i &moveStartPos;
        int &moveStartPointsLeft;
        bool &canUndoLastMove;
        Unit *&inspectTargetUnit;
        AttackResolutionController &attackResolution;
        DeploymentPhaseController &deploymentPhase;
        StateMachine<Scene> &sm;
        Renderer *renderer;
        std::string &topBattleText;
    };

    MenuContext makeMenuContext();

    void computeAttackRangeTiles();

    // Behaviors invoked by HumanTurnController / AttackResolutionController /
    // DeploymentPhaseController.
    void openBattleMenu(bool canMove, bool canAttack, bool canWait, KeyCode trigger);
    BattleMenuController &battleMenu() { return m_battleMenu; }
    void beginUnitWalk(Unit *unit, Vec2i dest, int pathCost, std::function<void()> onComplete);
    MovementAnimationController &movementAnimation() { return m_movementAnimation; }
    bool canActiveUnitMove() const;
    HitContext makeHitContext(Unit *attacker, Unit *target, const SkillData *skill) const;
    // The shared basic-attack skill every unit uses (from the skill DB, seeded
    // in code so it is always present). Handed to the AI so enemies resolve
    // their attacks through the exact same skill pipeline as the player.
    // Returned by value: the fallback is a temporary and must not be bound to
    // a reference.
    SkillData basicAttackSkill() const;
    // True when a learned skill (not the shared basic attack) is the current
    // selection — decides whether Esc from the targeting phase returns to the
    // Skills submenu or the action menu.
    bool isLearnedSkillSelected() const;
    void preparePendingAttack(Unit *active,
                              Vec2i targetPos,
                              Unit *directTarget,
                              const SkillData *skill);

    // Brief red flash for a refused cast (see m_invalidFlashTimer).
    // Public so HumanTurnController can trigger it when a locked tile turns out
    // to catch nobody.
    void flashInvalidTarget();

    // Battle-end queries/trigger — public so the controllers above (and the
    // EnemyAction path already in this file) can call them.
    // Single place where the battle result is evaluated against the board.
    // Called from the only two moments that can resolve a battle: after damage
    // (inside BattleSession::applyDamage) and at turn start.
    void evaluateBattleResult();
    void startBattleEnd(bool playerWon);
    void updatePendingBattleEnd();
    bool checkVictory() const;
    bool checkDefeat() const;

    void maybeShowBattleControlsTutorial();

    // Tutorial support: shows a one-line contextual tip (tutorial duels only),
    // and lets the system-menu Quit bail out of a tutorial instead of
    // entering the retry-on-defeat loop. Returns true when a tip dialog was
    // actually pushed, so callers can hold back work until it is dismissed.
    bool maybeShowTutorialTip(TutorialTip tip);
    void quitFromSystemMenu();

private:
#ifdef _DEBUG
    // Autoplay state machine for player-team units (debug only).
    enum class AutoPlayPhase
    {
        Idle,
        ShowMenu,
        MenuOpen,
        ExecuteAction
    };
#endif

    // ── Core engine references ─────────────────────────────────────────────
    StateMachine<Scene> &m_sm;
    Renderer *m_renderer = nullptr;
    DebugRenderer *m_debugRenderer = nullptr;
    std::unique_ptr<BattleRenderer> m_battleRenderer;
    Texture *m_tileset = nullptr;
    // Owns the built-in combat VFX textures and keeps them registered with
    // m_combatAnimations. Populated wholesale by CombatVfxLibrary::load() and
    // released in onExit().
    CombatVfxLibrary m_combatVfx;

    // ── Map data ───────────────────────────────────────────────────────────
    TileMapData m_mapData;
    BattleMap m_battleMap;
    BattleRequest m_request;
    std::function<void(bool)> m_onBattleFinished;

    FColor m_bgTop{10.0f / 255.0f, 18.0f / 255.0f, 55.0f / 255.0f, 1.0f};
    FColor m_bgBottom{25.0f / 255.0f, 60.0f / 255.0f, 120.0f / 255.0f, 1.0f};

    // ── Gameplay systems ───────────────────────────────────────────────────
    Grid m_grid{};
    BattleSession m_session; // owns all COMBAT units + the turn queue
    std::unordered_map<std::string, SkillData> m_skillDB;
    const BattleDefinition *m_battleDefinition = nullptr;
    BattleEventSystem m_eventSystem;
    int m_pendingRewardXp = 0;
    bool m_rewardXpAwarded = false;

    // Snapshot of the last victory reward grant, used by renderEndOverlay()
    // to summarize earnings. Computed once in applyVictoryRewards().
    int m_rewardGold = 0;
    int m_rewardXpEach = 0;
    std::vector<std::string> m_rewardDropNames;
    std::vector<int> m_rewardDropCounts;
    std::vector<std::string> m_rewardLevelUps;

    // ── Move undo ──────────────────────────────────────────────────────────
    Vec2i m_moveStartPos{0, 0};
    int m_moveStartPointsLeft = 0;
    bool m_canUndoLastMove = false;

    std::unordered_set<Vec2i, Vec2iHash> m_reachableTiles;
    std::unordered_map<Vec2i, int, Vec2iHash> m_reachableCosts;
    std::unordered_set<Vec2i, Vec2iHash> m_attackCenterTiles;
    RangeRule m_currentRangeRule;

    bool m_playerWon = false;
    bool m_showDefeatOverlay = false;
    bool m_showVictoryOverlay = false;

    // Battle end is deferred until the last damage number / hit ring has
    // finished, so the finishing blow's VFX are never cut off by the overlay.
    bool m_battleEndPending = false;
    bool m_battleEndPendingWin = false;

    // Full-screen fade-out before leaving battle
    bool m_battleEndFading = false;
    bool m_battleLeaveTutorialRetry = false;
    bool m_battleLeaveWin = false;

    ScreenTransition m_transition;

#ifdef _DEBUG
    AutoPlayPhase m_autoPlayPhase = AutoPlayPhase::Idle;
    float m_autoPlayTimer = 0.0f;
    int m_autoPlayActionIndex = 0;
#endif

    PlayerControlMode m_playerControlMode = PlayerControlMode::Human;
    HumanTurnPhase m_humanTurnPhase = HumanTurnPhase::FreeCursor;
    bool m_battleTutorialPending = true;
    // Tutorial-duel one-shot tip guards, reset in onEnter so a retry replays
    // them. m_tutorialQuitRequested lets the system menu escape the retry
    // loop (see quitFromSystemMenu()).
    bool m_tutorialQuitRequested = false;
    bool m_tutorialDeployTipShown = false;
    bool m_tipActionMenuShown = false;
    bool m_tipMoveShown = false;
    bool m_tipAttackShown = false;
    bool m_tipDefendShown = false;
    bool m_tipWaitShown = false;
    bool m_tipSkillShown = false;

    // Action menu held back while the ActionMenu tip explains it; the flags
    // replay the requested menu once that tip is dismissed (processUIEvents).
    bool m_openActionMenuAfterTip = false;
    bool m_tipMenuCanMove = false;
    bool m_tipMenuCanAttack = false;
    bool m_tipMenuCanWait = false;

    // ── Turn state ─────────────────────────────────────────────────────────
    TurnState m_turnState = TurnState::Idle;
    float m_turnTimer = 0.0f;

    BattleFlowPhase m_flowPhase = BattleFlowPhase::Deployment;

    PendingResolution m_pendingResolution = PendingResolution::None;
    Unit *m_pendingActor = nullptr;
    Unit *m_pendingTarget = nullptr;
    int m_pendingEnemyAliveBefore = 0;
    std::string m_pendingActionLabel;
    std::string m_topBattleText;
    // True while the enemy's rolled attack awaits its short VFX pause before
    // finishEnemyAction() runs (damage already applied + FX enqueued).
    bool m_enemyActionResolved = false;

    // ── Tileset layout ─────────────────────────────────────────────────────
    float m_texW = 0.0f;
    float m_texH = 0.0f;
    int m_tilesPerRow = 1;
    float m_spriteH = 0.0f;

    // ── Rendering ──────────────────────────────────────────────────────────
    int m_scale = 2;

    Camera m_camera;
    Camera m_previousCamera;

    // ── Invalid-target feedback ────────────────────────────────────────────
    // A refused cast (AoE splash that catches nobody, single-target skill with
    // no unit under the cursor) gets a brief red border vignette instead of
    // silently refusing, so the player knows the press registered.
    void renderInvalidFlashVignette() const;

    float m_invalidFlashTimer = 0.0f;

    Cursor m_cursor;
    float m_cursorHoverOffset = 50.0f;
    float m_cursorTriW = 8.0f;
    float m_cursorTriH = 20.0f;

    // ── UI ─────────────────────────────────────────────────────────────────
    UIManager m_uiManager;
    HumanTurnController m_humanTurn{*this};
    AttackResolutionController m_attackResolution{*this};
    DeploymentPhaseController m_deploymentPhase{*this};
    BattleMenuController m_battleMenu{*this};
    DialogueController m_dialogueController{*this};
    MovementAnimationController m_movementAnimation;

    UnitPanelWindow *m_unitPanelWindow = nullptr;

    DamagePreview m_damagePreview;
    Unit *m_hoveredUnit = nullptr;
    Unit *m_inspectTargetUnit = nullptr;

    std::string m_selectedSkillId;

    CombatAnimationSystem m_combatAnimations;

    FloatingTextSystem m_floatingText;

    // ── Render sub-steps (Seam #4 split) ────────────────────────────────────
    // Each does exactly what its old inline block in render() did — pure
    // decomposition, no behavior changes except renderEndOverlay(), which
    // now services both Defeat/Victory from one body (see .cpp).
    Camera buildInterpolatedCamera(float alpha) const;
    void renderSceneAndOverlays(const Camera &renderCam);
    void renderDeploymentGrabbedGhost(const Camera &renderCam);
    void syncUnitPanelWindow();
    void renderDeploymentHud();
    void renderTopBattleText();
    void renderWorldEffects(const Camera &renderCam);
    void renderNativeEffects();
    void renderUIStack();
    void renderEndOverlay(bool victory);

    void processUIEvents(Unit *active);

    void showDialogueFromEvent(const std::string &text);
    void spawnEnemyFromEvent(const std::string &templatePath);

    // Starts a multi-beat scripted sequence with per-beat camera retargeting
    // (see DialogueController). Distinct from showDialogueFromEvent(), which
    // stays the single-line system-message path used by existing
    // BattleEventSystem triggers. Called from BattleEventSystem::Callbacks
    // lambdas set up in onEnter(), same as showDialogueFromEvent.
    void startStoryDialogue(std::vector<DialogueBeat> beats, std::function<void()> onComplete = {});

    void finishEnemyAction();
    void resolveEnemyAttack(Unit &unit, Unit *target);
    void applyVictoryRewards();
};
