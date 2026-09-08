#include "renderer/BattleWorldProjection.h"

#include "engine/renderer/Camera.h"
#include "engine/data/TileMapData.h"
#include "battle/map/BattleMap.h"

namespace battleworld
{
    Vec2f tileFeetAnchor(const AnchorContext &ctx, Vec2i tilePos)
    {
        if (!ctx.camera.isTileInsideMap(tilePos))
            return ctx.camera.tileToScreen(tilePos);

        const float s = static_cast<float>(ctx.scale) * ctx.camera.getZoom();
        const float halfTH = static_cast<float>(ctx.mapData.tileHeight) * s * 0.5f;

        const Vec2f screenPos = ctx.camera.tileToScreen(tilePos);
        const float elev = static_cast<float>(ctx.battleMap.at(tilePos.x, tilePos.y).height) * halfTH;
        return {screenPos.x, screenPos.y - elev};
    }
}