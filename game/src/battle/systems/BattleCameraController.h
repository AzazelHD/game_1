#pragma once

#include "engine/math/Vec2.h"

#include <optional>

class Camera;

// Camera focus choreography for one battle frame.
//
// BattleState owns the Camera and supplies a Context snapshot every frame; this
// controller only decides WHERE the camera looks and HOW it gets there. Map
// bounds and which axes can pan at all come from the Camera itself
// (Camera::getPanAxes), so there is a single source of truth for both.
//
//   Free            - camera DEAD ZONE: a centred rectangle covering 80% of the
//                     play field. While the cursor's walkable floor surface is
//                     inside it the camera does not move; once it crosses an
//                     edge the camera pans by the minimum needed to bring it
//                     back (both axes on a diagonal crossing), then stops. No
//                     follow-cam, no recentering.
//
//                     The test uses the tile's FLOOR SURFACE, not its grid
//                     anchor: height layers are Y offsets, so an elevated
//                     tile's floor renders height * halfTileHeight above its
//                     anchor, mirroring the diamond BattleRenderer draws for
//                     the cursor. The footprint is padded symmetrically with
//                     what is drawn around it: Tuning::Camera::kMarkerHeadroom
//                     above (the floating marker) and half a tile below (the
//                     tile art hangs under the diamond), so the top and bottom
//                     gaps match. Dead zone fractions and easing live in
//                     config/UiTuning.h.
//
//   ResolutionCast  - glide to the caster while the cast animation plays.
//   ResolutionTrack - snap to each hit target as its effect plays; target-all
//                     skills (castOncePerArea) stay on the caster.
//
// Dialogue beats glide to the speaker. After any scripted beat the camera eases
// back to the framing it had before the beat started.
//
// An axis the map does not overflow is never panned (the Camera pins it).
//
// update() returns true when the frame used a hard snap, so BattleState can
// neutralise its between-frame render interpolation for a clean cut.
class BattleCameraController
{
public:
    enum class Phase
    {
        Free,
        ResolutionCast,
        ResolutionTrack
    };

    struct Context
    {
        Phase phase = Phase::Free;
        bool dialogueActive = false;
        Vec2i speakerTile{-1, -1};
        Vec2i cursorTile{-1, -1};
        int cursorTileHeight = 0; // elevation level of the cursor's tile (0 = ground)
        Vec2i casterTile{-1, -1};
        Vec2i focusTile{-1, -1}; // current hit target / blast tile
        bool stayOnCaster = false;
        Vec2f tileSize{0.0f, 0.0f};
    };

    bool update(float dt, Camera &camera, Vec2f viewSize, const Context &ctx);

    // Centres the camera on the cursor's floor surface in one step (clamped to
    // the map). Called on scene entry, where update() does not run during the
    // fade-in, so the battle opens on the cursor rather than the map origin.
    void frameCursor(Camera &camera, Vec2f viewSize, const Context &ctx);

private:
    bool updateTrack(Camera &camera, Vec2f viewSize, const Context &ctx, float dt);
    void trackScriptedBeat(const Camera &camera, const Context &ctx);
    void returnToBeat(Camera &camera, Vec2f viewSize, const Context &ctx, float dt);
    void resetBeatState();

    std::optional<Vec2i> m_lastSnappedTile;
    bool m_inScriptedBeat = false;
    bool m_returningToBeat = false;
    Vec2f m_beatOffset{0.0f, 0.0f};
};
