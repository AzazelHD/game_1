#pragma once

namespace GameConstants
{
    // ── View ─────────────────────────────────────────────────────────────
    constexpr float VIEW_W = 1280.f;
    constexpr float VIEW_H = 720.f;
    constexpr float VIEW_CX = VIEW_W * 0.5f;
    constexpr float VIEW_CY = VIEW_H * 0.5f;

    // ── Transitions ───────────────────────────────────────────────────────
    constexpr float TRANSITION_DURATION = 0.2f;

    // ── Battle ────────────────────────────────────────────────────────────
    constexpr int BATTLE_UNIT_CAPACITY = 64;

    // ── TurnQueue ─────────────────────────────────────────────────────────
    constexpr float TURN_BASE_WAIT_COST = 250.f;
    constexpr float TURN_BASE_MOVE_COST = 500.f;
    constexpr float TURN_BASE_ACTION_COST = 750.f;
}
