#include "battle/systems/BattleCameraController.h"

#include "config/UiTuning.h"
#include "engine/math/MathUtils.h"
#include "engine/renderer/Camera.h"

#include <algorithm>
#include <cmath>

namespace
{
    // Every camera correction eases with this time constant; targets are
    // recomputed from live state each frame, so motion is deterministic and
    // never overshoots.
    constexpr float kEaseTau = Tuning::Camera::kEaseTau;
    // A correction counts as finished once the camera is this close (iso units).
    constexpr float kSettleEpsilon = 0.5f;

    [[nodiscard]] bool isValid(Vec2i tile) { return tile.x >= 0 && tile.y >= 0; }

    [[nodiscard]] float screenScale(const Camera &camera)
    {
        return std::max(camera.getZoom(), 0.0001f) * camera.getRenderScale();
    }

    [[nodiscard]] Vec2f isoOf(Vec2i tile, Vec2f tileSize)
    {
        return tileToIso(tile, static_cast<int>(tileSize.x), static_cast<int>(tileSize.y));
    }

    // Camera offset that puts an iso-space point in the middle of the play field.
    [[nodiscard]] Vec2f offsetCentredOn(const Camera &camera, Vec2f viewSize, Vec2f iso)
    {
        const float z = screenScale(camera);
        return Vec2f{iso.x - viewSize.x * 0.5f / z, iso.y - viewSize.y * 0.5f / z};
    }

    void setClampedOffset(Camera &camera, Vec2f offset)
    {
        camera.setOffset(offset);
        camera.clampToBounds();
    }

    // Returns true once the camera sits on `target`, or is held short of it by
    // the map bounds (another step would not move it).
    bool easeOffsetTo(Camera &camera, Vec2f target, float dt)
    {
        const float ease = 1.0f - std::exp(-std::max(dt, 0.0f) / kEaseTau);
        const Vec2f current = camera.getOffset();
        const Vec2f desired = current + (target - current) * ease;
        setClampedOffset(camera, desired);

        const Vec2f after = camera.getOffset();
        const bool pinned = after.x != desired.x || after.y != desired.y;
        const bool reached = std::abs(after.x - target.x) < kSettleEpsilon &&
                             std::abs(after.y - target.y) < kSettleEpsilon;
        return pinned || reached;
    }

    // On-screen rectangle of a tile's WALKABLE FLOOR SURFACE: the diamond
    // BattleRenderer::drawCursorTicks draws for the cursor. Height levels are Y
    // offsets, so an elevated tile's floor renders `height * halfTileHeight`
    // above its grid anchor; this is the surface the cursor picks and units
    // stand on, not the anchor and not the art above it.
    struct FloorSurface
    {
        float left, right, top, bottom;
    };

    // Surface as it would appear with the camera at `offset`, padded by what is
    // drawn around the diamond: the cursor marker above (kMarkerHeadroom) and
    // the tile art below (a tile cell ends a full tile height under its anchor,
    // i.e. half a tile under the diamond's bottom vertex). tileToScreen is
    // (iso - offset) * z, so the live projection only needs shifting by the
    // offset delta.
    [[nodiscard]] FloorSurface floorSurfaceAt(const Camera &camera, Vec2f tileSize, Vec2i tile,
                                              int tileHeight, Vec2f offset)
    {
        const float z = screenScale(camera);
        const Vec2f anchor = camera.tileToScreen(tile) + (camera.getOffset() - offset) * z;
        const float halfW = tileSize.x * 0.5f * z;
        const float halfH = tileSize.y * 0.5f * z;
        const float lift = static_cast<float>(tileHeight) * halfH;
        const float top = anchor.y - lift - halfH - Tuning::Camera::kMarkerHeadroom * z;
        const float bottom = anchor.y - lift + halfH + halfH;
        return FloorSurface{anchor.x - halfW, anchor.x + halfW, top, bottom};
    }

    // Minimum camera-offset displacement that brings the surface back inside the
    // dead zone, or zero while it is already inside. screen = (iso - offset) * z,
    // so the offset moves the opposite way by delta / z. Axes the map cannot pan
    // on are skipped: nothing exists beyond the play field there.
    [[nodiscard]] Vec2f deadZoneCorrection(const FloorSurface &floor, Vec2f viewSize,
                                           Camera::PanAxes axes, float z)
    {
        const auto axisDelta = [](float low, float high, float extent, float fraction)
        {
            const float margin = extent * (1.0f - fraction) * 0.5f;
            if (low < margin)
                return margin - low;
            if (high > extent - margin)
                return (extent - margin) - high;
            return 0.0f;
        };

        const float dx = axes.x ? axisDelta(floor.left, floor.right, viewSize.x,
                                            Tuning::Camera::kDeadZoneFractionX)
                                : 0.0f;
        const float dy = axes.y ? axisDelta(floor.top, floor.bottom, viewSize.y,
                                            Tuning::Camera::kDeadZoneFractionY)
                                : 0.0f;
        return Vec2f{-dx / z, -dy / z};
    }

