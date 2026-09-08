#include "ui/FloatingTextSystem.h"

#include "engine/data/TileMapData.h"
#include "engine/renderer/Camera.h"
#include "engine/renderer/Renderer.h"

#include <algorithm>
#include <cstdint>

namespace
{
    // Total upward float in screen pixels at zoom 1; scales with tile scale.
    constexpr float kRisePx = 26.0f;
    // Fraction of the lifetime before the fade-in-to-out begins.
    constexpr float kFadeStart = 0.65f;
}

void FloatingTextSystem::enqueue(const std::string &text, Vec2i tilePos, Color color, float ttl)
{
    Entry e;
    e.text = text;
    e.tilePos = tilePos;
    e.color = color;
    e.ttl = std::max(0.0f, ttl);
    e.rise.start(0.0f, 1.0f, e.ttl, easeOut);
    m_entries.push_back(std::move(e));
}

void FloatingTextSystem::update(float dt)
{
    for (Entry &e : m_entries)
    {
        if (e.rise.isActive())
            e.rise.update(dt);
    }

    m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
                                   [](const Entry &e) { return e.rise.isFinished(); }),
                    m_entries.end());
}

void FloatingTextSystem::render(Renderer *renderer, const Font *font, const battleworld::AnchorContext &ctx) const
{
    if (!renderer || !font || m_entries.empty())
        return;

    const float s = static_cast<float>(ctx.scale) * ctx.camera.getZoom();
    const float halfTH = static_cast<float>(ctx.mapData.tileHeight) * s * 0.5f;
    const float lift = kRisePx * s;

    renderer->setBlendMode(Renderer::BlendMode::Blend);

    for (const Entry &e : m_entries)
    {
        if (e.rise.isFinished())
            continue;

        // Eased lift progress 0→1: floats up fast, drifts to a stop.
        const float p = e.rise.value();
        const Vec2f anchor = battleworld::tileFeetAnchor(ctx, e.tilePos);
        const float y = anchor.y - halfTH - lift * p;

        const Vec2f size = renderer->measureText(font, e.text, true);
        const Vec2f pos{anchor.x - size.x * 0.5f, y - size.y * 0.5f};

        // Fade is driven by LINEAR progress so alpha reaches exactly 0 the
        // frame the entry is erased — no fully-opaque tail, no end pop.
        const float t = e.rise.progress();
        const float fade = t < kFadeStart ? 1.0f : 1.0f - (t - kFadeStart) / (1.0f - kFadeStart);

        Color color = e.color;
        color.a = static_cast<std::uint8_t>(static_cast<float>(e.color.a) * fade);

        renderer->renderText(font, e.text, pos, color, true, false, false);
    }
}

void FloatingTextSystem::clear()
{
    m_entries.clear();
}