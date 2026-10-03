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
    constexpr float kItemH = 24.0f;
    constexpr float kItemSpacing = 6.0f;
    constexpr float kPad = 12.0f;
    constexpr float kBottomMargin = 12.0f;
    constexpr float kBottomPadExtra = 10.0f;
    constexpr int kMaxLabelChars = 24;
    constexpr float kMarkerGap = 8.0f;
    constexpr float kOpticalCenterBiasX = 1.0f;
    constexpr Color kDeleteXColor{230, 74, 74, 255};
    constexpr Color kHintColor{160, 162, 172, 255};
    constexpr float kHintGap = 6.0f;
    constexpr float kHintH = 20.0f;

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

void ButtonMenuWindow::setInitialSelectedIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(m_items.size()) || !m_items[index].enabled)
        return;

    int enabledBefore = 0;
    for (int i = 0; i < index; ++i)
        if (m_items[i].enabled)
            ++enabledBefore;

    for (int i = 0; i < enabledBefore; ++i)
        m_focus.focusNext();
    keepSelectionVisible();
}

void ButtonMenuWindow::handleInput(const Input &input)
{
    if (m_items.empty())
        return;

    const bool upDown = input.isKeyDown(KeyCode::Up) || input.isKeyDown(KeyCode::Left) ||
                        input.isKeyDown(KeyCode::W) || input.isKeyDown(KeyCode::A);
    const bool downDown = input.isKeyDown(KeyCode::Down) || input.isKeyDown(KeyCode::Right) ||
                          input.isKeyDown(KeyCode::S) || input.isKeyDown(KeyCode::D);

    const bool upFresh = input.isKeyPressed(KeyCode::Up, false) || input.isKeyPressed(KeyCode::Left, false) ||
                         input.isKeyPressed(KeyCode::W, false) || input.isKeyPressed(KeyCode::A, false);
    const bool downFresh = input.isKeyPressed(KeyCode::Down, false) || input.isKeyPressed(KeyCode::Right, false) ||
                           input.isKeyPressed(KeyCode::S, false) || input.isKeyPressed(KeyCode::D, false);

    const bool upRepeat = m_upRepeat.tick(m_lastDt, upDown);
    const bool downRepeat = m_downRepeat.tick(m_lastDt, downDown);

    if (upFresh)
    {
        const int before = m_focus.getSelectedIndex();
        m_focus.focusPrevious(); // FocusGroup wraps at the top
        keepSelectionVisible();
        notifyRowChanged(before);
        return;
    }

    if (downFresh)
    {
        const int before = m_focus.getSelectedIndex();
        m_focus.focusNext(); // FocusGroup wraps at the bottom
        keepSelectionVisible();
        notifyRowChanged(before);
        return;
    }

    const int selected = m_focus.getSelectedIndex();

    if (upRepeat && canClampStep(selected, -1))
    {
        m_focus.focusPrevious();
        keepSelectionVisible();
        notifyRowChanged(selected);
        return;
    }

    if (downRepeat && canClampStep(selected, +1))
    {
        m_focus.focusNext();
        keepSelectionVisible();
        notifyRowChanged(selected);
        return;
    }

    if (input.isKeyPressed(KeyCode::Back, false))
    {
        emit(UIEvent{.type = UIEventType::ActionCanceled, .windowId = id()});
        return;
    }

    if (m_deleteAction != ActionId::None && input.isKeyPressed(KeyCode::X, false))
    {
        emit(UIEvent{.type = UIEventType::ActionSelected,
                     .windowId = id(),
                     .actionId = m_deleteAction,
                     .index = m_focus.getSelectedIndex()});
        return;
    }

    // Details (Tab) on a row that has some: hand the owner the row index and
    // let it decide how to present the text.
    if (input.isKeyPressed(KeyCode::Details, false) && hasDetails())
    {
        emit(UIEvent{.type = UIEventType::ActionSelected,
                     .windowId = id(),
                     .actionId = ActionId::Details,
                     .index = m_focus.getSelectedIndex()});
        return;
    }

    if (input.isKeyPressed(KeyCode::Accept, false))
    {
        (void)m_focus.activateSelected();
    }
}

bool ButtonMenuWindow::canClampStep(int selected, int direction) const
{
    if (selected < 0 || selected >= static_cast<int>(m_items.size()))
        return false;

    // While a key is HELD (repeat), we must stop at the top/bottom instead of
    // wrapping. canClampStep is true when a focusable item still exists beyond
    // the current selection in that direction.
    for (int i = selected + direction; i >= 0 && i < static_cast<int>(m_items.size()); i += direction)
    {
        if (m_items[i].enabled)
            return true;
    }
    return false;
}

void ButtonMenuWindow::notifyRowChanged(int previousIndex)
{
    if (previousIndex == m_focus.getSelectedIndex())
        return;
    emit(UIEvent{.type = UIEventType::ActionChanged,
                 .windowId = id(),
                 .actionId = ActionId::Details,
                 .index = m_focus.getSelectedIndex()});
}

bool ButtonMenuWindow::hasDetails() const
{
    return std::any_of(m_items.begin(), m_items.end(),
                       [](const Item &item) { return !item.detail.empty(); });
}

