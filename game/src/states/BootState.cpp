#include "engine/core/App.h"
#include "engine/core/Log.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Renderer.h"
#include "states/BootState.h"
#include "states/MainMenuState.h"

#include <memory>

BootState::BootState(StateMachine<Scene> &sm, Renderer *renderer)
    : m_sm(sm), m_renderer(renderer)
{
}

void BootState::onEnter()
{
    // Load the single global UI font exactly once, at boot. Every state
    // consumes App::getDefaultFont() from here on — no per-state loading.
    if (!App::getDefaultFont())
    {
        Font *font = m_renderer
                         ? m_renderer->loadFont("assets/fonts/PixeloidSans.ttf", 24.0f)
                         : nullptr;
        if (!font)
            LOG_ERROR("Boot", "Failed to load default UI font");
        App::setDefaultFont(font);
    }

    m_readyToTransition = true;
}

void BootState::onExit()
{
}

void BootState::handleInput()
{
    // If the assets aren't even ready yet (for future async compatibility),
    // don't let them skip into an uninitialized main menu.
    if (!m_readyToTransition)
        return;

    const Input &input = Input::instance();

    // Check for a clean, non-repeated confirmation tap (Enter/Space/etc.)
    if (input.isKeyPressed(KeyCode::Accept, false) ||
        input.isKeyPressed(KeyCode::Advance, false))
    {
        finishBoot();
    }
}

void BootState::update(float /*dt*/)
{
    if (!m_readyToTransition)
        return;

    finishBoot();
}

void BootState::render(float alpha)
{
    // Optional loading screen hook.
    // If needed: clear screen or draw splash here.
}

void BootState::finishBoot()
{
    m_readyToTransition = false;

    m_sm.replace(std::make_unique<MainMenuState>(m_sm, m_renderer));
}
