#include "battle/systems/BattleCameraController.h"

#include "engine/math/MathUtils.h"
#include "engine/renderer/Camera.h"

namespace
{
    constexpr int kInvalid = -1;
    constexpr float kInnerBoxFraction = 0.75f;

    [[nodiscard]] bool isValid(Vec2i tile) { return tile.x >= 0 && tile.y >= 0; }

    void glideTo(Camera &camera, Vec2f viewSize, Vec2f tileSize, Vec2i tile, float dt)
    {
        const Vec2f iso = tileToIso(tile, static_cast<int>(tileSize.x), static_cast<int>(tileSize.y));
        camera.trackTarget(iso, viewSize, dt);
        camera.clampToBounds();
    }

    void snapTo(Camera &camera, Vec2f viewSize, Vec2f tileSize, Vec2i tile)
    {
        const Vec2f iso = tileToIso(tile, static_cast<int>(tileSize.x), static_cast<int>(tileSize.y));
        const float z = std::max(camera.getZoom(), 0.0001f) * camera.getRenderScale();
        camera.setOffset(Vec2f{iso.x - (viewSize.x * 0.5f) / z, iso.y - (viewSize.y * 0.5f) / z});
        camera.clampToBounds();
    }
} // namespace

bool BattleCameraController::update(float dt, Camera &camera, Vec2f viewSize,
                                    const Context &ctx)
{
    const Vec2f tileSize = ctx.tileSize;

    // Scripted dialogue wins over everything: follow the speaker, or hold.
    if (ctx.dialogueActive)
    {
        if (isValid(ctx.speakerTile))
            glideTo(camera, viewSize, tileSize, ctx.speakerTile, dt);
        return false;
    }

    if (!ctx.inCombat && ctx.phase == Phase::Free)
    {
        if (isValid(ctx.cursorTile))
            glideTo(camera, viewSize, tileSize, ctx.cursorTile, dt);
        return false;
    }

    if (ctx.phase == Phase::ResolutionCast)
    {
        if (isValid(ctx.casterTile))
            glideTo(camera, viewSize, tileSize, ctx.casterTile, dt);
        return false;
    }

    if (ctx.phase == Phase::ResolutionTrack)
    {
        // Target-all / single-bloom casts keep the camera on the caster.
        if (ctx.stayOnCaster && isValid(ctx.casterTile))
        {
            glideTo(camera, viewSize, tileSize, ctx.casterTile, dt);
            return false;
        }

        // First Track frame, or the hit index advanced: cut to the new target.
        if (isValid(ctx.focusTile) && (!m_snappedAny || ctx.focusTile != m_lastSnappedTile))
        {
            m_lastSnappedTile = ctx.focusTile;
            m_snappedAny = true;
            snapTo(camera, viewSize, tileSize, ctx.focusTile);
            return true;
        }
        return false;
    }

    // Free control: hold the active unit centred; only when the cursor leaves
    // the inner box do we pan toward it (FFTA-style edge-scroll).
    m_snappedAny = false;
    m_lastSnappedTile = Vec2i{kInvalid, kInvalid};

    if (isValid(ctx.anchorTile))
    {
        const Vec2f cursorScreen = camera.tileToScreen(ctx.cursorTile);
        const Vec2f margin = viewSize * (1.0f - kInnerBoxFraction) * 0.5f;
        const Rectf innerBox{margin.x, margin.y, viewSize.x * kInnerBoxFraction, viewSize.y * kInnerBoxFraction};
        if (innerBox.contains(cursorScreen))
            glideTo(camera, viewSize, tileSize, ctx.anchorTile, dt);
        else
            glideTo(camera, viewSize, tileSize, ctx.cursorTile, dt);
        return false;
    }

    if (isValid(ctx.cursorTile))
        glideTo(camera, viewSize, tileSize, ctx.cursorTile, dt);
    return false;
}