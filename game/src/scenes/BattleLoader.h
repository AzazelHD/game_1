#pragma once

#include "engine/math/Vec2.h"
#include "engine/renderer/Texture.h"
#include "engine/data/TileMapData.h"
#include "battle/BattleMap.h"
#include "battle/Grid.h"

class Renderer;

struct BattleLoadResult
{
    bool ok = false;
    TileMapData mapData;
    BattleMap battleMap;
    Grid grid;
    Texture *tileset = nullptr;
    float texW = 0.0f;
    float texH = 0.0f;
    int tilesPerRow = 1;
    float spriteH = 0.0f;
    Vec2f mapOrigin{0.0f, 0.0f};
};

class BattleLoader
{
public:
    explicit BattleLoader(Renderer *renderer) : m_renderer(renderer) {}

    // Loads map, builds grid, loads tileset, computes origin.
    // On any failure, result.ok == false (and any partial texture is freed).
    BattleLoadResult load(const char *mapPath, int scale, float spriteH);

private:
    static Vec2f computeMapOrigin(const TileMapData &mapData, int scale, float spriteH);

    Renderer *m_renderer = nullptr;
};
