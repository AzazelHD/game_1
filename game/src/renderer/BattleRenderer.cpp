#include "engine/math/Vec2.h"
#include "engine/math/MathUtils.h"
#include "engine/core/App.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/FontManager.h"
#include "engine/renderer/Renderer.h"
#include "engine/renderer/DebugRenderer.h"
#include "engine/renderer/Camera.h"
#include "config/GameConstants.h"
#include "renderer/BattleRenderer.h"
#include "battle/BattleMap.h"
#include "battle/Unit.h"
#include "ui/Cursor.h"
#include "ui/UnitPortrait.h"

#include <algorithm>

namespace
{
    // Spawn-overlay colours.
    constexpr FColor COL_PLAYER_SPAWN = {0.18f, 0.52f, 0.89f, 0.55f}; // blue
    constexpr FColor COL_ENEMY_SPAWN = {0.89f, 0.18f, 0.18f, 0.55f};  // red

    // Cursor colours.
    constexpr FColor COL_BLACK = {0.0f, 0.0f, 0.0f, 1.0f};
    constexpr FColor COL_RED = {1.0f, 0.0f, 0.0f, 1.0f};
}

BattleRenderer::BattleRenderer(Renderer *renderer)
    : m_renderer(renderer)
{
}

void BattleRenderer::drawScene(const BattleRendererContext &ctx) const
{
    if (!m_renderer || !ctx.tileset || ctx.mapData.isEmpty())
        return;

    const Vec2f offset = ctx.camera.getOffset();
    const IsoMetrics m = makeIsoMetrics(ctx);
    const auto tileLayers = collectTileLayers(ctx);
    const auto spawnGrid = buildSpawnGrid(ctx);
    const auto unitRender = buildUnitRenderList(ctx.units);

    m_renderer->setBlendMode(Renderer::BlendMode::Blend);

    for (int row = 0; row < ctx.mapData.height; ++row)
    {
        for (int col = 0; col < ctx.mapData.width; ++col)
        {
            const Vec2f iso = tileToIso(Vec2i{col, row}, ctx.mapData.tileWidth, ctx.mapData.tileHeight);
            const float ax = offset.x + iso.x * m.s;
            const float ay = offset.y + iso.y * m.s;

            drawTileLayersAt(row, col, ax, ay, m, tileLayers,
                             ctx.tileset, ctx.tilesPerRow, ctx.spriteH);

            if (ctx.showSpawnOverlays)
            {
                drawSpawnOverlayAt(row, col, ax, ay, m, spawnGrid, spawnGrid.size(),
                                   ctx.mapData.width, ctx.battleMap);
            }

            if (ctx.overlayTiles && ctx.overlayTiles->count({col, row}))
            {
                Color overlayColor = Color{255, 50, 50, 120};
                if (ctx.overlayMode == BattleOverlayMode::MoveRange)
                    overlayColor = Color{0, 100, 255, 120};
                else if (ctx.overlayMode == BattleOverlayMode::ConfirmTargets)
                    overlayColor = Color{255, 210, 80, 150};
                drawRangeOverlayAt(row, col, ax, ay, m, ctx.battleMap, overlayColor);
            }

            drawCursorAt(row, col, ax, ay, m,
                         ctx.cursor, ctx.battleMap,
                         ctx.cursorHoverOffset * m.s, ctx.cursorTriW, ctx.cursorTriH);

            drawUnitAt(row, col, ax, ay, m,
                       unitRender,
                       ctx.battleMap,
                       ctx.debugRenderer);
        }
    }
}

void BattleRenderer::drawBackground(FColor top, FColor bottom) const
{
    const std::vector<Renderer::Vertex> verts = {
        {{0.0f, 0.0f}, top},
        {{GameConstants::VIEW_W, 0.0f}, top},
        {{GameConstants::VIEW_W, GameConstants::VIEW_H}, bottom},
        {{0.0f, GameConstants::VIEW_H}, bottom}};
    const std::vector<int> indices = {0, 1, 2, 0, 2, 3};
    m_renderer->drawGeometry(verts, indices);
}

IsoMetrics BattleRenderer::makeIsoMetrics(const BattleRendererContext &ctx) const
{
    IsoMetrics m;
    m.s = static_cast<float>(ctx.scale) * ctx.camera.getZoom();
    m.tw = static_cast<float>(ctx.mapData.tileWidth) * m.s;
    m.th = static_cast<float>(ctx.mapData.tileHeight) * m.s;
    m.halfTW = m.tw * 0.5f;
    m.halfTH = m.th * 0.5f;
    m.ntw = static_cast<float>(ctx.mapData.tileWidth);
    m.elevStep = static_cast<float>(ctx.mapData.tileHeight) * 0.5f * m.s;
    return m;
}

