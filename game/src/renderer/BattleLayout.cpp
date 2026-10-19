#include "renderer/BattleLayout.h"

#include "config/UiTuning.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kRailFraction = Tuning::Layout::kRailFraction;
    constexpr int kRailMin = Tuning::Layout::kRailMin;
    constexpr int kRailMax = Tuning::Layout::kRailMax;

    // artScale is the largest whole multiple of the baseline that fits the window
    // height, so the world buffer and the sizes baked into it stay whole and
    // crisp, and tiles never render larger than the baseline ratio. worldScale is
    // the continuous ratio (window height / baseline); the remainder between the
    // two (e.g. 900p: 2.5 vs 2) is folded into the battle camera's zoom by
    // BattleState so tiles keep the same size relative to the window.
    constexpr float kBaselineViewH = Tuning::Layout::kBaselineViewH;
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
