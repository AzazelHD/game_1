#pragma once

#include "engine/scene/Scene.h"
#include "engine/ui/Slider.h"
#include "data/SettingsManager.h"
#include "ui/UIManager.h"

class Renderer;

template <typename T>
class StateMachine;

class AudioSettings : public Scene
{
public:
    AudioSettings(StateMachine<Scene> &sm, Renderer *renderer);

    void onEnter() override;
    void onExit() override {}
    void handleInput() override;
    void update(float dt) override;
    void render(float alpha) override;

private:
    void processUIEvents();
    bool hasVolumeChanges() const;
    void applyAndSaveVolumes();
    void discardVolumeChanges();
    void applyVolume();
    void showExitConfirm();

    StateMachine<Scene> &m_sm;
    Renderer *m_renderer;
    UIManager m_uiManager;

    Slider m_volumeSlider;
    Slider m_musicSlider;

    bool m_showExitConfirm = false;
};
