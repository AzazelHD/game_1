#include "ui/windows/PartyWindow.h"

#include "config/GameConstants.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Renderer.h"
#include "ui/UITheme.h"
#include "ui/WindowId.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kPanelW = 460.0f;
    constexpr float kPanelH = 340.0f;
    constexpr float kRowH = 40.0f;
    constexpr int kVisibleRows = 6;
    void drawCircle(Renderer *renderer, Vec2f center, float radius, Color color)
    {
        if (!renderer)
            return;

        const FColor c = {
            static_cast<float>(color.r) / 255.0f,
            static_cast<float>(color.g) / 255.0f,
            static_cast<float>(color.b) / 255.0f,
            static_cast<float>(color.a) / 255.0f,
        };

        std::vector<Renderer::Vertex> verts;
        std::vector<int> idx;
        constexpr int segments = 16;
        verts.reserve(segments + 2);
        idx.reserve(segments * 3);

        verts.push_back(Renderer::Vertex{center, c});
        constexpr float pi = 3.1415926535f;
        for (int i = 0; i <= segments; ++i)
        {
            const float a = 2.0f * pi * static_cast<float>(i) / static_cast<float>(segments);
            verts.push_back(Renderer::Vertex{Vec2f{center.x + std::cos(a) * radius, center.y + std::sin(a) * radius}, c});
            if (i > 0)
            {
                idx.push_back(0);
                idx.push_back(i);
                idx.push_back(i + 1);
            }
        }

        renderer->drawGeometry(verts, idx);
    }
}

PartyWindow::PartyWindow(WindowId id)
    : UIWindow(id, true, false)
{
}

void PartyWindow::setEntries(std::vector<Entry> entries)
{
    m_entries = std::move(entries);
    m_selected = std::clamp(m_selected, 0, std::max(0, static_cast<int>(m_entries.size()) - 1));
    m_scroll = 0;
}

void PartyWindow::handleInput(const Input &input)
{
    if (input.isKeyPressed(KeyCode::Up, false) && !m_entries.empty())
    {
        m_selected = std::max(0, m_selected - 1);
    }
    else if (input.isKeyPressed(KeyCode::Down, false) && !m_entries.empty())
    {
        m_selected = std::min(static_cast<int>(m_entries.size()) - 1, m_selected + 1);
    }

    if (m_selected < m_scroll)
        m_scroll = m_selected;
    if (m_selected >= m_scroll + kVisibleRows)
        m_scroll = m_selected - kVisibleRows + 1;

    if (input.isKeyPressed(KeyCode::Back, false) || input.isKeyPressed(KeyCode::Accept, false))
    {
        emit(UIEvent{.type = UIEventType::ActionCanceled, .windowId = id(), .actionId = ActionId::Close});
    }
}

void PartyWindow::update(float /*dt*/)
{
}

void PartyWindow::render(Renderer *renderer) const
{
    if (!renderer || !m_font)
        return;

    const float x = (GameConstants::VIEW_W - kPanelW) * 0.5f;
    const float y = (GameConstants::VIEW_H - kPanelH) * 0.5f;

    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(UITheme::PopupBG);
    renderer->fillRect(Rectf{x, y, kPanelW, kPanelH});
    renderer->setDrawColor(UITheme::PopupBorder);
    renderer->drawRect(Rectf{x, y, kPanelW, kPanelH});

    renderer->renderTextInRect(m_font,
                               "Party",
                               Rectf{x, y + 14.0f, kPanelW, 24.0f},
                               UITheme::SelectedText,
                               Renderer::HorizontalAlign::Center,
                               Renderer::VerticalAlign::Middle,
                               false,
                               false,
                               false);

    const int start = std::clamp(m_scroll, 0, std::max(0, static_cast<int>(m_entries.size()) - 1));
    const int end = std::min(static_cast<int>(m_entries.size()), start + kVisibleRows);

    float rowY = y + 56.0f;
    for (int i = start; i < end; ++i)
    {
        const bool selected = i == m_selected;
        const Color textColor = selected ? UITheme::SelectedText : UITheme::Text;

        const Rectf rowRect{x + 12.0f, rowY - 4.0f, kPanelW - 24.0f, kRowH};
        if (selected)
        {
            renderer->setDrawColor(Color{80, 96, 122, 140});
            renderer->fillRect(rowRect);
        }

        const float iconX = x + 28.0f;
        const float iconY = rowY + 12.0f;
        if (m_entries[static_cast<std::size_t>(i)].hasSprite)
        {
            renderer->setDrawColor(Color{180, 210, 255, 255});
            renderer->fillRect(Rectf{iconX - 8.0f, iconY - 8.0f, 16.0f, 16.0f});
            renderer->setDrawColor(Color{120, 150, 200, 255});
            renderer->drawRect(Rectf{iconX - 8.0f, iconY - 8.0f, 16.0f, 16.0f});
        }
        else
        {
            drawCircle(renderer, Vec2f{iconX, iconY}, 8.0f, Color{200, 200, 210, 255});
        }

        std::string label = selected ? ("> " + m_entries[static_cast<std::size_t>(i)].name + " <") : m_entries[static_cast<std::size_t>(i)].name;
        renderer->renderTextInRect(m_font,
                                   label,
                                   Rectf{x, rowY, kPanelW, kRowH},
                                   textColor,
                                   Renderer::HorizontalAlign::Center,
                                   Renderer::VerticalAlign::Middle,
                                   false,
                                   false,
                                   false);
        rowY += kRowH;
    }

    renderer->renderTextInRect(m_font,
                               "Party",
                               Rectf{x, y + kPanelH - 30.0f, kPanelW, 24.0f},
                               UITheme::Info,
                               Renderer::HorizontalAlign::Center,
                               Renderer::VerticalAlign::Middle,
                               false,
                               false,
                               false);
}
