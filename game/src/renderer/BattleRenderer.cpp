#include "engine/math/Vec2.h"
#include "engine/core/App.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/FontManager.h"
#include "engine/renderer/Renderer.h"
#include "engine/renderer/DebugRenderer.h"
#include "engine/renderer/Camera.h"
#include "config/GameConstants.h"
#include "renderer/BattleRenderer.h"
#include "battle/map/BattleMap.h"
#include "battle/unit/Unit.h"
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

    const float s = static_cast<float>(ctx.scale) * ctx.camera.getZoom();
    const float halfTW = static_cast<float>(ctx.mapData.tileWidth) * s * 0.5f;
    const float halfTH = static_cast<float>(ctx.mapData.tileHeight) * s * 0.5f;
    const auto tileLayers = collectTileLayers(ctx);
    const auto spawnGrid = buildSpawnGrid(ctx);
    const auto unitRender = buildUnitRenderList(ctx.units);

    m_renderer->setBlendMode(Renderer::BlendMode::Blend);

    for (int row = 0; row < ctx.mapData.height; ++row)
    {
        for (int col = 0; col < ctx.mapData.width; ++col)
        {
            const Vec2f screenPos = ctx.camera.tileToScreen(Vec2i{col, row});
            const float ax = screenPos.x;
            const float ay = screenPos.y;

            const float spriteSh = ctx.spriteH * s;
            const float cullL = ax - 2.0f * halfTW;
            const float cullR = ax + 2.0f * halfTW;
            const float cullT = ay - 2.0f * spriteSh;
            const float cullB = ay + 2.0f * spriteSh;
            if (cullR < 0.0f || cullL > GameConstants::VIEW_W ||
                cullB < 0.0f || cullT > GameConstants::VIEW_H)
                continue;

            drawTileLayersAt(row, col, ax, ay, s, halfTW, halfTH,
                             tileLayers,
                             ctx.tileset, ctx.tilesPerRow, ctx.spriteH,
                             static_cast<float>(ctx.mapData.tileWidth));

            if (ctx.showSpawnOverlays)
            {
                drawSpawnOverlayAt(row, col, ax, ay, s, halfTW, halfTH,
                                   spawnGrid, spawnGrid.size(),
                                   ctx.mapData.width, ctx.battleMap);
            }

            if (ctx.overlayTiles && ctx.overlayTiles->count({col, row}))
            {
                Color overlayColor = Color{255, 50, 50, 120};
                if (ctx.overlayMode == BattleOverlayMode::MoveRange)
                    overlayColor = Color{0, 100, 255, 120};
                else if (ctx.overlayMode == BattleOverlayMode::ConfirmTargets)
                    overlayColor = Color{255, 210, 80, 150};
                drawRangeOverlayAt(row, col, ax, ay, s, halfTW, halfTH,
                                   ctx.battleMap, overlayColor);
            }

            drawCursorAt(row, col, ax, ay, s, halfTW, halfTH,
                         ctx.cursor, ctx.battleMap,
                         ctx.cursorHoverOffset * s, ctx.cursorTriW, ctx.cursorTriH);

            drawUnitAt(row, col, ax, ay, s, halfTW, halfTH,
                       unitRender,
                       ctx.battleMap,
                       ctx.debugRenderer,
                       ctx.movementAnimation);
        }
    }
    drawAnimatingUnit(ctx, s, halfTW, halfTH, unitRender);
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
        out.push_back({u, u->getPosition(), u->getTeam(), !u->isDead(), label});
    }

    return out;
}

