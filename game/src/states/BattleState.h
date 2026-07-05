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

#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>
#include <unordered_set>

// BattleState — main gameplay state.
//
// Owns both the visual map data (TileMapData) and the gameplay grid
// (BattleMap). These are deliberately separate:
//   - TileMapData  → rendering, tile GIDs, layer offsets, tilesets.
//   - BattleMap    → gameplay grid (height, terrain class, occupancy, spawns).
//
// Lifecycle:
//   onEnter()  — acquire renderer, load TileMapData, build BattleMap,
//                load tileset texture, compute sprite layout and map origin.
//   update(dt) — dispatches to EnemyAI or waits for player input based on
//                whose turn TurnQueue surfaces.
//   render()   — gradient background, then all visible map layers (painter order).
//   onExit()   — destroy tileset texture, clear BattleMap.

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
    friend class HumanTurnController;

public:
    BattleState(StateMachine<Scene> &sm,
                Renderer *renderer,
                BattleRequest request,
                std::function<void(bool)> onBattleFinished);

    // Scene interface
    void onEnter() override;
    void onExit() override;
    void handleInput() override;
    void update(float dt) override;
    void render(float alpha) override;
    void processCurrentTurn();
    void advanceToNextUnit();

private:
    // ── Core engine references ────────────────────────────────────────────
    StateMachine<Scene> &m_sm;
    Renderer *m_renderer = nullptr; // non-owning; managed by app lifetime
    DebugRenderer *m_debugRenderer = nullptr;
    std::unique_ptr<BattleRenderer> m_battleRenderer; // owns all battle rendering code
    Texture *m_tileset = nullptr;                     // owning; created in onEnter, freed in onExit

    // ── Map data ──────────────────────────────────────────────────────────
    TileMapData m_mapData; // visual/rendering data loaded from Tiled JSON
    BattleMap m_battleMap; // gameplay grid derived from TileMapData
    BattleRequest m_request;
    std::function<void(bool)> m_onBattleFinished;
    FColor m_bgTop{10.0f / 255.0f, 18.0f / 255.0f, 55.0f / 255.0f, 1.0f};
    FColor m_bgBottom{25.0f / 255.0f, 60.0f / 255.0f, 120.0f / 255.0f, 1.0f};

    // ── Gameplay systems ──────────────────────────────────────────────────
    Grid m_grid{};                                        // logical grid representation
    std::vector<Unit *> m_units;                          // all active units (player + enemies)
    std::unordered_map<std::string, SkillData> m_skillDB; // id -> skill
    TurnQueue m_turnQueue;                                // turn scheduler / CT system
    const BattleDefinition *m_battleDefinition = nullptr;
    DeploymentSystem m_deployment;
    BattleEventSystem m_eventSystem;
    int m_pendingRewardXp = 0;

    // ── Move-undo tracking ───────────────────────────────────────────────
    // Only the MOST RECENT move segment can be undone, and only until a
    // major action (attack/skill/defend) is taken — see Unit.h's
    // MajorAction docs for the full turn grammar this supports.
    Vec2i m_moveStartPos{0, 0};     // position before the last move segment
    int m_moveStartPointsLeft = 0;  // active->getMoveRangeLeft() before the last move segment
    bool m_canUndoLastMove = false; // true iff there's a move segment eligible for undo right now

    // Reachable tiles for the current move phase
    std::unordered_set<Vec2i, Vec2iHash> m_reachableTiles;
    // Reachable tiles for the current attack phase
    std::unordered_set<Vec2i, Vec2iHash> m_attackRangeTiles;

    // Effective attack range for the current attack (1 = basic, skill range otherwise)
    int m_currentAttackRange = 1;

    // Result of the battle used after exit transition
    bool m_playerWon = false;
    bool m_showDefeatOverlay = false;

    // Fullscreen transition system (supports multiple transition types)
    ScreenTransition m_transition;

    enum class TurnState
    {
        Idle,
        ProcessingTurn,
        WaitingForAnimation
    };

#ifdef _DEBUG
    // Autoplay state machine for player‑team units (debug only).
    enum class AutoPlayPhase
    {
        Idle,         // not autoplaying
        ShowMenu,     // preparing to open the menu
        MenuOpen,     // menu is visible, waiting to auto‑select
        ExecuteAction // menu closed, executing the AI turn
    };
    AutoPlayPhase m_autoPlayPhase = AutoPlayPhase::Idle;
    float m_autoPlayTimer = 0.0f;
    int m_autoPlayActionIndex = 0; // which option the AI chose (0=Move,1=Attack,2=Wait)
