#include "battle/systems/BattleCameraController.h"

#include "engine/math/MathUtils.h"
#include "engine/renderer/Camera.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr int kInvalid = -1;

    // Camera dead zone: a centred rectangle covering 80% of the play field's
    // width and height (a 10% margin on each side). The cursor's walkable floor
    // surface may roam it freely and the camera never moves while it stays
    // inside; the moment it crosses an edge the camera pans by exactly the
    // minimum needed to bring it back — never a follow-cam, never a recenter.
    constexpr float kDeadZoneBoxFraction = 0.80f;

    [[nodiscard]] float deadZoneMargin(float viewExtent)
    {
        float margin = viewExtent * (1.0f - kDeadZoneBoxFraction) * 0.5f;
        // Both margins must never meet: keep a central roaming area even on a
        // very small play field.
        return std::min(margin, std::max(viewExtent * 0.5f - 1.0f, 0.0f));
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

    // Easing time constant of the dead-zone correction. The camera eases towards
    // the position that satisfies the dead zone instead of cutting to it, so
    // walking the cursor across the board reads as one continuous pan rather
    // than a jump per tile step (and never blinks). Small enough to feel
    // immediate (≈90% of the way in 0.35s); the target itself is recomputed
    // every frame from the live cursor tile, so the motion is deterministic.
    constexpr float kEdgeScrollTau = 0.15f;

    [[nodiscard]] bool isValid(Vec2i tile) { return tile.x >= 0 && tile.y >= 0; }

    // The board's on-screen size against the play field. Mirrors (and must stay
    // in step with) Camera::clampToBounds()'s own fit test, which is what decides
    // whether an axis can be centred/pinned or only clamped: the vertical extent
    // includes `mapTopExtra`, the art raised tiles draw above the top anchor row
    // (Camera::setMapBoundsTopExtra).
    struct BoardFit
    {
        bool pansX = true;
        bool pansY = true;
        [[nodiscard]] bool any() const { return pansX || pansY; }
    };

    [[nodiscard]] BoardFit boardFit(Vec2i mapSize, Vec2f viewSize, Vec2f tileSize,
                                    float mapTopExtra, float z)
    {
        if (!isValid(mapSize) || z <= 0.0f)
            return BoardFit{};
        return BoardFit{boardExtent(mapSize, tileSize.x, z) > viewSize.x,
                        boardExtent(mapSize, tileSize.y, z) + mapTopExtra * z > viewSize.y};
    }

    [[nodiscard]] float screenScale(const Camera &camera)
    {
        return std::max(camera.getZoom(), 0.0001f) * camera.getRenderScale();
    }

    // On-screen rectangle of a tile's WALKABLE FLOOR SURFACE: the diamond
    // BattleRenderer::drawCursorTicks draws for the cursor — centred on the
    // tile's anchor and lifted by the tile's height (height levels are Y
    // offsets, so an elevated tile's floor renders higher than its grid
    // position). This is the surface the cursor actually picks and units stand
    // on, NOT the base grid anchor and NOT the sprite art above it.
    struct FloorFootprint
    {
        float left = 0.0f;
        float right = 0.0f;
        float top = 0.0f;
        float bottom = 0.0f;
    };

    [[nodiscard]] FloorFootprint floorFootprintAtOffset(Camera &camera, Vec2f tileSize,
                                                        Vec2i tile, int tileHeight, float z,
                                                        Vec2f offset)
    {
        // tileToScreen is (iso - offset) * z, so the anchor the camera WOULD
        // produce at `offset` is the live one shifted by the offset delta — no
        // need to re-derive the iso projection.
        const Vec2f anchor =
            camera.tileToScreen(tile) + (camera.getOffset() - offset) * z;
        const float halfW = tileSize.x * 0.5f * z;
        const float halfH = tileSize.y * 0.5f * z;
        const float elev = static_cast<float>(tileHeight) * halfH;
        return FloorFootprint{anchor.x - halfW, anchor.x + halfW,
                              anchor.y - elev - halfH, anchor.y - elev + halfH};
    }

    // On-screen rectangle of the tile's floor surface at the camera's LIVE
    // offset (see floorFootprintAtOffset).
    [[nodiscard]] FloorFootprint floorFootprint(Camera &camera, Vec2f tileSize, Vec2i tile,
                                               int tileHeight, float z)
    {
        return floorFootprintAtOffset(camera, tileSize, tile, tileHeight, z,
                                      camera.getOffset());
    }

    void glideTo(Camera &camera, Vec2f viewSize, Vec2f tileSize, Vec2i tile, float dt)
    {
        const Vec2f iso = tileToIso(tile, static_cast<int>(tileSize.x), static_cast<int>(tileSize.y));
        camera.trackTarget(iso, viewSize, dt);
        // Slide (not center) so a board that overflows one axis only can still
        // use the slack on the fitting axis to frame the acting unit: centering
        // would pin that axis to the middle and the glide would stop short of its
        // target. Entirely-fitting boards never reach here — update() holds the
        // camera still instead (see boardFit()).
        camera.clampToBounds(Camera::SmallMapFit::Slide);
    }

    void snapTo(Camera &camera, Vec2f viewSize, Vec2f tileSize, Vec2i tile)
    {
        const Vec2f iso = tileToIso(tile, static_cast<int>(tileSize.x), static_cast<int>(tileSize.y));
        const float z = screenScale(camera);
        camera.setOffset(Vec2f{iso.x - (viewSize.x * 0.5f) / z, iso.y - (viewSize.y * 0.5f) / z});
        // Slide for the same reason as glideTo(): only reachable on a board that
        // overflows at least one axis.
        camera.clampToBounds(Camera::SmallMapFit::Slide);
    }

    // Minimum camera-offset displacement that brings the floor surface back
    // inside the dead zone, or zero while it is already inside. screen = (iso -
    // offset) * z, so the offset has to move the opposite way by delta / z. A
    // diagonal crossing corrects both axes independently.
    //
    // Pan is gated per axis by BoardFit: if no map content exists beyond the
    // play field on an axis, that axis never pans (clampToBounds would pin it
    // there anyway), so a board that fits inside the play field never moves.
    [[nodiscard]] Vec2f deadZoneCorrection(const FloorFootprint &floor, Vec2f viewSize,
                                           BoardFit fit)
    {
        const float marginX = deadZoneMargin(viewSize.x);
        const float marginY = deadZoneMargin(viewSize.y);

        Vec2f delta{0.0f, 0.0f};
        if (fit.pansX)
        {
            if (floor.left < marginX)
                delta.x = marginX - floor.left;
            else if (floor.right > viewSize.x - marginX)
                delta.x = (viewSize.x - marginX) - floor.right;
        }
        if (fit.pansY)
        {
            if (floor.top < marginY)
                delta.y = marginY - floor.top;
            else if (floor.bottom > viewSize.y - marginY)
                delta.y = (viewSize.y - marginY) - floor.bottom;
        }
        return delta;
    }

    // Dead-zone correction: pan the camera by exactly the minimum displacement
    // that brings the cursor's walkable floor surface back inside the dead zone
    // (see deadZoneMargin). No-op while the surface is inside it, so the camera
    // never moves during normal navigation and stops the instant the cursor
    // returns.
    //
    // The surface (not the anchor) is what gets tested: an elevated tile's floor
    // sits `height * halfTileHeight` above its grid position on screen, so a
    // raised tile near the top of the zone still pulls the camera up to it.
    void scrollCursorIntoDeadZone(Camera &camera, Vec2f viewSize, Vec2f tileSize,
                                  Vec2i cursorTile, int cursorTileHeight, BoardFit fit,
                                  float dt)
    {
        const float z = screenScale(camera);
        if (z <= 0.0f)
            return;

        const Vec2f delta = deadZoneCorrection(
            floorFootprint(camera, tileSize, cursorTile, cursorTileHeight, z), viewSize, fit);
        if (delta.x == 0.0f && delta.y == 0.0f)
            return;

        // Ease towards the fixing position rather than cutting to it. The delta
        // is recomputed from the live offset every frame, so the camera settles
        // on the dead-zone edge and stops, with no overshoot or drift past it.
        const Vec2f current = camera.getOffset();
        const Vec2f target = current - delta / z;
        const float ease = 1.0f - std::exp(-std::max(dt, 0.0f) / kEdgeScrollTau);
        camera.setOffset(current + (target - current) * ease);
        // Slide, not center: only the axis being panned is moving, and Slide
        // leaves the other (fitting) axis exactly where it is. Center would pin
        // that axis to the middle and yank it back after a framing beat.
        camera.clampToBounds(Camera::SmallMapFit::Slide);
    }

    // Instant variant of the dead-zone correction: same minimum displacement,
    // applied in one step (no easing). Used on scene entry, where the drive does
    // not run during the fade-in.
    void correctDeadZoneNow(Camera &camera, Vec2f viewSize, Vec2f tileSize, Vec2i cursorTile,
                            int cursorTileHeight, BoardFit fit)
    {
        const float z = screenScale(camera);
        if (z <= 0.0f)
            return;

        const Vec2f delta = deadZoneCorrection(
            floorFootprint(camera, tileSize, cursorTile, cursorTileHeight, z), viewSize, fit);
        if (delta.x == 0.0f && delta.y == 0.0f)
            return;

        camera.setOffset(camera.getOffset() - delta / z);
        camera.clampToBounds(Camera::SmallMapFit::Slide);
    }
} // namespace

