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

    constexpr float PANEL_X = 16.0f;
    constexpr float PANEL_W = 300.0f;
    constexpr float PANEL_H = 116.0f;
    const float panelY = GameConstants::VIEW_H - PANEL_H - 16.0f;

    drawPanel(renderer, Rectf{PANEL_X, panelY, PANEL_W, PANEL_H});

    const float pad = 10.0f;
    float x = PANEL_X + pad;
    float y = panelY + pad;

    // ── Name / level / team swatch ────────────────────────────────────────
    char buf[96];
    std::snprintf(buf, sizeof(buf), "%s  Lv%d", m_unit->getName().c_str(), m_unit->getLevel());
    drawText(renderer, m_font, x, y, TEXT_COLOR, buf);

    const Color tColor = teamColor(m_unit->getTeam());
    Rectf swatch{PANEL_X + PANEL_W - pad - 14.0f, y - 2.0f, 14.0f, 14.0f};
    renderer->setDrawColor(tColor);
    renderer->fillRect(swatch);
    renderer->setDrawColor(Color{0, 0, 0, 255});
    renderer->drawRect(swatch);

    y += 18.0f;

    // ── HP / MP bars ────────────────────────────────────────────────────
    const float barW = PANEL_W - pad * 2.0f;
    const float barH = 14.0f;

    std::snprintf(buf, sizeof(buf), "HP %d/%d", m_unit->getCurrentHp(), m_unit->getMaxHp());
    drawBar(renderer, m_font, x, y, barW, barH,
            m_unit->getMaxHp() > 0 ? static_cast<float>(m_unit->getCurrentHp()) / static_cast<float>(m_unit->getMaxHp()) : 0.0f,
            Color{80, 200, 90, 255}, buf);
    y += barH + 6.0f;

    std::snprintf(buf, sizeof(buf), "MP %d/%d", m_unit->getCurrentMp(), m_unit->getMaxMp());
    drawBar(renderer, m_font, x, y, barW, barH,
            m_unit->getMaxMp() > 0 ? static_cast<float>(m_unit->getCurrentMp()) / static_cast<float>(m_unit->getMaxMp()) : 0.0f,
            Color{80, 140, 230, 255}, buf);
    y += barH + 10.0f;

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