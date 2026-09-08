#pragma once

#include "engine/math/Vec2.h"

class Camera;
class BattleMap;
struct TileMapData;

// Shared screen-space anchor math for world-anchored battle effects (combat
// VFX rings, floating damage text).
//
// BattleRenderer projects tiles/units with Camera::tileToScreen + an elevation
// offset (halfTileHeight-per-height-step). Effects that hover on a tile must
// use the same projection or they will float at the wrong spot — this helper
// is the single source of that offset so the placeholder VFX and any future
// sprites agree.
namespace battleworld
{
    struct AnchorContext
    {
        const Camera &camera;
        const TileMapData &mapData;
        const BattleMap &battleMap;
        int scale = 1;
    };

    // Screen-space "feet" anchor of a tile's standing spot: the tile center
    // projected through the camera, lifted by the tile's height offset.
    // Effects (rings, text balloons) stack upward from here.
    Vec2f tileFeetAnchor(const AnchorContext &ctx, Vec2i tilePos);
}