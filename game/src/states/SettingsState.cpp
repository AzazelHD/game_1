#include "config/GameConstants.h"
#include "states/SettingsState.h"
#include "engine/core/App.h"
#include "engine/core/Window.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Renderer.h"
#include "engine/ui/Insets.h"
#include "engine/ui/VerticalLayout.h"
#include "engine/ui/HorizontalLayout.h"
#include "ui/UITheme.h"
#include "ui/UIScale.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

SettingsState::SettingsState(StateMachine<Scene> &sm, Renderer *renderer)
    : m_sm(sm), m_renderer(renderer)
{
}

namespace
{
    constexpr float kPanelX = 330.0f;
    constexpr float kPanelY = 230.0f;
    constexpr float kPanelW = 620.0f;
    constexpr float kRowH = 54.0f;
    constexpr float kTrackW = 280.0f;
    constexpr float kTrackH = 22.0f;
    constexpr const char *kSettingsFile = "settings.json";

    float g_sessionMasterVolume = 1.0f;
    float g_sessionMusicVolume = 1.0f;

    const char *windowModeLabel(bool borderless)
    {
        return borderless ? "Borderless" : "Windowed";
    }

    void drawRow(Renderer *renderer, Rectf rect, bool focused)
    {
        renderer->setBlendMode(Renderer::BlendMode::Blend);
        if (focused)
        {
            renderer->setDrawColor(Color{166, 78, 48, 228});
            renderer->fillRect(rect);
            renderer->setDrawColor(Color{255, 196, 118, 255});
            renderer->drawRect(rect);
        }
        else
        {
            renderer->setDrawColor(Color{20, 28, 44, 190});
            renderer->fillRect(rect);
            renderer->setDrawColor(Color{58, 74, 108, 255});
            renderer->drawRect(rect);
        }
    }

    constexpr float kRowBoxH = kRowH - 6.0f; // visual height of each row's highlight box
    constexpr float kRowGap = 6.0f;          // default margin-bottom between stacked rows

    // Single source of truth for the 3 uniformly-spaced rows shared by the
    // Main and Audio pages (each row: fixed height, 6px margin-bottom gap
    // to the next one — nothing special beyond that).
    std::vector<Rectf> computeUniformRows(float ui)
    {
        const std::vector<VerticalLayout::Item> rows = {
            {kPanelW * ui, kRowBoxH * ui, Insets{0.0f, 0.0f, kRowGap * ui, 0.0f}},
            {kPanelW * ui, kRowBoxH * ui, Insets{0.0f, 0.0f, kRowGap * ui, 0.0f}},
            {kPanelW * ui, kRowBoxH * ui, Insets{}},
        };
        return VerticalLayout::stack(rows, Vec2f{kPanelX * ui, kPanelY * ui});
    }

    // Shared slider + "NN%" label layout for one audio row. The slider
    // keeps its own fixed size (kTrackW/kTrackH); the percentage text is
    // placed with a margin-left right after it, filling whatever space
    // remains up to the panel's right edge.
    struct AudioRowLayout
    {
        Rectf sliderRect;
        Rectf pctRect;
    };

    AudioRowLayout computeAudioRowLayout(float rowY, float rowH, float ui)
    {
        constexpr float kSliderMarginTop = 16.0f; // slider sits a bit below the row's label baseline
        constexpr float kPctMarginLeft = 14.0f;   // gap between slider and its "%" text

        const float trackX = (kPanelX + 290.0f) * ui;
        const float panelRight = (kPanelX + kPanelW) * ui;
        const float pctWidth = std::max(0.0f, panelRight - (trackX + kTrackW * ui) - kPctMarginLeft * ui);

        const std::vector<HorizontalLayout::Item> items = {
            {kTrackW * ui, kTrackH * ui, Insets{kSliderMarginTop * ui, 0.0f, 0.0f, 0.0f}},
            {pctWidth, rowH, Insets{0.0f, 0.0f, 0.0f, kPctMarginLeft * ui}},
        };

        const std::vector<Rectf> rects = HorizontalLayout::stack(items, Vec2f{trackX, rowY});
        return AudioRowLayout{rects[0], rects[1]};
    }
}

