#include "battle/controllers/DialogueController.h"

#include "battle/unit/Unit.h"
#include "engine/renderer/FontManager.h"
#include "ui/windows/DialogWindow.h"

void DialogueController::begin(DialogWindow *window, std::vector<DialogueBeat> beats,
                               std::function<void()> onComplete)
{
    m_window = window;
    m_beats = std::move(beats);
    m_onComplete = std::move(onComplete);

    if (!m_window || m_beats.empty())
    {
        m_window = nullptr;
        return;
    }

    std::vector<DialogWindow::Line> lines;
    lines.reserve(m_beats.size());
    for (const DialogueBeat &beat : m_beats)
        lines.push_back(DialogWindow::Line{.speaker = beat.speakerName, .text = beat.text});

    m_window->start(std::move(lines));
}

Unit *DialogueController::currentSpeaker() const
{
    if (!m_window)
        return nullptr;

    const int index = m_window->currentLineIndex();
    if (index < 0 || index >= static_cast<int>(m_beats.size()))
        return nullptr;

    return m_beats[static_cast<std::size_t>(index)].speaker;
}

void DialogueController::notifyDialogFinished()
{
    if (!m_window)
        return;

    m_window = nullptr;
    m_beats.clear();

    if (m_onComplete)
    {
        auto callback = std::move(m_onComplete);
        m_onComplete = nullptr;
        callback();
    }
}