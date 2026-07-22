#include "ui/windows/ConfirmWindow.h"

#include "config/GameConstants.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Renderer.h"
#include "engine/ui/HorizontalLayout.h"
#include "engine/ui/Insets.h"
#include "ui/WindowId.h"
#include "ui/UITheme.h"
#include "ui/UIScale.h"
#include "ui/UIUtils.h"

#include <algorithm>
#include <sstream>
#include <vector>

namespace
{

    constexpr float kMinW = 300.0f;
    constexpr float kMaxW = 520.0f;
    constexpr float kHorizontalPad = 40.0f; // total left+right padding around text
    constexpr float kTopPad = 24.0f;        // top padding above first line of text
    constexpr float kBottomPad = 24.0f;     // bottom padding below last line of text
    constexpr float kLineSpacing = 4.0f;    // vertical spacing between lines of text
    constexpr float kButtonsGap = 56.0f;    // gap between last text line and the buttons row
    constexpr float kButtonSpacing = 40.0f; // gap between Cancel and Confirm

    std::vector<std::string> wrapText(Renderer *renderer, const Font *font,
                                      const std::string &text, float maxWidth)
    {
        std::vector<std::string> lines;
        if (!renderer || !font || text.empty())
            return lines;

        // Split on manual line breaks first, then word-wrap each paragraph
        // independently — so an explicit \n always starts a new line, and
        // long paragraphs still wrap automatically within maxWidth.
        std::istringstream paragraphStream(text);
        std::string paragraph;

        while (std::getline(paragraphStream, paragraph, '\n'))
        {
            std::istringstream wordStream(paragraph);
            std::string word;
            std::string currentLine;

            while (wordStream >> word)
            {
                const std::string candidate = currentLine.empty() ? word : currentLine + " " + word;
                const float candidateWidth = renderer->measureText(font, candidate).x;
                if (candidateWidth <= maxWidth || currentLine.empty())
                    currentLine = candidate;
                else
                {
                    lines.push_back(currentLine);
                    currentLine = word;
                }
            }

            // Push whatever's left, even if the paragraph was empty (keeps a
            // blank line for consecutive \n\n, instead of silently dropping it).
            lines.push_back(currentLine);
        }

        return lines;
    }

} // anonymous namespace

ConfirmWindow::ConfirmWindow(WindowId id)
    : UIWindow(id, true, false)
{
    m_cancelButton = std::make_unique<ButtonControl>(
        [this]() -> std::string
        { return m_cancelLabelText; },
        [this]()
        { emit(UIEvent{.type = UIEventType::ConfirmResult, .windowId = this->id(), .actionId = ActionId::Cancel, .confirmed = false}); });
    m_cancelButton->setLabelFormatter([](const std::string &label, bool selected)
                                      { return UIUtils::formatButtonLabel(label, selected); });

    m_confirmButton = std::make_unique<ButtonControl>(
        [this]() -> std::string
        { return m_confirmLabelText; },
        [this]()
        { emit(UIEvent{.type = UIEventType::ConfirmResult, .windowId = this->id(), .actionId = ActionId::Confirm, .confirmed = true}); });
    m_confirmButton->setLabelFormatter([](const std::string &label, bool selected)
                                       { return UIUtils::formatButtonLabel(label, selected); });

    m_focus.resetFromPointers({m_cancelButton.get(), m_confirmButton.get()});
    // m_focus.focusNext(); // default to Confirm selected, matching prior behavior
}