void SettingsState::onEnter()
{
    m_page = Page::Main;
    m_focusIndex = 0;
    m_editingSlider = false;
    m_showExitConfirm = false;
    m_confirmApplySelected = true;

    m_resolutions = {
        Resolution{1280, 720},
        Resolution{1366, 768},
        Resolution{1600, 900},
        Resolution{1920, 1080},
    };

    loadPersistedSettings();

    Slider::RenderStyle style;
    style.trackHeight = 8.0f;
    style.handleWidth = 12.0f;
    style.handleHeight = 18.0f;
    style.offsetY = 0.0f;

    const std::vector<Rectf> audioRows = computeUniformRows(1.0f);
    const AudioRowLayout volumeLayout = computeAudioRowLayout(audioRows[0].y, audioRows[0].h, 1.0f);
    const AudioRowLayout musicLayout = computeAudioRowLayout(audioRows[1].y, audioRows[1].h, 1.0f);

    m_volumeSlider.setTrackRect(volumeLayout.sliderRect);
    m_volumeSlider.setRange(0.0f, 1.0f);
    m_volumeSlider.setValue(g_sessionMasterVolume);
    m_volumeSlider.setRenderStyle(style);

    m_musicSlider.setTrackRect(musicLayout.sliderRect);
    m_musicSlider.setRange(0.0f, 1.0f);
    m_musicSlider.setValue(g_sessionMusicVolume);
    m_musicSlider.setRenderStyle(style);
}

void SettingsState::onExit()
{
    applySettings();
}

void SettingsState::loadPersistedSettings()
{
    using json = nlohmann::json;

    m_resolutionIndex = 0;
    m_windowMode = WindowMode::Windowed;
    g_sessionMasterVolume = 1.0f;
    g_sessionMusicVolume = 1.0f;

    std::ifstream file(kSettingsFile);
    if (file.is_open())
    {
        try
        {
            const json j = json::parse(file);
            const int savedRes = j.value("resolutionIndex", 0);
            m_resolutionIndex = std::clamp(savedRes, 0, static_cast<int>(m_resolutions.size()) - 1);
            m_windowMode = j.value("borderless", false) ? WindowMode::Borderless : WindowMode::Windowed;
            g_sessionMasterVolume = std::clamp(j.value("masterVolume", 1.0f), 0.0f, 1.0f);
            g_sessionMusicVolume = std::clamp(j.value("musicVolume", 1.0f), 0.0f, 1.0f);
        }
        catch (...)
        {
        }
    }

    m_appliedResolutionIndex = m_resolutionIndex;
    m_appliedWindowMode = m_windowMode;
    m_appliedMasterVolume = g_sessionMasterVolume;
    m_appliedMusicVolume = g_sessionMusicVolume;
}

void SettingsState::savePersistedSettings() const
{
    using json = nlohmann::json;
    json j;
    j["resolutionIndex"] = m_appliedResolutionIndex;
    j["borderless"] = (m_appliedWindowMode == WindowMode::Borderless);
    j["masterVolume"] = m_appliedMasterVolume;
    j["musicVolume"] = m_appliedMusicVolume;

    std::ofstream file(kSettingsFile, std::ios::trunc);
    if (!file.is_open())
        return;
    file << j.dump(2);
}

bool SettingsState::hasPendingChanges() const
{
    return m_resolutionIndex != m_appliedResolutionIndex ||
           m_windowMode != m_appliedWindowMode ||
           std::abs(m_volumeSlider.getValue() - m_appliedMasterVolume) > 0.0001f ||
           std::abs(m_musicSlider.getValue() - m_appliedMusicVolume) > 0.0001f;
}

void SettingsState::handleMainInput(const Input &input)
{
    if (input.isKeyPressed(KeyCode::Up, true) || input.isKeyPressed(KeyCode::W, true))
        m_focusIndex = std::max(0, m_focusIndex - 1);
    if (input.isKeyPressed(KeyCode::Down, true) || input.isKeyPressed(KeyCode::S, true))
        m_focusIndex = std::min(2, m_focusIndex + 1);

    if (input.isKeyPressed(KeyCode::Accept, false))
    {
        if (m_focusIndex == 0)
        {
            m_page = Page::Graphics;
            m_focusIndex = 0;
            return;
        }
        if (m_focusIndex == 1)
        {
            m_page = Page::Audio;
            m_focusIndex = 0;
            m_editingSlider = false;
            return;
        }
        m_sm.pop();
        return;
    }

    if (input.isKeyPressed(KeyCode::Back, false))
        m_sm.pop();
}

