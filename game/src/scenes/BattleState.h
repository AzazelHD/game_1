#pragma once

#include "engine/scene/Scene.h"
#include "engine/effects/ScreenTransition.h"
#include "engine/data/TileMapData.h"
#include "engine/math/Rect.h"
#include "engine/renderer/Camera.h"
#include "engine/statemachine/StateMachine.h"
#include "engine/input/KeyCode.h"
#include "battle/HumanTurnController.h"
#include "battle/AttackResolutionController.h"
#include "battle/DeploymentPhaseController.h"
#include "battle/PendingAttackController.h"
#include "battle/BattleMenuController.h"
#include "battle/Grid.h"
#include "battle/BattleMap.h"
#include "battle/BattleSession.h"
#include "battle/MovementRange.h"
#include "renderer/BattleRenderer.h"
#include "config/BattleCatalog.h"
#include "events/BattleEventSystem.h"
#include "systems/CombatAnimationSystem.h"
#include "ui/UIManager.h"
#include "ui/BattleUIManager.h"
#include "ui/BattleMenuItem.h"
#include "ui/Cursor.h"
#include "ui/DamagePreview.h"
#include "ui/FloatingTextSystem.h"
#include "ui/windows/UnitPanelWindow.h"
#include "data/SkillLoader.h"

#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
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
    void advanceToNextUnit();

    // ── Public enums ──────────────────────────────────────────────────────
    enum class PlayerControlMode
    {
        AI,
        Human
    };

    enum class HumanTurnPhase
    {
        FreeCursor,
        ActionMenu,
        MoveTarget,
        AttackTarget,
        AttackConfirm,
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
        bool hudOpen;
        HumanTurnPhase &phase;
        Cursor &cursor;
        BattleSession &session;
        std::unordered_set<Vec2i, Vec2iHash> &reachableTiles;
        Vec2i &moveStartPos;
        int &moveStartPointsLeft;
        Grid &grid;
        bool &canUndoLastMove;
        BattleEventSystem &eventSystem;
        int currentAttackRange;
        std::string &selectedSkillId;
        const std::unordered_map<std::string, SkillData> &skillDB;
        DamagePreview &damagePreview;
        std::string &topBattleText;
        PendingAttackController &pendingAttack;
        std::string &pendingSkillId;
        UIManager &uiManager;
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
        BattleUIManager &hud;
        DamagePreview &damagePreview;
        FloatingTextSystem &floatingText;
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
        int &currentAttackRange;
        UIManager &uiManager;
        BattleUIManager &hud;
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
    };

    MenuContext makeMenuContext();

    void computeAttackRangeTiles();

    // Behaviors invoked by HumanTurnController / AttackResolutionController /
    // DeploymentPhaseController.
    void openBattleMenu(bool canMove, bool canAttack, bool canWait, KeyCode trigger);
    BattleMenuController &battleMenu() { return m_battleMenu; }
    bool canActiveUnitMove() const;
    HitContext makeHitContext(Unit *attacker, Unit *target, const SkillData *skill) const;
    void preparePendingAttack(Unit *active,
                              Vec2i targetPos,
                              Unit *directTarget,
                              const SkillData *skill);

    // Battle-end queries/trigger — public so the controllers above (and the
    // EnemyAction path already in this file) can call them.
    void startBattleEnd(bool playerWon);
    bool checkVictory() const;
    bool checkDefeat() const;

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

    // ── Move undo ──────────────────────────────────────────────────────────
    Vec2i m_moveStartPos{0, 0};
    int m_moveStartPointsLeft = 0;
    bool m_canUndoLastMove = false;

    std::unordered_set<Vec2i, Vec2iHash> m_reachableTiles;
    std::unordered_set<Vec2i, Vec2iHash> m_attackRangeTiles;
    int m_currentAttackRange = 1;

    bool m_playerWon = false;
    bool m_showDefeatOverlay = false;
    bool m_showVictoryOverlay = false;

    ScreenTransition m_transition;

#ifdef _DEBUG
    AutoPlayPhase m_autoPlayPhase = AutoPlayPhase::Idle;
    float m_autoPlayTimer = 0.0f;
    int m_autoPlayActionIndex = 0;
#endif

    PlayerControlMode m_playerControlMode = PlayerControlMode::Human;
    HumanTurnPhase m_humanTurnPhase = HumanTurnPhase::FreeCursor;

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

    // ── Tileset layout ─────────────────────────────────────────────────────
    float m_texW = 0.0f;
    float m_texH = 0.0f;
    int m_tilesPerRow = 1;
    float m_spriteH = 0.0f;

    // ── Rendering ──────────────────────────────────────────────────────────
    int m_scale = 2;

    Camera m_camera;
    Camera m_previousCamera;

    Cursor m_cursor;
    float m_cursorHoverOffset = 50.0f;
    float m_cursorTriW = 8.0f;
    float m_cursorTriH = 20.0f;

    // ── UI ─────────────────────────────────────────────────────────────────
    UIManager m_uiManager;
    BattleUIManager m_hud{m_uiManager};
    HumanTurnController m_humanTurn{*this};
    AttackResolutionController m_attackResolution{*this};
    DeploymentPhaseController m_deploymentPhase{*this};
    BattleMenuController m_battleMenu{*this};

    UnitPanelWindow *m_unitPanelWindow = nullptr;

    DamagePreview m_damagePreview;
    Unit *m_hoveredUnit = nullptr;
    Unit *m_inspectTargetUnit = nullptr;

    std::string m_selectedSkillId;

    CombatAnimationSystem m_combatAnimations;

    FloatingTextSystem m_floatingText;

    Unit *unitAt(Vec2i pos) const;

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
};