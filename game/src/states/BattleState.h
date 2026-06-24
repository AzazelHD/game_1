#pragma once

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

#include "engine/scene/Scene.h"
#include "engine/data/TileMapData.h"
#include "engine/renderer/Camera.h"
#include "engine/renderer/Texture.h"
#include "engine/renderer/DebugRenderer.h"
#include "engine/statemachine/StateMachine.h"
#include "engine/ui/MenuPanel.h"
#include "battle/BattleMap.h"
#include "battle/TurnQueue.h"
#include "battle/Unit.h"
#include "battle/Grid.h"
#include "ui/Cursor.h"
#include "ui/UnitPanel.h"
#include "ui/DamagePreview.h"

#include <memory>
#include <vector>
#include <cstdint>
#include <functional>

class Input;
class Renderer;
struct TileLayerData;

// ─────────────────────────────────────────────────────────────────────────────
// IsoMetrics
//
// All isometric / elevation constants derived from the current map + scale.
// Built once per frame in makeIsoMetrics() and passed by const-ref to every
// per-cell draw helper, so none of them recompute or duplicate these values.
// ─────────────────────────────────────────────────────────────────────────────
struct IsoMetrics
{
    float s;        // uniform scale factor
    float tw;       // scaled tile width
    float th;       // scaled tile height
    float halfTW;   // half tile width
    float halfTH;   // half tile height
    float ntw;      // native tile width — used for UV source rects
    float elevStep; // vertical pixel shift per height level
};

// ─────────────────────────────────────────────────────────────────────────────
// TileLayerRef
//
// Lightweight, pre-validated view into one tile layer.
// Collected once before the main draw loop so the inner loop never touches
// m_mapData.layers directly.
// ─────────────────────────────────────────────────────────────────────────────
struct TileLayerRef
{
    const TileLayerData *layer;
    float opacity;
    float offsetX;
    float offsetY;
};

// ─────────────────────────────────────────────────────────────────────────────
// BattleState
// ─────────────────────────────────────────────────────────────────────────────
class BattleState : public Scene
{
public:
    BattleState(StateMachine<Scene> &sm, Renderer *renderer);

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
    Texture *m_tileset = nullptr; // owning; created in onEnter, freed in onExit

    // ── Map data ──────────────────────────────────────────────────────────
    TileMapData m_mapData; // visual/rendering data loaded from Tiled JSON
    BattleMap m_battleMap; // gameplay grid derived from TileMapData

    // ── Gameplay systems ──────────────────────────────────────────────────
    Grid m_grid{};                 // logical grid representation (buildGameplayGrid)
    std::vector<Unit *> m_units;   // all active units (player + enemies)
    TurnQueue m_turnQueue;         // turn scheduler / CT system
    Vec2i m_moveStartPos{0, 0};    // starting position before a move
    bool m_actedAfterMove = false; // true if an action happened after the last move

    // Result of the battle used after exit transition
    bool m_playerWon = false;

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
        TurnEnded     // (internal) used to signal that the turn is over and we should advance
    };

    HumanTurnPhase m_humanTurnPhase = HumanTurnPhase::FreeCursor;

    TurnState m_turnState = TurnState::Idle;
    float m_turnTimer = 0.0f;

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
    void handleActiveTurn(const Input &input);
    void startBattleEnd(bool playerWon);
    bool checkVictory() const;
    bool checkDefeat() const;

    // ── Loading / initialization (onEnter helpers) ───────────────────────
    bool loadMap();
    bool buildGameplayGrid();
    bool loadTileset();
    Vec2f computeMapOrigin();

    // ── Rendering ─────────────────────────────────────────────────────────
    void drawBackground() const;
    void drawScene(const Camera &camera) const;

    // ── drawScene: frame setup ────────────────────────────────────────────
    IsoMetrics makeIsoMetrics(float zoom) const;
    std::vector<TileLayerRef> collectTileLayers() const;
    std::vector<std::uint8_t> buildSpawnGrid() const;

    // ── drawScene: per-cell rendering ────────────────────────────────────
    void drawTileLayersAt(int row, int col, float ax, float ay,
                          const IsoMetrics &m,
                          const std::vector<TileLayerRef> &layers) const;

    void drawSpawnOverlayAt(int row, int col, float ax, float ay,
                            const IsoMetrics &m,
                            const std::vector<std::uint8_t> &spawnGrid,
                            std::size_t gridSize) const;

    void drawUnitAt(int row, int col, float ax, float ay, const IsoMetrics &m) const;

    void drawCursorAt(int row, int col, float ax, float ay,
                      const IsoMetrics &m) const;

    // ── Cursor sub-rendering helpers ──────────────────────────────────────
    void drawCursorTriangle(float cx, float cy, const IsoMetrics &m) const;
    void drawCursorTicks(float ax, float ay, int tileHeight,
                         const IsoMetrics &m) const;

    // ── UI ──────────────────────────────────────────────────────────────
    UnitPanel m_unitPanel;
    UnitPanel m_targetPanel;
    MenuPanel m_menuPanel;
    DamagePreview m_damagePreview;
    Unit *m_hoveredUnit = nullptr;
    void showBattleMenu(bool canMove, bool canAttack, bool canWait);
    void showStatusMenu(Unit *unit);
};