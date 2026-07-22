#include "engine/core/App.h"
#include "engine/core/Window.h"
#include "data/SettingsManager.h"
#include "ui/UIScale.h"

#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

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
        return;

    if (m_settings.windowMode == WindowMode::Borderless)
    {
        window->setBorderlessWindowed(true);
    }
    else
    {
        window->setBorderlessWindowed(false);
        const Resolution &res = m_settings.resolutions[m_settings.resolutionIndex];
        window->setSize(res.width, res.height);
    }
    UIScale::refresh();
}
