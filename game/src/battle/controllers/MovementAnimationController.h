#pragma once

#include "engine/math/Vec2.h"

#include <cstddef>
#include <functional>
#include <vector>

class Unit;

// Plays back a unit's movement as tile-to-tile segments: a plain glide when
// consecutive tiles share height, or a "hop" when height changes — pause at
// the tile center, arc through the air, land at the next tile center.
//
// Purely visual — the unit's logical position/occupancy is committed by the
// caller BEFORE playback starts. This class only tracks an interpolated
// tile position + elevation for rendering, and fires a completion callback
// on arrival. It knows nothing about pixels/iso-projection — elevation is
// expressed in the same "tile height" units GameTile::height already uses.
class MovementAnimationController
{
public:
    // path and heights must be the same length, aligned tile-for-tile —
    // heights[i] is the GameTile height of path[i]. Both run start-to-dest
    // inclusive.
    void begin(Unit *unit, std::vector<Vec2i> path, std::vector<int> heights,
               std::function<void()> onComplete);

    void update(float dt);

    [[nodiscard]] bool isAnimating() const { return m_unit != nullptr; }
    [[nodiscard]] Unit *animatingUnit() const { return m_unit; }

    // Fractional tile-space position, e.g. {2.4f, 1.0f}. Project with the
    // Vec2f overload of tileToIso — the transform is linear, so this is a
    // true smooth glide, not a stepped approximation.
    [[nodiscard]] Vec2f getVisualTilePos() const;

    // Effective elevation (tile-height units — can exceed both endpoints'
    // heights during a hop's arc peak). Multiply by elevStep for pixels,
    // same conversion already used for an integer GameTile::height.
    // TODO: once real walk-cycle sprites exist, this is also where you'd
    // pick facing/frame from the current segment's direction, and a
    // distinct airborne pose during a hop's non-paused portion.
    [[nodiscard]] float getVisualHeight() const;

    void setWalkSecondsPerTile(float s) { m_walkSecondsPerTile = s; }
    void setHopSecondsPerTile(float s) { m_hopSecondsPerTile = s; }
    void setHopPauseFraction(float f) { m_hopPauseFraction = f; }
    void setHopArcHeight(float h) { m_hopArcHeight = h; }

private:
    struct SegmentTiming
    {
        float t = 0.0f;     // 0..1 across the WHOLE segment (pause+jump for hops)
        float moveT = 0.0f; // 0..1 across just the movement portion (post-pause for hops)
        bool inPause = false;
    };

    [[nodiscard]] bool currentSegmentIsHop() const;
    [[nodiscard]] float currentSegmentDuration() const;
    [[nodiscard]] SegmentTiming computeSegmentTiming(bool isHop) const;
    void finish();

    Unit *m_unit = nullptr;
    std::vector<Vec2i> m_path;
    std::vector<int> m_heights;
    std::size_t m_index = 0;
    float m_segmentElapsed = 0.0f;
    std::function<void()> m_onComplete;

    float m_walkSecondsPerTile = 0.15f;
    float m_hopSecondsPerTile = 0.35f;
    float m_hopPauseFraction = 0.25f; // fraction of a hop segment paused at the tile center before launching
    float m_hopArcHeight = 1.0f;      // extra elevation (tile-height units) at the peak of a hop
};