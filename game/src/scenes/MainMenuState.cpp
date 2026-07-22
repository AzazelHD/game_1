#include "engine/core/App.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/MathUtils.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/FontManager.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Renderer.h"
#include "engine/statemachine/StateMachine.h"
#include "config/GameConstants.h"
#include "scenes/MainMenuState.h"
#include "scenes/SettingsState.h"
#include "ui/windows/ButtonMenuWindow.h"
#include "ui/ActionId.h"
#include "ui/WindowId.h"
#include "scenes/WorldMapState.h"

MainMenuState::MainMenuState(StateMachine<Scene> &sm, Renderer *renderer, bool fadeInOnEnter)
    : m_stateMachine(sm),
      m_renderer(renderer),
      m_fadeInOnEnter(fadeInOnEnter)
{
}

void MainMenuState::onEnter()
{
    m_transitioning = false;
    m_uiManager.clear();

    auto *menu = m_uiManager.push<ButtonMenuWindow>(WindowId::MainMenu);
    menu->setFont(FontManager::instance().get(FontRole::Heading));

    // Center horizontally (box width now follows the font); only fix the
    // vertical anchor below the title.
    menu->setPanelPosition(Vec2f{0.0f, GameConstants::VIEW_CY - 40.0f});
    menu->centerHorizontally(true);
    menu->setItems({
        ButtonMenuWindow::Item{.id = ActionId::StartGame, .label = "Start Game", .enabled = true},
        ButtonMenuWindow::Item{.id = ActionId::OpenSettings, .label = "Options", .enabled = true},
        ButtonMenuWindow::Item{.id = ActionId::QuitGame, .label = "Quit", .enabled = true},
    });

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
    m_uiManager.clear();
}

void MainMenuState::handleInput()
{
    const Input &input = Input::instance();

    // 1. GUARD FIRST: Block all actions if we are currently mid-transition
    if (m_transition.isActive() || m_transitioning)
        return;

    m_uiManager.handleInput(input);
    processUIEvents();
}

void MainMenuState::update(float dt)
{
    // This is purely for background visuals, fading animations, or UI transitions.
    if (m_transition.isActive())
    {
        m_transition.update(dt);
    }

    m_uiManager.update(dt);
}

void MainMenuState::render(float alpha)
{
    (void)alpha;
    m_renderer->clear(Color{18, 21, 29, 255});

    const std::string title = "TRPG";
    m_renderer->renderTextInRect(FontManager::instance().get(FontRole::Title),
                                 title,
                                 Rectf{0.0f, 130.0f, GameConstants::VIEW_W, 64.0f},
                                 Color{235, 240, 250, 255},
                                 Renderer::HorizontalAlign::Center,
                                 Renderer::VerticalAlign::Middle,
                                 false,
                                 false,
                                 false);

    m_uiManager.render(m_renderer);

    m_transition.render(m_renderer, GameConstants::VIEW_W, GameConstants::VIEW_H);
}

void MainMenuState::processUIEvents()
{
    auto events = m_uiManager.drainEvents();
    for (const UIEvent &event : events)
    {
        if (event.windowId != WindowId::MainMenu || event.type != UIEventType::ActionSelected)
            continue;

        m_transitioning = true;

        switch (event.actionId)
        {
        case ActionId::StartGame:
        {
            m_stateMachine.replace(std::make_unique<WorldMapState>(m_stateMachine, m_renderer));
            break;
        }
        case ActionId::OpenSettings:
        {
            m_stateMachine.push(std::make_unique<SettingsState>(m_stateMachine, m_renderer));
            m_transitioning = false;
            break;
        }
        case ActionId::QuitGame:
        {
            App::requestQuit();
            break;
        }

        default:
        {
            m_transitioning = false;
            break;
        }
        }
    }
}
