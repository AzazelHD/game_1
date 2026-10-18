#include "engine/core/Log.h"
#include "engine/math/MathUtils.h"
#include "engine/data/TiledJsonLoader.h"
#include "engine/renderer/Renderer.h"
#include "config/GameConstants.h"
#include "scenes/BattleLoader.h"

#include <algorithm>

BattleLoadResult BattleLoader::load(const char *mapPath, int scale, float spriteH, Vec2f viewport)
{
    BattleLoadResult r;

    // ── loadMap ──
    TiledJsonLoader loader;
    std::string error;
    auto opt = loader.loadFromFile(mapPath, error);
    if (!opt)
    {
        LOG_ERROR("Battle", "Map load FAILED: %s", error.c_str());
        return r;
    }
    r.mapData = std::move(*opt);
    LOG_INFO("Battle", "Map loaded: %dx%d tiles, %d layers, tileW=%d tileH=%d",
             r.mapData.width, r.mapData.height,
             static_cast<int>(r.mapData.layers.size()),
             r.mapData.tileWidth, r.mapData.tileHeight);

    // ── buildGameplayGrid ──
    if (!r.battleMap.buildFrom(r.mapData))
    {
        LOG_ERROR("Battle", "BattleMap build FAILED - battle cannot proceed");
        return r;
    }
    r.grid = Grid(r.mapData.width, r.mapData.height);

    // ── loadTileset ──
    r.tileset = m_renderer->loadTexture("assets/sprites/0.5H_IsoTiles.png");
    if (!r.tileset)
    {
        LOG_ERROR("Battle", "Tileset load FAILED");
        return r;
    }
    if (r.mapData.tileWidth <= 0)
    {
        LOG_ERROR("Battle", "tileWidth is 0 - cannot compute sprite layout");
        delete r.tileset;
        r.tileset = nullptr;
        return r;
    }
    m_renderer->setTextureScaleMode(r.tileset, Renderer::ScaleMode::Nearest);
    r.texW = static_cast<float>(r.tileset->getWidth());
    r.texH = static_cast<float>(r.tileset->getHeight());
    r.tilesPerRow = std::max(1, static_cast<int>(r.texW / r.mapData.tileWidth));
    r.spriteH = spriteH;

    // ── computeMapOrigin ──
    r.mapOrigin = computeMapOrigin(r.mapData, scale, spriteH, viewport);

    // ── computeMapTopExtra ──
    r.mapTopExtra = computeMapTopExtra(r.mapData, spriteH);

    r.ok = true;
    return r;
}

Vec2f BattleLoader::computeMapOrigin(const TileMapData &mapData, int scale, float spriteH, Vec2f viewport)
{
    if (mapData.width <= 0 || mapData.height <= 0 ||
        mapData.tileWidth <= 0 || mapData.tileHeight <= 0)
    {
        return {viewport.x * 0.5f, viewport.y * 0.5f};
    }

    const Vec2f c00 = tileToIso(Vec2i{0, 0}, mapData.tileWidth, mapData.tileHeight);
    const Vec2f c10 = tileToIso(Vec2i{mapData.width - 1, 0}, mapData.tileWidth, mapData.tileHeight);
    const Vec2f c01 = tileToIso(Vec2i{0, mapData.height - 1}, mapData.tileWidth, mapData.tileHeight);
    const Vec2f c11 = tileToIso(Vec2i{mapData.width - 1, mapData.height - 1}, mapData.tileWidth, mapData.tileHeight);

    const float minIsoX = std::min({c00.x, c10.x, c01.x, c11.x});
    const float maxIsoX = std::max({c00.x, c10.x, c01.x, c11.x});
    const float minIsoY = std::min({c00.y, c10.y, c01.y, c11.y});
    const float maxIsoY = std::max({c00.y, c10.y, c01.y, c11.y});

    const float tileH = static_cast<float>(mapData.tileHeight);
    const float gridH = maxIsoY - minIsoY;
    const float centerIsoX = (minIsoX + maxIsoX) * 0.5f;

    float minLayerOffsetY = 0.0f;
    for (const auto &layer : mapData.layers)
        minLayerOffsetY = std::min(minLayerOffsetY, layer.offsetY);

    const float s = static_cast<float>(scale);

    float originX = viewport.x * 0.5f - centerIsoX * s;
    float originY = viewport.y * 0.5f +
                    s * (spriteH - 2.0f * tileH - minLayerOffsetY - gridH) / 2.0f;

    LOG_INFO("Battle", "Map origin: (%.0f, %.0f), gridH=%.0f, minLayerOffsetY=%.0f",
             originX, originY, gridH, minLayerOffsetY);

    return {originX, originY};
}

float BattleLoader::computeMapTopExtra(const TileMapData &mapData, float spriteH)
{
    const float tileH = static_cast<float>(mapData.tileHeight);
    if (tileH <= 0.0f || mapData.width <= 0 || mapData.height <= 0)
        return 0.0f;

    // A tile is drawn as a sprite cell whose top edge sits this far from the
    // tile's anchor (BattleRenderer draws dst.y = ay - spriteH + 2 * halfTH), and
    // each height layer lifts its tiles by its own -Y offset.
    const float cellTop = tileH - spriteH;

    float highest = 0.0f;
    for (const auto &layer : mapData.layers)
    {
        if (layer.type != LayerType::Tile)
            continue;

        const int cols = layer.width;
        const int rows = layer.height;
        if (cols <= 0 || rows <= 0)
            continue;

        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                const std::size_t index =
                    static_cast<std::size_t>(row) * static_cast<std::size_t>(cols) +
                    static_cast<std::size_t>(col);
                if (index >= layer.tiles.size() || layer.tiles[index] == 0)
                    continue;

                const float isoY = tileToIso(Vec2i{col, row}, mapData.tileWidth, mapData.tileHeight).y;
                highest = std::min(highest, isoY + layer.offsetY + cellTop);
            }
        }
    }

    // The camera's bounds start at the top anchor row (iso y = 0 for the top
    // corner tile), so the art reach *above* them is what it needs.
    return std::max(0.0f, -highest);
}