bool BattleCameraController::update(float dt, Camera &camera, Vec2f viewSize,
                                    const Context &ctx)
{
    const Vec2f tileSize = ctx.tileSize;

    // No map content exists beyond the play field on either axis, so there is
    // nothing to pan toward: the camera holds perfectly still for the whole
    // battle. Only an axis the board *overflows* can carry something off screen,
    // so only such an axis is ever panned (see scrollCursorIntoDeadZone). This
    // is also what keeps the camera dead still on the small boards (e.g. the
    // 5x5 tutorial) and on the fitting axis of a half-overflowing board.
    const BoardFit fit = boardFit(ctx.mapSize, viewSize, tileSize, ctx.mapTopExtra,
                                  screenScale(camera));
    if (isValid(ctx.mapSize) && !fit.any())
    {
        m_snappedAny = false;
        m_inScriptedBeat = false;
        m_returningToBeat = false;
        return false;
    }

    // A scripted beat (dialogue glide, cast glide, hit snap) takes the camera
    // AWAY from the tactical framing to centre it on a unit. Remember the offset
    // it starts from, so that when the beat ends control resumes there instead
    // of wherever the beat parked the camera.
    //
    // The cursor cannot move while a beat runs — dialogue, ResolvingHits and
    // EnemyAction all block tactical input — so the remembered offset is still a
    // dead-zone-valid framing for the live cursor: restoring it is not a
    // recentering, it is undoing the beat's excursion.
    const bool scriptedBeat = ctx.dialogueActive || ctx.phase != Phase::Free;
    if (!scriptedBeat && m_inScriptedBeat)
        m_returningToBeat = true; // beat just ended: take the camera back
    // Skipped while a previous return is still in flight — that return's
    // framing is the real tactical one, not the transient mid-ease offset a
    // chained beat would hand us.
    if (scriptedBeat && !m_inScriptedBeat && !m_returningToBeat)
        m_beatOffset = camera.getOffset();
    m_inScriptedBeat = scriptedBeat;

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

    // Free control: the dead zone is the only driver. No focus glide, no
    // recentering — the cursor's walkable floor surface alone decides whether
    // the camera moves, and then only by the minimum correction needed.
    m_snappedAny = false;
    m_lastSnappedTile = Vec2i{kInvalid, kInvalid};

    if (!isValid(ctx.cursorTile) || !isValid(ctx.mapSize))
        return false;

    // A scripted beat just ended and left the camera parked on the unit it
    // centred (an enemy after its attack, say). The dead zone alone would keep
    // it there: the cursor still sits comfortably inside the box of that parked
    // position, so nothing would ever pull the view back. Ease to the framing
    // the beat displaced instead, so the camera hands back where the player was
    // looking.
    //
    // The target is re-aimed by the dead zone for the live cursor, so a cursor
    // that somehow moved while the beat ran still gets a valid framing; in
    // practice the cursor is frozen during a beat, so this is a straight restore.
    if (m_returningToBeat)
    {
        const float z = screenScale(camera);
        const Vec2f delta = deadZoneCorrection(
            floorFootprintAtOffset(camera, tileSize, ctx.cursorTile, ctx.cursorTileHeight, z,
                                   m_beatOffset),
            viewSize, fit);
        const Vec2f target = m_beatOffset - delta / z;

        const Vec2f before = camera.getOffset();
        const float ease = 1.0f - std::exp(-std::max(dt, 0.0f) / kEdgeScrollTau);
        const Vec2f desired = before + (target - before) * ease;
        camera.setOffset(desired);
        camera.clampToBounds(Camera::SmallMapFit::Slide);
        const Vec2f after = camera.getOffset();

        // Settled once the target is reached, or once the map bounds hold the
        // camera short of it (the clamp rejected the eased position, so further
        // easing would be a no-op loop that locks the dead zone out forever).
        // Hand back either way: the cursor's floor is inside the box at the
        // target, and the dead zone takes over from whatever we stopped at.
        const bool reached =
            std::abs(after.x - target.x) < 0.5f && std::abs(after.y - target.y) < 0.5f;
        const bool pinned = after.x != desired.x || after.y != desired.y;
        if (reached || pinned)
        {
            camera.setOffset(target);
            camera.clampToBounds(Camera::SmallMapFit::Slide);
            m_returningToBeat = false;
        }
        return false;
    }

    scrollCursorIntoDeadZone(camera, viewSize, tileSize, ctx.cursorTile,
                             ctx.cursorTileHeight, fit, dt);
    return false;
}

void BattleCameraController::focusImmediately(Camera &camera, Vec2f viewSize, const Context &ctx)
{
    // Fresh battle: no beat has taken the camera anywhere yet.
    m_inScriptedBeat = false;
    m_returningToBeat = false;

    if (!isValid(ctx.cursorTile) || viewSize.x <= 0.0f || viewSize.y <= 0.0f)
        return;

    const BoardFit fit = boardFit(ctx.mapSize, viewSize, ctx.tileSize, ctx.mapTopExtra,
                                  screenScale(camera));
    // A board that fits the play field is already fully visible from the scene's
    // centred clamp: correcting would only shove it off centre for nothing, and
    // the controller is not going to move it again afterwards.
    if (isValid(ctx.mapSize) && !fit.any())
        return;

    correctDeadZoneNow(camera, viewSize, ctx.tileSize, ctx.cursorTile, ctx.cursorTileHeight, fit);
}
