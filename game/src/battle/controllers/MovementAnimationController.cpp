#include "battle/controllers/MovementAnimationController.h"
#include "engine/math/MathUtils.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kPi = 3.1415926535897932384626433832795f;
}

void MovementAnimationController::begin(Unit *unit, std::vector<Vec2i> path, std::vector<int> heights,
                                        std::function<void()> onComplete)
{
    if (!unit || path.size() < 2 || path.size() != heights.size())
    {
        m_unit = nullptr;
        if (onComplete)
            onComplete();
        return;
    }

    m_unit = unit;
    m_path = std::move(path);
    m_heights = std::move(heights);
    m_index = 0;
    m_segmentElapsed = 0.0f;
    m_onComplete = std::move(onComplete);
}

bool MovementAnimationController::currentSegmentIsHop() const
{
    if (m_index + 1 >= m_heights.size())
        return false;
    return m_heights[m_index] != m_heights[m_index + 1];
}

float MovementAnimationController::currentSegmentDuration() const
{
    return currentSegmentIsHop() ? m_hopSecondsPerTile : m_walkSecondsPerTile;
}

void MovementAnimationController::update(float dt)
{
    if (!isAnimating())
        return;

    m_segmentElapsed += dt;
    while (m_segmentElapsed >= currentSegmentDuration())
    {
        m_segmentElapsed -= currentSegmentDuration();
        ++m_index;

        if (m_index >= m_path.size() - 1)
        {
            finish();
            return;
        }
    }
}

MovementAnimationController::SegmentTiming MovementAnimationController::computeSegmentTiming(bool isHop) const
{
    SegmentTiming out;
    const float duration = currentSegmentDuration();
    out.t = std::clamp(m_segmentElapsed / duration, 0.0f, 1.0f);

    if (!isHop)
    {
        out.moveT = out.t;
        return out;
    }

    if (out.t < m_hopPauseFraction)
    {
        out.inPause = true;
        out.moveT = 0.0f;
    }
    else
    {
        const float span = std::max(0.0001f, 1.0f - m_hopPauseFraction);
        out.moveT = std::clamp((out.t - m_hopPauseFraction) / span, 0.0f, 1.0f);
    }
    return out;
}

Vec2f MovementAnimationController::getVisualTilePos() const
{
    if (m_path.empty())
        return Vec2f{0.0f, 0.0f};

    if (!isAnimating() || m_index >= m_path.size() - 1)
    {
        const Vec2i &last = m_path.back();
        return Vec2f{static_cast<float>(last.x), static_cast<float>(last.y)};
    }

    const bool isHop = currentSegmentIsHop();
    const SegmentTiming timing = computeSegmentTiming(isHop);

    const Vec2i &from = m_path[m_index];
    const Vec2i &to = m_path[m_index + 1];

    const float eased = isHop ? easeInOut(timing.moveT) : timing.moveT;

    return Vec2f{
        lerp(static_cast<float>(from.x), static_cast<float>(to.x), eased),
        lerp(static_cast<float>(from.y), static_cast<float>(to.y), eased),
    };
}

float MovementAnimationController::getVisualHeight() const
{
    if (m_path.empty() || m_heights.empty())
        return 0.0f;

    if (!isAnimating() || m_index >= m_heights.size() - 1)
        return static_cast<float>(m_heights.back());

    const bool isHop = currentSegmentIsHop();
    const SegmentTiming timing = computeSegmentTiming(isHop);

    const float fromH = static_cast<float>(m_heights[m_index]);
    if (!isHop)
        return fromH; // == toH on a flat segment

    if (timing.inPause)
        return fromH;

    const float toH = static_cast<float>(m_heights[m_index + 1]);
    const float baseHeight = lerp(fromH, toH, timing.moveT);
    const float arc = m_hopArcHeight * std::sin(kPi * timing.moveT); // peaks at moveT=0.5
    return baseHeight + arc;
}

void MovementAnimationController::finish()
{
    std::function<void()> callback = std::move(m_onComplete);
    m_unit = nullptr;
    m_path.clear();
    m_heights.clear();
    m_index = 0;
    m_segmentElapsed = 0.0f;
    m_onComplete = nullptr;

    if (callback)
        callback();
}