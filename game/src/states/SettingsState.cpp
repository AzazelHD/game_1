#include "config/GameConstants.h"
#include "states/SettingsState.h"
#include "engine/math/Rect.h"
#include "engine/input/Input.h"
#include "engine/renderer/Renderer.h"
#include "engine/renderer/Color.h"

#include <cstdio>

SettingsState::SettingsState(StateMachine<Scene> &sm, Renderer *renderer)
    : m_sm(sm), m_renderer(renderer)
{
}

namespace
{
    constexpr float TRACK_X = 490.0f;
    constexpr float TRACK_W = 300.0f;
    constexpr float TRACK_H = 12.0f;

    constexpr float VOLUME_Y = 280.0f;
    constexpr float MUSIC_Y = 340.0f;
    constexpr float BACK_Y = 440.0f;

    constexpr float BACK_W = 160.0f;
    constexpr float BACK_H = 48.0f;

    // Session-persistent values while the game process is alive.
    float g_sessionMasterVolume = 1.0f;
    float g_sessionMusicVolume = 1.0f;

    void drawFocusRow(Renderer *renderer, float y, bool focused)
    {
        const Rectf rowRect = {380.0f, y - 16.0f, 520.0f, 48.0f};
        if (focused)
        {
            renderer->setDrawColor(Color{182, 82, 46, 220});
            renderer->fillRect(rowRect);
            renderer->setDrawColor(Color{255, 191, 120, 255});
            renderer->drawRect(rowRect);
        }
        else
        {
            renderer->setDrawColor(Color{20, 28, 44, 180});
            renderer->fillRect(rowRect);
            renderer->setDrawColor(Color{56, 72, 106, 255});
            renderer->drawRect(rowRect);
        }
    }
}

void SettingsState::onEnter()
{
    m_focusIndex = 0;

    m_volumeSlider.setTrackRect({TRACK_X, VOLUME_Y, TRACK_W, TRACK_H});
    m_volumeSlider.setRange(0.0f, 1.0f);
    m_volumeSlider.setValue(g_sessionMasterVolume);

    m_musicSlider.setTrackRect({TRACK_X, MUSIC_Y, TRACK_W, TRACK_H});
    m_musicSlider.setRange(0.0f, 1.0f);
    m_musicSlider.setValue(g_sessionMusicVolume);

    const float backX = (GameConstants::VIEW_W - BACK_W) / 2.0f;
    m_backButton.setPosition({backX, BACK_Y});
    m_backButton.setEnabled(true);
    m_backButton.setSelected(false);
}

void SettingsState::onExit()
{
    applySettings();
}

void SettingsState::handleInput()
{
    Input &input = Input::instance();

    // 1. Navigation – repeat enabled for smooth focus movement
    if (input.isKeyPressed(KeyCode::Up, true))
        m_focusIndex = (m_focusIndex > 0) ? m_focusIndex - 1 : 0;
    if (input.isKeyPressed(KeyCode::Down, true))
        m_focusIndex = (m_focusIndex < 2) ? m_focusIndex + 1 : 2;

    // 2. Sliders – repeat enabled for continuous adjustment
    if (m_focusIndex == 0)
    {
        if (input.isKeyPressed(KeyCode::Left, true))
            m_volumeSlider.step(-0.05f);
        if (input.isKeyPressed(KeyCode::Right, true))
            m_volumeSlider.step(0.05f);
    }
    if (m_focusIndex == 1)
    {
        if (input.isKeyPressed(KeyCode::Left, true))
            m_musicSlider.step(-0.05f);
        if (input.isKeyPressed(KeyCode::Right, true))
            m_musicSlider.step(0.05f);
    }

    // 3. Exit Triggers – no repeat for confirm/cancel
    const bool confirm = input.isKeyPressed(KeyCode::Accept, false);
    const bool cancel = input.isKeyPressed(KeyCode::Back, false);

    if (cancel || (confirm && m_focusIndex == 2))
    {
        m_sm.pop();
    }
}

void SettingsState::update(float /*dt*/)
{
}

void SettingsState::render(float /*alpha*/)
{
    if (!m_renderer)
    {
        return;
    }

    m_renderer->setBlendMode(Renderer::BlendMode::Blend);
    m_renderer->setDrawColor(Color{6, 10, 20, 220});
    m_renderer->fillRect(Rectf{0.0f, 0.0f, GameConstants::VIEW_W, GameConstants::VIEW_H});

    m_renderer->setDrawColor(Color{220, 230, 255, 255});
    m_renderer->drawDebugText({472.0f, 200.0f}, "SETTINGS");
    m_renderer->drawDebugText({368.0f, 236.0f}, "UP/DOWN: Select  LEFT/RIGHT: Change  ENTER/BACK: Close");

    drawFocusRow(m_renderer, VOLUME_Y, m_focusIndex == 0);
    drawFocusRow(m_renderer, MUSIC_Y, m_focusIndex == 1);

    char volumeText[32];
    char musicText[32];
    std::snprintf(volumeText, sizeof(volumeText), "%d%%", static_cast<int>(m_volumeSlider.getValue() * 100.0f + 0.5f));
    std::snprintf(musicText, sizeof(musicText), "%d%%", static_cast<int>(m_musicSlider.getValue() * 100.0f + 0.5f));

    m_renderer->setDrawColor(Color{240, 245, 255, 255});
    m_renderer->drawDebugText({410.0f, VOLUME_Y - 2.0f}, "Master Volume");
    m_renderer->drawDebugText({410.0f, MUSIC_Y - 2.0f}, "Music Volume");
    m_renderer->drawDebugText({812.0f, VOLUME_Y - 2.0f}, volumeText);
    m_renderer->drawDebugText({812.0f, MUSIC_Y - 2.0f}, musicText);

    // TODO: Slider/Button still take SDL_Renderer* - bridge until migrated.
    m_volumeSlider.render(m_renderer);
    m_musicSlider.render(m_renderer);
    m_backButton.setSelected(m_focusIndex == 2);
    m_backButton.render(m_renderer);
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
    m_volumeSlider.setValue(1.0f);
    m_musicSlider.setValue(1.0f);
    m_focusIndex = 0;
}