void ConfirmWindow::handleInput(const Input &input)
{
    // Q/E are used elsewhere (e.g. Battle's attack-target cycling) — kept
    // as-is, independent of Cancel/Confirm selection.
    if (input.isKeyPressed(KeyCode::Q, false))
    {
        emit(UIEvent{.type = UIEventType::NavigatePrevious, .windowId = id()});
        return;
    }
    if (input.isKeyPressed(KeyCode::E, false))
    {
        emit(UIEvent{.type = UIEventType::NavigateNext, .windowId = id()});
        return;
    }

    if (input.isKeyPressed(KeyCode::Left, false) || input.isKeyPressed(KeyCode::A, false) ||
        input.isKeyPressed(KeyCode::Right, false) || input.isKeyPressed(KeyCode::D, false))
    {
        m_focus.focusNext(); // only 2 items — either direction just toggles
        return;
    }

    if (input.isKeyPressed(KeyCode::Back, false))
    {
        emit(UIEvent{.type = UIEventType::ConfirmResult, .windowId = id(), .actionId = ActionId::Cancel, .confirmed = false});
        return;
    }

    if (input.isKeyPressed(KeyCode::Accept, false))
    {
        (void)m_focus.activateSelected(); // the selected button's own callback emits ConfirmResult
    }
}

void ConfirmWindow::update(float /*dt*/)
{
}

void ConfirmWindow::render(Renderer *renderer) const
{
    if (!renderer || !m_font)
        return;

    UIScale::refresh();
    const float ui = UIScale::factor();

    const float maxBoxW = kMaxW * ui;
    const float innerMaxTextW = maxBoxW - kHorizontalPad * ui;

    const std::vector<std::string> lines = wrapText(renderer, m_font, m_prompt, innerMaxTextW);

    float widestLineW = 0.0f;
    for (const std::string &line : lines)
        widestLineW = std::max(widestLineW, renderer->measureText(m_font, line).x);

    // Worst-case (bracketed) button widths — computed here too, so the box
    // is guaranteed wide enough for them, not just for the message text.
    const float cancelW = renderer->measureText(m_font, "> " + m_cancelLabelText + " <").x;
    const float confirmW = renderer->measureText(m_font, "> " + m_confirmLabelText + " <").x;
    const float buttonsRowW = cancelW + kButtonSpacing * ui + confirmW;

    const float widestContentW = std::max(widestLineW, buttonsRowW);
    const float w = std::clamp(widestContentW + kHorizontalPad * ui, kMinW * ui, maxBoxW);

    const float lineH = renderer->measureText(m_font, "Ag").y;
    const float linesBlockH = static_cast<float>(lines.size()) * (lineH + kLineSpacing * ui);
    const float h = (kTopPad * ui) + linesBlockH + (kButtonsGap * ui) + (kBottomPad * ui);

    const float x = (GameConstants::VIEW_W - w) * 0.5f;
    const float y = (GameConstants::VIEW_H - h) * 0.5f;

    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(UITheme::PopupBG);
    renderer->fillRect(Rectf{x, y, w, h});
    renderer->setDrawColor(UITheme::PopupBorder);
    renderer->drawRect(Rectf{x, y, w, h});

    float lineY = y + kTopPad * ui;
    for (const std::string &line : lines)
    {
        renderer->renderTextInRect(m_font,
                                   line,
                                   Rectf{x, lineY, w, lineH},
                                   UITheme::Text,
                                   Renderer::HorizontalAlign::Center,
                                   Renderer::VerticalAlign::Middle,
                                   false, false, false);
        lineY += lineH + kLineSpacing * ui;
    }

    const HorizontalLayout::Container cancelContainer{
        .items = {{cancelW, lineH, Insets{}}},
        .padding = Insets{},
    };
    const HorizontalLayout::Container confirmContainer{
        .items = {{confirmW, lineH, Insets{}}},
        .padding = Insets{},
    };

    const float totalButtonsWidth = cancelW + kButtonSpacing * ui + confirmW;
    const float buttonsStartX = x + (w - totalButtonsWidth) * 0.5f;
    const float buttonsY = y + h - kBottomPad * ui - lineH;

    const auto results = HorizontalLayout::layoutContainers(
        {cancelContainer, confirmContainer},
        Vec2f{buttonsStartX, buttonsY},
        kButtonSpacing * ui);

    m_cancelButton->render(renderer, m_font, results[0].itemRects[0], ui, UITheme::Text, UITheme::SelectedText);
    m_confirmButton->render(renderer, m_font, results[1].itemRects[0], ui, UITheme::Text, UITheme::SelectedText);
}
