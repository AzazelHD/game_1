#include "config/GameConstants.h"
#include "states/PauseState.h"
#include "states/MainMenuState.h"
#include "engine/input/Input.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Renderer.h"

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
    m_menu = MenuPanel{};
    m_menu.setPosition({PANEL_X, PANEL_Y});
    m_menu.setVerticalLayout(VerticalLayoutConfig{.spacing = 16.0f});
    m_menu.addButton(Button(Rectf{0.0f, 0.0f, 220.0f, 52.0f}, "Resume"));
    m_menu.addButton(Button(Rectf{0.0f, 0.0f, 220.0f, 52.0f}, "Quit"));
}

void PauseState::onExit()
{
}

void PauseState::handleInput()
{
    const Input &input = Input::instance();

    // 1. Handle Cancel (Back / Escape) immediately
    if (input.isKeyPressed(KeyCode::Back, false))
    {
        m_sm.pop();
        return;
    }

    // 2. Let the menu panel process arrow/WASD navigation internally
    m_menu.handleInput();

    // 3. Handle Confirm (Accept / Enter) button activation
    if (input.isKeyPressed(KeyCode::Accept, false) && m_menu.activateSelected())
    {
        switch (m_menu.getSelectedIndex())
        {
        case 0: // Resume
            m_sm.pop();
            break;
        case 1: // Quit
            m_sm.replace(std::make_unique<MainMenuState>(m_sm, m_renderer));
            break;
        default:
            break;
        }
    }
}

void PauseState::update(float /*dt*/)
{
}

void PauseState::render(float /*alpha*/)
{
    if (!m_renderer)
        return;

    // Semi-transparent dark overlay over the frozen BattleState framebuffer.
    m_renderer->setBlendMode(Renderer::BlendMode::Blend);
    m_renderer->setDrawColor(Color{0, 0, 0, 160});
    m_renderer->fillRect(Rectf{0.0f, 0.0f, GameConstants::VIEW_W, GameConstants::VIEW_H});

    // TODO: MenuPanel/Button still take SDL_Renderer* - bridge until UI widgets
    // are migrated to engine::Renderer.
    m_menu.render(m_renderer);
}