std::string ButtonMenuWindow::detailAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_items.size()))
        return {};
    return m_items[static_cast<std::size_t>(index)].detail;
}

ButtonMenuWindow::MenuGeometry ButtonMenuWindow::computeGeometry(float ui, int count, float itemH) const
{
    const float itemSpacing = kItemSpacing * ui;
    const float pad = kPad * ui;
    const float contentH = count > 0
                               ? (static_cast<float>(count) * (itemH + itemSpacing) - itemSpacing + kBottomPadExtra * ui)
                               : itemH;
    const float naturalH = 2.0f * pad + contentH;

    // Non-scrollable menus grow until the content just fits (small breathing
    // margin); scrollable ones stay bounded and roll the extra rows instead.
    const float growCap = GameConstants::VIEW_H - 8.0f * ui;
    const float scrollCap = GameConstants::VIEW_H - 40.0f * ui;
    const bool isScrollMenu = m_scrollable || naturalH > growCap;
    const float menuH = isScrollMenu ? std::min(naturalH, scrollCap) : naturalH;
    const int visibleCount = isScrollMenu
                                 ? std::max(1, static_cast<int>((menuH - 2.0f * pad + itemSpacing) / (itemH + itemSpacing)))
                                 : count;
    return {menuH, visibleCount, isScrollMenu};
}

void ButtonMenuWindow::keepSelectionVisible()
{
    const int count = static_cast<int>(m_items.size());
    if (count <= 0)
        return;

    UIScale::refresh();
    const float ui = UIScale::factor();
    const int visible = std::max(1, count);
    const float itemH = std::max(kItemH * ui, m_lastItemH);
    const MenuGeometry geo = computeGeometry(ui, visible, itemH);
    if (!geo.isScrollMenu)
    {
        m_scroll = 0;
        return;
    }

    const int selected = m_focus.getSelectedIndex();
    if (selected < 0)
        return;

    if (selected < m_scroll)
        m_scroll = selected;
    if (selected >= m_scroll + std::max(1, geo.visibleCount))
        m_scroll = selected - std::max(1, geo.visibleCount) + 1;
}

void ButtonMenuWindow::update(float dt)
{
    // Cache dt so handleInput (which the engine dispatches *before* update
    // on the same frame) can drive the hold-to-repeat trackers.
    m_lastDt = dt;
    m_upRepeat.start(0.35f, 0.09f);
    m_downRepeat.start(0.35f, 0.09f);
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
    const float xGlyphW = hasDeleteMode() ? renderer->measureText(m_font, "X").x : 0.0f;
    const float deleteGutter = hasDeleteMode() ? xGlyphW + 2.0f * pad : 0.0f;
    float maxTextW = 0.0f;
    for (const Item &item : m_items)
        maxTextW = std::max(maxTextW, renderer->measureText(m_font, clipLabel(item.label)).x);
    const float menuW = std::max(kMinMenuW * ui,
                                 maxTextW + 2.0f * (markerW + markerGap) + 2.0f * pad + deleteGutter);

    const int count = static_cast<int>(m_items.size());
    m_lastItemH = itemH;
    const MenuGeometry geo = computeGeometry(ui, count, itemH);
    const float menuH = geo.menuH;
    const int start = geo.isScrollMenu ? std::clamp(m_scroll, 0, std::max(0, count - 1)) : 0;
    const int end = std::min(count, start + std::max(1, geo.visibleCount));

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
                ? Rectf{panelX, y, menuW - deleteGutter, itemH}
                : Rectf{panelX + pad + markerSpace, y, menuW - 2.0f * pad - 2.0f * markerSpace - deleteGutter, itemH};

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

        if (hasDeleteMode())
        {
            const Vec2f xSize = renderer->measureText(m_font, "X");
            const Vec2f xPos = renderer->alignInRect(
                Rectf{panelX + menuW - pad - xGlyphW, y, xGlyphW, itemH},
                xSize,
                HorizontalAlign::Center,
                VerticalAlign::Middle);
            renderer->renderText(m_font, "X", xPos, kDeleteXColor, false, false, false);
        }
        y += itemH + itemSpacing;
    }

    if (hasDeleteMode() && !m_deleteHint.empty())
    {
        const Vec2f hintSize = renderer->measureText(m_font, m_deleteHint);
        const Rectf hintRect{panelX, panelY + menuH + kHintGap * ui, menuW, kHintH * ui};
        const Vec2f hintPos = renderer->alignInRect(hintRect, hintSize, HorizontalAlign::Center, VerticalAlign::Top);
        renderer->renderText(m_font, m_deleteHint.c_str(), hintPos, kHintColor, false, false, false);
    }

    if (!m_hintLine.empty())
    {
        const Vec2f hintSize = renderer->measureText(m_font, m_hintLine);
        const Rectf hintRect{panelX, panelY + menuH + kHintGap * ui, menuW, kHintH * ui};
        const Vec2f hintPos = renderer->alignInRect(hintRect, hintSize, HorizontalAlign::Center, VerticalAlign::Top);
        renderer->renderText(m_font, m_hintLine.c_str(), hintPos, kHintColor, false, false, false);
    }
}