std::vector<TileLayerRef> BattleRenderer::collectTileLayers(const BattleRendererContext &ctx) const
{
    std::vector<TileLayerRef> out;
    out.reserve(ctx.mapData.layers.size());

    for (const auto &layer : ctx.mapData.layers)
    {
        if (layer.type != LayerType::Tile || layer.tiles.empty())
            continue;

        const std::size_t expected =
            static_cast<std::size_t>(layer.width) *
            static_cast<std::size_t>(layer.height);

        if (layer.tiles.size() < expected)
            continue;

        out.push_back({&layer, layer.opacity, layer.offsetX, layer.offsetY});
    }

    return out;
}

std::vector<std::uint8_t> BattleRenderer::buildSpawnGrid(const BattleRendererContext &ctx) const
{
    const std::size_t gridSize =
        static_cast<std::size_t>(ctx.mapData.width) *
        static_cast<std::size_t>(ctx.mapData.height);

    std::vector<std::uint8_t> grid(gridSize, 0u);

    for (const GameTile *t : ctx.battleMap.playerSpawns)
    {
        const std::size_t idx =
            static_cast<std::size_t>(t->row * ctx.mapData.width + t->col);
        if (idx < gridSize)
            grid[idx] = 1u;
    }

    for (const GameTile *t : ctx.battleMap.allySpawns)
    {
        const std::size_t idx =
            static_cast<std::size_t>(t->row * ctx.mapData.width + t->col);
        if (idx < gridSize)
            grid[idx] = 2u;
    }

    for (const auto &pair : ctx.battleMap.enemySpawnsByTeam)
    {
        for (const GameTile *t : pair.second)
        {
            const std::size_t idx =
                static_cast<std::size_t>(t->row * ctx.mapData.width + t->col);
            if (idx < gridSize)
                grid[idx] = 3u;
        }
    }

    return grid;
}

std::vector<UnitRenderProxy> BattleRenderer::buildUnitRenderList(const std::vector<Unit *> &units) const
{
    std::vector<UnitRenderProxy> out;
    out.reserve(units.size());

    for (const Unit *u : units)
    {
        if (!u)
            continue;
        const std::string &name = u->getName();
        const std::string label = name.empty() ? std::string() : std::string(1, name[0]);
        out.push_back({u->getPosition(), u->getTeam(), !u->isDead(), label});
    }

    return out;
}

void BattleRenderer::drawUnitAt(int row, int col, float ax, float ay,
                                const IsoMetrics &m,
                                const std::vector<UnitRenderProxy> &units,
                                const BattleMap &battleMap,
                                DebugRenderer *debugRenderer) const
{
    if (!debugRenderer)
        return;

    const UnitRenderProxy *unit = nullptr;

    for (const auto &u : units)
    {
        if (u.alive && u.pos.x == col && u.pos.y == row)
        {
            unit = &u;
            break;
        }
    }

    if (!unit)
        return;

    const GameTile &gt = battleMap.at(col, row);

    Color color;
    switch (unit->team)
    {
    case 0:
        color = {64, 128, 255, 220};
        break;
    case 1:
        color = {64, 255, 64, 220};
        break;
    default:
        color = {255, 64, 64, 220};
        break;
    }

    const float elev = static_cast<float>(gt.height) * m.elevStep;
    const float floatOffset = 6.0f * m.s;
    const float cx = ax;
    const float cy = ay - elev - m.halfTH - floatOffset;
    float radius = m.halfTW * 0.6f;

    UnitPortrait::drawPlaceholderSprite(m_renderer, FontManager::instance().get(FontRole::Body), Vec2f{cx, cy}, radius * 2.0f, unit->team, unit->debugLabel);
}

// ── Cursor rendering (using passed parameters) ──────────────────────────────

void BattleRenderer::drawCursorAt(int row, int col, float ax, float ay,
                                  const IsoMetrics &m,
                                  const Cursor &cursor,
                                  const BattleMap &battleMap,
                                  float hoverOffset,
                                  float triW, float triH) const
{
    const Vec2i pos = cursor.getPosition();
    if (pos.x != col || pos.y != row)
        return;

    const GameTile &gt = battleMap.at(col, row);
    const float elev = static_cast<float>(gt.height) * m.elevStep;
    const float cx = ax;
    const float cy = ay - elev - m.halfTH - hoverOffset;

    drawCursorTriangle(cx, cy, m, triW, triH);
    drawCursorTicks(ax, ay, gt.height, m);
}

void BattleRenderer::drawCursorTriangle(float cx, float cy,
                                        const IsoMetrics &m,
                                        float triW, float triH) const
{
    const float tw = triW * m.s;
    const float th = triH * m.s;
    const float border = 2.0f;
    const std::vector<int> indices = {0, 1, 2};

    const std::vector<Renderer::Vertex> borderVerts = {
        {{cx, cy + th + border}, COL_BLACK},
        {{cx - tw - border, cy - border}, COL_BLACK},
        {{cx + tw + border, cy - border}, COL_BLACK}};
    m_renderer->drawGeometry(borderVerts, indices);

    const std::vector<Renderer::Vertex> fillVerts = {
        {{cx, cy + th}, COL_RED},
        {{cx - tw, cy}, COL_RED},
        {{cx + tw, cy}, COL_RED}};
    m_renderer->drawGeometry(fillVerts, indices);
}

