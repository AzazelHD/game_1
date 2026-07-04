#pragma once

#include "engine/scene/Scene.h"
#include "engine/statemachine/StateMachine.h"
#include "engine/ui/Slider.h"

#include <vector>

class Renderer;
class Input;

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

    enum class Page
    {
        Main,
        Graphics,
        Audio,
    };

    struct Resolution
    {
        int width = 1280;
        int height = 720;
    };

    enum class WindowMode
    {
        Windowed,
        Borderless,
    };

    void handleMainInput(const Input &input);
    void handleGraphicsInput(const Input &input);
    void handleAudioInput(const Input &input);
    void handlePendingExitConfirmInput(const Input &input);
    void applyGraphicsSettings();
    bool hasPendingChanges() const;
    void loadPersistedSettings();
    void savePersistedSettings() const;

    Page m_page = Page::Main;
    int m_focusIndex = 0;

    std::vector<Resolution> m_resolutions;
    int m_resolutionIndex = 0;
    int m_appliedResolutionIndex = 0;
    WindowMode m_windowMode = WindowMode::Windowed;
    WindowMode m_appliedWindowMode = WindowMode::Windowed;

    bool m_editingSlider = false;
    bool m_showExitConfirm = false;
    bool m_confirmApplySelected = true;

    float m_appliedMasterVolume = 1.0f;
    float m_appliedMusicVolume = 1.0f;
};