#endif

    // ── Player control mode ─────────────────────────────────────────────
    enum class PlayerControlMode
    {
        AI,   // current autoplay (debug only)
        Human // manual phased turn
    };
    PlayerControlMode m_playerControlMode = PlayerControlMode::Human;

    enum class HumanTurnPhase
    {
        FreeCursor,   // player moves cursor freely; Enter triggers context actions
        ActionMenu,   // Move/Attack/Wait menu
        MoveTarget,   // selecting destination tile
        AttackTarget, // selecting enemy to attack
        AttackConfirm,
        TurnEnded // (internal) used to signal that the turn is over and we should advance
    };

    HumanTurnPhase m_humanTurnPhase = HumanTurnPhase::FreeCursor;

    TurnState m_turnState = TurnState::Idle;
    float m_turnTimer = 0.0f;

    enum class BattleFlowPhase
    {
        Deployment,
        Combat,
    };
    BattleFlowPhase m_flowPhase = BattleFlowPhase::Deployment;

    enum class PendingResolution
    {
        None,
        PlayerConfirmedAttack,
        EnemyAction,
    };

    PendingResolution m_pendingResolution = PendingResolution::None;
    Unit *m_pendingActor = nullptr;
    Unit *m_pendingTarget = nullptr;
    int m_pendingEnemyAliveBefore = 0;
    std::string m_pendingActionLabel;
    std::string m_topBattleText;

    // ── Tileset layout ────────────────────────────────────────────────────
    float m_texW = 0.0f;
    float m_texH = 0.0f;
    int m_tilesPerRow = 1;
    float m_spriteH = 0.0f;

    // ── Render scale ──────────────────────────────────────────────────────
    int m_scale = 2;

    // ── Camera ──────────────────────────────────────────────────────────
    Camera m_camera;         // current camera state (updated each frame)
    Camera m_previousCamera; // state from previous frame (for interpolation)

    // ── Cursor system ─────────────────────────────────────────────────────
    Cursor m_cursor;
    float m_cursorHoverOffset = 50.0f;
    float m_cursorTriW = 8.0f;  // logical px (scaled at render time)
    float m_cursorTriH = 20.0f; // logical px (scaled at render time)

    // ── Battle flow control ───────────────────────────────────────────────
    // Triggered when battle ends; handled via ScreenTransition lifecycle
    void startBattleEnd(bool playerWon);
    bool checkVictory() const;
    bool checkDefeat() const;
    int countAliveEnemies() const;
    int countAlivePlayers() const;

    // ── Range overlay rendering ───────────────────────────────────────────
    void computeAttackRangeTiles();

    // ── UI ────────────────────────────────────────────────────────────────
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

    // Pending attack confirmation state.
    PendingAttackController m_pendingAttack;
    std::string m_pendingSkillId;

    void showSkillMenu();
    void showBattleMenu(bool canMove, bool canAttack, bool canWait);
    void showSystemMenu();
    void showStatusMenu(Unit *unit);
    void showInspectWindow(Unit *unit);
    void openBattleMenu(bool canMove, bool canAttack, bool canWait, KeyCode trigger);
    void openStatusMenu(Unit *unit);
    bool canActiveUnitMove() const;
    Unit *unitAt(Vec2i pos) const;
    HitContext makeHitContext(Unit *attacker, Unit *target, const SkillData *skill) const;
    void setUnitPanelPreviewFromEntry(const DeploymentEntry *entry);

    void processUIEvents(Unit *active);
    void initializeDeploymentPhase();
    void syncDeploymentPreviewUnits();
    void refreshDeploymentWindow();
    void startCombatPhase();
    void showDialogueFromEvent(const std::string &text);
    void spawnEnemyFromEvent(const std::string &templatePath);

    void preparePendingAttack(Unit *active, Vec2i targetPos, Unit *directTarget, const SkillData *skill);
    void cyclePendingAttackTarget(int delta);
    void updatePendingAttackPreview(Unit *active);
    void applyPendingAttack(Unit *active, const SkillData *skill);
    void cancelPendingAttack();
};
