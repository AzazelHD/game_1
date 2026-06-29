#include "config/GameConstants.h"
#include "states/MainMenuState.h"
#include "states/BattleState.h"
#include "states/SettingsState.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/MathUtils.h"
#include "engine/statemachine/StateMachine.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Renderer.h"

MainMenuState::MainMenuState(StateMachine<Scene> &sm, Renderer *renderer, bool fadeInOnEnter)
    : m_stateMachine(sm),
      m_renderer(renderer),
      m_fadeInOnEnter(fadeInOnEnter)
{
}

void MainMenuState::onEnter()
{
    m_transitioning = false;

    m_menu = MenuPanel{};
    m_menu.setPosition({460.0f, 320.0f});
    m_menu.setVerticalLayout(VerticalLayoutConfig{.spacing = 14.0f});
    m_menu.addButton(Button(Rectf{0.0f, 0.0f, 320.0f, 56.0f}, "Start Battle"));
    m_menu.addButton(Button(Rectf{0.0f, 0.0f, 320.0f, 56.0f}, "Settings"));

    LOG_INFO("MainMenu", "onEnter: menu size=%d, selectedIndex=%d", m_menu.size(), m_menu.getSelectedIndex());

    if (m_fadeInOnEnter)
    {
        m_transition.start({
            .transition = ScreenTransitions::FadeIn,
            .duration = 0.5f,
            .easing = easeInOut,
        });
    }
}

void MainMenuState::onExit()
{
}

void MainMenuState::handleInput()
{
    const Input &input = Input::instance();

    // 1. GUARD FIRST: Block all actions if we are currently mid-transition
    if (m_transition.isActive() || m_transitioning)
        return;

    // 2. NOW let the menu panel process arrow/WASD navigation safely
    m_menu.update();

    // 3. Handle activation cleanly
    if (input.isKeyPressed(KeyCode::Accept, false) && m_menu.activateSelected())
    {
        m_transitioning = true;

        switch (m_menu.getSelectedIndex())
        {
        case 0:
            m_stateMachine.replace(std::make_unique<BattleState>(m_stateMachine, m_renderer));
            break;
        case 1:
            m_stateMachine.push(std::make_unique<SettingsState>(m_stateMachine, m_renderer));
            m_transitioning = false;
            break;
        default:
            m_transitioning = false;
            break;
        }
    }
}

void MainMenuState::update(float dt)
{
    // This is purely for background visuals, fading animations, or UI transitions.
    if (m_transition.isActive())
    {
        m_transition.update(dt);
    }
}

void MainMenuState::render(float alpha)
{
    (void)alpha;
    m_renderer->clear(Color{20, 24, 32, 255});

    m_renderer->setDrawColor(Color{70, 90, 140, 255});
    m_renderer->fillRect(Rectf{220.f, 180.f, 360.f, 120.f});

    // TODO: MenuPanel/ScreenTransition still take SDL_Renderer* - bridge until
    // those engine UI widgets are migrated to engine::Renderer.
    m_menu.render(m_renderer);
    m_transition.render(m_renderer, GameConstants::VIEW_W, GameConstants::VIEW_H);
}