#include "battle/systems/BattleCameraController.h"

#include "engine/math/MathUtils.h"
#include "engine/renderer/Camera.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr int kInvalid = -1;

    // Outer band (as a fraction of the viewport on each side) that triggers a
    // scroll: the cursor may roam the central area freely and the camera only
    // moves once it reaches this band near the edge.
    constexpr float kEdgeBandFraction = 0.15f;

    // Cap on the one-time focus glide so a clamped target (map corner) can't
    // keep the controller "focusing" forever.
    constexpr float kFreeFocusDuration = 0.4f;

    [[nodiscard]] bool isValid(Vec2i tile) { return tile.x >= 0 && tile.y >= 0; }

    [[nodiscard]] float screenScale(const Camera &camera)
    {
        return std::max(camera.getZoom(), 0.0001f) * camera.getRenderScale();
    }

    void glideTo(Camera &camera, Vec2f viewSize, Vec2f tileSize, Vec2i tile, float dt)
    {
        const Vec2f iso = tileToIso(tile, static_cast<int>(tileSize.x), static_cast<int>(tileSize.y));
        camera.trackTarget(iso, viewSize, dt);
        camera.clampToBounds();
    }

    void snapTo(Camera &camera, Vec2f viewSize, Vec2f tileSize, Vec2i tile)
    {
        const Vec2f iso = tileToIso(tile, static_cast<int>(tileSize.x), static_cast<int>(tileSize.y));
        const float z = screenScale(camera);
        camera.setOffset(Vec2f{iso.x - (viewSize.x * 0.5f) / z, iso.y - (viewSize.y * 0.5f) / z});
        camera.clampToBounds();
    }

    [[nodiscard]] bool centeredOn(Camera &camera, Vec2f viewSize, Vec2f tileSize, Vec2i tile)
    {
        const Vec2f iso = tileToIso(tile, static_cast<int>(tileSize.x), static_cast<int>(tileSize.y));
        const float z = screenScale(camera);
        const Vec2f desired{iso.x - (viewSize.x * 0.5f) / z, iso.y - (viewSize.y * 0.5f) / z};
        const Vec2f off = camera.getOffset();
        return std::abs(off.x - desired.x) < 0.5f && std::abs(off.y - desired.y) < 0.5f;
    }

    // FFTA-style edge scroll: shift the camera just far enough that the cursor
    // lands back on the inner edge of the band. No-op while the cursor is in
    // the central area, so the camera stays put during normal navigation and
    // stops immediately once the cursor leaves the edge region.
    void scrollCursorIntoBand(Camera &camera, Vec2f viewSize, Vec2f tileSize, Vec2i cursorTile)
    {
        const float z = screenScale(camera);
        if (z <= 0.0f)
            return;

        const Vec2f screen = camera.tileToScreen(cursorTile);
        const float minX = viewSize.x * kEdgeBandFraction;
        const float maxX = viewSize.x - minX;
        const float minY = viewSize.y * kEdgeBandFraction;
        const float maxY = viewSize.y - minY;

        // Desired change in the cursor's screen position (0 when already inside
        // the band). screen = (iso - offset) * z, so offset moves the opposite
        // way by delta / z.
        Vec2f delta{0.0f, 0.0f};
        if (screen.x < minX)
            delta.x = minX - screen.x;
        else if (screen.x > maxX)
            delta.x = maxX - screen.x;
        if (screen.y < minY)
            delta.y = minY - screen.y;
        else if (screen.y > maxY)
            delta.y = maxY - screen.y;

        if (delta.x == 0.0f && delta.y == 0.0f)
            return;

        camera.setOffset(camera.getOffset() - delta / z);
        camera.clampToBounds();
    }
} // namespace

bool BattleCameraController::update(float dt, Camera &camera, Vec2f viewSize,
                                    const Context &ctx)
{
    const Vec2f tileSize = ctx.tileSize;
    const Phase prevPhase = m_lastPhase;
    m_lastPhase = ctx.phase;

    // Scripted dialogue wins over everything: follow the speaker, or hold.
    if (ctx.dialogueActive)
    {
        if (isValid(ctx.speakerTile))
            glideTo(camera, viewSize, tileSize, ctx.speakerTile, dt);
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

    // Free control: stay put and only edge-scroll. The lone exception is a
    // one-time focus glide to the acting unit when the turn changes or when a
    // resolution hands control back, so the acting unit is always framed.
    m_snappedAny = false;
    m_lastSnappedTile = Vec2i{kInvalid, kInvalid};

    const Vec2i focusTile = isValid(ctx.anchorTile) ? ctx.anchorTile : ctx.cursorTile;
    if (ctx.turnSerial != m_freeTurnSerial || prevPhase != Phase::Free)
    {
        m_freeTurnSerial = ctx.turnSerial;
        m_freeFocusing = isValid(focusTile);
        m_freeFocusTimer = 0.0f;
    }

    if (m_freeFocusing && isValid(focusTile))
    {
        glideTo(camera, viewSize, tileSize, focusTile, dt);
        m_freeFocusTimer += dt;
        if (m_freeFocusTimer >= kFreeFocusDuration ||
            centeredOn(camera, viewSize, tileSize, focusTile))
            m_freeFocusing = false;
        return false;
    }

    if (isValid(ctx.cursorTile))
        scrollCursorIntoBand(camera, viewSize, tileSize, ctx.cursorTile);
    return false;
}