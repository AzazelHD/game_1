#pragma once

#include "engine/scene/Scene.h"
#include "engine/effects/ScreenTransition.h"
#include "engine/data/TileMapData.h"
#include "engine/math/Rect.h"
#include "engine/renderer/Camera.h"
#include "engine/statemachine/StateMachine.h"
#include "engine/input/KeyCode.h"
#include "states/HumanTurnController.h"
#include "battle/Grid.h"
#include "battle/BattleMap.h"
#include "battle/TurnQueue.h"
#include "battle/MovementRange.h"
#include "battle/PendingAttackController.h"
#include "renderer/BattleRenderer.h"
#include "config/BattleCatalog.h"
#include "events/BattleEventSystem.h"
#include "systems/DeploymentSystem.h"
#include "systems/CombatAnimationSystem.h"
#include "ui/UIManager.h"
#include "ui/BattleHud.h"
#include "ui/BattleMenuItem.h"
#include "ui/Cursor.h"
#include "ui/DamagePreview.h"
#include "ui/windows/DeploymentWindow.h"
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
        TurnQueue &turnQueue;
        std::vector<Unit *> &units;
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

    // Behaviors invoked by HumanTurnController.
    void openBattleMenu(bool canMove, bool canAttack, bool canWait, KeyCode trigger);
    bool canActiveUnitMove() const;
    void showInspectWindow(Unit *unit);
    HitContext makeHitContext(Unit *attacker, Unit *target, const SkillData *skill) const;
    void preparePendingAttack(Unit *active,
                              Vec2i targetPos,
                              Unit *directTarget,
                              const SkillData *skill);

private:
    // ── Private types ──────────────────────────────────────────────────────
    enum class TurnState
    {
        Idle,
        ProcessingTurn,
        WaitingForAnimation
    };

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

    enum class BattleFlowPhase
    {
        Deployment,
        Combat,
    };

    enum class PendingResolution
    {
        None,
        PlayerConfirmedAttack,
        EnemyAction,
    };

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
    std::vector<Unit *> m_units;
    std::unordered_map<std::string, SkillData> m_skillDB;
    TurnQueue m_turnQueue;
    const BattleDefinition *m_battleDefinition = nullptr;
    DeploymentSystem m_deployment;
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
    BattleHud m_hud{m_uiManager};
    HumanTurnController m_humanTurn{*this};

    UnitPanelWindow *m_unitPanelWindow = nullptr;
    DeploymentWindow *m_deploymentWindow = nullptr;
    class InspectWindow *m_inspectWindow = nullptr;

    DamagePreview m_damagePreview;
    Unit *m_hoveredUnit = nullptr;
    Unit *m_inspectTargetUnit = nullptr;

    std::string m_selectedSkillId;

    CombatAnimationSystem m_combatAnimations;

    PendingAttackController m_pendingAttack;
    std::string m_pendingSkillId;

    // ── Battle flow helpers ────────────────────────────────────────────────
    void startBattleEnd(bool playerWon);
    bool checkVictory() const;
    bool checkDefeat() const;
    int countAliveEnemies() const;
    int countAlivePlayers() const;

    void computeAttackRangeTiles();

    void showSkillMenu();
    void showBattleMenu(bool canMove, bool canAttack, bool canWait);
    void showSystemMenu();
    void showStatusMenu(Unit *unit);
    void showInspectWindowFromTemplate(const std::string &templatePath);

    void syncCursorToSelection();
    void openStatusMenu(Unit *unit);
    Unit *unitAt(Vec2i pos) const;
    void setUnitPanelPreviewFromEntry(const DeploymentEntry *entry);

    void processUIEvents(Unit *active);

    void initializeDeploymentPhase();
    void syncDeploymentPreviewUnits();
    void refreshDeploymentWindow();
    void startCombatPhase();

    void showDialogueFromEvent(const std::string &text);
    void spawnEnemyFromEvent(const std::string &templatePath);

    void cyclePendingAttackTarget(int delta);
    void updatePendingAttackPreview(Unit *active);
    void applyPendingAttack(Unit *active, const SkillData *skill);
    void cancelPendingAttack();
};