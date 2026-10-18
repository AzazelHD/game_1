#include "renderer/BattleLayout.h"

#include <algorithm>
#include <cmath>

namespace
{
    // Rails carry the actor card / turn queue: ~18% of the window each side,
    // clamped so they stay readable on small windows and never eat the
    // battlefield on very wide ones.
    constexpr float kRailFraction = 0.18f;
    constexpr int kRailMin = 200;
    constexpr int kRailMax = 340;

    // Baseline world scale: 2 device px per source texel at 720p (the fixed
    // scale the world used before the framed layout). artScale is the largest
    // whole multiple that still fits the window height, so the world buffer and
    // the sizes baked into it (tile quads, sprite boxes) stay whole and crisp,
    // and tiles never render *larger* than the baseline ratio. The window height
    // is deliberately not rounded to the nearest multiple: a 900p window would
    // then jump to 3x and magnify the battlefield by 50%, which reads as a
    // zoom-in rather than a bigger viewport.
    //
    // worldScale is the *continuous* ratio the baseline implies (window height /
    // 360). It equals artScale whenever the window height is an exact multiple of
    // 360 (720p: 2, 1080p: 3, 1440p: 4), which is where the framed layout is
    // exact; in between (900p: 2.5 vs artScale 2) the remainder is folded into
    // the battle camera's zoom by BattleState, so tiles keep the same size
    // *relative to the window* at every resolution instead of the world quietly
    // shrinking and showing more map.
    constexpr float kBaselineViewH = 360.0f;
}

BattleLayout computeBattleLayout(int outW, int outH)
{
    BattleLayout layout;
    layout.output = Vec2i{outW, outH};
    if (outW <= 0 || outH <= 0)
        return layout;

    int rail = static_cast<int>(std::lround(static_cast<float>(outW) * kRailFraction));
    rail = std::clamp(rail, kRailMin, kRailMax);
    // Never let the rails squeeze the play field below half the window.
    rail = std::min(rail, outW / 4);

    layout.railW = rail;

    const int artScale = static_cast<int>(std::floor(static_cast<float>(outH) / kBaselineViewH));
    layout.artScale = std::clamp(artScale, 1, 8);
    layout.worldScale = std::max(static_cast<float>(outH) / kBaselineViewH, 1.0f);

    layout.playRect = Recti{rail, 0, outW - 2 * rail, outH};
    return layout;
}
