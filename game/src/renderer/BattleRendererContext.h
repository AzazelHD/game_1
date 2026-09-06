#pragma once

#include "engine/data/TileMapData.h"
#include "engine/math/Vec2.h"
#include "battle/map/MovementRange.h"
#include "battle/controllers/MovementAnimationController.h"

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

struct TileLayerRef
{
    const TileLayerData *layer;
    float opacity;
    float offsetX;
    float offsetY;
};

struct UnitRenderProxy
{
    const Unit *unit; // identity, needed to match against the animating unit
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
    const MovementAnimationController *movementAnimation = nullptr;
    bool showSpawnOverlays = false;

    BattleOverlayMode overlayMode;
    const std::unordered_set<Vec2i, Vec2iHash> *overlayTiles;
};
