#include "ui/windows/SettingsRowWindow.h"

#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Renderer.h"
#include "engine/ui/HorizontalLayout.h"
#include "engine/ui/IRowControl.h"
#include "engine/ui/Insets.h"
#include "config/GameConstants.h"
#include "ui/UIScale.h"
#include "ui/UITheme.h"
#include "ui/UIUtils.h"
#include "ui/WindowId.h"

#include <algorithm>

namespace
{
    constexpr float kPanelX = 330.0f;
    constexpr float kPanelY = 230.0f;
    constexpr float kPanelW = 620.0f;
    constexpr float kRowH = 54.0f;
    constexpr float kRowBoxH = kRowH - 6.0f;
    constexpr float kGapLabelControls = 24.0f;
    constexpr float kRowInnerPad = 16.0f;
    constexpr Insets kDefaultRowPadding{0.0f, 16.0f, 0.0f, 16.0f};

    using RowItem = SettingsRowWindow::RowItem;
    using RowList = std::vector<RowItem>;

    struct RowLayout
    {
        Rectf rowBox;
        Rectf labelRect;
        Rectf controlRect;
    };

    float measureMaxLabelWidth(Renderer *renderer, const Font *font, const RowList &rows)
    {
        float maxW = 0.0f;
        if (!renderer || !font)
            return maxW;
        for (const auto &row : rows)
        {
            if (row.control && row.control->isFullWidth())
                continue;
            maxW = std::max(maxW, renderer->measureText(font, row.label).x);
        }
        return maxW;
    }

    float measureMaxControlsWidth(Renderer *renderer, const Font *font,
                                  const RowList &rows, float ui)
    {
        float maxW = 0.0f;
        for (const auto &row : rows)
        {
            if (!row.control || row.control->isFullWidth())
                continue;
            maxW = std::max(maxW, row.control->measureWidth(renderer, font, ui));
        }
        return maxW;
    }

    // The shared label column every non-full-width row uses. Built once per
    // render() call from the widest label in the window — not per-row.
    HorizontalLayout::Container makeLabelColumn(float sharedLabelWidth, float rowH, float ui)
    {
        return HorizontalLayout::Container{
            .items = {{sharedLabelWidth, rowH, Insets{}}},
            .padding = Insets{0.0f, 0.0f, 0.0f, kRowInnerPad * ui},
        };
    }

    // The shared controls column every non-full-width row uses. Built once
    // per render() call from the widest control in the window.
    HorizontalLayout::Container makeControlsColumn(float sharedControlsWidth, float rowH, float ui)
    {
        return HorizontalLayout::Container{
            .items = {{sharedControlsWidth, rowH, Insets{}}},
            .padding = Insets{0.0f, kRowInnerPad * ui, 0.0f, 0.0f},
        };
    }

    // Total width of label + gap + controls, derived from the two columns'
    // own bounds — not re-measured from text a second time.
    float computeContentWidth(const HorizontalLayout::Container &labelColumn,
                              const HorizontalLayout::Container &controlsColumn,
                              float ui)
    {
        const float labelWidth =
            HorizontalLayout::computeBounds(labelColumn.items, Vec2f{}, labelColumn.padding).w;
        const float controlsWidth =
            HorizontalLayout::computeBounds(controlsColumn.items, Vec2f{}, controlsColumn.padding).w;
        return labelWidth + (kGapLabelControls * ui) + controlsWidth;
    }

    // Arranges one row using the already-built shared columns. Knows
    // nothing about slider/setting/button — only whether this particular
    // row is full-width (button-style) or a label+control row.
    RowLayout computeRowLayout(const HorizontalLayout::Container &labelColumn,
                               const HorizontalLayout::Container &controlsColumn,
                               float contentWidth,
                               const Insets &rowPadding,
                               bool fullWidth,
                               float rowY,
                               float rowH,
                               float ui)
    {
        RowLayout out;
        const float centeredX = (kPanelX * ui) + ((kPanelW * ui) - contentWidth) * 0.5f;
        const Insets scaledPadding{
            rowPadding.top * ui,
            rowPadding.right * ui,
            rowPadding.bottom * ui,
            rowPadding.left * ui,
        };

        if (fullWidth)
        {
            const HorizontalLayout::Container wrapper{
                .items = {{contentWidth, rowH, Insets{}}},
                .padding = scaledPadding,
            };
            out.rowBox = HorizontalLayout::computeBounds(wrapper.items, Vec2f{centeredX, rowY}, wrapper.padding);
            out.controlRect = out.rowBox;
            return out;
        }

        const auto results = HorizontalLayout::layoutContainers(
            {labelColumn, controlsColumn},
            Vec2f{centeredX, rowY},
            kGapLabelControls * ui);

        const Rectf &labelBox = results[0].box;

        const HorizontalLayout::Container rowWrapper{
            .items = {{contentWidth, rowH, Insets{}}},
            .padding = scaledPadding,
        };
        out.rowBox = HorizontalLayout::computeBounds(rowWrapper.items, Vec2f{labelBox.x, rowY}, rowWrapper.padding);
        out.labelRect = results[0].itemRects[0];
        out.controlRect = results[1].itemRects[0];
        return out;
    }
} // anonymous namespace

