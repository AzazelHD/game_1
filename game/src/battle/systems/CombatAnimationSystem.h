#pragma once

#include "engine/animation/Animator.h"
#include "engine/animation/TimedState.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Color.h"
#include "renderer/BattleWorldProjection.h"

#include <string>
#include <unordered_map>
#include <vector>

class Renderer;
class Texture;

// Plays short-lived combat VFX (skill casts, hit-impact/dodge flashes). Each
// queued effect owns a one-shot engine Animator — a timed "effect" state plus
// an exit-time transition to "done" — so expiry is driven by the engine
// animation layer instead of the hand-rolled TTL sweep this system used to
// mirror from FloatingTextSystem.
//
// Effects render as an expanding, fading ring by default. Registering a Visual
// with a sprite for an effect id replaces that placeholder with a real texture
// (see SpriteMotion for the motion curve); textures are owned by the caller,
// never by this system.
class CombatAnimationSystem
{
public:
    // How a sprite-backed effect animates over its [0,1] progress.
    enum class SpriteMotion
    {
        // Legacy placeholder: an expanding ring that fades out.
        Ring,
        // Fades in at the target's centre, holds, then drifts upward while
        // fading away — the "something just happened to this unit" beat.
        FadeInHoldRise,
    };

    struct Visual
    {
        const Texture *sprite = nullptr;
        SpriteMotion motion = SpriteMotion::Ring;
        Color tint{255, 255, 255, 255};
        // Sprite width as a fraction of half a tile's width.
        float scale = 1.0f;
    };

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

    // Attaches a sprite to an effect id. The Texture must outlive the battle
    // scene (this system only borrows the pointer). Unknown ids keep the
    // default expanding ring.
    void setVisual(const std::string &id, Visual visual);

    [[nodiscard]] bool isActive() const { return !m_events.empty(); }

private:
    std::vector<Event> m_events;
    std::unordered_map<std::string, Visual> m_visuals;
};