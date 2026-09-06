#pragma once

#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Renderer.h"

#include <cmath>
#include <string>
#include <vector>

namespace UIUtils
{

    inline std::string formatButtonLabel(const std::string &label, bool selected,
                                         const std::string &prefix = "> ",
                                         const std::string &suffix = " <")
    {
        return selected ? prefix + label + suffix : label;
    }

    // Draws a settings‑style row highlight.
    // Uses hard‑coded colours that match the existing look.
    inline void drawRow(Renderer *r, const Rectf &rect, bool focused)
    {
        r->setBlendMode(Renderer::BlendMode::Blend);
        if (focused)
        {
            r->setDrawColor(Color{166, 78, 48, 228});
            r->fillRect(rect);
            r->setDrawColor(Color{255, 196, 118, 255});
            r->drawRect(rect);
        }
        else
        {
            r->setDrawColor(Color{20, 28, 44, 190});
            r->fillRect(rect);
            r->setDrawColor(Color{58, 74, 108, 255});
            r->drawRect(rect);
        }
    }

    inline void drawCircle(Renderer *renderer, Vec2f center, float radius, Color color, int segments = 20)
    {
        if (!renderer)
            return;

        const FColor c = color;

        std::vector<Renderer::Vertex> verts;
        std::vector<int> indices;
        verts.reserve(static_cast<std::size_t>(segments + 2));
        indices.reserve(static_cast<std::size_t>(segments * 3));

        verts.push_back(Renderer::Vertex{center, c});

        constexpr float pi = 3.1415926535f;
        for (int i = 0; i <= segments; ++i)
        {
            const float angle = 2.0f * pi * static_cast<float>(i) / static_cast<float>(segments);
            verts.push_back(Renderer::Vertex{
                Vec2f{center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius},
                c});

            if (i > 0)
            {
                indices.push_back(0);
                indices.push_back(i);
                indices.push_back(i + 1);
            }
        }

        renderer->drawGeometry(verts, indices);
    }

} // namespace UIUtils
