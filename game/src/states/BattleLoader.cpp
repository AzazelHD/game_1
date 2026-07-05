#include "engine/core/Log.h"
#include "engine/math/MathUtils.h"
#include "engine/data/TiledJsonLoader.h"
#include "engine/renderer/Renderer.h"
#include "config/GameConstants.h"
#include "states/BattleLoader.h"

#include <algorithm>

BattleLoadResult BattleLoader::load(const char *mapPath, int scale, float spriteH)
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
    r.mapOrigin = computeMapOrigin(r.mapData, scale, spriteH);

    r.ok = true;
    return r;
}

Vec2f BattleLoader::computeMapOrigin(const TileMapData &mapData, int scale, float spriteH)
{
    if (mapData.width <= 0 || mapData.height <= 0 ||
        mapData.tileWidth <= 0 || mapData.tileHeight <= 0)
    {
        return {GameConstants::VIEW_CX, GameConstants::VIEW_CY};
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

    float originX = GameConstants::VIEW_CX - centerIsoX * s;
    float originY = GameConstants::VIEW_CY +
                    s * (spriteH - 2.0f * tileH - minLayerOffsetY - gridH) / 2.0f;

    LOG_INFO("Battle", "Map origin: (%.0f, %.0f), gridH=%.0f, minLayerOffsetY=%.0f",
             originX, originY, gridH, minLayerOffsetY);

    return {originX, originY};
}