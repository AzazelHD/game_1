#include "ui/windows/ActionMenuWindow.h"

#include "config/GameConstants.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Renderer.h"
#include "ui/UIScale.h"
#include "ui/UITheme.h"

#include <algorithm>

namespace
{
    constexpr float kMenuW = 260.0f;
    constexpr float kMenuH = 224.0f;
    constexpr float kItemH = 24.0f;
    constexpr float kItemSpacing = 6.0f;
    constexpr float kPad = 12.0f;
    constexpr float kBottomMargin = 12.0f;
    constexpr float kBottomPadExtra = 10.0f;
    constexpr int kMaxLabelChars = 24;
    constexpr float kMarkerGap = 8.0f;
    constexpr float kOpticalCenterBiasX = 1.0f;

    std::string clipLabel(const std::string &label)
    {
        if (static_cast<int>(label.size()) <= kMaxLabelChars)
            return label;
        return label.substr(0, kMaxLabelChars - 3) + "...";
    }
}

ActionMenuWindow::ActionMenuWindow(std::string id)
    : UIWindow(std::move(id), true, false)
{
}

void ActionMenuWindow::setItems(std::vector<Item> items)
{
    m_items = std::move(items);
    m_selected = 0;
    m_scroll = 0;

    for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
    {
        if (m_items[i].enabled)
        {
            m_selected = i;
            break;
        }
    }
}

void ActionMenuWindow::handleInput(const Input &input)
{
    if (m_items.empty())
        return;

    if (input.isKeyPressed(KeyCode::Up, false) || input.isKeyPressed(KeyCode::Left, false))
    {
        moveSelection(-1);
        return;
    }

    if (input.isKeyPressed(KeyCode::Down, false) || input.isKeyPressed(KeyCode::Right, false))
    {
        moveSelection(1);
        return;
    }

    if (input.isKeyPressed(KeyCode::Back, false))
    {
        emit(UIEvent{.type = UIEventType::ActionCanceled, .windowId = id()});
        return;
    }

    if (input.isKeyPressed(KeyCode::Accept, false))
    {
        if (m_selected < 0 || m_selected >= static_cast<int>(m_items.size()))
            return;

        const Item &item = m_items[m_selected];
        if (!item.enabled)
            return;

        emit(UIEvent{.type = UIEventType::ActionSelected,
                     .windowId = id(),
                     .actionId = item.id,
                     .index = m_selected});
    }
}

void ActionMenuWindow::update(float /*dt*/)
{
}

void ActionMenuWindow::render(Renderer *renderer) const
{
    if (!renderer || !m_font || m_items.empty())
        return;

    UIScale::refresh();
    const float ui = UIScale::factor();

    const float itemSpacing = kItemSpacing * ui;
    const float pad = kPad * ui;
    const float markerGap = kMarkerGap * ui;
    const float opticalBias = kOpticalCenterBiasX * ui;

    // Item height from the real font metrics (with a small floor) so the box
    // grows vertically when the font gets bigger.
    const float fontH = renderer->measureText(m_font, "Ag").y;
    const float itemH = std::max(kItemH * ui, fontH + 6.0f * ui);

    // Width from the widest label + selection markers + padding, never below
    // the default width.
    const float markerW = renderer->measureText(m_font, ">").x;
    float maxTextW = 0.0f;
    for (const Item &item : m_items)
        maxTextW = std::max(maxTextW, renderer->measureText(m_font, clipLabel(item.label)).x);
    const float menuW = std::max(kMenuW * ui,
                                 maxTextW + 2.0f * (markerW + markerGap) + 2.0f * pad);

    const int count = static_cast<int>(m_items.size());
    const float contentH = count > 0
                               ? (static_cast<float>(count) * (itemH + itemSpacing) - itemSpacing + kBottomPadExtra * ui)
                               : itemH;
    // Grow to fit content, but never taller than the screen.
    const float menuH = std::min(2.0f * pad + contentH, GameConstants::VIEW_H - 40.0f * ui);
    const int visibleCount = std::max(1, static_cast<int>((menuH - 2.0f * pad + itemSpacing) / (itemH + itemSpacing)));
    const int start = std::clamp(m_scroll, 0, std::max(0, count - 1));
    const int end = std::min(count, start + visibleCount);

    const float panelX = m_centerX             ? (GameConstants::VIEW_W - menuW) * 0.5f
                         : m_useCustomPanelPos ? m_panelPos.x
                         : m_anchorBottomRight ? (GameConstants::VIEW_W - menuW - m_bottomRightMargin.x * ui)
                                               : (GameConstants::VIEW_W - menuW) * 0.5f;
    const float panelY = (m_useCustomPanelPos || m_centerX) ? m_panelPos.y
                         : m_anchorBottomRight              ? (GameConstants::VIEW_H - menuH - m_bottomRightMargin.y * ui)
                                                            : (GameConstants::VIEW_H - menuH) * 0.5f;

    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(UITheme::Panel);
    renderer->fillRect(Rectf{panelX, panelY, menuW, menuH});
    renderer->setDrawColor(UITheme::Border);
    renderer->drawRect(Rectf{panelX, panelY, menuW, menuH});

    float y = panelY + pad;
    for (int i = start; i < end; ++i)
    {
        const Item &item = m_items[i];
        const bool selected = (i == m_selected && item.enabled);

        const Color color = !item.enabled ? Color{130, 130, 130, 255}
                            : selected    ? UITheme::SelectedText
                                          : UITheme::Text;

        const std::string clipped = clipLabel(item.label);
        const Vec2f textSize = renderer->measureText(m_font, clipped);
        Vec2f aligned = renderer->alignInRect(Rectf{panelX, y, menuW, itemH},
                                              textSize,
                                              Renderer::HorizontalAlign::Center,
                                              Renderer::VerticalAlign::Middle);
        aligned.x += opticalBias;

        const float baseX = aligned.x;
        const float baseW = textSize.x;
        const float textY = aligned.y;

        renderer->renderText(m_font, clipped.c_str(), aligned, color, false, false, false);

        if (selected)
        {
            const std::string leftMarker = ">";
            const std::string rightMarker = "<";
            const float leftW = renderer->measureText(m_font, leftMarker).x;

            renderer->renderText(m_font,
                                 leftMarker,
                                 Vec2f{baseX - markerGap - leftW, textY},
                                 color,
                                 false,
                                 false,
                                 false);
            renderer->renderText(m_font,
                                 rightMarker,
                                 Vec2f{baseX + baseW + markerGap, textY},
                                 color,
                                 false,
                                 false,
                                 false);
        }
        y += itemH + itemSpacing;
    }
}

void ActionMenuWindow::moveSelection(int delta)
{
    if (m_items.empty())
        return;

    const int count = static_cast<int>(m_items.size());
    int next = m_selected;

    for (int i = 0; i < count; ++i)
    {
        next = (next + delta + count) % count;
        if (m_items[next].enabled)
        {
            m_selected = next;

            const float contentH = count > 0
                                       ? (static_cast<float>(count) * (kItemH + kItemSpacing) - kItemSpacing + kBottomPadExtra)
                                       : kItemH;
            const float menuH = std::clamp(2.0f * kPad + contentH, 64.0f, kMenuH);
            const int visibleCount = std::max(1, static_cast<int>((menuH - 2.0f * kPad + kItemSpacing) / (kItemH + kItemSpacing)));
            if (m_selected < m_scroll)
                m_scroll = m_selected;
            if (m_selected >= m_scroll + visibleCount)
                m_scroll = m_selected - visibleCount + 1;
            return;
        }
    }
}
