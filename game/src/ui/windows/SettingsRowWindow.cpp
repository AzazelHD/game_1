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
    constexpr float kRowSideGap = 16.0f;

    struct RowLayout
    {
        Rectf rowBox;
        Rectf labelRect;
        Rectf controlRect;
    };

    float measureMaxLabelWidth(Renderer *renderer, const Font *font,
                               const std::vector<SettingsRowWindow::RowItem> &rows)
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
                                  const std::vector<SettingsRowWindow::RowItem> &rows, float ui)
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

    RowLayout computeRowLayout(float sharedLabelWidth, float sharedControlsWidth,
                               float rowY, float rowH, float ui)
    {
        const HorizontalLayout::Container labelContainer{
            .items = {{sharedLabelWidth, rowH, Insets{}}},
            .padding = Insets{0.0f, 0.0f, 0.0f, kRowInnerPad * ui},
        };
        const HorizontalLayout::Container controlsContainer{
            .items = {{sharedControlsWidth, rowH, Insets{}}},
            .padding = Insets{0.0f, kRowInnerPad * ui, 0.0f, 0.0f},
        };

        const float labelWidth = HorizontalLayout::computeBounds(
                                     labelContainer.items, Vec2f{}, labelContainer.padding)
                                     .w;
        const float controlsWidth = HorizontalLayout::computeBounds(
                                        controlsContainer.items, Vec2f{}, controlsContainer.padding)
                                        .w;
        const float totalWidth = labelWidth + (kGapLabelControls * ui) + controlsWidth;
        const float centeredX = (kPanelX * ui) + ((kPanelW * ui) - totalWidth) * 0.5f;

        const auto results = HorizontalLayout::layoutContainers(
            {labelContainer, controlsContainer}, Vec2f{centeredX, rowY}, kGapLabelControls * ui);

        const Rectf &labelBox = results[0].box;
        const Rectf &controlsBox = results[1].box;
        const float contentWidth = (controlsBox.x + controlsBox.w) - labelBox.x;

        const HorizontalLayout::Container rowWrapper{
            .items = {{contentWidth, rowH, Insets{}}},
            .padding = Insets{0.0f, kRowSideGap * ui, 0.0f, kRowSideGap * ui},
        };
        const Rectf rowBox = HorizontalLayout::computeBounds(
            rowWrapper.items, Vec2f{labelBox.x, rowY}, rowWrapper.padding);

        RowLayout out;
        out.rowBox = rowBox;
        out.labelRect = results[0].itemRects[0];
        out.controlRect = results[1].itemRects[0];
        return out;
    }

    Rectf computeButtonRowBox(float sharedLabelWidth, float sharedControlsWidth,
                              float rowY, float rowH, float ui)
    {
        const float totalContentWidth = sharedLabelWidth + (kGapLabelControls * ui) + sharedControlsWidth;
        const float centeredX = (kPanelX * ui) + ((kPanelW * ui) - totalContentWidth) * 0.5f;
        const HorizontalLayout::Container wrapper{
            .items = {{totalContentWidth, rowH, Insets{}}},
            .padding = Insets{0.0f, kRowSideGap * ui, 0.0f, kRowSideGap * ui},
        };
        return HorizontalLayout::computeBounds(wrapper.items, Vec2f{centeredX, rowY}, wrapper.padding);
    }
} // anonymous namespace

SettingsRowWindow::SettingsRowWindow(WindowId id)
    : UIWindow(id, true, false)
{
}

void SettingsRowWindow::setRows(std::vector<RowItem> rows)
{
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

    const float sharedLabelWidth = measureMaxLabelWidth(renderer, m_font, m_rows);
    const float sharedControlsWidth = measureMaxControlsWidth(renderer, m_font, m_rows, ui);

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

        if (row.control->isFullWidth())
        {
            const Rectf rowBox = computeButtonRowBox(sharedLabelWidth, sharedControlsWidth,
                                                     currentY, kRowBoxH * ui, ui);
            UIUtils::drawRow(renderer, rowBox, focused);
            row.control->render(renderer, m_font, rowBox, ui, UITheme::Text, UITheme::SelectedText);
        }
        else
        {
            const RowLayout layout = computeRowLayout(sharedLabelWidth, sharedControlsWidth,
                                                      currentY, kRowBoxH * ui, ui);
            UIUtils::drawRow(renderer, layout.rowBox, focused);

            renderer->renderTextInRect(m_font, row.label, layout.labelRect,
                                       focused ? UITheme::SelectedText : UITheme::Text,
                                       Renderer::HorizontalAlign::Left,
                                       Renderer::VerticalAlign::Middle,
                                       false, false, false);

            row.control->render(renderer, m_font, layout.controlRect, ui,
                                UITheme::Text, UITheme::SelectedText);
        }

        currentY += kRowH * ui;
    }
}