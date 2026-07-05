#include "engine/input/Input.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Renderer.h"
#include "engine/core/App.h"
#include "config/GameConstants.h"
#include "states/PauseState.h"
#include "states/MainMenuState.h"
#include "ui/windows/ActionMenuWindow.h"

#include <memory>

namespace
{
    constexpr float PANEL_X = 540.0f;
    constexpr float PANEL_Y = 260.0f;
}

PauseState::PauseState(StateMachine<Scene> &sm, Renderer *renderer)
    : m_sm(sm), m_renderer(renderer)
{
}

void PauseState::onEnter()
{
    m_uiManager.clear();

    auto *menu = m_uiManager.push<ActionMenuWindow>("pause.main");
    menu->setFont(App::getDefaultFont());
    menu->setPanelPosition(Vec2f{PANEL_X, PANEL_Y});
    menu->setItems({
        ActionMenuWindow::Item{.id = "resume", .label = "Resume", .enabled = true},
        ActionMenuWindow::Item{.id = "quit", .label = "Quit", .enabled = true},
    });
}

void PauseState::onExit()
{
    m_uiManager.clear();
}

void PauseState::handleInput()
{
    const Input &input = Input::instance();

    // 1. Handle Cancel (Back / Escape) immediately.
    //    Escape closes the pause menu and resumes BattleState underneath.
    if (input.isKeyPressed(KeyCode::Back, false))
    {
        m_sm.pop();
        return;
    }

    // 2. Feed input to the UI stack: ActionMenuWindow handles arrow/WASD
    //    navigation and Confirm/Accept internally, queuing UIEvents.
    m_uiManager.handleInput(input);

    // 3. Drain those queued events and act on them (Resume / Quit).
    processUIEvents();
}

void PauseState::processUIEvents()
{
    for (const UIEvent &event : m_uiManager.drainEvents())
    {
        if (event.windowId != "pause.main" || event.type != UIEventType::ActionSelected)
            continue;
        if (event.actionId == "resume")
            m_sm.pop();
        else if (event.actionId == "quit")
            m_sm.replace(std::make_unique<MainMenuState>(m_sm, m_renderer));
    }
}

void PauseState::update(float dt)
{
    m_uiManager.update(dt);
}

void PauseState::render(float /*alpha*/)
{
    // alpha (frame-interpolation factor) is unused: the pause overlay and menu
    // are static between frames, so there is nothing to interpolate.
    if (!m_renderer)
        return;

    // Semi-transparent dark overlay over the frozen BattleState framebuffer.
    m_renderer->setBlendMode(Renderer::BlendMode::Blend);
    m_renderer->setDrawColor(Color{0, 0, 0, 160});
    m_renderer->fillRect(Rectf{0.0f, 0.0f, GameConstants::VIEW_W, GameConstants::VIEW_H});

    // Draw the pause menu (ActionMenuWindow) on top of the overlay.
    m_uiManager.render(m_renderer);
}