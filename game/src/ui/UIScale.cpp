#include "ui/UIScale.h"

#include "engine/core/App.h"
#include "engine/core/Window.h"

#include <algorithm>

namespace
{
    constexpr float kRefW = 1280.0f;
    constexpr float kRefH = 720.0f;
    float g_uiScale = 1.0f;
}

void UIScale::refresh()
{
    Window *window = App::getWindow();
    if (!window)
    {
        g_uiScale = 1.0f;
        return;
    }

    const float w = static_cast<float>(window->getWidth());
    const float h = static_cast<float>(window->getHeight());
    if (w <= 0.0f || h <= 0.0f)
    {
        g_uiScale = 1.0f;
        return;
    }

    // Keep scaling conservative to avoid clipping in fixed logical layouts.
    const float relative = std::min(w / kRefW, h / kRefH);
    g_uiScale = std::clamp(relative, 0.75f, 1.0f);
}

float UIScale::factor()
{
    return g_uiScale;
}

float UIScale::scale(float value)
{
    return value * g_uiScale;
}
