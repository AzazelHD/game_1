#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Renderer.h"
#include "engine/ui/HorizontalLayout.h"
#include "config/GameConstants.h"
#include "ui/UIScale.h"
#include "ui/UITheme.h"
#include "ui/UIUtils.h"
#include "ui/WindowId.h"
#include "ui/windows/SettingsRowWindow.h"

#include <algorithm>
#include <cstdio>

namespace
{

    constexpr float kPanelX = 330.0f;
    constexpr float kPanelY = 230.0f;
    constexpr float kPanelW = 620.0f;
    constexpr float kRowH = 54.0f;
    constexpr float kRowBoxH = kRowH - 6.0f;
    constexpr float kTrackW = 280.0f;
    constexpr float kTrackH = 22.0f;
    constexpr float kGapLabelControls = 24.0f;
    constexpr float kSliderMarginTop = 16.0f;
    constexpr float kPctMarginLeft = 28.0f;
    constexpr float kRowInnerPad = 16.0f;
    constexpr float kRowSideGap = 16.0f;

    struct RowLayout
    {
        Rectf rowBox;
        Rectf labelRect;
        Rectf controlRect;
        Rectf pctRect;
    };

    RowLayout computeRowLayout(Renderer *renderer, const Font *font,
                               float sharedLabelWidth,
                               bool isSlider,
                               const std::string &valueText,
                               float rowY, float rowH, float ui)
    {
        const float labelTextW = sharedLabelWidth;

        const HorizontalLayout::Container labelContainer{
            .items = {{labelTextW, rowH, Insets{}}},
            .padding = Insets{0.0f, 0.0f, 0.0f, kRowInnerPad * ui},
        };

        HorizontalLayout::Container controlsContainer;
        if (isSlider)
        {
            const float pctTextW = (renderer && font) ? renderer->measureText(font, "100%").x : 0.0f;
            const float sliderTopOffset = (rowH - kTrackH * ui) * 0.5f;
            controlsContainer = HorizontalLayout::Container{
                .items = {
                    {kTrackW * ui, kTrackH * ui, Insets{sliderTopOffset, 0.0f, 0.0f, 0.0f}},
                    {pctTextW, rowH, Insets{0.0f, 0.0f, 0.0f, kPctMarginLeft * ui}},
                },
                .padding = Insets{0.0f, kRowInnerPad * ui, 0.0f, 0.0f},
            };
        }
        else
        {
            const float valueTextW = (renderer && font) ? renderer->measureText(font, valueText).x : 0.0f;
            controlsContainer = HorizontalLayout::Container{
                .items = {{valueTextW, rowH, Insets{}}},
                .padding = Insets{0.0f, kRowInnerPad * ui, 0.0f, 0.0f},
            };
        }

        const float labelWidth = HorizontalLayout::computeBounds(
                                     labelContainer.items, Vec2f{}, labelContainer.padding)
                                     .w;
        const float controlsWidth = HorizontalLayout::computeBounds(
                                        controlsContainer.items, Vec2f{}, controlsContainer.padding)
                                        .w;
        const float totalWidth = labelWidth + (kGapLabelControls * ui) + controlsWidth;
        const float centeredX = (kPanelX * ui) + ((kPanelW * ui) - totalWidth) * 0.5f;

        const auto results = HorizontalLayout::layoutRow(
            {labelContainer, controlsContainer},
            Vec2f{centeredX, rowY},
            kGapLabelControls * ui);

        const Rectf &labelBox = results[0].box;
        const Rectf &controlsBox = results[1].box;
        const float contentWidth = (controlsBox.x + controlsBox.w) - labelBox.x;

        // The row's outer highlight box: the label+controls content, wrapped
        // in a real Insets padding (top, right, bottom, left) via the same
        // computeBounds() helper the containers already use — not hand-rolled
        // x/width arithmetic.
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
        if (isSlider)
            out.pctRect = results[1].itemRects[1];
        return out;
    }

    float measureMaxLabelWidth(Renderer *renderer, const Font *font,
                               const std::vector<SettingsRowWindow::RowItem> &rows)
    {
        float maxW = 0.0f;
        if (!renderer || !font)
            return maxW;
        for (const auto &row : rows)
            maxW = std::max(maxW, renderer->measureText(font, row.label).x);
        return maxW;
    }
}

SettingsRowWindow::SettingsRowWindow(WindowId id)
    : UIWindow(id, true, false)
{
}

void SettingsRowWindow::setRows(std::vector<RowItem> rows)
{
    m_rows = std::move(rows);
    m_focusedIndex = 0;
}

