#include "ui/windows/DialogWindow.h"

#include "config/GameConstants.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Aligment.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Renderer.h"
#include "engine/ui/TextBlock.h"
#include "ui/UITheme.h"
#include "ui/WindowId.h"

namespace
{
    constexpr float kCharInterval = 0.02f;
}

DialogWindow::DialogWindow(WindowId id)
    : UIWindow(id, true, false)
{
}

void DialogWindow::start(std::vector<Line> lines)
{
    m_lines = std::move(lines);
    m_lineIndex = 0;
    m_visibleChars = 0;
    m_timer = 0.0f;
}

void DialogWindow::handleInput(const Input &input)
{
    if (m_lines.empty())
        return;

    if (!(input.isKeyPressed(KeyCode::Accept, false) || input.isKeyPressed(KeyCode::Back, false)))
        return;

    if (!isCurrentLineFinished())
    {
        m_visibleChars = static_cast<int>(m_lines[m_lineIndex].text.size());
        emit(UIEvent{.type = UIEventType::DialogAdvanced, .windowId = id()});
        return;
    }

    ++m_lineIndex;
    m_visibleChars = 0;
    m_timer = 0.0f;

    emit(UIEvent{.type = UIEventType::DialogAdvanced, .windowId = id()});

    if (m_lineIndex >= static_cast<int>(m_lines.size()))
    {
        emit(UIEvent{.type = UIEventType::DialogFinished, .windowId = id()});
    }
}

void DialogWindow::update(float dt)
{
    if (m_lines.empty() || m_lineIndex < 0 || m_lineIndex >= static_cast<int>(m_lines.size()))
        return;

    if (isCurrentLineFinished())
        return;

    m_timer += dt;
    while (m_timer >= kCharInterval && !isCurrentLineFinished())
    {
        // Reveal a whole UTF-8 code point, never half of one: m_visibleChars
        // is a byte index, and splitting a multi-byte char (e.g. the em-dash
        // "—") renders as garbage for a frame — a visible blink per char.
        const std::string &text = m_lines[m_lineIndex].text;
        std::size_t i = static_cast<std::size_t>(m_visibleChars) + 1;
        while (i < text.size() && (static_cast<unsigned char>(text[i]) & 0xC0) == 0x80)
            ++i;
        m_visibleChars = static_cast<int>(i);
        m_timer -= kCharInterval;
    }
}

void DialogWindow::render(Renderer *renderer) const
{
    if (!renderer || !m_font || m_lines.empty())
        return;

    if (m_lineIndex < 0 || m_lineIndex >= static_cast<int>(m_lines.size()))
        return;

    const float w = GameConstants::VIEW_W - 80.0f;
    const float x = 40.0f;
    const float inset = 20.0f;
    const float textW = w - (inset * 2.0f);

    const Line &line = m_lines[m_lineIndex];
    constexpr float kLineSpacing = 4.0f;
    const float speakerH = line.speaker.empty() ? 8.0f : 38.0f;

    // Measure the full text once to size the box, so the dialog doesn't
    // reflow while the typewriter effect reveals characters.
    const TextBlock::Layout fullLayout =
        TextBlock::measure(*renderer, {TextBlock::Line{m_font, line.text, UITheme::Text}}, textW, kLineSpacing);
    const float h = std::max(140.0f, 14.0f + speakerH + fullLayout.height());
    const float y = GameConstants::VIEW_H - h - 30.0f;

    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(UITheme::PopupBG);
    renderer->fillRect(Rectf{x, y, w, h});
    renderer->setDrawColor(UITheme::PopupBorder);
    renderer->drawRect(Rectf{x, y, w, h});

    if (!line.speaker.empty())
    {
        renderer->renderTextInRect(m_font,
                                   line.speaker,
                                   Rectf{x, y + 8.0f, w, 24.0f},
                                   UITheme::Cursor,
                                   HorizontalAlign::Center,
                                   VerticalAlign::Middle,
                                   false,
                                   false,
                                   false);
    }

    // Reveal wraps the currently visible substring — the typewriter reveals
    // characters across wrapped rows exactly as before, but long lines now
    // flow onto multiple rows inside the box instead of overflowing it.
    const std::string visible = line.text.substr(0, static_cast<std::size_t>(m_visibleChars));
    const Rectf textArea{x + inset, y + 6.0f + speakerH, textW, h - speakerH - 16.0f};
    TextBlock::render(*renderer,
                      {TextBlock::Line{m_font, visible, UITheme::Text}},
                      textArea,
                      TextBlock::Anchor::TopLeft,
                      textW,
                      kLineSpacing,
                      HorizontalAlign::Center,
                      true);
}

bool DialogWindow::isCurrentLineFinished() const
{
    if (m_lineIndex < 0 || m_lineIndex >= static_cast<int>(m_lines.size()))
        return true;

    return m_visibleChars >= static_cast<int>(m_lines[m_lineIndex].text.size());
}