void SettingsState::handleGraphicsInput(const Input &input)
{
    if (input.isKeyPressed(KeyCode::Up, true) || input.isKeyPressed(KeyCode::W, true))
        m_focusIndex = std::max(0, m_focusIndex - 1);
    if (input.isKeyPressed(KeyCode::Down, true) || input.isKeyPressed(KeyCode::S, true))
        m_focusIndex = std::min(2, m_focusIndex + 1);

    if (m_focusIndex == 0)
    {
        if (input.isKeyPressed(KeyCode::Left, true) || input.isKeyPressed(KeyCode::A, true))
            m_resolutionIndex = (m_resolutionIndex - 1 + static_cast<int>(m_resolutions.size())) % static_cast<int>(m_resolutions.size());
        if (input.isKeyPressed(KeyCode::Right, true) || input.isKeyPressed(KeyCode::D, true))
            m_resolutionIndex = (m_resolutionIndex + 1) % static_cast<int>(m_resolutions.size());
    }
    else if (m_focusIndex == 1)
    {
        if (input.isKeyPressed(KeyCode::Left, true) ||
            input.isKeyPressed(KeyCode::A, true) ||
            input.isKeyPressed(KeyCode::Right, true) ||
            input.isKeyPressed(KeyCode::D, true))
        {
            m_windowMode = (m_windowMode == WindowMode::Windowed) ? WindowMode::Borderless : WindowMode::Windowed;
        }
    }

    if (input.isKeyPressed(KeyCode::Accept, false))
    {
        if (m_focusIndex == 2)
        {
            if (hasPendingChanges())
            {
                applyGraphicsSettings();
                applySettings();
                m_appliedResolutionIndex = m_resolutionIndex;
                m_appliedWindowMode = m_windowMode;
                m_appliedMasterVolume = m_volumeSlider.getValue();
                m_appliedMusicVolume = m_musicSlider.getValue();
                savePersistedSettings();
            }

            m_page = Page::Main;
            m_focusIndex = 0;
            return;
        }
    }

    if (input.isKeyPressed(KeyCode::Back, false))
    {
        if (hasPendingChanges())
        {
            m_showExitConfirm = true;
            m_confirmApplySelected = true;
        }
        else
        {
            m_page = Page::Main;
            m_focusIndex = 0;
        }
    }
}

void SettingsState::handleAudioInput(const Input &input)
{
    if (m_editingSlider)
    {
        Slider *activeSlider = (m_focusIndex == 0) ? &m_volumeSlider : &m_musicSlider;

        if (input.isKeyPressed(KeyCode::Left, true) || input.isKeyPressed(KeyCode::A, true))
            activeSlider->step(-0.05f);
        if (input.isKeyPressed(KeyCode::Right, true) || input.isKeyPressed(KeyCode::D, true))
            activeSlider->step(0.05f);

        if (input.isKeyPressed(KeyCode::Accept, false) || input.isKeyPressed(KeyCode::Back, false))
        {
            m_editingSlider = false;
            applySettings();
        }
        return;
    }

    if (input.isKeyPressed(KeyCode::Up, true) || input.isKeyPressed(KeyCode::W, true))
        m_focusIndex = std::max(0, m_focusIndex - 1);
    if (input.isKeyPressed(KeyCode::Down, true) || input.isKeyPressed(KeyCode::S, true))
        m_focusIndex = std::min(2, m_focusIndex + 1);

    if (m_focusIndex <= 1)
    {
        Slider *activeSlider = (m_focusIndex == 0) ? &m_volumeSlider : &m_musicSlider;
        if (input.isKeyPressed(KeyCode::Left, true) || input.isKeyPressed(KeyCode::A, true))
            activeSlider->step(-0.03f);
        if (input.isKeyPressed(KeyCode::Right, true) || input.isKeyPressed(KeyCode::D, true))
            activeSlider->step(0.03f);
    }

    if (input.isKeyPressed(KeyCode::Accept, false))
    {
        if (m_focusIndex <= 1)
        {
            m_editingSlider = true;
            return;
        }

        m_page = Page::Main;
        m_focusIndex = 1;
        return;
    }

    if (input.isKeyPressed(KeyCode::Back, false))
    {
        m_page = Page::Main;
        m_focusIndex = 1;
    }
}

