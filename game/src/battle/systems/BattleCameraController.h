#pragma once

#include "engine/math/Vec2.h"

class Camera;

// Camera focus choreography for one battle frame.
//
// Slices battle presentation into focus moments and drives the engine Camera.
// BattleState owns the Camera and supplies a Context snapshot every frame;
// this controller only decides WHERE the camera should look and HOW (glide vs
// snap) it should get there. All map->iso math stays inside, so the controller
// is self-contained and needs nothing but the tile dimensions and the board
// size in tiles.
//
//   Free            - camera DEAD ZONE. A centred rectangle covering 80% of the
//                     play field (10% margin on each side) never moves the
//                     camera: while the cursor's walkable floor surface roams
//                     it, the camera stays put. The moment the surface crosses
//                     an edge, the camera pans by exactly the minimum needed to
//                     bring it back inside — one tile over the line moves the
//                     camera by that one tile's worth, a diagonal step corrects
//                     both axes, a jump far outside corrects the whole gap and
//                     nothing more. No follow-cam, no recentering: the camera
//                     stops as soon as the cursor is back inside.
//
//                     The test runs on the tile's WALKABLE FLOOR SURFACE, not
//                     its grid anchor: height layers are Y offsets, so an
//                     elevated tile's floor renders (and is stood on) higher
//                     than its base position. The footprint here mirrors the
//                     diamond BattleRenderer draws for the cursor (anchor lifted
//                     by height * halfTileHeight), so a raised tile near the top
//                     of the dead zone still pulls the camera up to it.
//
// A board that fits inside the play field is never panned at all: no map content
// exists beyond the play field on either axis, so there is nothing to pan toward
// and update() holds the camera still for the whole battle. The same holds per
// axis: an axis the board fits on is skipped even when the other overflows.
//   ResolutionCast  - the cast beat: glide to the caster while the skill's
//                     cast animation plays (covers the pre-effect timer).
//   ResolutionTrack - after the cast, each target hit SNAPS into frame as its
//                     effect/damage plays; control then returns to the dead
//                     zone, which re-corrects minimally from wherever the last
//                     snap left the camera. Target-all skills
//                     (castOncePerArea) never leave caster.
//
// update() returns true when the frame used an immediate snap, so BattleState
// can neutralise its between-frame render interpolation for a hard cut (the
// "snap between targets" the design calls for).
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
        bool inCombat = false;
        bool dialogueActive = false;
        Vec2i speakerTile{-1, -1};
        Vec2i cursorTile{-1, -1};
        int cursorTileHeight = 0; // elevation level of the cursor's tile (0 = ground)
        Vec2i casterTile{-1, -1};
        Vec2i focusTile{-1, -1}; // current hit target / blast tile
        bool stayOnCaster = false;
        Vec2f tileSize{0.0f, 0.0f};
        Vec2i mapSize{-1, -1}; // board size in tiles, for the dead zone
        float mapTopExtra = 0.0f; // world units of art above the top anchor row
    };

    // Returns true when the frame applied a hard snap; BattleState then
    // neutralises its between-frame render interpolation for a clean cut.
    bool update(float dt, Camera &camera, Vec2f viewSize, const Context &ctx);

    // Applies the dead zone's minimum correction in a single step (no easing).
    // Called on scene entry: the camera drive does not run while the fade-in
    // transition plays, so without this the battle would fade in on the clamped
    // map origin and then visibly pan once control starts. A no-op when the
    // cursor's floor surface is already inside the dead zone, and a no-op for a
    // board that fits the play field — it is fully visible from the scene's
    // centred clamp, and nothing will move it again.
    void focusImmediately(Camera &camera, Vec2f viewSize, const Context &ctx);

private:
    bool m_snappedAny = false;
    Vec2i m_lastSnappedTile{-1, -1};

    // Framing bookkeeping for scripted beats (dialogue glide, cast glide, hit
    // snap). Those beats take the camera AWAY from the tactical framing by
    // centreing on a unit, and when they end the cursor usually still sits
    // comfortably inside the dead zone of that parked position — so the dead
    // zone alone would leave the view stuck on the beat's subject (an enemy,
    // say) instead of where the player was looking. Remember the offset the
    // beat started from and ease back to it when control returns.
    //
    // The return is re-aimed by the dead zone for the live cursor every frame,
    // so a cursor moved while the beat ran still pulls the camera correctly; it
    // settles as soon as it stops covering ground and hands back to the dead
    // zone, which is then a no-op (the cursor's floor is inside by then).
    bool m_inScriptedBeat = false;
    bool m_returningToBeat = false;
    Vec2f m_beatOffset{0.0f, 0.0f};
};