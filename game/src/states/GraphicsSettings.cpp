#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/FontManager.h"
#include "engine/renderer/Renderer.h"
#include "engine/statemachine/StateMachine.h"
#include "engine/ui/ValueControl.h"
#include "engine/ui/ButtonControl.h"
#include "config/GameConstants.h"
#include "data/SettingsManager.h"
#include "states/GraphicsSettings.h"
#include "ui/ActionId.h"
#include "ui/UIScale.h"
#include "ui/UITheme.h"
#include "ui/WindowId.h"
#include "ui/UIUtils.h"
#include "ui/windows/ConfirmWindow.h"
#include "ui/windows/SettingsRowWindow.h"

#include <cstdio>
#include <string>

GraphicsSettings::GraphicsSettings(StateMachine<Scene> &sm, Renderer *r)
    : m_sm(sm), m_renderer(r) {}

void GraphicsSettings::onEnter()
{
    m_showExitConfirm = false;
    m_uiManager.clear();

    auto *window = m_uiManager.push<SettingsRowWindow>(WindowId::SettingsGraphics);
    window->setFont(FontManager::instance().get(FontRole::Heading));

    auto &s = SettingsManager::instance().data();

    std::vector<SettingsRowWindow::RowItem> rows;

    SettingsRowWindow::RowItem resolutionRow;
    resolutionRow.label = "Resolution";
    resolutionRow.control = std::make_unique<ValueControl>(
        [&s]() -> std::string
        {
            const Resolution &res = s.resolutions[s.resolutionIndex];
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%d x %d", res.width, res.height);
            return std::string("< ") + buf + " >";
        },
        [&s](bool right)
        {
            const int count = static_cast<int>(s.resolutions.size());
            if (right)
                s.resolutionIndex = (s.resolutionIndex + 1) % count;
            else
                s.resolutionIndex = (s.resolutionIndex - 1 + count) % count;
        });
    rows.push_back(std::move(resolutionRow));

    SettingsRowWindow::RowItem windowModeRow;
    windowModeRow.label = "Window Mode";
    windowModeRow.control = std::make_unique<ValueControl>(
        [&s]() -> std::string
        {
            return std::string("< ") +
                   (s.windowMode == WindowMode::Borderless ? "Borderless" : "Windowed") + " >";
        },
        [&s](bool)
        {
            s.windowMode = (s.windowMode == WindowMode::Windowed)
                               ? WindowMode::Borderless
                               : WindowMode::Windowed;
        });
    rows.push_back(std::move(windowModeRow));

    SettingsRowWindow::RowItem backRow;
    {
        auto btn = std::make_unique<ButtonControl>(
            [this]() -> std::string
            { return hasGraphicsChanges() ? "Apply Changes" : "Back"; },
            []() {});

        // Use the game's label formatter (defaults to "> label <" when selected)
        btn->setLabelFormatter([](const std::string &label, bool selected)
                               { return UIUtils::formatButtonLabel(label, selected); });

        backRow.control = std::move(btn);
    }
    rows.push_back(std::move(backRow));

    window->setRows(std::move(rows));
}

void GraphicsSettings::handleInput()
{
    m_uiManager.handleInput(Input::instance());
    processUIEvents();
}

void GraphicsSettings::update(float dt)
{
    m_uiManager.update(dt);
}

void GraphicsSettings::render(float /*alpha*/)
{
    if (!m_renderer)
        return;
    m_renderer->clear(Color{12, 14, 20, 255});
    m_uiManager.render(m_renderer);
}

void GraphicsSettings::processUIEvents()
{
    auto events = m_uiManager.drainEvents();
    for (const auto &event : events)
    {
        if (event.windowId == WindowId::SettingsGraphics)
        {
            if (event.type == UIEventType::ActionSelected)
            {
                if (event.actionId == ActionId::Confirm)
                {
                    // User pressed Enter on the button row
                    if (hasGraphicsChanges())
                    {
                        applyAndSaveGraphics();
                    }
                    m_sm.pop();
                    return;
                }
                // AdjustLeft/AdjustRight are handled automatically by the window;
                // no extra action needed here because the state owns the data and
                // the callbacks already update SettingsManager.
            }
            else if (event.type == UIEventType::ActionCanceled)
            {
                // Back pressed
                if (hasGraphicsChanges())
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
                    applyAndSaveGraphics();
                else
                    discardGraphicsChanges();

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

bool GraphicsSettings::hasGraphicsChanges() const
{
    return SettingsManager::instance().hasGraphicsChanges();
}

void GraphicsSettings::applyAndSaveGraphics()
{
    auto &mgr = SettingsManager::instance();
    mgr.applyGraphics();
    auto &s = mgr.data();
    s.appliedResolutionIndex = s.resolutionIndex;
    s.appliedWindowMode = s.windowMode;
    mgr.saveToFile();
}

void GraphicsSettings::discardGraphicsChanges()
{
    auto &s = SettingsManager::instance().data();
    s.resolutionIndex = s.appliedResolutionIndex;
    s.windowMode = s.appliedWindowMode;
}

void GraphicsSettings::showExitConfirm()
{
    m_showExitConfirm = true;
    auto *confirm = m_uiManager.push<ConfirmWindow>(WindowId::SettingsExitConfirm);
    confirm->setFont(FontManager::instance().get(FontRole::Heading));
    confirm->setPrompt("You have unapplied graphics changes");
}