#pragma once

#include "engine/math/Vec2.h"
#include "engine/renderer/Texture.h"
#include "engine/data/TileMapData.h"
#include "battle/map/BattleMap.h"
#include "battle/map/Grid.h"

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

    // How far the map's drawn art reaches *above* the top of its tile-anchor
    // box, in world (source-texel) units. Tiles draw their surface above the
    // anchor and the height layers (h1/h2/...) lift a raised tile another half
    // tile height per level, so the art is taller than the grid the camera's
    // bounds know about. Handed to Camera::setMapBoundsTopExtra() so clamping
    // and small-map centring account for the top rows.
    float mapTopExtra = 0.0f;
};

class BattleLoader
{
public:
    explicit BattleLoader(Renderer *renderer) : m_renderer(renderer) {}

    // Loads map, builds grid, loads tileset, computes origin.
    // `viewport` is the world viewport size in native px (the play field); it
    // centres the initial map framing. On any failure, result.ok == false (and
    // any partial texture is freed).
    BattleLoadResult load(const char *mapPath, int scale, float spriteH, Vec2f viewport);

private:
    static Vec2f computeMapOrigin(const TileMapData &mapData, int scale, float spriteH, Vec2f viewport);

    // Art reach above the map's top tile anchors (see BattleLoadResult).
    static float computeMapTopExtra(const TileMapData &mapData, float spriteH);

    Renderer *m_renderer = nullptr;
};
