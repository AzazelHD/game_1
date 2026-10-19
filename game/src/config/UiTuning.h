#pragma once

// Central, designer-facing tuning knobs for the battle camera, cursor marker and
// framed layout. Everything here is a plain constant: edit, rebuild, look.
// See docs/TUNING.md for the full "where do I change X" map (fonts, windows, ...).

namespace Tuning
{
    namespace Cursor
    {
        // World units the red marker floats above the floor diamond's top vertex.
        constexpr float kHoverOffset = 50.0f;
        constexpr float kTriangleHalfWidth = 8.0f;
        constexpr float kTriangleHeight = 20.0f;
    }

    namespace Camera
    {
        // Dead zone: centred share of the play field the cursor may roam without
        // moving the camera. Margin per side = (1 - fraction) / 2. Higher = pans
        // later; lower = pans earlier. X and Y are independent.
        constexpr float kDeadZoneFractionX = 0.55f;
        constexpr float kDeadZoneFractionY = 0.70f;

        // Extra world units reserved ABOVE the cursor tile's floor so the floating
        // marker stays on screen. Tracks the marker's real reach (the marker is
        // drawn kHoverOffset above the floor), so lowering kHoverOffset shrinks
        // the top gap too. Reserving less than this clips the marker whenever the
        // cursor sits on the dead zone's top edge. Also feeds the map's top
        // scroll bound (BattleState::computeCameraTopExtra).
        constexpr float kMarkerHeadroom = Cursor::kHoverOffset;

        // Pan smoothing time constant, seconds (~90% of the way in 2.3x this).
        // Smaller = snappier, larger = floatier.
        constexpr float kEaseTau = 0.15f;

        // Follow time of scripted glides (cast / dialogue), seconds.
        constexpr float kGlideSeconds = 0.2f;

        // Screen pixels of slack kept around the map so unit sprites at the edges
        // are not clipped. Also decides whether an axis is pannable at all.
        constexpr float kMapEdgeMarginPx = 112.0f;
    }

    namespace Layout
    {
        // Side rails (actor card / turn queue): share of window width, clamped.
        constexpr float kRailFraction = 0.18f;
        constexpr int kRailMin = 200;
        constexpr int kRailMax = 340;

        // Window height (px) that corresponds to 1 device px per source texel;
        // artScale = floor(windowH / this). 360 => 2x at 720p.
        constexpr float kBaselineViewH = 360.0f;
    }
}
