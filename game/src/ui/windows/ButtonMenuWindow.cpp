#include "config/GameConstants.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Aligment.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Renderer.h"
#include "ui/UIScale.h"
#include "ui/UITheme.h"
#include "ui/WindowId.h"
#include "ui/windows/ButtonMenuWindow.h"

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

ButtonMenuWindow::ButtonMenuWindow(WindowId id)
    : UIWindow(id, true, false)
{
}

void ButtonMenuWindow::setItems(std::vector<Item> items)
{
    m_items = std::move(items);
    m_scroll = 0;

    m_focusRows.clear();
    m_focusRows.reserve(m_items.size());
    std::vector<IFocusable *> focusableItems;
    focusableItems.reserve(m_items.size());

    for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
    {
        const ActionId actionId = m_items[i].id;
        const int index = i;

        auto row = std::make_unique<Button>(Rectf{0.0f, 0.0f, 0.0f, 0.0f}, "");
        row->setEnabled(m_items[i].enabled);
        row->setOnClick([this, actionId, index]()
                        {
                            emit(UIEvent{.type = UIEventType::ActionSelected,
                                         .windowId = id(),
                                         .actionId = actionId,
                                         .index = index});
                        });

        focusableItems.push_back(row.get());
        m_focusRows.push_back(std::move(row));
    }

    m_focus.resetFromPointers(std::move(focusableItems));
}

void ButtonMenuWindow::handleInput(const Input &input)
{
    if (m_items.empty())
        return;

    if (input.isKeyPressed(KeyCode::Up, false) || input.isKeyPressed(KeyCode::Left, false))
    {
        m_focus.focusPrevious();
        keepSelectionVisible();
        return;
    }

    if (input.isKeyPressed(KeyCode::Down, false) || input.isKeyPressed(KeyCode::Right, false))
    {
        m_focus.focusNext();
        keepSelectionVisible();
        return;
    }

    if (input.isKeyPressed(KeyCode::Back, false))
    {
        emit(UIEvent{.type = UIEventType::ActionCanceled, .windowId = id()});
        return;
    }

    if (input.isKeyPressed(KeyCode::Accept, false))
    {
        (void)m_focus.activateSelected();
    }
}

void ButtonMenuWindow::keepSelectionVisible()
{
    const int count = static_cast<int>(m_items.size());
    if (count <= 0)
        return;

    const float contentH = count > 0
                               ? (static_cast<float>(count) * (kItemH + kItemSpacing) - kItemSpacing + kBottomPadExtra)
                               : kItemH;
    const float menuH = std::clamp(2.0f * kPad + contentH, 64.0f, kMenuH);
    const int visibleCount = std::max(1, static_cast<int>((menuH - 2.0f * kPad + kItemSpacing) / (kItemH + kItemSpacing)));
    const int selected = m_focus.getSelectedIndex();
    if (selected < 0)
        return;

    if (selected < m_scroll)
        m_scroll = selected;
    if (selected >= m_scroll + visibleCount)
        m_scroll = selected - visibleCount + 1;
}

void ButtonMenuWindow::update(float /*dt*/)
{
}

void ButtonMenuWindow::render(Renderer *renderer) const
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

    // Width from the widest label + selection markers + padding. No large
    // fixed floor — short labels (Move/Attack) should get a small box;
    // long ones (future skill names) grow it naturally. Small floor only
    // to avoid a degenerate sliver for 1-2 character labels.
    constexpr float kMinMenuW = 80.0f;
    const float markerW = renderer->measureText(m_font, ">").x;
    float maxTextW = 0.0f;
    for (const Item &item : m_items)
        maxTextW = std::max(maxTextW, renderer->measureText(m_font, clipLabel(item.label)).x);
    const float menuW = std::max(kMinMenuW * ui,
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
        const bool selected = (i == m_focus.getSelectedIndex() && item.enabled);

        const Color color = !item.enabled ? Color{130, 130, 130, 255}
                            : selected    ? UITheme::SelectedText
                                          : UITheme::Text;

        const std::string clipped = clipLabel(item.label);
        const Vec2f textSize = renderer->measureText(m_font, clipped);

        // Reserve marker-gutter space on BOTH sides regardless of alignment
        // — Left/Right align still show "> label <" markers when selected,
        // so both sides need room even though the text itself hugs one side.
        const float markerSpace = markerW + markerGap;
        const Rectf textRect =
            (m_textAlign == TextAlign::Center)
                ? Rectf{panelX, y, menuW, itemH}
                : Rectf{panelX + pad + markerSpace, y, menuW - 2.0f * pad - 2.0f * markerSpace, itemH};

        HorizontalAlign align;

        switch (m_textAlign)
        {
        case TextAlign::Left:
            align = HorizontalAlign::Left;
            break;
        case TextAlign::Right:
            align = HorizontalAlign::Right;
            break;
        default:
            align = HorizontalAlign::Center;
            break;
        }

        Vec2f aligned = renderer->alignInRect(
            textRect,
            textSize,
            align,
            VerticalAlign::Middle);

        if (m_textAlign == TextAlign::Center)
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
