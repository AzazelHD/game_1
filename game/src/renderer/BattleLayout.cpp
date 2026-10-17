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
    // whole multiple at or below the window height, so the battlefield never
    // appears smaller than that baseline.
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

    layout.playRect = Recti{rail, 0, outW - 2 * rail, outH};
    return layout;
}
