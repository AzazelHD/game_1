#include "ui/DialogSystem.h"

#include "engine/input/Input.h"
#include "engine/renderer/Renderer.h"
#include "ui/windows/DialogWindow.h"

void DialogSystem::start(std::vector<DialogLine> lines, std::function<void()> onFinish, const Font *font)
{
    m_lines = std::move(lines);
    m_onFinish = std::move(onFinish);
    m_active = !m_lines.empty();

    m_uiManager.clear();
    m_window = nullptr;

    if (!m_active)
        return;

    m_window = m_uiManager.push<DialogWindow>("dialog.system");
    m_window->setFont(font);

    std::vector<DialogWindow::Line> windowLines;
    windowLines.reserve(m_lines.size());
    for (const DialogLine &line : m_lines)
    {
        windowLines.push_back(DialogWindow::Line{.speaker = line.speakerName, .text = line.text});
    }

    m_window->start(std::move(windowLines));
}

void DialogSystem::update(float dt, const Input &input)
{
    if (!m_active)
        return;

    m_uiManager.handleInput(input);
    m_uiManager.update(dt);

    auto events = m_uiManager.drainEvents();
    for (const UIEvent &event : events)
    {
        if (event.windowId == "dialog.system" && event.type == UIEventType::DialogFinished)
        {
            m_active = false;
            m_uiManager.clear();
            m_window = nullptr;
            if (m_onFinish)
                m_onFinish();
            break;
        }
    }
}

void DialogSystem::render(Renderer *renderer) const
{
    if (!m_active)
        return;
    m_uiManager.render(renderer);
}
