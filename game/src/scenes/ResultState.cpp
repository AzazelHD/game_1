#include "config/GameConstants.h"
#include "scenes/ResultState.h"
#include "scenes/MainMenuState.h"
#include "engine/core/App.h"
#include "engine/math/Rect.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/statemachine/StateMachine.h"
#include "engine/renderer/Renderer.h"
#include "engine/renderer/Color.h"

#include <memory>

ResultState::ResultState(StateMachine<Scene> &sm, Renderer *renderer, bool playerWon)
    : m_sm(sm), m_renderer(renderer), m_playerWon(playerWon)
{
}

void ResultState::onEnter()
{
    m_transition.start({
        .transition = ScreenTransitions::FadeIn,
        .duration = 0.5f,
        .easing = easeInOut,
    });
}

void ResultState::onExit()
{
}

void ResultState::handleInput()
{
    // Guard: Don't accept inputs if the screen is fading in or fading out
    if (m_transition.isActive())
        return;

    const Input &input = Input::instance();

    // Only trigger on a fresh press, not on repeat
    if (input.isKeyPressed(KeyCode::Accept, false) ||
        input.isKeyPressed(KeyCode::Advance, false))
    {
        m_transition.start({.transition = ScreenTransitions::FadeOut,
                            .duration = 0.5f,
                            .easing = easeInOut,
                            .onComplete = [this]()
                            {
                                m_sm.replace(std::make_unique<MainMenuState>(m_sm, m_renderer, true));
                            }});
    }
}

void ResultState::update(float dt)
{
    m_transition.update(dt);
}

void ResultState::render(float /*alpha*/)
{
    if (!m_renderer)
    {
        return;
    }

    m_renderer->setDrawColor(Color{14, 14, 18, 255});
    m_renderer->fillRect(Rectf{0.0f, 0.0f, GameConstants::VIEW_W, GameConstants::VIEW_H});

    const Rectf panel = {360.0f, 210.0f, 560.0f, 300.0f};

    if (m_playerWon)
    {
        m_renderer->setDrawColor(Color{28, 92, 44, 255});
    }
    else
    {
        m_renderer->setDrawColor(Color{100, 34, 34, 255});
    }
    m_renderer->fillRect(panel);

    m_renderer->setDrawColor(Color{240, 240, 240, 255});
    m_renderer->drawRect(panel);

    // TODO: ScreenTransition still takes SDL_Renderer* - bridge until migrated.
    m_transition.render(m_renderer, GameConstants::VIEW_W, GameConstants::VIEW_H);
}
