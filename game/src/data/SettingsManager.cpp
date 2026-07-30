#include "engine/core/App.h"
#include "engine/core/Log.h"
#include "engine/core/Window.h"
#include "data/SettingsManager.h"
#include "ui/UIScale.h"

#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

void SettingsManager::detectHighResolutionOptions()
{
    DisplayResolution native = Window::GetPrimaryDesktopResolution();
    if (native.width <= 0 || native.height <= 0)
        return;

    // Helper to add a resolution if not already in the list.
    auto addIfMissing = [this](int w, int h)
    {
        for (auto &res : m_settings.resolutions)
            if (res.width == w && res.height == h)
                return;
        m_settings.resolutions.push_back({w, h});
    };

    // Add 2560x1440 if the monitor is at least 1440p-class.
    if (native.width >= 2560 && native.height >= 1440)
        addIfMissing(2560, 1440);

    // Add 3840x2160 if the monitor is at least 4K.
    if (native.width >= 3840 && native.height >= 2160)
        addIfMissing(3840, 2160);
}

SettingsManager &SettingsManager::instance()
{
    static SettingsManager s;
    if (!s.m_loaded)
    {
        s.loadFromFile();
        s.m_loaded = true;
    }
    return s;
}

void SettingsManager::loadFromFile()
{
    detectHighResolutionOptions();

    using json = nlohmann::json;
    std::ifstream file("settings.json");
    if (!file.is_open())
        return;

    try
    {
        json j = json::parse(file);
        m_settings.resolutionIndex = std::clamp(
            j.value("resolutionIndex", 0), 0,
            static_cast<int>(m_settings.resolutions.size()) - 1);
        m_settings.windowMode = j.value("borderless", false)
                                    ? WindowMode::Borderless
                                    : WindowMode::Windowed;
        m_settings.masterVolume = std::clamp(
            j.value("masterVolume", 1.0f), 0.0f, 1.0f);
        m_settings.musicVolume = std::clamp(
            j.value("musicVolume", 1.0f), 0.0f, 1.0f);
    }
    catch (...)
    {
    }

    m_settings.appliedResolutionIndex = m_settings.resolutionIndex;
    m_settings.appliedWindowMode = m_settings.windowMode;
    m_settings.appliedMasterVolume = m_settings.masterVolume;
    m_settings.appliedMusicVolume = m_settings.musicVolume;
}

void SettingsManager::saveToFile() const
{
    using json = nlohmann::json;
    json j;
    j["resolutionIndex"] = m_settings.appliedResolutionIndex;
    j["borderless"] = (m_settings.appliedWindowMode == WindowMode::Borderless);
    j["masterVolume"] = m_settings.appliedMasterVolume;
    j["musicVolume"] = m_settings.appliedMusicVolume;

    std::ofstream file("settings.json", std::ios::trunc);
    if (file.is_open())
        file << j.dump(2);
}

bool SettingsManager::hasGraphicsChanges() const
{
    return m_settings.resolutionIndex != m_settings.appliedResolutionIndex ||
           m_settings.windowMode != m_settings.appliedWindowMode;
}

void SettingsManager::applyGraphics()
{
    Window *window = App::getWindow();
    if (!window)
    {
        LOG_ERROR("Settings", "applyGraphics: no window!");
        return;
    }

    LOG_INFO("Settings", "applyGraphics: mode=%s resIndex=%d",
             m_settings.windowMode == WindowMode::Borderless ? "Borderless" : "Windowed",
             m_settings.resolutionIndex);

    if (m_settings.windowMode == WindowMode::Borderless)
    {
        LOG_INFO("Settings", "Calling setBorderlessWindowed(true)");
        window->setBorderlessWindowed(true);
        window->getRenderer().setPresentationMode(Renderer::PresentationMode::Stretch);
    }
    else
    {
        LOG_INFO("Settings", "Calling setBorderlessWindowed(false), setSize");
        window->setBorderlessWindowed(false);
        const Resolution &res = m_settings.resolutions[m_settings.resolutionIndex];
        window->setSize(res.width, res.height);
        window->getRenderer().setPresentationMode(Renderer::PresentationMode::Letterbox);
    }
    UIScale::refresh();
}
