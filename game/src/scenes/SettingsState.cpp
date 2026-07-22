#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/FontManager.h"
#include "engine/renderer/Renderer.h"
#include "engine/statemachine/StateMachine.h"
#include "config/GameConstants.h"
#include "scenes/SettingsState.h"
#include "scenes/GraphicsSettings.h"
#include "scenes/AudioSettings.h"
#include "ui/ActionId.h"
#include "ui/UITheme.h"
#include "ui/WindowId.h"
#include "ui/windows/ButtonMenuWindow.h"

SettingsState::SettingsState(StateMachine<Scene> &sm, Renderer *renderer)
    : m_sm(sm), m_renderer(renderer) {}

void SettingsState::onEnter()
{
    m_uiManager.clear();

    auto *menu = m_uiManager.push<ButtonMenuWindow>(WindowId::SettingsMenu);
    menu->setFont(FontManager::instance().get(FontRole::Heading));

    menu->centerHorizontally(true);
    menu->setPanelPosition(Vec2f{0.0f, GameConstants::VIEW_CY - 40.0f});

    menu->setItems({
        ButtonMenuWindow::Item{.id = ActionId::OpenGraphicSettings, .label = "Graphics", .enabled = true},
        ButtonMenuWindow::Item{.id = ActionId::OpenAudioSettings, .label = "Audio", .enabled = true},
        ButtonMenuWindow::Item{.id = ActionId::Back, .label = "Back", .enabled = true},
    });
}

void SettingsState::handleInput()
{
    m_uiManager.handleInput(Input::instance());
    processUIEvents();
}

void SettingsState::update(float dt)
{
    m_uiManager.update(dt);
}

void SettingsState::render(float /*alpha*/)
{
    if (!m_renderer)
        return;

    m_renderer->clear(Color{12, 14, 20, 255});

    const std::string title = "Settings";
    m_renderer->renderTextInRect(FontManager::instance().get(FontRole::Title),
                                 title,
                                 Rectf{0.0f, 130.0f, GameConstants::VIEW_W, 64.0f},
                                 Color{240, 245, 255, 255},
                                 Renderer::HorizontalAlign::Center,
                                 Renderer::VerticalAlign::Middle,
                                 false, false, false);

    m_uiManager.render(m_renderer);
}

void SettingsState::processUIEvents()
{
    auto events = m_uiManager.drainEvents();
    for (const auto &event : events)
    {
        if (event.windowId != WindowId::SettingsMenu)
            continue;

        if (event.type == UIEventType::ActionCanceled)
        {
            m_sm.pop();
            return;
        }

        if (event.type == UIEventType::ActionSelected)
        {
            switch (event.actionId)
            {
            case ActionId::OpenGraphicSettings:
                m_sm.push(std::make_unique<GraphicsSettings>(m_sm, m_renderer));
                break;
            case ActionId::OpenAudioSettings:
                m_sm.push(std::make_unique<AudioSettings>(m_sm, m_renderer));
                break;
            case ActionId::Back:
                m_sm.pop();
                break;
            default:
                break;
            }
        }
    }
}
