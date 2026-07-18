#include "ui/windows/DialogWindow.h"

#include "config/GameConstants.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Renderer.h"
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
        ++m_visibleChars;
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
    const float h = 140.0f;
    const float x = 40.0f;
    const float y = GameConstants::VIEW_H - h - 30.0f;

    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(UITheme::PopupBG);
    renderer->fillRect(Rectf{x, y, w, h});
    renderer->setDrawColor(UITheme::PopupBorder);
    renderer->drawRect(Rectf{x, y, w, h});

    const Line &line = m_lines[m_lineIndex];
    if (!line.speaker.empty())
    {
        renderer->renderTextInRect(m_font,
                                   line.speaker,
                                   Rectf{x, y + 10.0f, w, 20.0f},
                                   UITheme::Cursor,
                                   Renderer::HorizontalAlign::Center,
                                   Renderer::VerticalAlign::Middle,
                                   false,
                                   false,
                                   false);
    }

    std::string visible = line.text.substr(0, static_cast<std::size_t>(m_visibleChars));
    renderer->renderTextInRect(m_font,
                               visible,
                               Rectf{x, y + 40.0f, w, 24.0f},
                               UITheme::Text,
                               Renderer::HorizontalAlign::Center,
                               Renderer::VerticalAlign::Middle,
                               false,
                               false,
                               false);
}

bool DialogWindow::isCurrentLineFinished() const
{
    if (m_lineIndex < 0 || m_lineIndex >= static_cast<int>(m_lines.size()))
        return true;

    return m_visibleChars >= static_cast<int>(m_lines[m_lineIndex].text.size());
}
