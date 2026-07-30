#pragma once

#include <vector>

struct Resolution
{
    int width;
    int height;
};

enum class WindowMode
{
    Windowed,
    Borderless,
    // Fullscreen,

    Count
};

class SettingsManager
{
public:
    struct Settings
    {
        int resolutionIndex = 0;
        std::vector<Resolution> resolutions = {
            {1280, 720},
            {1366, 768},
            {1600, 900},
            {1920, 1080}};

        WindowMode windowMode = WindowMode::Windowed;

        float masterVolume = 1.0f;
        float musicVolume = 1.0f;

        // Last applied / saved values
        int appliedResolutionIndex = 0;
        WindowMode appliedWindowMode = WindowMode::Windowed;

        float appliedMasterVolume = 1.0f;
        float appliedMusicVolume = 1.0f;
    };

    // Adds 2560x1440 and/or 3840x2160 to the resolution list if the
    // primary monitor’s native resolution is at least that high.
    void detectHighResolutionOptions();

    static SettingsManager &instance();

    // Mutable access for settings pages
    Settings &data() { return m_settings; }
    const Settings &data() const { return m_settings; }

    void loadFromFile();
    void saveToFile() const;
    bool hasGraphicsChanges() const;
    void applyGraphics();

private:
    SettingsManager() = default;

    Settings m_settings;
    bool m_loaded = false;
};