#include "ui/windows/InspectWindow.h"

#include "config/GameConstants.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Renderer.h"
#include "battle/UnitData.h"
#include "ui/UITheme.h"

#include <string>
#include <algorithm>

namespace
{
    constexpr float kPanelW = 520.0f;
    constexpr float kPanelH = 360.0f;
    constexpr float kLineStep = 20.0f;
    constexpr int kVisibleLines = 14;
}

InspectWindow::InspectWindow(std::string id)
    : UIWindow(std::move(id), true, false)
{
}

void InspectWindow::setLines(std::vector<std::string> lines)
{
    m_lines = std::move(lines);
    m_scroll = 0;
}

void InspectWindow::handleInput(const Input &input)
{
    if (input.isKeyPressed(KeyCode::Up, false))
    {
        m_scroll = std::max(0, m_scroll - 1);
        return;
    }

    if (input.isKeyPressed(KeyCode::Down, false))
    {
        const int maxScroll = std::max(0, static_cast<int>(m_lines.size()) - kVisibleLines);
        m_scroll = std::min(maxScroll, m_scroll + 1);
        return;
    }

    if (input.isKeyPressed(KeyCode::Back, false) || input.isKeyPressed(KeyCode::Accept, false))
    {
        emit(UIEvent{.type = UIEventType::ActionCanceled, .windowId = id(), .actionId = "close"});
    }
}

void InspectWindow::update(float /*dt*/)
{
}

void InspectWindow::render(Renderer *renderer) const
{
    if (!renderer || !m_font)
        return;

    const float x = (GameConstants::VIEW_W - kPanelW) * 0.5f;
    const float y = (GameConstants::VIEW_H - kPanelH) * 0.5f;

    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(Color{0, 0, 0, 160});
    renderer->fillRect(Rectf{0.0f, 0.0f, GameConstants::VIEW_W, GameConstants::VIEW_H});

    renderer->setDrawColor(UITheme::PopupBG);
    renderer->fillRect(Rectf{x, y, kPanelW, kPanelH});
    renderer->setDrawColor(UITheme::PopupBorder);
    renderer->drawRect(Rectf{x, y, kPanelW, kPanelH});

    renderer->renderTextInRect(m_font,
                               m_title,
                               Rectf{x, y + 14.0f, kPanelW, 24.0f},
                               UITheme::SelectedText,
                               Renderer::HorizontalAlign::Center,
                               Renderer::VerticalAlign::Middle,
                               false,
                               false,
                               false);

    const int maxLines = static_cast<int>(m_lines.size());
    const int start = std::clamp(m_scroll, 0, std::max(0, maxLines - 1));
    const int end = std::min(maxLines, start + kVisibleLines);

    float lineY = y + 48.0f;
    for (int i = start; i < end; ++i)
    {
        const std::string &line = m_lines[static_cast<std::size_t>(i)];
        renderer->renderTextInRect(m_font,
                                   line,
                                   Rectf{x, lineY, kPanelW, kLineStep},
                                   UITheme::Text,
                                   Renderer::HorizontalAlign::Center,
                                   Renderer::VerticalAlign::Middle,
                                   false,
                                   false,
                                   false);
        lineY += kLineStep;
    }

    renderer->renderTextInRect(m_font,
                               "Inspect",
                               Rectf{x, y + kPanelH - 30.0f, kPanelW, 24.0f},
                               UITheme::Info,
                               Renderer::HorizontalAlign::Center,
                               Renderer::VerticalAlign::Middle,
                               false,
                               false,
                               false);
}

std::vector<std::string> InspectWindow::buildLines(const UnitData &data)
{
    std::vector<std::string> lines;
    lines.reserve(32);

    lines.push_back("Name: " + data.name);
    lines.push_back("Class: " + data.className);
    lines.push_back("SpriteSet: " + data.spriteSetId);
    lines.push_back("Acquisition: " + std::to_string(data.acquisitionIndex));
    lines.push_back("Race: " + std::string(toString(data.race)));
    lines.push_back("Gender: " + std::string(toString(data.gender)));
    lines.push_back("Level: " + std::to_string(data.level));
    lines.push_back("Team: " + std::to_string(data.team));
    lines.push_back("HP: " + std::to_string(data.maxHp));
    lines.push_back("MP: " + std::to_string(data.maxMp));
    lines.push_back("ATK: " + std::to_string(data.attack));
    lines.push_back("DEF: " + std::to_string(data.defense));
    lines.push_back("MAG: " + std::to_string(data.magic));
    lines.push_back("MDEF: " + std::to_string(data.magicDefense));
    lines.push_back("MOVE: " + std::to_string(data.moveRange));
    lines.push_back("RANGE: " + std::to_string(data.atkRange));
    lines.push_back("EVA: " + std::to_string(data.evasion));
    lines.push_back("JUMP: " + std::to_string(data.jump));
    lines.push_back("SPD: " + std::to_string(data.speed));

    lines.push_back("Affinities:");
    if (data.affinities.empty())
    {
        lines.push_back("  <none>");
    }
    else
    {
        for (const ElementAffinity &aff : data.affinities)
            lines.push_back("  " + std::string(toString(aff.element)) + ": " + toString(aff.affinity));
    }

    lines.push_back("Skills:");
    if (data.skillIds.empty())
    {
        lines.push_back("  <none>");
    }
    else
    {
        for (const std::string &id : data.skillIds)
            lines.push_back("  " + id);
    }

    return lines;
}
