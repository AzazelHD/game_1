#include "config/GameConstants.h"
#include "states/MainMenuState.h"
#include "states/WorldMapState.h"
#include "states/SettingsState.h"
#include "engine/core/App.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/MathUtils.h"
#include "engine/statemachine/StateMachine.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Renderer.h"
#include "ui/windows/ActionMenuWindow.h"
#include <SDL3/SDL.h>

namespace
{
    Font *g_mainTitleFont = nullptr;
    Font *g_mainMenuFont = nullptr;
}

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

    if (!g_mainMenuFont && m_renderer)
        g_mainMenuFont = m_renderer->loadFont("assets/fonts/PixeloidSans.ttf", 32.0f);

    auto *menu = m_uiManager.push<ActionMenuWindow>("menu.main");
    menu->setFont(g_mainMenuFont ? g_mainMenuFont : App::getDefaultFont());

    // Center horizontally (box width now follows the font); only fix the
    // vertical anchor below the title.
    menu->setPanelPosition(Vec2f{0.0f, GameConstants::VIEW_CY - 40.0f});
    menu->centerHorizontally(true);
    menu->setItems({
        ActionMenuWindow::Item{.id = "start", .label = "Start Game", .enabled = true},
        ActionMenuWindow::Item{.id = "options", .label = "Options", .enabled = true},
        ActionMenuWindow::Item{.id = "quit", .label = "Quit", .enabled = true},
    });

    if (!g_mainTitleFont && m_renderer)
        g_mainTitleFont = m_renderer->loadFont("assets/fonts/PixeloidSans.ttf", 72.0f);

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

    const Font *font = App::getDefaultFont();
    const Font *titleFont = g_mainTitleFont ? g_mainTitleFont : font;
    if (titleFont)
    {
        const std::string title = "TRPG";
        m_renderer->renderTextInRect(titleFont,
                                     title,
                                     Rectf{0.0f, 130.0f, GameConstants::VIEW_W, 64.0f},
                                     Color{235, 240, 250, 255},
                                     Renderer::HorizontalAlign::Center,
                                     Renderer::VerticalAlign::Middle,
                                     false,
                                     false,
                                     false);
    }

    m_uiManager.render(m_renderer);

    m_transition.render(m_renderer, GameConstants::VIEW_W, GameConstants::VIEW_H);
}

void MainMenuState::processUIEvents()
{
    auto events = m_uiManager.drainEvents();
    for (const UIEvent &event : events)
    {
        if (event.windowId != "menu.main" || event.type != UIEventType::ActionSelected)
            continue;

        m_transitioning = true;
        if (event.actionId == "start")
        {
            m_stateMachine.replace(std::make_unique<WorldMapState>(m_stateMachine, m_renderer));
        }
        else if (event.actionId == "options")
        {
            m_stateMachine.push(std::make_unique<SettingsState>(m_stateMachine, m_renderer));
            m_transitioning = false;
        }
        else if (event.actionId == "quit")
        {
            SDL_Event quitEvent{};
            quitEvent.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quitEvent);
            m_transitioning = false;
        }
        else
        {
            m_transitioning = false;
        }
    }
}