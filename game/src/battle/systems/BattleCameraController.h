#pragma once

#include "engine/math/Vec2.h"

class Camera;

// Camera focus choreography for one battle frame.
//
// Slices battle presentation into focus moments and drives the engine Camera.
// BattleState owns the Camera and supplies a Context snapshot every frame;
// this controller only decides WHERE the camera should look and HOW (glide vs
// snap) it should get there. All map->iso math stays inside, so the controller
// is self-contained and needs nothing but tile dimensions.
//
//   Free            - FFTA-style edge-scroll. The camera stays put while the
//                     cursor roams the central area; it scrolls only when the
//                     cursor reaches a thin band near the viewport edge, and
//                     only far enough to keep the cursor inside that band. It
//                     never continuously follows the cursor or the active unit.
//                     It glides once to the active unit at the start of a turn
//                     (or on returning from a resolution) so the acting unit is
//                     always framed, then stays put again.
//   ResolutionCast  - the cast beat: glide to the caster while the skill's
//                     cast animation plays (covers the pre-effect timer).
//   ResolutionTrack - after the cast, each target hit SNAPS into frame as its
//                     effect/damage plays, then the camera returns to the
//                     active unit when the resolve closes (Free re-anchor).
//                     Target-all skills (castOncePerArea) never leave caster.
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
        Vec2i anchorTile{-1, -1}; // active unit during free control
        int turnSerial = 0;       // bumped when a new unit's turn starts
        Vec2i casterTile{-1, -1};
        Vec2i focusTile{-1, -1}; // current hit target / blast tile
        bool stayOnCaster = false;
        Vec2f tileSize{0.0f, 0.0f};
    };

    // Returns true when the frame applied a hard snap; BattleState then
    // neutralises its between-frame render interpolation for a clean cut.
    bool update(float dt, Camera &camera, Vec2f viewSize, const Context &ctx);

private:
    bool m_snappedAny = false;
    Vec2i m_lastSnappedTile{-1, -1};

    // Free-control edge-scroll bookkeeping. The one-time focus glide runs when
    // the turn changes (or on returning from a resolution phase); afterwards
    // the camera only scrolls to keep the cursor inside the edge band.
    Phase m_lastPhase = Phase::Free;
    int m_freeTurnSerial = -1;
    bool m_freeFocusing = false;
    float m_freeFocusTimer = 0.0f;
};