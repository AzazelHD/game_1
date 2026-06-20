// game_1/src/ui/Cursor.cpp
#include "ui/Cursor.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// Public
// ─────────────────────────────────────────────────────────────────────────────

void Cursor::update(int cols, int rows, float dt)
{
    const Vec2i delta = readMovementInput();
    const bool moving = (delta.x != 0 || delta.y != 0);

    if (!moving)
    {
        // No input — reset repeat state.
        m_repeatTimer = 0.0f;
        m_wasMoving = false;
        return;
    }

    bool doMove = false;

    if (!m_wasMoving)
    {
        // First frame of movement — fire immediately.
        doMove = true;
        m_repeatTimer = 0.0f;
        m_wasMoving = true;
    }
    else
    {
        // Key held — advance repeat timer.
        m_repeatTimer += dt;

        const float threshold =
            (m_repeatTimer < k_repeatDelay)
                ? k_repeatDelay
                : k_repeatDelay + k_repeatInterval *
                                      static_cast<int>((m_repeatTimer - k_repeatDelay) / k_repeatInterval + 1);

        if (m_repeatTimer >= threshold - k_repeatInterval)
            doMove = true;
    }

    if (doMove)
    {
        // Apply delta and clamp to grid bounds.
        m_pos.x = std::max(0, std::min(cols - 1, m_pos.x + delta.x));
        m_pos.y = std::max(0, std::min(rows - 1, m_pos.y + delta.y));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Private
// ─────────────────────────────────────────────────────────────────────────────

Vec2i Cursor::readMovementInput() const
{
    const Input &input = Input::instance();

    Vec2i delta = {0, 0};

    // Arrow keys and WASD both move the cursor.
    // If both axes are pressed simultaneously, prefer the last pressed.
    // For simplicity, horizontal takes priority over vertical when both active.

    if (input.isKeyDown(KeyCode::Left) || input.isKeyDown(KeyCode::A))
        delta.y = 1;
    else if (input.isKeyDown(KeyCode::Right) || input.isKeyDown(KeyCode::D))
        delta.y = -1;

    if (input.isKeyDown(KeyCode::Up) || input.isKeyDown(KeyCode::W))
        delta.x = -1;
    else if (input.isKeyDown(KeyCode::Down) || input.isKeyDown(KeyCode::S))
        delta.x = 1;

    return delta;
}