SettingsRowWindow::SettingsRowWindow(WindowId id)
    : UIWindow(id, true, false)
{
}

void SettingsRowWindow::setRows(RowList rows)
{
    for (auto &row : rows)
    {
        if (!row.padding.has_value())
            row.padding = kDefaultRowPadding;
    }
    m_rows = std::move(rows);

    std::vector<IFocusable *> focusables;
    focusables.reserve(m_rows.size());
    for (auto &row : m_rows)
        focusables.push_back(row.control.get());

    m_focus.resetFromPointers(std::move(focusables));
}

void SettingsRowWindow::handleInput(const Input &input)
{
    if (m_rows.empty())
        return;

    if (input.isKeyPressed(KeyCode::Up, true) || input.isKeyPressed(KeyCode::W, true))
    {
        m_focus.focusPrevious();
        return;
    }
    if (input.isKeyPressed(KeyCode::Down, true) || input.isKeyPressed(KeyCode::S, true))
    {
        m_focus.focusNext();
        return;
    }

    if (input.isKeyPressed(KeyCode::Left, true) || input.isKeyPressed(KeyCode::A, true))
    {
        if (m_focus.handleSelectedLeft())
            emit(UIEvent{.type = UIEventType::ActionSelected, .windowId = id(), .actionId = ActionId::AdjustLeft, .index = m_focus.getSelectedIndex()});
        return;
    }
    if (input.isKeyPressed(KeyCode::Right, true) || input.isKeyPressed(KeyCode::D, true))
    {
        if (m_focus.handleSelectedRight())
            emit(UIEvent{.type = UIEventType::ActionSelected, .windowId = id(), .actionId = ActionId::AdjustRight, .index = m_focus.getSelectedIndex()});
        return;
    }
    if (input.isKeyPressed(KeyCode::Accept, false))
    {
        const int selectedIndex = m_focus.getSelectedIndex();
        if (m_focus.activateSelected())
            emit(UIEvent{.type = UIEventType::ActionSelected, .windowId = id(), .actionId = ActionId::Confirm, .index = selectedIndex});
        return;
    }
    if (input.isKeyPressed(KeyCode::Back, false))
    {
        emit(UIEvent{.type = UIEventType::ActionCanceled, .windowId = id()});
        return;
    }
}

void SettingsRowWindow::update(float /*dt*/)
{
}

void SettingsRowWindow::render(Renderer *renderer) const
{
    if (!renderer || !m_font || m_rows.empty())
        return;

    UIScale::refresh();
    const float ui = UIScale::factor();
    const float rowH = kRowBoxH * ui;

    const float sharedLabelWidth = measureMaxLabelWidth(renderer, m_font, m_rows);
    const float sharedControlsWidth = measureMaxControlsWidth(renderer, m_font, m_rows, ui);

    // Columns built ONCE per render call, not per row and not twice —
    // every row (including full-width button rows, via contentWidth)
    // reuses these same measurements.
    const HorizontalLayout::Container labelColumn = makeLabelColumn(sharedLabelWidth, rowH, ui);
    const HorizontalLayout::Container controlsColumn = makeControlsColumn(sharedControlsWidth, rowH, ui);
    const float sharedContentWidth = computeContentWidth(labelColumn, controlsColumn, ui);

    const int selectedIndex = m_focus.getSelectedIndex();
    float currentY = kPanelY * ui;

    for (int i = 0; i < static_cast<int>(m_rows.size()); ++i)
    {
        const auto &row = m_rows[i];
        const bool focused = (i == selectedIndex);

        if (!row.control)
        {
            currentY += kRowH * ui;
            continue;
        }

        const RowLayout layout = computeRowLayout(
            labelColumn,
            controlsColumn,
            sharedContentWidth,
            *row.padding,
            row.control->isFullWidth(),
            currentY,
            rowH,
            ui);

        UIUtils::drawRow(renderer, layout.rowBox, focused);

        if (!row.control->isFullWidth())
        {
            renderer->renderTextInRect(
                m_font,
                row.label,
                layout.labelRect,
                focused ? UITheme::SelectedText : UITheme::Text,
                Renderer::HorizontalAlign::Left,
                Renderer::VerticalAlign::Middle,
                false, false, false);
        }

        row.control->render(
            renderer,
            m_font,
            layout.controlRect,
            ui,
            UITheme::Text,
            UITheme::SelectedText);

        currentY += kRowH * ui;
    }
}