#pragma once

#include "engine/scene/Scene.h"
#include "engine/statemachine/StateMachine.h"
#include "engine/ui/Button.h"
#include "engine/ui/Slider.h"

class Renderer;

class SettingsState final : public Scene
{
public:
    SettingsState(StateMachine<Scene> &sm, Renderer *renderer);

    void onEnter() override;
    void onExit() override;
    void handleInput() override;
    void update(float dt) override;
    void render(float alpha) override;
    void applySettings();
    void resetDefaults();

private:
    StateMachine<Scene> &m_sm;
    Renderer *m_renderer = nullptr;
    Slider m_volumeSlider;
    Slider m_musicSlider;
    Button m_backButton{Rectf{}, "Back"};
    int m_focusIndex = 0; // 0=volume 1=music 2=back
};