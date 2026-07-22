#pragma once

#include "engine/scene/Scene.h"
#include "engine/statemachine/StateMachine.h"

class Renderer;

// BootState runs once at startup: load shared assets, then transition to MainMenuState.
//
// [x]: Implement Scene interface.
//   onEnter(): start loading assets (textures, fonts). For now a placeholder is fine.
//   update(dt): if loading is done, call m_sm.replace(std::make_unique<MainMenuState>(...));
//   render(): show a loading screen or just a black screen.
//   onExit(): nothing needed here.
//
// Tip: BootState needs a reference to the StateMachine so it can trigger the transition.
//      Pass it in the constructor and store it as a reference member.

class BootState final : public Scene
{
public:
    BootState(StateMachine<Scene> &sm, Renderer *renderer);

    BootState(const BootState &) = delete;
    BootState &operator=(const BootState &) = delete;
    BootState(BootState &&) = delete;
    BootState &operator=(BootState &&) = delete;

    ~BootState() override = default;

    void onEnter() override;
    void onExit() override;
    void handleInput() override;
    void update(float dt) override;
    void render(float alpha) override;

private:
    void finishBoot();

private:
    StateMachine<Scene> &m_sm;
    Renderer *m_renderer = nullptr;

    bool m_readyToTransition = false;
};
