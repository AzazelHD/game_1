#pragma once

#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"

// Framed battle layout — permanent side rails with a crisp play field between
// them (see docs/design/FRAMED_LAYOUT.md).
//
// The rails are a designed layout element, never leftover space, so they exist
// at every resolution. The world renders inside playRect at a whole number of
// device pixels per source texel (artScale), so it stays crisp with no shimmer
// at any window size. playRect is centred, so its centre is the window centre —
// the world and the UI/design transform share the same centre.
struct BattleLayout
{
    Vec2i output{0, 0};         // native output size this layout was built for
    int railW = 0;              // width of each side rail (native px)
    Recti playRect{0, 0, 0, 0}; // world viewport (native px, centred)
    int artScale = 2;           // integer device px per source texel
    float worldScale = 2.0f;    // continuous device px per source texel implied by
                                // the window height (worldScale >= artScale); the
                                // camera folds the remainder into its zoom so
                                // tiles keep their size relative to the window
                                // at resolutions between art-scale steps
};

// Builds the layout for a native output size. railW tracks the window width
// with a floor and ceiling so the rails stay usable without eating the
// battlefield; artScale keeps the battlefield at the same physical size relative
// to the window at every resolution while remaining a whole number (crisp), and
// worldScale carries the exact ratio for the camera to fold in.
BattleLayout computeBattleLayout(int outW, int outH);
