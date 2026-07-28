#pragma once

#include "engine/scene/Scene.h"
#include "ui/UIManager.h"

class Renderer;

template <typename T>
class StateMachine;

class SettingsState : public Scene
{
public:
    SettingsState(StateMachine<Scene> &sm, Renderer *renderer);

    void onEnter() override;
    void onExit() override {}
    void handleInput() override;
    void update(float dt) override;
    void render(float alpha) override;

private:
    void processUIEvents();

    StateMachine<Scene> &m_sm;
    Renderer *m_renderer;
    UIManager m_uiManager;
};
