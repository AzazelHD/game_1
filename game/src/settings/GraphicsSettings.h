#pragma once

#include "engine/scene/Scene.h"
#include "data/SettingsManager.h"
#include "ui/UIManager.h"

class Renderer;

template <typename T>
class StateMachine;

class GraphicsSettings : public Scene
{
public:
    GraphicsSettings(StateMachine<Scene> &sm, Renderer *renderer);

    void onEnter() override;
    void onExit() override {}
    void handleInput() override;
    void update(float dt) override;
    void render(float alpha) override;

private:
    void processUIEvents();
    bool hasGraphicsChanges() const;
    void applyAndSaveGraphics();
    void discardGraphicsChanges();
    void showExitConfirm();

    StateMachine<Scene> &m_sm;
    Renderer *m_renderer;
    UIManager m_uiManager;
    bool m_showExitConfirm = false;
};
