// game_1/src/ui/Cursor.h
#pragma once
//
// Cursor — player-controlled tile selector on the battle map.
//
// Owns its grid position and movement logic. Rendering is handled by
// BattleState::drawScene() so the cursor is drawn in the correct
// per-cell painter's order alongside tiles and overlays.
//
// Movement:
//   Arrow keys or WASD move the cursor one tile per press.
//   Position is clamped to grid bounds (Fire Emblem style — no wrap).
//   A repeat delay is applied so holding a key moves at a steady rate
//   rather than at raw key-repeat speed.
//
// The cursor does NOT know about BattleMap or terrain — it only tracks
// a grid position. BattleState queries the position each frame.

#include "engine/math/Vec2.h"

class Cursor
{
public:
    // Update cursor position from input. Call once per frame.
    // cols/rows are the grid dimensions used for clamping.
    void update(int cols, int rows, float dt);

    [[nodiscard]] Vec2i getPosition() const { return m_pos; }
    void setPosition(Vec2i pos) { m_pos = pos; }

private:
    Vec2i m_pos = {0, 0};

    // Key-repeat state — first move fires immediately on press,
    // then after k_repeatDelay seconds fires every k_repeatInterval seconds.
    float m_repeatTimer = 0.0f;
    bool m_wasMoving = false;

    static constexpr float k_repeatDelay = 0.3f;    // seconds before repeat starts
    static constexpr float k_repeatInterval = 0.1f; // seconds between repeat moves

    // Returns the movement delta this frame based on current input state.
    [[nodiscard]] Vec2i readMovementInput() const;
};
