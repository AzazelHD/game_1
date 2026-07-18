#include "ui/windows/UnitPanelWindow.h"

#include "battle/Unit.h"
#include "config/GameConstants.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Renderer.h"
#include "ui/UITheme.h"
#include "ui/UIScale.h"
#include "ui/WindowId.h"
#include "ui/UnitPortrait.h"

#include <cstdio>

namespace
{
    constexpr float kGlyphW = 8.0f;

    float centeredTextX(float x, float w, const std::string &text)
    {
        const float textW = static_cast<float>(text.size()) * kGlyphW;
        return x + (w - textW) * 0.5f;
    }
}

UnitPanelWindow::UnitPanelWindow(WindowId id)
    : UIWindow(id, false, false)
{
}

void UnitPanelWindow::setTurnInfo(const Unit *activeUnit, int round)
{
    m_turnUnit = activeUnit;
    m_round = round;
}

void UnitPanelWindow::setSingle(const Unit *unit, int team)
{
    m_hasPreview = false;
    m_leftUnit = unit;
    m_rightUnit = nullptr;
    m_singleTeam = team;
}

void UnitPanelWindow::setDuel(const Unit *left, const Unit *right, bool rightIsEnemy)
{
    m_hasPreview = false;
    m_leftUnit = left;
    m_rightUnit = right;
    m_rightIsEnemy = rightIsEnemy;
}

void UnitPanelWindow::setPreview(std::string name, int level, int hp, int mp, int team, bool isPlaced)
{
    m_previewName = std::move(name);
    m_previewLevel = level;
    m_previewHp = hp;
    m_previewMp = mp;
    m_previewTeam = team;
    m_previewPlaced = isPlaced;
    m_hasPreview = true;
    m_leftUnit = nullptr;
    m_rightUnit = nullptr;
}

void UnitPanelWindow::clearPreview()
{
    m_hasPreview = false;
}

void UnitPanelWindow::clearPanels()
{
    m_leftUnit = nullptr;
    m_rightUnit = nullptr;
    m_hasPreview = false;
}

void UnitPanelWindow::handleInput(const Input & /*input*/)
{
}

void UnitPanelWindow::update(float /*dt*/)
{
}

void UnitPanelWindow::render(Renderer *renderer) const
{
    if (!renderer || !m_font)
        return;

    UIScale::refresh();
    const float ui = UIScale::factor();

    renderTurnBanner(renderer);

    const float leftX = 16.0f * ui;
    const float baseY = GameConstants::VIEW_H - 132.0f * ui;

    if (m_leftUnit && m_rightUnit)
    {
        const Rectf leftRect =
            UnitPortrait::render(renderer,
                                 m_font,
                                 *m_leftUnit,
                                 Vec2f{leftX, baseY},
                                 UnitPortrait::PortraitStyle{.team = m_leftUnit->getTeam(), .mirrored = false, .transparent = false});

        const float rightX = GameConstants::VIEW_W - leftRect.w - 16.0f * ui;

        UnitPortrait::render(renderer,
                             m_font,
                             *m_rightUnit,
                             Vec2f{rightX, baseY},
                             UnitPortrait::PortraitStyle{.team = m_rightUnit->getTeam(), .mirrored = true, .transparent = false});
        return;
    }

    if (m_leftUnit)
    {
        UnitPortrait::render(renderer,
                             m_font,
                             *m_leftUnit,
                             Vec2f{leftX, baseY},
                             UnitPortrait::PortraitStyle{.team = m_singleTeam, .mirrored = false, .transparent = false});
        return;
    }

    if (m_hasPreview)
    {
        UnitPortrait::renderFromStats(renderer,
                                      m_font,
                                      m_previewName,
                                      m_previewLevel,
                                      m_previewHp,
                                      m_previewHp,
                                      m_previewMp,
                                      m_previewMp,
                                      Vec2f{leftX, baseY},
                                      UnitPortrait::PortraitStyle{.team = m_previewTeam, .mirrored = false, .transparent = false, .showPlacedMarker = m_previewPlaced});
    }
}

void UnitPanelWindow::renderTurnBanner(Renderer *renderer) const
{
    if (!m_turnUnit)
        return;

    UIScale::refresh();
    const float ui = UIScale::factor();

    const float w = 360.0f * ui;
    const float h = 30.0f * ui;
    const float x = (GameConstants::VIEW_W - w) * 0.5f;
    const float y = 8.0f * ui;

    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(UITheme::Panel);
    renderer->fillRect(Rectf{x, y, w, h});
    renderer->setDrawColor(UITheme::Border);
    renderer->drawRect(Rectf{x, y, w, h});

    char buf[128];
    if (m_round > 0)
        std::snprintf(buf, sizeof(buf), "Round %d - %s", m_round, m_turnUnit->getName().c_str());
    else
        std::snprintf(buf, sizeof(buf), "%s's Turn", m_turnUnit->getName().c_str());

    renderer->renderText(m_font, buf, Vec2f{centeredTextX(x, w, buf), y + 10.0f * ui}, UITheme::Text, false, false, false);
}