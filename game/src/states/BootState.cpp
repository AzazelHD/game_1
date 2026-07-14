#include "engine/core/App.h"
#include "engine/core/Log.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/FontManager.h"
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
    // Every font role is loaded once, here, at boot. Every state consumes
    // FontManager::instance().get(role) from here on — no per-state loading.
    FontManager::instance().loadAll(m_renderer);

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
