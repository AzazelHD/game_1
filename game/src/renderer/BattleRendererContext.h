#pragma once

#include "engine/data/TileMapData.h"
#include "engine/math/Vec2.h"
#include "battle/MovementRange.h"

#include <vector>
#include <string>
#include <cstdint>
#include <unordered_set>

class Camera;
class Texture;
class DebugRenderer;
class Cursor;
class Unit;
class BattleMap;
struct TileLayerData;

enum class BattleOverlayMode
{
    None,
    MoveRange,
    AttackRange,
    ConfirmTargets
};

struct IsoMetrics
{
    float s;
    float tw;
    float th;
    float halfTW;
    float halfTH;
    float ntw;
    float elevStep;
};

struct TileLayerRef
{
    const TileLayerData *layer;
    float opacity;
    float offsetX;
    float offsetY;
};

struct UnitRenderProxy
{
    Vec2i pos;
    int team;
    bool alive;
    std::string debugLabel; // first letter of the unit's name
};

struct BattleRendererContext
{
    const Camera &camera;
    const TileMapData &mapData;
    const BattleMap &battleMap;
    Texture *tileset;
    int tilesPerRow;
    float spriteH;
    int scale;

    const Cursor &cursor;
    float cursorHoverOffset;
    float cursorTriW;
    float cursorTriH;

    const std::vector<Unit *> &units;
    DebugRenderer *debugRenderer;
    bool showSpawnOverlays = false;

    BattleOverlayMode overlayMode;
    const std::unordered_set<Vec2i, Vec2iHash> *overlayTiles;
};
