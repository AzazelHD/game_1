#include "ui/windows/ConfirmWindow.h"

#include "config/GameConstants.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Renderer.h"
#include "ui/UITheme.h"
#include "ui/UIScale.h"
#include "ui/WindowId.h"

ConfirmWindow::ConfirmWindow(WindowId id)
    : UIWindow(id, true, false)
{
}

void ConfirmWindow::handleInput(const Input &input)
{
    if (input.isKeyPressed(KeyCode::Left, false) ||
        input.isKeyPressed(KeyCode::A, false) ||
        input.isKeyPressed(KeyCode::Up, false) ||
        input.isKeyPressed(KeyCode::W, false))
    {
        m_confirmSelected = false;
        return;
    }

    if (input.isKeyPressed(KeyCode::Q, false))
    {
        emit(UIEvent{.type = UIEventType::NavigatePrevious,
                     .windowId = id()});
        return;
    }

    if (input.isKeyPressed(KeyCode::E, false))
    {
        emit(UIEvent{.type = UIEventType::NavigateNext,
                     .windowId = id()});
        return;
    }

    if (input.isKeyPressed(KeyCode::Right, false) ||
        input.isKeyPressed(KeyCode::D, false) ||
        input.isKeyPressed(KeyCode::Down, false) ||
        input.isKeyPressed(KeyCode::S, false))
    {
        m_confirmSelected = true;
        return;
    }

    if (input.isKeyPressed(KeyCode::Back, false))
    {
        emit(UIEvent{.type = UIEventType::ConfirmResult,
                     .windowId = id(),
                     .actionId = ActionId::Cancel,
                     .confirmed = false});
        return;
    }

    if (input.isKeyPressed(KeyCode::Accept, false))
    {
        emit(UIEvent{.type = UIEventType::ConfirmResult,
                     .windowId = id(),
                     .actionId = m_confirmSelected ? ActionId::Confirm : ActionId::Cancel,
                     .confirmed = m_confirmSelected});
    }
}

void ConfirmWindow::update(float /*dt*/)
{
}

void ConfirmWindow::render(Renderer *renderer) const
{
    if (!renderer || !m_font)
        return;

    UIScale::refresh();
    const float ui = UIScale::factor();

    const float w = 300.0f * ui;
    const float h = 130.0f * ui;
    const float x = (GameConstants::VIEW_W - w) * 0.5f;
    const float y = (GameConstants::VIEW_H - h) * 0.5f;

    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(UITheme::PopupBG);
    renderer->fillRect(Rectf{x, y, w, h});
    renderer->setDrawColor(UITheme::PopupBorder);
    renderer->drawRect(Rectf{x, y, w, h});

    renderer->renderTextInRect(m_font,
                               m_prompt,
                               Rectf{x, y + 20.0f * ui, w, 24.0f * ui},
                               UITheme::Text,
                               Renderer::HorizontalAlign::Center,
                               Renderer::VerticalAlign::Middle,
                               false,
                               false,
                               false);

    const std::string cancelLabel = m_confirmSelected ? "Cancel" : "> Cancel <";
    const std::string confirmLabel = m_confirmSelected ? "> Confirm <" : "Confirm";

    const float rowY = y + 82.0f * ui;
    const float columnInset = 14.0f * ui;
    const float columnW = (w - columnInset * 3.0f) * 0.5f;
    renderer->renderTextInRect(m_font,
                               cancelLabel,
                               Rectf{x + columnInset, rowY, columnW, 24.0f * ui},
                               m_confirmSelected ? UITheme::Text : UITheme::SelectedText,
                               Renderer::HorizontalAlign::Center,
                               Renderer::VerticalAlign::Middle,
                               false,
                               false,
                               false);
    renderer->renderTextInRect(m_font,
                               confirmLabel,
                               Rectf{x + columnInset * 2.0f + columnW, rowY, columnW, 24.0f * ui},
                               m_confirmSelected ? UITheme::SelectedText : UITheme::Text,
                               Renderer::HorizontalAlign::Center,
                               Renderer::VerticalAlign::Middle,
                               false,
                               false,
                               false);
}