void BattleRenderer::drawUnitAt(int row, int col, float ax, float ay,
                                float s, float halfTW, float halfTH,
                                const std::vector<UnitRenderProxy> &units,
                                const BattleMap &battleMap,
                                DebugRenderer *debugRenderer,
                                const MovementAnimationController *movementAnimation) const
{
    if (!debugRenderer)
        return;

    const Unit *animatingUnit = (movementAnimation && movementAnimation->isAnimating())
                                    ? movementAnimation->animatingUnit()
                                    : nullptr;

    const UnitRenderProxy *unit = nullptr;

    for (const auto &u : units)
    {
        if (!u.alive)
            continue;
        // The animating unit is skipped at its LOGICAL tile — it's drawn
        // once, separately, at its interpolated position below instead.
        if (u.unit == animatingUnit)
            continue;
        if (u.pos.x == col && u.pos.y == row)
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

    const float elev = static_cast<float>(gt.height) * halfTH;
    const float floatOffset = 6.0f * s;
    const float cx = ax;
    const float cy = ay - elev - halfTH - floatOffset;
    float radius = halfTW * 0.6f;

    UnitPortrait::drawPlaceholderSprite(m_renderer, FontManager::instance().get(FontRole::Body), Vec2f{cx, cy}, radius * 2.0f, unit->team, unit->debugLabel);
}

void BattleRenderer::drawAnimatingUnit(const BattleRendererContext &ctx,
                                       float s, float halfTW, float halfTH,
                                       const std::vector<UnitRenderProxy> &units) const
{
    if (!ctx.debugRenderer || !ctx.movementAnimation || !ctx.movementAnimation->isAnimating())
        return;

    const Unit *animatingUnit = ctx.movementAnimation->animatingUnit();
    const UnitRenderProxy *proxy = nullptr;
    for (const auto &u : units)
    {
        if (u.unit == animatingUnit)
        {
            proxy = &u;
            break;
        }
    }
    if (!proxy)
        return;

    const Vec2f visualTile = ctx.movementAnimation->getVisualTilePos();
    const Vec2f screenPos = ctx.camera.tileToScreen(visualTile);
    const float ax = screenPos.x;
    const float ay = screenPos.y;

    const float elev = ctx.movementAnimation->getVisualHeight() * halfTH;

    Color color;
    switch (proxy->team)
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

    const float floatOffset = 6.0f * s;
    const float cx = ax;
    const float cy = ay - elev - halfTH - floatOffset;
    const float radius = halfTW * 0.6f;

    // TODO: swap placeholder circle for a real walk-cycle/jump sprite once
    // art exists — pick facing/frame from the segment's direction of
    // travel, and a distinct airborne pose for a hop's non-paused portion.
    UnitPortrait::drawPlaceholderSprite(m_renderer, FontManager::instance().get(FontRole::Body), Vec2f{cx, cy}, radius * 2.0f, proxy->team, proxy->debugLabel);
}

// ── Cursor rendering (using passed parameters) ──────────────────────────────

void BattleRenderer::drawCursorAt(int row, int col, float ax, float ay,
                                  float s, float halfTW, float halfTH,
                                  const Cursor &cursor,
                                  const BattleMap &battleMap,
                                  float hoverOffset,
                                  float triW, float triH) const
{
    const Vec2i pos = cursor.getPosition();
    if (pos.x != col || pos.y != row)
        return;

    const GameTile &gt = battleMap.at(col, row);
    const float elev = static_cast<float>(gt.height) * halfTH;
    const float cx = ax;
    const float cy = ay - elev - halfTH - hoverOffset;

    drawCursorTriangle(cx, cy, s, triW, triH);
    drawCursorTicks(ax, ay, gt.height, s, halfTW, halfTH);
}

void BattleRenderer::drawCursorTriangle(float cx, float cy,
                                        float s,
                                        float triW, float triH) const
{
    const float tw = triW * s;
    const float th = triH * s;
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
                                     float s, float halfTW, float halfTH) const
{
    const float elev = static_cast<float>(tileHeight) * halfTH;
    const float tickLen = 4.0f * s;

    const Vec2f corners[4] = {
        {ax, ay - halfTH - elev},
        {ax + halfTW, ay - elev},
        {ax, ay + halfTH - elev},
        {ax - halfTW, ay - elev}};
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
                                      float s, float halfTW, float halfTH,
                                      const std::vector<TileLayerRef> &layers,
                                      Texture *tileset,
                                      int tilesPerRow,
                                      float spriteH,
                                      float tileW) const
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
            static_cast<int>(static_cast<float>(srcCol) * tileW),
            static_cast<int>(static_cast<float>(srcRow) * spriteH),
            static_cast<int>(tileW),
            static_cast<int>(spriteH)};
        const Rectf dst = {
            ax - halfTW + ref.offsetX * s,
            ay - spriteH * s + 2.0f * halfTH + ref.offsetY * s,
            2.0f * halfTW,
            spriteH * s};

        if (dst.x + dst.w < 0.0f || dst.x > GameConstants::VIEW_W ||
            dst.y + dst.h < 0.0f || dst.y > GameConstants::VIEW_H)
            continue;

        m_renderer->setTextureAlphaMod(tileset, ref.opacity);
        m_renderer->drawTexture(tileset, src, dst);
    }
}

void BattleRenderer::drawSpawnOverlayAt(int row, int col,
                                        float ax, float ay,
                                        float s, float halfTW, float halfTH,
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
    const float oy = ay - static_cast<float>(gt.height) * halfTH;

    const FColor &overlayColor =
        spawnGrid[idx] == 1u ? COL_PLAYER_SPAWN : COL_ENEMY_SPAWN;

    const std::vector<Renderer::Vertex> verts = {
        {{ox, oy - halfTH}, overlayColor},
        {{ox + halfTW, oy}, overlayColor},
        {{ox, oy + halfTH}, overlayColor},
        {{ox - halfTW, oy}, overlayColor}};

    const std::vector<int> indices = {0, 1, 2, 0, 2, 3};
    m_renderer->drawGeometry(verts, indices);
}

void BattleRenderer::drawRangeOverlayAt(int row, int col, float ax, float ay,
                                        float s, float halfTW, float halfTH,
                                        const BattleMap &battleMap,
                                        Color color) const
{
    const GameTile &gt = battleMap.at(col, row);
    const float ox = ax;
    const float oy = ay - static_cast<float>(gt.height) * halfTH;

    const std::vector<Renderer::Vertex> verts = {
        Renderer::Vertex{{ox, oy - halfTH}, color},
        Renderer::Vertex{{ox + halfTW, oy}, color},
        Renderer::Vertex{{ox, oy + halfTH}, color},
        Renderer::Vertex{{ox - halfTW, oy}, color}};

    const std::vector<int> indices = {0, 1, 2, 0, 2, 3};
    m_renderer->drawGeometry(verts, indices);
}