void SettingsState::handlePendingExitConfirmInput(const Input &input)
{
    if (input.isKeyPressed(KeyCode::Left, false) || input.isKeyPressed(KeyCode::A, false) ||
        input.isKeyPressed(KeyCode::Right, false) || input.isKeyPressed(KeyCode::D, false))
    {
        m_confirmApplySelected = !m_confirmApplySelected;
    }

    if (input.isKeyPressed(KeyCode::Back, false))
    {
        m_showExitConfirm = false;
        return;
    }

    if (!input.isKeyPressed(KeyCode::Accept, false))
        return;

    if (m_confirmApplySelected)
    {
        applyGraphicsSettings();
        applySettings();
        m_appliedResolutionIndex = m_resolutionIndex;
        m_appliedWindowMode = m_windowMode;
        m_appliedMasterVolume = m_volumeSlider.getValue();
        m_appliedMusicVolume = m_musicSlider.getValue();
        savePersistedSettings();
    }
    else
    {
        m_resolutionIndex = m_appliedResolutionIndex;
        m_windowMode = m_appliedWindowMode;
        m_volumeSlider.setValue(m_appliedMasterVolume);
        m_musicSlider.setValue(m_appliedMusicVolume);
    }

    m_showExitConfirm = false;
    m_page = Page::Main;
    m_focusIndex = 0;
}

void SettingsState::handleInput()
{
    const Input &input = Input::instance();

    if (m_showExitConfirm)
    {
        handlePendingExitConfirmInput(input);
        return;
    }

    switch (m_page)
    {
    case Page::Main:
        handleMainInput(input);
        break;
    case Page::Graphics:
        handleGraphicsInput(input);
        break;
    case Page::Audio:
        handleAudioInput(input);
        break;
    }
}

void SettingsState::update(float /*dt*/)
{
    UIScale::refresh();
    const float ui = UIScale::factor();

    const std::vector<Rectf> audioRows = computeUniformRows(ui);
    const AudioRowLayout volumeLayout = computeAudioRowLayout(audioRows[0].y, audioRows[0].h, ui);
    const AudioRowLayout musicLayout = computeAudioRowLayout(audioRows[1].y, audioRows[1].h, ui);

    m_volumeSlider.setTrackRect(volumeLayout.sliderRect);
    m_musicSlider.setTrackRect(musicLayout.sliderRect);
}

