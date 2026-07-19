#pragma once

#include "engine/math/Rect.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Renderer.h"

#include <string>

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

} // namespace UIUtils