void SettingsRowWindow::handleInput(const Input &input)
{
    if (m_rows.empty())
        return;

    if (input.isKeyPressed(KeyCode::Up, true) || input.isKeyPressed(KeyCode::W, true))
    {
        moveFocus(-1);
        return;
    }
    if (input.isKeyPressed(KeyCode::Down, true) || input.isKeyPressed(KeyCode::S, true))
    {
        moveFocus(1);
        return;
    }

    const auto &focusedRow = m_rows[m_focusedIndex];

    if (focusedRow.type == RowType::Slider || focusedRow.type == RowType::Setting)
    {
        if (input.isKeyPressed(KeyCode::Left, true) || input.isKeyPressed(KeyCode::A, true))
        {
            if (focusedRow.type == RowType::Slider)
                adjustFocusedSlider(false);
            else
                adjustFocusedSetting(false);
            return;
        }
        if (input.isKeyPressed(KeyCode::Right, true) || input.isKeyPressed(KeyCode::D, true))
        {
            if (focusedRow.type == RowType::Slider)
                adjustFocusedSlider(true);
            else
                adjustFocusedSetting(true);
            return;
        }
    }

    if (focusedRow.type == RowType::Button)
    {
        if (input.isKeyPressed(KeyCode::Accept, false))
        {
            activateFocusedButton();
            return;
        }
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

    float currentY = kPanelY * ui;
    const float sharedLabelWidth = measureMaxLabelWidth(renderer, m_font, m_rows);

    for (int i = 0; i < static_cast<int>(m_rows.size()); ++i)
    {
        const auto &row = m_rows[i];
        const bool focused = (i == m_focusedIndex);

        const Rectf rowBox{kPanelX * ui, currentY, kPanelW * ui, kRowBoxH * ui};

        if (row.type == RowType::Button)
        {
            // Button rows use the full row box for their centered label
            std::string buttonLabel = row.getDisplayValue ? row.getDisplayValue() : row.label;
            std::string formatted = UIUtils::formatButtonLabel(buttonLabel, focused);
            const Color btnColor = focused ? UITheme::SelectedText : UITheme::Text;
            renderer->renderTextInRect(m_font,
                                       formatted,
                                       rowBox,
                                       btnColor,
                                       Renderer::HorizontalAlign::Center,
                                       Renderer::VerticalAlign::Middle,
                                       false, false, false);
        }
        else
        {
            std::string valueText;
            if (row.type == RowType::Slider && row.slider)
            {
                char pct[16];
                std::snprintf(pct, sizeof(pct), "%d%%",
                              static_cast<int>(row.slider->getValue() * 100.0f + 0.5f));
                valueText = pct;
            }
            else if (row.getDisplayValue)
            {
                valueText = row.getDisplayValue();
            }

            const RowLayout layout = computeRowLayout(renderer, m_font, sharedLabelWidth,
                                                      row.type == RowType::Slider,
                                                      valueText, currentY, kRowBoxH * ui, ui);

            UIUtils::drawRow(renderer, layout.rowBox, focused);

            const Color labelColor = focused ? UITheme::SelectedText : UITheme::Text;
            renderer->renderTextInRect(m_font,
                                       row.label,
                                       layout.labelRect,
                                       labelColor,
                                       Renderer::HorizontalAlign::Left,
                                       Renderer::VerticalAlign::Middle,
                                       false, false, false);

            if (row.type == RowType::Slider && row.slider)
            {
                row.slider->setTrackRect(layout.controlRect);
                row.slider->render(renderer);

                const Color pctColor = focused ? UITheme::SelectedText : UITheme::Text;
                renderer->renderTextInRect(m_font,
                                           valueText,
                                           layout.pctRect,
                                           pctColor,
                                           Renderer::HorizontalAlign::Left,
                                           Renderer::VerticalAlign::Middle,
                                           false, false, false);
            }
            else if (row.getDisplayValue)
            {
                const Color valueColor = focused ? UITheme::SelectedText : UITheme::Text;
                renderer->renderTextInRect(m_font,
                                           valueText,
                                           layout.controlRect,
                                           valueColor,
                                           Renderer::HorizontalAlign::Right,
                                           Renderer::VerticalAlign::Middle,
                                           false, false, false);
            }
        }

        currentY += kRowH * ui;
    }
}

// ── Private helpers ───────────────────────────────────────────────────────────

void SettingsRowWindow::moveFocus(int delta)
{
    if (m_rows.empty())
        return;
    const int count = static_cast<int>(m_rows.size());
    m_focusedIndex = (m_focusedIndex + delta + count) % count;
}

void SettingsRowWindow::adjustFocusedSetting(bool right)
{
    if (m_focusedIndex < 0 || m_focusedIndex >= static_cast<int>(m_rows.size()))
        return;
    const auto &row = m_rows[m_focusedIndex];
    if (row.type == RowType::Setting && row.onAdjust)
    {
        row.onAdjust(right);
        emit(UIEvent{.type = UIEventType::ActionSelected,
                     .windowId = id(),
                     .actionId = right ? ActionId::AdjustRight : ActionId::AdjustLeft,
                     .index = m_focusedIndex});
    }
}

void SettingsRowWindow::adjustFocusedSlider(bool right)
{
    if (m_focusedIndex < 0 || m_focusedIndex >= static_cast<int>(m_rows.size()))
        return;
    const auto &row = m_rows[m_focusedIndex];
    if (row.type == RowType::Slider && row.slider)
    {
        row.slider->step(right ? 0.03f : -0.03f);
        emit(UIEvent{.type = UIEventType::ActionSelected,
                     .windowId = id(),
                     .actionId = right ? ActionId::AdjustRight : ActionId::AdjustLeft,
                     .index = m_focusedIndex});
    }
}

void SettingsRowWindow::activateFocusedButton()
{
    if (m_focusedIndex < 0 || m_focusedIndex >= static_cast<int>(m_rows.size()))
        return;
    const auto &row = m_rows[m_focusedIndex];
    if (row.type == RowType::Button)
    {
        emit(UIEvent{.type = UIEventType::ActionSelected,
                     .windowId = id(),
                     .actionId = row.actionId,
                     .index = m_focusedIndex});
    }
}