#include "battle/systems/BattleCameraController.h"

#include "engine/math/MathUtils.h"
#include "engine/renderer/Camera.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr int kInvalid = -1;

    // Comfort box, as a fraction of the viewport kept free on each side: the
    // cursor may roam the central box freely and the camera only moves once the
    // cursor's tile leaves it.
    constexpr float kEdgeBandFraction = 0.15f;

    // Comfort box margin. clampToBounds() never lets the cursor come closer to
    // the viewport edge than its map-bounds margin (that margin keeps unit
    // sprites on screen at a map edge), so a box narrower than the margin is
    // unreachable on an overflowing axis and the camera silently stops
    // scrolling there — which is what the vertical axis used to do, since
    // 0.15 * 720px of height is less than the 112px margin.
    constexpr float kEdgeBandComfort = 64.0f;

    // A box that covers a whole board would never be left, leaving a map that
    // fits the play field with nothing to scroll to (the camera would look dead
    // on every small map, e.g. deployment on the tutorial duel). The box is
    // therefore also capped to this fraction of the board, so the cursor always
    // has board left to leave and the camera always reacts by at most
    // (1 - cover) of the board drifting across the slack.
    constexpr float kEdgeBandBoardCover = 0.7f;

    [[nodiscard]] float edgeBand(const Camera &camera, float viewExtent, float boardExtent)
    {
        float band = std::max(viewExtent * kEdgeBandFraction,
                              camera.getMapBoundsMargin() + kEdgeBandComfort);
        band = std::max(band, (viewExtent - boardExtent * kEdgeBandBoardCover) * 0.5f);
        // Both side bands must never meet: keep a central roaming area even on
        // a very small play field.
        return std::min(band, std::max(viewExtent * 0.5f - 1.0f, 0.0f));
    }

    // On-screen extent of the board's iso diamond along one axis, given that
    // axis' tile extent (tile width for x, tile height for y) and the screen
    // scale. The four corner tiles project to a diamond whose width is (w + h)
    // half-tile-widths and whose height is (w + h) half-tile-heights, so only
    // the tile count sum matters.
    [[nodiscard]] float boardExtent(Vec2i mapSize, float tileExtent, float pixelsPerIsoUnit)
    {
        return static_cast<float>(mapSize.x + mapSize.y) * tileExtent * 0.5f * pixelsPerIsoUnit;
    }

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
        // Slide (not center) so a map that fits the play field can still be
        // panned: centering would pin it to the middle and the glide would stop
        // short of its target, leaving the free-scroll pass to jolt the board
        // into the comfort box on the very next frame.
        camera.clampToBounds(Camera::SmallMapFit::Slide);
    }

    void snapTo(Camera &camera, Vec2f viewSize, Vec2f tileSize, Vec2i tile)
    {
        const Vec2f iso = tileToIso(tile, static_cast<int>(tileSize.x), static_cast<int>(tileSize.y));
        const float z = screenScale(camera);
        camera.setOffset(Vec2f{iso.x - (viewSize.x * 0.5f) / z, iso.y - (viewSize.y * 0.5f) / z});
        camera.clampToBounds(Camera::SmallMapFit::Slide);
    }

    [[nodiscard]] bool centeredOn(Camera &camera, Vec2f viewSize, Vec2f tileSize, Vec2i tile)
    {
        const Vec2f iso = tileToIso(tile, static_cast<int>(tileSize.x), static_cast<int>(tileSize.y));
        const float z = screenScale(camera);
        const Vec2f desired{iso.x - (viewSize.x * 0.5f) / z, iso.y - (viewSize.y * 0.5f) / z};
        const Vec2f off = camera.getOffset();
        return std::abs(off.x - desired.x) < 0.5f && std::abs(off.y - desired.y) < 0.5f;
    }

    // FFTA-style edge scroll: shift the camera just far enough that the cursor's
    // tile lands back inside the comfort box. No-op while the tile is inside it,
    // so the camera stays put during normal navigation and stops immediately
    // once the cursor leaves the edge region.
    //
    // The test uses the tile's whole on-screen footprint, not just its anchor:
    // a tile's iso anchor is the TOP corner of its diamond, which reaches a full
    // tile height below that anchor and half a tile width to either side. In iso
    // the tile's height *is* the y offset between rows, so comparing the anchor
    // alone would let the bottom half of the tile (and the sprite standing on
    // it) hang outside the box before the camera reacts.
    void scrollCursorIntoBand(Camera &camera, Vec2f viewSize, Vec2f tileSize, Vec2i mapSize,
                              Vec2i cursorTile)
    {
        const float z = screenScale(camera);
        if (z <= 0.0f)
            return;

        const Vec2f anchor = camera.tileToScreen(cursorTile);
        const float halfW = tileSize.x * 0.5f * z;
        const float tileH = tileSize.y * z;
        const float left = anchor.x - halfW;
        const float right = anchor.x + halfW;
        const float top = anchor.y;
        const float bottom = anchor.y + tileH;

        const float bandX = edgeBand(camera, viewSize.x, boardExtent(mapSize, tileSize.x, z));
        const float bandY = edgeBand(camera, viewSize.y, boardExtent(mapSize, tileSize.y, z));
        const float minX = bandX;
        const float maxX = viewSize.x - bandX;
        const float minY = bandY;
        const float maxY = viewSize.y - bandY;

        // Desired change in the cursor tile's screen position (0 when already
        // inside the box). screen = (iso - offset) * z, so offset moves the
        // opposite way by delta / z.
        Vec2f delta{0.0f, 0.0f};
        if (left < minX)
            delta.x = minX - left;
        else if (right > maxX)
            delta.x = maxX - right;
        if (top < minY)
            delta.y = minY - top;
        else if (bottom > maxY)
            delta.y = maxY - bottom;

        if (delta.x == 0.0f && delta.y == 0.0f)
            return;

        camera.setOffset(camera.getOffset() - delta / z);
        // Slide, not center: on a map that fits the play field the camera must
        // still be able to drift across the slack, otherwise a fitting axis is
        // pinned and the box can never be satisfied.
        camera.clampToBounds(Camera::SmallMapFit::Slide);
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

    if (isValid(ctx.cursorTile) && isValid(ctx.mapSize))
        scrollCursorIntoBand(camera, viewSize, tileSize, ctx.mapSize, ctx.cursorTile);
    return false;
}