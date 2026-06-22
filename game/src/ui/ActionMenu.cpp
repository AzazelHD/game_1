#include "config/GameConstants.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/core/Log.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/renderer/Renderer.h"
#include "engine/renderer/Font.h"
#include "ui/ActionMenu.h"

// [X]: Implement show(), hide(), update(), render(), setOnConfirm(), setOnCancel()
// (setOnConfirm/setOnCancel are implemented inline in the header)

namespace
{
    constexpr float MENU_X = GameConstants::VIEW_W - 270.0f;
    constexpr float MENU_Y_OFFSET = 100.0f;
    constexpr float OPTION_HEIGHT = 30.0f;
    constexpr float OPTION_WIDTH = 220.0f;
    constexpr float PADDING = 16.0f;

    constexpr Color BG_COLOR = {12, 12, 20, 230};
    constexpr Color BORDER_COLOR = {180, 180, 180, 255};
    constexpr Color TEXT_COLOR = {235, 235, 235, 255};
    constexpr Color DISABLED_TEXT_COLOR = {100, 100, 100, 255};
    constexpr Color SELECTED_BG_COLOR = {50, 50, 90, 230};
    constexpr Color SELECTED_BORDER_COLOR = {255, 255, 255, 255};
}
void ActionMenu::show(std::vector<std::string> options, std::vector<bool> enabled)
{
    m_options = std::move(options);
    m_enabled = enabled.empty()
                    ? std::vector<bool>(m_options.size(), true)
                    : std::move(enabled);

    // Find first enabled option as default selection
    m_selectedIndex = 0;
    for (size_t i = 0; i < m_options.size(); ++i)
    {
        if (m_enabled[i])
        {
            m_selectedIndex = static_cast<int>(i);
            break;
        }
    }

    m_visible = true;
    m_justShown = true;
    LOG_INFO("ActionMenu", "Shown with %zu options", m_options.size());
}

void ActionMenu::hide()
{
    m_visible = false;
    m_options.clear();
    m_selectedIndex = 0;
    LOG_INFO("ActionMenu", "Hidden");
}

void ActionMenu::update(float /*dt*/)
{
    if (!m_visible || m_options.empty())
        return;

    if (m_justShown)
    {
        m_justShown = false;
        return;
    }

    const Input &input = Input::instance();

    bool moveUp = input.isKeyPressed(KeyCode::Up, true) || input.isKeyPressed(KeyCode::W, true);
    bool moveDown = input.isKeyPressed(KeyCode::Down, true) || input.isKeyPressed(KeyCode::S, true);

    if (moveUp && !moveDown)
    {
        int newIdx = m_selectedIndex;
        do
        {
            newIdx = (newIdx - 1 + m_options.size()) % m_options.size();
        } while (newIdx != m_selectedIndex && !m_enabled[newIdx]);
        if (m_enabled[newIdx])
            m_selectedIndex = newIdx;
    }
    else if (moveDown && !moveUp)
    {
        int newIdx = m_selectedIndex;
        do
        {
            newIdx = (newIdx + 1) % m_options.size();
        } while (newIdx != m_selectedIndex && !m_enabled[newIdx]);
        if (m_enabled[newIdx])
            m_selectedIndex = newIdx;
    }

    if (input.isKeyPressed(KeyCode::Accept, false) && m_enabled[m_selectedIndex])
    {
        if (m_onConfirm)
            m_onConfirm(m_selectedIndex);
        hide();
    }
    else if (input.isKeyPressed(KeyCode::Back, false))
    {
        if (m_onCancel)
            m_onCancel();
        hide();
    }
}

void ActionMenu::render(Renderer *renderer, const Font *font) const
{
    if (!m_visible || !renderer || !font)
        return;

    const float menuH = m_options.size() * OPTION_HEIGHT;
    const float menuY = GameConstants::VIEW_H - menuH - MENU_Y_OFFSET;
    const Rectf menuRect{MENU_X, menuY, OPTION_WIDTH, menuH};

    // Background
    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(BG_COLOR);
    renderer->fillRect(menuRect);
    renderer->setDrawColor(BORDER_COLOR);
    renderer->drawRect(menuRect);

    // Options
    for (size_t i = 0; i < m_options.size(); ++i)
    {
        Rectf optRect{MENU_X, menuY + i * OPTION_HEIGHT, OPTION_WIDTH, OPTION_HEIGHT};

        if (i == m_selectedIndex)
        {
            renderer->setDrawColor(SELECTED_BG_COLOR);
            renderer->fillRect(optRect);
            renderer->setDrawColor(SELECTED_BORDER_COLOR);
            renderer->drawRect(optRect);
        }

        const float textX = optRect.x + PADDING;
        const float textY = optRect.y + (OPTION_HEIGHT - 8.0f) * 0.5f;

        Color textColor = m_enabled[i] ? TEXT_COLOR : DISABLED_TEXT_COLOR;
        renderer->renderText(font, m_options[i], Vec2f{textX, textY}, textColor, false, false, false);
    }
}