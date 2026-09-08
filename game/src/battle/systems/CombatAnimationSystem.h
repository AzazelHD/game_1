#pragma once

#include "engine/animation/Animator.h"
#include "engine/animation/TimedState.h"
#include "engine/math/Vec2.h"
#include "renderer/BattleWorldProjection.h"

#include <string>
#include <vector>

class Renderer;

// Plays short-lived combat VFX (skill casts, hit-impact/dodge flashes). Each
// queued effect owns a one-shot engine Animator — a timed "effect" state plus
// an exit-time transition to "done" — so expiry is driven by the engine
// animation layer instead of the hand-rolled TTL sweep this system used to
// mirror from FloatingTextSystem.
//
// Rendering stays a placeholder: an expanding, fading ring at the effect's
// tile (or scene center for arena-wide events). Real sprites can replace the
// ring without touching the queue or expiry logic below.
class CombatAnimationSystem
{
public:
    struct Event
    {
        std::string id;
        Vec2i tilePos{};
        bool anchored = false;
        float ttl = 0.0f;
        Animator animator;
        const TimedState *effectState = nullptr;
    };

    // Arena-wide effect (skill casts, screen story beats) — drawn at scene center.
    void enqueue(const std::string &id, float ttl = 0.8f);

    // World-anchored effect (hit impact, dodge) on a specific map tile.
    // Kept short so the flash is fully gone while the floating number is still
    // settling — a lingering faint ring under a bright, stationary number read
    // as a "flicker" when the ring expired.
    void enqueue(const std::string &id, Vec2i tilePos, float ttl = 0.45f);

    void update(float dt);
    void render(Renderer *renderer, const battleworld::AnchorContext &ctx) const;
    void clear();

    [[nodiscard]] bool isActive() const { return !m_events.empty(); }

private:
    std::vector<Event> m_events;
};