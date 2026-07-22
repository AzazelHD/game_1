#include "ui/windows/UnitInspectWindow.h"

#include "battle/UnitData.h"
#include "config/GameConstants.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Renderer.h"
#include "ui/UITheme.h"
#include "ui/WindowId.h"

namespace
{
    constexpr float kPanelW = 360.0f;
    constexpr float kPanelH = 320.0f;
    constexpr float kBorderThickness = 3.0f; // drawn as N concentric 1px rects

    constexpr Color kPlayerBorder{90, 140, 255, 255};
    constexpr Color kAllyBorder{110, 220, 130, 255};
    constexpr Color kEnemyBorder{230, 90, 90, 255};

    Color borderColorFor(UnitInspectWindow::Relation relation)
    {
        switch (relation)
        {
        case UnitInspectWindow::Relation::Player:
            return kPlayerBorder;
        case UnitInspectWindow::Relation::Ally:
            return kAllyBorder;
        case UnitInspectWindow::Relation::Enemy:
            return kEnemyBorder;
        }
        return kPlayerBorder;
    }
}

UnitInspectWindow::UnitInspectWindow(WindowId id)
    : UIWindow(id, true, false)
{
}

UnitInspectWindow::Section UnitInspectWindow::buildStatsSection(const UnitData &data)
{
    Section section;
    section.title = "Stats";
    section.lines = {
        {"HP", std::to_string(data.maxHp)},
        {"MP", std::to_string(data.maxMp)},
        {"Attack", std::to_string(data.attack)},
        {"Defense", std::to_string(data.defense)},
        {"Magic", std::to_string(data.magic)},
        {"Magic Def", std::to_string(data.magicDefense)},
        {"Speed", std::to_string(data.speed)},
        {"Move", std::to_string(data.moveRange)},
        {"Range", std::to_string(data.atkRange)},
        {"Evasion", std::to_string(data.evasion)},
    };
    return section;
}

void UnitInspectWindow::handleInput(const Input &input)
{
    if (input.isKeyPressed(KeyCode::Back, false) || input.isKeyPressed(KeyCode::Accept, false))
    {
        emit(UIEvent{.type = UIEventType::ActionCanceled, .windowId = id(), .actionId = ActionId::Close});
    }
}

void UnitInspectWindow::update(float /*dt*/)
{
}

void UnitInspectWindow::render(Renderer *renderer) const
{
    if (!renderer || !m_font)
        return;

    const float x = (GameConstants::VIEW_W - kPanelW) * 0.5f;
    const float y = (GameConstants::VIEW_H - kPanelH) * 0.5f;

    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(UITheme::PopupBG);
    renderer->fillRect(Rectf{x, y, kPanelW, kPanelH});

    const Color border = borderColorFor(m_relation);
    renderer->setDrawColor(border);
    for (float i = 0.0f; i < kBorderThickness; i += 1.0f)
        renderer->drawRect(Rectf{x - i, y - i, kPanelW + i * 2.0f, kPanelH + i * 2.0f});

    float lineY = y + 16.0f;

    renderer->renderTextInRect(m_font, m_name,
                               Rectf{x, lineY, kPanelW, 24.0f},
                               UITheme::SelectedText,
                               Renderer::HorizontalAlign::Center,
                               Renderer::VerticalAlign::Middle,
                               false, false, false);
    lineY += 26.0f;

    renderer->renderTextInRect(m_font, m_job,
                               Rectf{x, lineY, kPanelW, 20.0f},
                               UITheme::Info,
                               Renderer::HorizontalAlign::Center,
                               Renderer::VerticalAlign::Middle,
                               false, false, false);
    lineY += 34.0f;

    for (const Section &section : m_sections)
    {
        if (!section.title.empty())
        {
            renderer->renderTextInRect(m_font, section.title,
                                       Rectf{x + 20.0f, lineY, kPanelW - 40.0f, 20.0f},
                                       UITheme::SelectedText,
                                       Renderer::HorizontalAlign::Left,
                                       Renderer::VerticalAlign::Middle,
                                       false, false, false);
            lineY += 26.0f;
        }

        for (const StatLine &line : section.lines)
        {
            renderer->renderTextInRect(m_font, line.label,
                                       Rectf{x + 32.0f, lineY, (kPanelW - 64.0f) * 0.5f, 20.0f},
                                       UITheme::Text,
                                       Renderer::HorizontalAlign::Left,
                                       Renderer::VerticalAlign::Middle,
                                       false, false, false);
            renderer->renderTextInRect(m_font, line.value,
                                       Rectf{x + kPanelW * 0.5f, lineY, kPanelW * 0.5f - 32.0f, 20.0f},
                                       UITheme::Text,
                                       Renderer::HorizontalAlign::Right,
                                       Renderer::VerticalAlign::Middle,
                                       false, false, false);
            lineY += 22.0f;
        }
        lineY += 12.0f;
    }
}
