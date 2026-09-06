#pragma once

#include "engine/renderer/Color.h"
#include "renderer/BattleRendererContext.h"

#include <vector>
#include <cstdint>

// ── Forward declarations ────────────────────────────────────────────────
class Renderer;
class DebugRenderer;
class Texture;
class Cursor;
class BattleMap;
class Unit;
class Camera;

struct TileLayerData;

// ─────────────────────────────────────────────────────────────────────────────
// BattleRenderer
// ─────────────────────────────────────────────────────────────────────────────
class BattleRenderer
{
public:
    explicit BattleRenderer(Renderer *renderer);

    // Draws the battle backdrop using map-specific configured gradient colors.
    void drawBackground(FColor top, FColor bottom) const;
    void drawScene(const BattleRendererContext &ctx) const;

    void drawTileLayersAt(int row, int col,
                          float ax, float ay,
                          float s,
                          float halfTW, float halfTH,
                          const std::vector<TileLayerRef> &layers,
                          Texture *tileset,
                          int tilesPerRow,
                          float spriteH,
                          float tileW) const;

    void drawSpawnOverlayAt(int row, int col, float ax, float ay,
                            float s,
                            float halfTW, float halfTH,
                            const std::vector<std::uint8_t> &spawnGrid,
                            std::size_t gridSize,
                            int mapWidth,
                            const BattleMap &battleMap) const;

    void drawRangeOverlayAt(int row, int col, float ax, float ay,
                            float s,
                            float halfTW, float halfTH,
                            const BattleMap &battleMap,
                            Color color) const;

    void drawCursorAt(int row, int col, float ax, float ay,
                      float s,
                      float halfTW, float halfTH,
                      const Cursor &cursor,
                      const BattleMap &battleMap,
                      float hoverOffset,
                      float triW, float triH) const;

    void drawCursorTriangle(float cx, float cy,
                            float s,
                            float triW, float triH) const;

    void drawCursorTicks(float ax, float ay, int tileHeight,
                         float s,
                         float halfTW, float halfTH) const;

    void drawUnitAt(int row, int col, float ax, float ay,
                    float s,
                    float halfTW, float halfTH,
                    const std::vector<UnitRenderProxy> &units,
                    const BattleMap &battleMap,
                    DebugRenderer *debugRenderer,
                    const MovementAnimationController *movementAnimation) const;

    void drawAnimatingUnit(const BattleRendererContext &ctx,
                           float s,
                           float halfTW, float halfTH,
                           const std::vector<UnitRenderProxy> &units) const;

    std::vector<UnitRenderProxy> buildUnitRenderList(const std::vector<Unit *> &units) const;

private:
    std::vector<TileLayerRef> collectTileLayers(const BattleRendererContext &ctx) const;
    std::vector<std::uint8_t> buildSpawnGrid(const BattleRendererContext &ctx) const;

    Renderer *m_renderer = nullptr;
};
