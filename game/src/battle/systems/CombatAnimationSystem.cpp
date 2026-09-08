#include "battle/systems/CombatAnimationSystem.h"

#include "config/GameConstants.h"
#include "engine/data/TileMapData.h"
#include "engine/math/MathUtils.h"
#include "engine/renderer/Camera.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Renderer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

namespace
{
    // Placeholder colors until sprite VFX exist — keyed off effect-id substrings.
    Color effectColor(const std::string &id)
    {
        if (id.find("heal") != std::string::npos)
            return Color{120, 255, 140, 255}; // heal — soft green
        if (id.find("buff") != std::string::npos)
            return Color{120, 190, 255, 255}; // buff — cool blue
        if (id.find("debuff") != std::string::npos)
            return Color{200, 120, 255, 255}; // debuff — violet
        if (id.find("hit") != std::string::npos || id.find("impact") != std::string::npos)
            return Color{255, 90, 60, 255}; // hit impact — hot red
        if (id.find("dodg") != std::string::npos || id.find("miss") != std::string::npos)
            return Color{230, 230, 230, 255}; // dodge — white whoosh
        return Color{255, 200, 90, 255};      // casts/story beats — amber
    }

    // Ring between inner and outer radius, drawn as two sampled circle
    // outlines. The previous integer-scanline disc was fine everywhere except
    // the top/bottom poles, where adjacent rows collapse into short horizontal
    // bars that read as "red line" artifacts at ~π and ~3π/2. Outlines (with
    // integer-snapped endpoints) stay clean regardless of radius.
    void drawRing(Renderer *renderer, Vec2f center, float inner, float outer, const Color &color)
    {
        renderer->setBlendMode(Renderer::BlendMode::Blend);

        const float cx = std::round(center.x);
        const float cy = std::round(center.y);

        const float maxR = std::max(inner, outer);
        const int steps = std::max(16, static_cast<int>(std::ceil(6.28318530f * maxR / 1.5f)));
        const float dTheta = 6.28318530f / static_cast<float>(steps);

        auto drawCircle = [&](float radius, const Color &col)
        {
            renderer->setDrawColor(col);
            Vec2f prev{std::round(cx + radius), std::round(cy)};
            for (int i = 1; i <= steps; ++i)
            {
                const float a = static_cast<float>(i) * dTheta;
                const Vec2f next{std::round(cx + radius * std::cos(a)),
                                 std::round(cy + radius * std::sin(a))};
                renderer->drawLine(prev, next);
                prev = next;
            }
        };

        drawCircle(outer, color);
        drawCircle(inner, color);
    }
}

void CombatAnimationSystem::enqueue(const std::string &id, float ttl)
{
    Event ev;
    ev.id = id;
    ev.anchored = false;
    ev.ttl = std::max(0.0f, ttl);

    auto effect = std::make_unique<TimedState>(ev.ttl);
    ev.effectState = effect.get();
    ev.animator.addState("effect", std::move(effect));
    ev.animator.addState("done", std::make_unique<AnimationState>());
    ev.animator.addTransition("effect", "done", ""); // exit-time: when ttl elapses
    ev.animator.play("effect");

    m_events.push_back(std::move(ev));
}

void CombatAnimationSystem::enqueue(const std::string &id, Vec2i tilePos, float ttl)
{
    enqueue(id, ttl);
    m_events.back().anchored = true;
    m_events.back().tilePos = tilePos;
}

void CombatAnimationSystem::update(float dt)
{
    for (Event &ev : m_events)
    {
        if (ev.animator.isActive())
            ev.animator.update(dt);
    }

    // The Animator flips an expired effect to "done"; that's the removal signal.
    m_events.erase(std::remove_if(m_events.begin(), m_events.end(),
                                  [](const Event &ev)
                                  { return ev.animator.currentStateName() == "done"; }),
                   m_events.end());
}

void CombatAnimationSystem::render(Renderer *renderer, const battleworld::AnchorContext &ctx) const
{
    if (!renderer || m_events.empty())
        return;

    const float s = static_cast<float>(ctx.scale) * ctx.camera.getZoom();
    const float halfTW = static_cast<float>(ctx.mapData.tileWidth) * s * 0.5f;

    for (const Event &ev : m_events)
    {
        if (!ev.animator.isActive() || ev.animator.currentStateName() != "effect" || !ev.effectState)
            continue;

        const float p = ev.effectState->progress();

        const Vec2f center = ev.anchored
                                 ? battleworld::tileFeetAnchor(ctx, ev.tilePos)
                                 : Vec2f{GameConstants::VIEW_CX, GameConstants::VIEW_CY};

        // Expand outward while fading: pop at the moment of impact, then dissolve.
        const float e = easeOutCubic(p);
        const float inner = halfTW * 0.35f;
        const float outer = halfTW * (0.35f + 1.05f * e);

        Color color = effectColor(ev.id);
        color.a = static_cast<std::uint8_t>(255.0f * (1.0f - p));
        drawRing(renderer, center, inner, outer, color);
    }
}

void CombatAnimationSystem::clear()
{
    m_events.clear();
}