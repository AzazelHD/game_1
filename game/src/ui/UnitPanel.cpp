#include "ui/UnitPanel.h"
#include "engine/math/Rect.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Renderer.h"
#include "battle/Unit.h"
#include "config/GameConstants.h"

#include <algorithm>
#include <cstdio>

namespace
{
    constexpr Color PANEL_BG = {12, 12, 20, 215};
    constexpr Color PANEL_BORDER = {180, 180, 180, 255};
    constexpr Color TEXT_COLOR = {235, 235, 235, 255};

    Color teamColor(int team)
    {
        switch (team)
        {
        case 0:
            return Color{64, 128, 255, 255};
        case 1:
            return Color{64, 255, 64, 255};
        default:
            return Color{255, 64, 64, 255};
        }
    }

    void drawPanel(Renderer *renderer, Rectf rect)
    {
        renderer->setBlendMode(Renderer::BlendMode::Blend);
        renderer->setDrawColor(PANEL_BG);
        renderer->fillRect(rect);
        renderer->setDrawColor(PANEL_BORDER);
        renderer->drawRect(rect);
    }

    // Background + proportional fill + border + label, e.g. "HP 24/30".
    void drawBar(Renderer *renderer, const Font *font, float x, float y, float w, float h,
                 float ratio, Color fillColor, const char *label)
    {
        ratio = std::clamp(ratio, 0.0f, 1.0f);

        Rectf bg{x, y, w, h};
        renderer->setDrawColor(Color{40, 40, 40, 255});
        renderer->fillRect(bg);

        Rectf fill{x, y, w * ratio, h};
        renderer->setDrawColor(fillColor);
        renderer->fillRect(fill);

        renderer->setDrawColor(Color{200, 200, 200, 255});
        renderer->drawRect(bg);

        if (font)
            renderer->renderText(font, label, Vec2f{x + 4.0f, y + (h - 8.0f) * 0.5f}, TEXT_COLOR, false, false, false);
    }

    void drawText(Renderer *renderer, const Font *font, float x, float y, Color color, const char *text)
    {
        if (font)
            renderer->renderText(font, text, Vec2f{x, y}, color, false, false, false);
    }

    void drawPortrait(Renderer *renderer, Rectf rect, Color borderColor)
    {
        // Placeholder: just a coloured rectangle with border
        renderer->setDrawColor(Color{60, 60, 60, 255});
        renderer->fillRect(rect);
        renderer->setDrawColor(borderColor);
        renderer->drawRect(rect);
    }
}

void UnitPanel::show(const Unit *unit)
{
    m_unit = unit;
    m_visible = (unit != nullptr);
}

void UnitPanel::hide()
{
    m_unit = nullptr;
    m_visible = false;
}

void UnitPanel::setTurnInfo(int round, const Unit *activeUnit)
{
    m_round = round;
    m_activeUnit = activeUnit;
}

void UnitPanel::render(Renderer *renderer) const
{
    if (!renderer)
        return;

    renderTurnBanner(renderer);

    if (m_visible && m_unit)
        renderStatsPanel(renderer);
}

void UnitPanel::renderStatsPanel(Renderer *renderer) const
{
    if (!m_font)
        return;

    constexpr float PANEL_W = 300.0f;
    constexpr float PANEL_H = 116.0f;
    const float panel_x = m_panelPos.x;
    const float panel_y = m_panelPos.y;

    // Background with team colour border
    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(PANEL_BG);
    renderer->fillRect(Rectf{panel_x, panel_y, PANEL_W, PANEL_H});
    renderer->setDrawColor(m_teamColor);
    renderer->drawRect(Rectf{panel_x, panel_y, PANEL_W, PANEL_H});

    float pad = 10.0f;
    float x = panel_x + pad;
    float y = panel_y + pad;

    // ── Portrait (always shown when stat panel is visible) ────────────────
    constexpr float PORTRAIT_W = 20.0f;
    constexpr float PORTRAIT_H = 30.0f;

    Rectf portraitRect;
    float px, py = panel_y + pad;

    if (panel_x < GameConstants::VIEW_W / 2.0f)
    {
        px = panel_x + pad;
        x = px + PORTRAIT_W + pad; // shift text right
    }
    else
    {
        px = panel_x + PANEL_W - pad - PORTRAIT_W;
        // text x stays at default (panel_x + pad) – no shift needed
    }
    portraitRect = Rectf{px, py, PORTRAIT_W, PORTRAIT_H};

    renderer->setDrawColor(Color{60, 60, 60, 255});
    renderer->fillRect(portraitRect);
    renderer->setDrawColor(m_teamColor);
    renderer->drawRect(portraitRect);

    // Name / level / team swatch
    char buf[96];
    std::snprintf(buf, sizeof(buf), "%s  Lv%d", m_unit->getName().c_str(), m_unit->getLevel());
    drawText(renderer, m_font, x, y, TEXT_COLOR, buf);

    const Color tColor = teamColor(m_unit->getTeam());
    Rectf swatch{panel_x + PANEL_W - pad - 14.0f, y - 2.0f, 14.0f, 14.0f};
    renderer->setDrawColor(tColor);
    renderer->fillRect(swatch);
    renderer->setDrawColor(Color{0, 0, 0, 255});
    renderer->drawRect(swatch);

    y += 18.0f;

    // ── HP / MP bars ────────────────────────────────────────────────────
    std::snprintf(buf, sizeof(buf), "HP %d/%d", m_unit->getCurrentHp(), m_unit->getMaxHp());
    drawText(renderer, m_font, x, y, TEXT_COLOR, buf);
    y += 16.0f;

    std::snprintf(buf, sizeof(buf), "MP %d/%d", m_unit->getCurrentMp(), m_unit->getMaxMp());
    drawText(renderer, m_font, x, y, TEXT_COLOR, buf);
    y += 16.0f;

    // ── Core stats, two columns ────────────────────────────────────────
    std::snprintf(buf, sizeof(buf), "ATK %d   DEF %d   MAG %d", m_unit->getAttack(), m_unit->getDefense(), m_unit->getMagic());
    drawText(renderer, m_font, x, y, TEXT_COLOR, buf);
    y += 16.0f;

    std::snprintf(buf, sizeof(buf), "MDF %d   SPD %d   MOV %d   RNG %d",
                  m_unit->getMagicDefense(), m_unit->getSpeed(), m_unit->getMoveRange(), m_unit->getAtkRange());
    drawText(renderer, m_font, x, y, TEXT_COLOR, buf);
}

void UnitPanel::renderTurnBanner(Renderer *renderer) const
{
    if (!m_font || !m_activeUnit)
        return;

    constexpr float BANNER_W = 360.0f;
    constexpr float BANNER_H = 28.0f;
    const float bannerX = GameConstants::VIEW_CX - BANNER_W * 0.5f;
    constexpr float bannerY = 8.0f;

    drawPanel(renderer, Rectf{bannerX, bannerY, BANNER_W, BANNER_H});

    char buf[96];
    std::snprintf(buf, sizeof(buf), "Round %d - %s's Turn (Team %d)",
                  m_round, m_activeUnit->getName().c_str(), m_activeUnit->getTeam());
    drawText(renderer, m_font, bannerX + 10.0f, bannerY + (BANNER_H - 8.0f) * 0.5f, TEXT_COLOR, buf);
}