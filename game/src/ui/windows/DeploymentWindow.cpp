#include "ui/windows/DeploymentWindow.h"

#include "config/GameConstants.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Renderer.h"
#include "ui/UITheme.h"
#include "ui/WindowId.h"

#include <cstdio>

namespace
{
    constexpr float kGlyphW = 8.0f;

    float centeredTextX(float panelX, float panelW, const std::string &text)
    {
        const float textW = static_cast<float>(text.size()) * kGlyphW;
        return panelX + (panelW - textW) * 0.5f;
    }
}

DeploymentWindow::DeploymentWindow(WindowId id)
    : UIWindow(id, false, false)
{
}

void DeploymentWindow::setDeploymentStatus(int placed, int maxUnits, bool canStart)
{
    m_placed = placed;
    m_maxUnits = maxUnits;
    m_canStart = canStart;
}

void DeploymentWindow::handleInput(const Input &input)
{
    if (input.isKeyPressed(KeyCode::Q, false))
    {
        emit(UIEvent{.type = UIEventType::ActionSelected, .windowId = id(), .actionId = ActionId::CyclePrev});
        return;
    }

    if (input.isKeyPressed(KeyCode::E, false))
    {
        emit(UIEvent{.type = UIEventType::ActionSelected, .windowId = id(), .actionId = ActionId::CycleNext});
        return;
    }

    if (input.isKeyPressed(KeyCode::Accept, false))
    {
        emit(UIEvent{.type = UIEventType::ActionSelected, .windowId = id(), .actionId = ActionId::Accept});
        return;
    }

    if (input.isKeyPressed(KeyCode::Details, false))
    {
        emit(UIEvent{.type = UIEventType::ActionSelected, .windowId = id(), .actionId = ActionId::Details});
        return;
    }

    if (input.isKeyPressed(KeyCode::Back, false))
    {
        emit(UIEvent{.type = UIEventType::ActionSelected, .windowId = id(), .actionId = ActionId::Back});
        return;
    }

    if (input.isKeyPressed(KeyCode::Advance, false))
        emit(UIEvent{.type = UIEventType::ActionSelected, .windowId = id(), .actionId = ActionId::StartCombat});
}

void DeploymentWindow::update(float /*dt*/)
{
}

void DeploymentWindow::render(Renderer *renderer) const
{
    (void)renderer;
}