void BattleRenderer::drawCursorTicks(float ax, float ay, int tileHeight,
                                     const IsoMetrics &m) const
{
    const float elev = static_cast<float>(tileHeight) * m.elevStep;
    const float tickLen = 4.0f * m.s;

    const Vec2f corners[4] = {
        {ax, ay - m.halfTH - elev},
        {ax + m.halfTW, ay - elev},
        {ax, ay + m.halfTH - elev},
        {ax - m.halfTW, ay - elev}};
    const Vec2f inward[4] = {
        {0.0f, 1.0f},  // top    → down
        {-1.0f, 0.0f}, // right  → left
        {0.0f, -1.0f}, // bottom → up
        {1.0f, 0.0f}   // left   → right
    };

    m_renderer->setDrawColor(Color{255, 255, 255, 255});
    for (int i = 0; i < 4; ++i)
    {
        m_renderer->drawLine(corners[i],
                             Vec2f{corners[i].x + inward[i].x * tickLen,
                                   corners[i].y + inward[i].y * tickLen});
    }
}

void BattleRenderer::drawTileLayersAt(int row, int col,
                                      float ax, float ay,
                                      const IsoMetrics &m,
                                      const std::vector<TileLayerRef> &layers,
                                      Texture *tileset,
                                      int tilesPerRow,
                                      float spriteH) const
{
    for (const auto &ref : layers)
    {
        const auto gid = ref.layer->tiles[static_cast<std::size_t>(row * ref.layer->width + col)];
        if (gid == 0)
            continue;

        const int localId = static_cast<int>(gid) - 1;
        const int srcCol = localId % tilesPerRow;
        const int srcRow = localId / tilesPerRow;

        const Recti src = {
            static_cast<int>(static_cast<float>(srcCol) * m.ntw),
            static_cast<int>(static_cast<float>(srcRow) * spriteH),
            static_cast<int>(m.ntw),
            static_cast<int>(spriteH)};
        const Rectf dst = {
            ax - m.halfTW + ref.offsetX * m.s,
            ay - spriteH * m.s + m.th + ref.offsetY * m.s,
            m.tw,
            spriteH * m.s};

        m_renderer->setTextureAlphaMod(tileset, ref.opacity);
        m_renderer->drawTexture(tileset, src, dst);
    }
}

void BattleRenderer::drawSpawnOverlayAt(int row, int col,
                                        float ax, float ay,
                                        const IsoMetrics &m,
                                        const std::vector<std::uint8_t> &spawnGrid,
                                        std::size_t gridSize,
                                        int mapWidth,
                                        const BattleMap &battleMap) const
{
    const std::size_t idx =
        static_cast<std::size_t>(row * mapWidth + col);

    if (idx >= gridSize || idx >= spawnGrid.size() || spawnGrid[idx] == 0u)
        return;

    const GameTile &gt = battleMap.at(col, row);
    const float ox = ax;
    const float oy = ay - static_cast<float>(gt.height) * m.elevStep;

    const FColor &overlayColor =
        spawnGrid[idx] == 1u ? COL_PLAYER_SPAWN : COL_ENEMY_SPAWN;

    const std::vector<Renderer::Vertex> verts = {
        {{ox, oy - m.halfTH}, overlayColor},
        {{ox + m.halfTW, oy}, overlayColor},
        {{ox, oy + m.halfTH}, overlayColor},
        {{ox - m.halfTW, oy}, overlayColor}};

    const std::vector<int> indices = {0, 1, 2, 0, 2, 3};
    m_renderer->drawGeometry(verts, indices);
}

void BattleRenderer::drawRangeOverlayAt(int row, int col, float ax, float ay,
                                        const IsoMetrics &m,
                                        const BattleMap &battleMap,
                                        Color color) const
{
    const GameTile &gt = battleMap.at(col, row);
    const float ox = ax;
    const float oy = ay - static_cast<float>(gt.height) * m.elevStep;

    const std::vector<Renderer::Vertex> verts = {
        Renderer::Vertex{{ox, oy - m.halfTH}, color},
        Renderer::Vertex{{ox + m.halfTW, oy}, color},
        Renderer::Vertex{{ox, oy + m.halfTH}, color},
        Renderer::Vertex{{ox - m.halfTW, oy}, color}};

    const std::vector<int> indices = {0, 1, 2, 0, 2, 3};
    m_renderer->drawGeometry(verts, indices);
}