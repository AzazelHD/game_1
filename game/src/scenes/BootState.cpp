#include "engine/core/App.h"
#include "engine/core/Log.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/FontManager.h"
#include "engine/renderer/Renderer.h"
#include "data/SettingsManager.h"
#include "scenes/BootState.h"
#include "scenes/MainMenuState.h"

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

    // The Window's initial size/border were already set from settings.json
    // once, before this scene existed (see App's WindowConfigFactory in
    // main.cpp) — that's what gets something correct on screen as early as
    // possible. Calling applyGraphics() here makes SettingsManager the
    // single source of truth going forward: it re-applies (or corrects, if
    // the two ever drift) window state using the exact same code path that
    // runs whenever the player changes settings later, instead of trusting
    // the constructor-time config to have matched perfectly.
    SettingsManager::instance().applyGraphics();

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
