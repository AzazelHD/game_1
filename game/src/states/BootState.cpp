#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "states/BootState.h"
#include "states/MainMenuState.h"

#include <memory>

BootState::BootState(StateMachine<Scene> &sm, Renderer *renderer)
    : m_sm(sm), m_renderer(renderer)
{
}

void BootState::onEnter()
{
    // Placeholder boot pipeline (currently synchronous):
    // - preload shared assets
    // - load config
    // - init systems

    // If this becomes async later, set m_readyToTransition when done.
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