    // Offset that satisfies the dead zone for the cursor, starting from `from`.
    [[nodiscard]] Vec2f deadZoneTarget(const Camera &camera, Vec2f viewSize,
                                       const BattleCameraController::Context &ctx, Vec2f from)
    {
        const FloorSurface floor =
            floorSurfaceAt(camera, ctx.tileSize, ctx.cursorTile, ctx.cursorTileHeight, from);
        return from + deadZoneCorrection(floor, viewSize, camera.getPanAxes(), screenScale(camera));
    }

    void glideTo(Camera &camera, Vec2f viewSize, Vec2f tileSize, Vec2i tile, float dt)
    {
        camera.trackTarget(isoOf(tile, tileSize), viewSize, dt);
        camera.clampToBounds();
    }

    void snapTo(Camera &camera, Vec2f viewSize, Vec2f tileSize, Vec2i tile)
    {
        setClampedOffset(camera, offsetCentredOn(camera, viewSize, isoOf(tile, tileSize)));
    }
} // namespace

bool BattleCameraController::update(float dt, Camera &camera, Vec2f viewSize,
                                    const Context &ctx)
{
    // Nothing exists beyond the play field: the camera never moves.
    if (!camera.getPanAxes().any())
    {
        resetBeatState();
        return false;
    }

    trackScriptedBeat(camera, ctx);

    if (ctx.dialogueActive)
    {
        if (isValid(ctx.speakerTile))
            glideTo(camera, viewSize, ctx.tileSize, ctx.speakerTile, dt);
        return false;
    }

    switch (ctx.phase)
    {
    case Phase::ResolutionCast:
        if (isValid(ctx.casterTile))
            glideTo(camera, viewSize, ctx.tileSize, ctx.casterTile, dt);
        return false;

    case Phase::ResolutionTrack:
        return updateTrack(camera, viewSize, ctx, dt);

    case Phase::Free:
        break;
    }

    m_lastSnappedTile.reset();
    if (!isValid(ctx.cursorTile))
        return false;

    if (m_returningToBeat)
        returnToBeat(camera, viewSize, ctx, dt);
    else
        easeOffsetTo(camera, deadZoneTarget(camera, viewSize, ctx, camera.getOffset()), dt);
    return false;
}

void BattleCameraController::frameCursor(Camera &camera, Vec2f viewSize, const Context &ctx)
{
    resetBeatState();
    if (!isValid(ctx.cursorTile) || !camera.getPanAxes().any())
        return;

    // Centre on the cursor's floor surface (lifted by its height), like the
    // dead zone measures it; the clamp holds it inside the map.
    Vec2f iso = isoOf(ctx.cursorTile, ctx.tileSize);
    iso.y -= static_cast<float>(ctx.cursorTileHeight) * ctx.tileSize.y * 0.5f;
    setClampedOffset(camera, offsetCentredOn(camera, viewSize, iso));
}

bool BattleCameraController::updateTrack(Camera &camera, Vec2f viewSize, const Context &ctx,
                                         float dt)
{
    // Target-all / single-bloom casts keep the camera on the caster.
    if (ctx.stayOnCaster && isValid(ctx.casterTile))
    {
        glideTo(camera, viewSize, ctx.tileSize, ctx.casterTile, dt);
        return false;
    }

    // Cut to each new hit target as the hit index advances.
    if (isValid(ctx.focusTile) && ctx.focusTile != m_lastSnappedTile)
    {
        m_lastSnappedTile = ctx.focusTile;
        snapTo(camera, viewSize, ctx.tileSize, ctx.focusTile);
        return true;
    }
    return false;
}

// A scripted beat (dialogue, cast glide, hit snap) pulls the camera onto a unit.
// The cursor cannot move while one runs, so the offset it started from is still
// a valid framing: remember it and ease back when control returns, instead of
// leaving the view parked on the beat's subject.
void BattleCameraController::trackScriptedBeat(const Camera &camera, const Context &ctx)
{
    const bool inBeat = ctx.dialogueActive || ctx.phase != Phase::Free;

    if (m_inScriptedBeat && !inBeat)
        m_returningToBeat = true;
    // A return still in flight owns the real tactical framing; a chained beat
    // must not overwrite it with a mid-ease offset.
    if (inBeat && !m_inScriptedBeat && !m_returningToBeat)
        m_beatOffset = camera.getOffset();
    m_inScriptedBeat = inBeat;
}

// Eases back to the pre-beat framing, re-aimed by the dead zone for the live
// cursor. Hands control back to the dead zone once settled, or once the map
// bounds hold the camera short of the target.
void BattleCameraController::returnToBeat(Camera &camera, Vec2f viewSize, const Context &ctx,
                                          float dt)
{
    const Vec2f target = deadZoneTarget(camera, viewSize, ctx, m_beatOffset);
    if (easeOffsetTo(camera, target, dt))
    {
        setClampedOffset(camera, target);
        m_returningToBeat = false;
    }
}

void BattleCameraController::resetBeatState()
{
    m_lastSnappedTile.reset();
    m_inScriptedBeat = false;
    m_returningToBeat = false;
}
