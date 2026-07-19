
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/FontManager.h"
#include "engine/renderer/Renderer.h"
#include "engine/statemachine/StateMachine.h"
#include "engine/ui/SliderControl.h"
#include "engine/ui/ButtonControl.h"
#include "config/GameConstants.h"
#include "data/SettingsManager.h"
#include "states/AudioSettings.h"
#include "ui/ActionId.h"
#include "ui/UIScale.h"
#include "ui/UITheme.h"
#include "ui/WindowId.h"
#include "ui/UIUtils.h"
#include "ui/windows/ConfirmWindow.h"
#include "ui/windows/SettingsRowWindow.h"

#include <cmath>

AudioSettings::AudioSettings(StateMachine<Scene> &sm, Renderer *r)
    : m_sm(sm), m_renderer(r) {}

void AudioSettings::onEnter()
{
    m_showExitConfirm = false;
    m_uiManager.clear();

    // Initialise sliders from persisted settings
    Slider::RenderStyle style;
    style.trackHeight = 8.0f;
    style.handleWidth = 12.0f;
    style.handleHeight = 18.0f;
    style.offsetY = 0.0f;

    auto &s = SettingsManager::instance().data();

    m_volumeSlider.setRange(0.0f, 1.0f);
    m_volumeSlider.setValue(s.masterVolume);
    m_volumeSlider.setRenderStyle(style);

    m_musicSlider.setRange(0.0f, 1.0f);
    m_musicSlider.setValue(s.musicVolume);
    m_musicSlider.setRenderStyle(style);

    // Build the settings window rows
    auto *window = m_uiManager.push<SettingsRowWindow>(WindowId::SettingsAudio);
    window->setFont(FontManager::instance().get(FontRole::Heading));

    std::vector<SettingsRowWindow::RowItem> rows;

    SettingsRowWindow::RowItem masterRow;
    masterRow.label = "Master Volume";
    masterRow.control = std::make_unique<SliderControl>(&m_volumeSlider);
    rows.push_back(std::move(masterRow));

    SettingsRowWindow::RowItem musicRow;
    musicRow.label = "Music Volume";
    musicRow.control = std::make_unique<SliderControl>(&m_musicSlider);
    rows.push_back(std::move(musicRow));

    SettingsRowWindow::RowItem backRow;
    {
        auto btn = std::make_unique<ButtonControl>(
            [this]() -> std::string
            { return hasVolumeChanges() ? "Apply Changes" : "Back"; },
            []() {}); // Accept handled via the emitted event below, not this callback

        // Use the game's label formatter (defaults to "> label <" when selected)
        btn->setLabelFormatter([](const std::string &label, bool selected)
                               { return UIUtils::formatButtonLabel(label, selected); });

        backRow.control = std::move(btn);
    }
    rows.push_back(std::move(backRow));

    window->setRows(std::move(rows));
}

void AudioSettings::handleInput()
{
    if (m_showExitConfirm)
        return; // confirm window blocks input

    m_uiManager.handleInput(Input::instance());
    processUIEvents();
}

void AudioSettings::update(float dt)
{
    m_uiManager.update(dt);
}

void AudioSettings::render(float /*alpha*/)
{
    if (!m_renderer)
        return;
    m_renderer->clear(Color{12, 14, 20, 255});
    m_uiManager.render(m_renderer);
}

void AudioSettings::processUIEvents()
{
    auto events = m_uiManager.drainEvents();
    for (const auto &event : events)
    {
        if (event.windowId == WindowId::SettingsAudio)
        {
            if (event.type == UIEventType::ActionSelected)
            {
                if (event.actionId == ActionId::AdjustLeft ||
                    event.actionId == ActionId::AdjustRight)
                {
                    // Slider value changed: sync SettingsManager and apply to audio system
                    auto &s = SettingsManager::instance().data();
                    s.masterVolume = m_volumeSlider.getValue();
                    s.musicVolume = m_musicSlider.getValue();
                    applyVolume();
                }
                else if (event.actionId == ActionId::Confirm)
                {
                    // User pressed Enter on the button row
                    if (hasVolumeChanges())
                        applyAndSaveVolumes();
                    m_sm.pop();
                    return;
                }
            }
            else if (event.type == UIEventType::ActionCanceled)
            {
                // Back pressed
                if (hasVolumeChanges())
                    showExitConfirm();
                else
                    m_sm.pop();
                return;
            }
        }
        else if (event.windowId == WindowId::SettingsExitConfirm)
        {
            if (event.type == UIEventType::ConfirmResult)
            {
                m_uiManager.popById(WindowId::SettingsExitConfirm);
                m_showExitConfirm = false;

                if (event.confirmed)
                    applyAndSaveVolumes();
                else
                    discardVolumeChanges();

                m_sm.pop();
                return;
            }
            else if (event.type == UIEventType::ActionCanceled)
            {
                m_uiManager.popById(WindowId::SettingsExitConfirm);
                m_showExitConfirm = false;
                return;
            }
        }
    }
}

bool AudioSettings::hasVolumeChanges() const
{
    const auto &s = SettingsManager::instance().data();
    return std::abs(m_volumeSlider.getValue() - s.appliedMasterVolume) > 0.0001f ||
           std::abs(m_musicSlider.getValue() - s.appliedMusicVolume) > 0.0001f;
}

void AudioSettings::applyAndSaveVolumes()
{
    auto &s = SettingsManager::instance().data();
    s.appliedMasterVolume = m_volumeSlider.getValue();
    s.appliedMusicVolume = m_musicSlider.getValue();
    s.masterVolume = s.appliedMasterVolume;
    s.musicVolume = s.appliedMusicVolume;
    SettingsManager::instance().saveToFile();
    applyVolume();
}

void AudioSettings::discardVolumeChanges()
{
    auto &s = SettingsManager::instance().data();
    m_volumeSlider.setValue(s.appliedMasterVolume);
    m_musicSlider.setValue(s.appliedMusicVolume);
    s.masterVolume = s.appliedMasterVolume;
    s.musicVolume = s.appliedMusicVolume;
    applyVolume();
}

void AudioSettings::applyVolume()
{
    // TODO: AudioManager::instance().setMasterVolume(m_volumeSlider.getValue());
    // TODO: AudioManager::instance().setMusicVolume(m_musicSlider.getValue());
}

void AudioSettings::showExitConfirm()
{
    m_showExitConfirm = true;
    auto *confirm = m_uiManager.push<ConfirmWindow>(WindowId::SettingsExitConfirm);
    confirm->setFont(FontManager::instance().get(FontRole::Heading));
    confirm->setPrompt("You have unapplied volume changes");
}