void SettingsState::render(float /*alpha*/)
{
    if (!m_renderer)
        return;

    UIScale::refresh();
    const float ui = UIScale::factor();

    m_renderer->setBlendMode(Renderer::BlendMode::Blend);
    m_renderer->setDrawColor(Color{6, 10, 20, 220});
    m_renderer->fillRect(Rectf{0.0f, 0.0f, GameConstants::VIEW_W, GameConstants::VIEW_H});

    const Font *font = App::getDefaultFont();
    if (!font)
        return;

    const Color titleColor = Color{240, 245, 255, 255};
    const Color baseColor = Color{214, 220, 238, 255};
    const Color selectedColor = Color{255, 244, 182, 255};

    const char *pageTitle = "Settings";
    if (m_page == Page::Graphics)
        pageTitle = "Settings / Graphics";
    else if (m_page == Page::Audio)
        pageTitle = "Settings / Audio";

    m_renderer->renderTextInRect(font,
                                 pageTitle,
                                 Rectf{0.0f, 180.0f * ui, GameConstants::VIEW_W, 28.0f * ui},
                                 titleColor,
                                 Renderer::HorizontalAlign::Center,
                                 Renderer::VerticalAlign::Middle,
                                 false,
                                 false,
                                 false);

    if (m_page == Page::Main)
    {
        const char *labels[3] = {"Graphics", "Audio", "Back"};
        for (int i = 0; i < 3; ++i)
        {
            const Rectf row{kPanelX * ui, (kPanelY + static_cast<float>(i) * kRowH) * ui, kPanelW * ui, (kRowH - 6.0f) * ui};
            drawRow(m_renderer, row, m_focusIndex == i);
            std::string line = (m_focusIndex == i) ? std::string("> ") + labels[i] + " <" : labels[i];
            m_renderer->renderTextInRect(font,
                                         line,
                                         row,
                                         m_focusIndex == i ? selectedColor : baseColor,
                                         Renderer::HorizontalAlign::Center,
                                         Renderer::VerticalAlign::Middle,
                                         false,
                                         false,
                                         false);
        }
    }
    else if (m_page == Page::Graphics)
    {
        const Resolution current = m_resolutions[static_cast<std::size_t>(m_resolutionIndex)];
        char resBuf[64];
        std::snprintf(resBuf, sizeof(resBuf), "%d x %d", current.width, current.height);

        const std::string actionLabel = hasPendingChanges() ? "Apply Changes and Save" : "Back / Return to main menu";

        constexpr float kActionRowGap = 24.0f; // extra breathing room above the standalone action button

        const char *labels[2] = {"Resolution", "Window Mode"};
        std::string values[2] = {
            std::string("< ") + resBuf + " >",
            std::string("< ") + windowModeLabel(m_windowMode == WindowMode::Borderless) + " >",
        };

        for (int i = 0; i < 2; ++i)
        {
            const Rectf row{kPanelX * ui, (kPanelY + static_cast<float>(i) * kRowH) * ui, kPanelW * ui, (kRowH - 6.0f) * ui};
            drawRow(m_renderer, row, m_focusIndex == i);

            m_renderer->renderTextInRect(font,
                                         labels[i],
                                         Rectf{row.x + 16.0f * ui, row.y, row.w * 0.40f, row.h},
                                         m_focusIndex == i ? selectedColor : baseColor,
                                         Renderer::HorizontalAlign::Left,
                                         Renderer::VerticalAlign::Middle,
                                         false,
                                         false,
                                         false);

            m_renderer->renderTextInRect(font,
                                         values[i],
                                         Rectf{row.x + row.w * 0.40f, row.y, row.w * 0.60f - 16.0f * ui, row.h},
                                         m_focusIndex == i ? selectedColor : baseColor,
                                         Renderer::HorizontalAlign::Right,
                                         Renderer::VerticalAlign::Middle,
                                         false,
                                         false,
                                         false);
        }

        // Standalone action button: extra gap above it, full-width centered label.
        const Rectf actionRow{kPanelX * ui,
                              (kPanelY + 2.0f * kRowH + kActionRowGap) * ui,
                              kPanelW * ui,
                              (kRowH - 6.0f) * ui};
        drawRow(m_renderer, actionRow, m_focusIndex == 2);
        m_renderer->renderTextInRect(font,
                                     actionLabel,
                                     actionRow,
                                     m_focusIndex == 2 ? selectedColor : baseColor,
                                     Renderer::HorizontalAlign::Center,
                                     Renderer::VerticalAlign::Middle,
                                     false,
                                     false,
                                     false);
    }
    else
    {
        const std::vector<Rectf> audioRows = computeUniformRows(ui);
        const Rectf volumeRow = audioRows[0];
        const Rectf musicRow = audioRows[1];
        const Rectf backRow = audioRows[2];

        drawRow(m_renderer, volumeRow, m_focusIndex == 0);
        drawRow(m_renderer, musicRow, m_focusIndex == 1);
        drawRow(m_renderer, backRow, m_focusIndex == 2);

        m_renderer->renderTextInRect(font,
                                     "Master Volume",
                                     Rectf{volumeRow.x + 16.0f * ui, volumeRow.y, 210.0f * ui, volumeRow.h},
                                     m_focusIndex == 0 ? selectedColor : baseColor,
                                     Renderer::HorizontalAlign::Left,
                                     Renderer::VerticalAlign::Middle,
                                     false,
                                     false,
                                     false);

        m_renderer->renderTextInRect(font,
                                     "Music Volume",
                                     Rectf{musicRow.x + 16.0f * ui, musicRow.y, 210.0f * ui, musicRow.h},
                                     m_focusIndex == 1 ? selectedColor : baseColor,
                                     Renderer::HorizontalAlign::Left,
                                     Renderer::VerticalAlign::Middle,
                                     false,
                                     false,
                                     false);

        m_volumeSlider.render(m_renderer);
        m_musicSlider.render(m_renderer);

        char volumePct[16];
        char musicPct[16];
        std::snprintf(volumePct, sizeof(volumePct), "%d%%", static_cast<int>(m_volumeSlider.getValue() * 100.0f + 0.5f));
        std::snprintf(musicPct, sizeof(musicPct), "%d%%", static_cast<int>(m_musicSlider.getValue() * 100.0f + 0.5f));

        // Percentage sits right after the slider (margin-left), like a CSS
        // box in flow — never overlapping the slider itself, regardless of
        // slider width/position, since both come from the same layout call.
        const AudioRowLayout volumeLayout = computeAudioRowLayout(volumeRow.y, volumeRow.h, ui);
        const AudioRowLayout musicLayout = computeAudioRowLayout(musicRow.y, musicRow.h, ui);

        m_renderer->renderTextInRect(font,
                                     volumePct,
                                     volumeLayout.pctRect,
                                     m_focusIndex == 0 ? selectedColor : baseColor,
                                     Renderer::HorizontalAlign::Right,
                                     Renderer::VerticalAlign::Middle,
                                     false,
                                     false,
                                     false);
        m_renderer->renderTextInRect(font,
                                     musicPct,
                                     musicLayout.pctRect,
                                     m_focusIndex == 1 ? selectedColor : baseColor,
                                     Renderer::HorizontalAlign::Right,
                                     Renderer::VerticalAlign::Middle,
                                     false,
                                     false,
                                     false);

        std::string backLine = m_focusIndex == 2 ? "> Back <" : "Back";
        m_renderer->renderTextInRect(font,
                                     backLine,
                                     backRow,
                                     m_focusIndex == 2 ? selectedColor : baseColor,
                                     Renderer::HorizontalAlign::Center,
                                     Renderer::VerticalAlign::Middle,
                                     false,
                                     false,
                                     false);
    }

    if (m_showExitConfirm)
    {
        const float w = 420.0f * ui;
        const float h = 140.0f * ui;
        const float x = (GameConstants::VIEW_W - w) * 0.5f;
        const float y = (GameConstants::VIEW_H - h) * 0.5f;

        m_renderer->setDrawColor(UITheme::PopupBG);
        m_renderer->fillRect(Rectf{x, y, w, h});
        m_renderer->setDrawColor(UITheme::PopupBorder);
        m_renderer->drawRect(Rectf{x, y, w, h});

        m_renderer->renderTextInRect(font,
                                     "You have unapplied graphics changes",
                                     Rectf{x, y + 16.0f * ui, w, 24.0f * ui},
                                     UITheme::Text,
                                     Renderer::HorizontalAlign::Center,
                                     Renderer::VerticalAlign::Middle,
                                     false,
                                     false,
                                     false);

        m_renderer->renderTextInRect(font,
                                     m_confirmApplySelected ? "> Apply and Save <" : "Apply and Save",
                                     Rectf{x + 18.0f * ui, y + 70.0f * ui, w * 0.5f - 24.0f * ui, 24.0f * ui},
                                     m_confirmApplySelected ? UITheme::SelectedText : UITheme::Text,
                                     Renderer::HorizontalAlign::Center,
                                     Renderer::VerticalAlign::Middle,
                                     false,
                                     false,
                                     false);
        m_renderer->renderTextInRect(font,
                                     m_confirmApplySelected ? "Discard Changes" : "> Discard Changes <",
                                     Rectf{x + w * 0.5f + 6.0f * ui, y + 70.0f * ui, w * 0.5f - 24.0f * ui, 24.0f * ui},
                                     m_confirmApplySelected ? UITheme::Text : UITheme::SelectedText,
                                     Renderer::HorizontalAlign::Center,
                                     Renderer::VerticalAlign::Middle,
                                     false,
                                     false,
                                     false);
    }
}

void SettingsState::applyGraphicsSettings()
{
    Window *window = App::getWindow();
    if (!window)
        return;

    if (m_windowMode == WindowMode::Borderless)
    {
        window->setBorderlessWindowed(true);
    }
    else
    {
        window->setBorderlessWindowed(false);
        const Resolution &res = m_resolutions[static_cast<std::size_t>(m_resolutionIndex)];
        window->setSize(res.width, res.height);
    }

    UIScale::refresh();
}

void SettingsState::applySettings()
{
    g_sessionMasterVolume = m_volumeSlider.getValue();
    g_sessionMusicVolume = m_musicSlider.getValue();

    // TODO: push values to an AudioManager or similar
    // AudioManager::instance().setMasterVolume(m_volumeSlider.getValue());
    // AudioManager::instance().setMusicVolume(m_musicSlider.getValue());
}

void SettingsState::resetDefaults()
{
    m_resolutionIndex = 0;
    m_windowMode = WindowMode::Windowed;
    m_volumeSlider.setValue(1.0f);
    m_musicSlider.setValue(1.0f);
    m_page = Page::Main;
    m_focusIndex = 0;
}