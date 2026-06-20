#include "ui/UnitPanel.h"

#include "battle/Unit.h"
#include "config/GameConstants.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdio>

namespace
{
    constexpr SDL_Color PANEL_BG = {12, 12, 20, 215};
    constexpr SDL_Color PANEL_BORDER = {180, 180, 180, 255};
    constexpr SDL_Color TEXT_COLOR = {235, 235, 235, 255};

    SDL_Color teamColor(int team)
    {
        switch (team)
        {
        case 0:
            return SDL_Color{64, 128, 255, 255};
        case 1:
            return SDL_Color{64, 255, 64, 255};
        default:
            return SDL_Color{255, 64, 64, 255};
        }
    }

    void drawPanel(SDL_Renderer *renderer, SDL_FRect rect)
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, PANEL_BG.r, PANEL_BG.g, PANEL_BG.b, PANEL_BG.a);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, PANEL_BORDER.r, PANEL_BORDER.g, PANEL_BORDER.b, PANEL_BORDER.a);
        SDL_RenderRect(renderer, &rect);
    }

    // Background + proportional fill + border + label, e.g. "HP 24/30".
    void drawBar(SDL_Renderer *renderer, float x, float y, float w, float h,
                 float ratio, SDL_Color fillColor, const char *label)
    {
        ratio = std::clamp(ratio, 0.0f, 1.0f);

        SDL_FRect bg{x, y, w, h};
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_RenderFillRect(renderer, &bg);

        SDL_FRect fill{x, y, w * ratio, h};
        SDL_SetRenderDrawColor(renderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
        SDL_RenderFillRect(renderer, &fill);

        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderRect(renderer, &bg);

        SDL_SetRenderDrawColor(renderer, TEXT_COLOR.r, TEXT_COLOR.g, TEXT_COLOR.b, TEXT_COLOR.a);
        SDL_RenderDebugText(renderer, x + 4.0f, y + (h - 8.0f) * 0.5f, label);
    }

    void drawText(SDL_Renderer *renderer, float x, float y, SDL_Color color, const char *text)
    {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderDebugText(renderer, x, y, text);
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

void UnitPanel::render(SDL_Renderer *renderer) const
{
    if (!renderer)
        return;

    renderTurnBanner(renderer);

    if (m_visible && m_unit)
        renderStatsPanel(renderer);
}

void UnitPanel::renderStatsPanel(SDL_Renderer *renderer) const
{
    constexpr float PANEL_X = 16.0f;
    constexpr float PANEL_W = 300.0f;
    constexpr float PANEL_H = 116.0f;
    const float panelY = GameConstants::VIEW_H - PANEL_H - 16.0f;

    drawPanel(renderer, SDL_FRect{PANEL_X, panelY, PANEL_W, PANEL_H});

    const float pad = 10.0f;
    float x = PANEL_X + pad;
    float y = panelY + pad;

    // ── Name / level / team swatch ────────────────────────────────────────
    char buf[96];
    std::snprintf(buf, sizeof(buf), "%s  Lv%d", m_unit->getName().c_str(), m_unit->getLevel());
    drawText(renderer, x, y, TEXT_COLOR, buf);

    const SDL_Color tColor = teamColor(m_unit->getTeam());
    SDL_FRect swatch{PANEL_X + PANEL_W - pad - 14.0f, y - 2.0f, 14.0f, 14.0f};
    SDL_SetRenderDrawColor(renderer, tColor.r, tColor.g, tColor.b, tColor.a);
    SDL_RenderFillRect(renderer, &swatch);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &swatch);

    y += 18.0f;

    // ── HP / MP bars ────────────────────────────────────────────────────
    const float barW = PANEL_W - pad * 2.0f;
    const float barH = 14.0f;

    std::snprintf(buf, sizeof(buf), "HP %d/%d", m_unit->getCurrentHp(), m_unit->getMaxHp());
    drawBar(renderer, x, y, barW, barH,
            m_unit->getMaxHp() > 0 ? static_cast<float>(m_unit->getCurrentHp()) / static_cast<float>(m_unit->getMaxHp()) : 0.0f,
            SDL_Color{80, 200, 90, 255}, buf);
    y += barH + 6.0f;

    std::snprintf(buf, sizeof(buf), "MP %d/%d", m_unit->getCurrentMp(), m_unit->getMaxMp());
    drawBar(renderer, x, y, barW, barH,
            m_unit->getMaxMp() > 0 ? static_cast<float>(m_unit->getCurrentMp()) / static_cast<float>(m_unit->getMaxMp()) : 0.0f,
            SDL_Color{80, 140, 230, 255}, buf);
    y += barH + 10.0f;

    // ── Core stats, two columns ────────────────────────────────────────
    std::snprintf(buf, sizeof(buf), "ATK %d   DEF %d   MAG %d", m_unit->getAttack(), m_unit->getDefense(), m_unit->getMagic());
    drawText(renderer, x, y, TEXT_COLOR, buf);
    y += 16.0f;

    std::snprintf(buf, sizeof(buf), "MDF %d   SPD %d   MOV %d   RNG %d",
                  m_unit->getMagicDefense(), m_unit->getSpeed(), m_unit->getMoveRange(), m_unit->getAtkRange());
    drawText(renderer, x, y, TEXT_COLOR, buf);
}

void UnitPanel::renderTurnBanner(SDL_Renderer *renderer) const
{
    if (!m_activeUnit)
        return;

    constexpr float BANNER_W = 360.0f;
    constexpr float BANNER_H = 28.0f;
    const float bannerX = GameConstants::VIEW_CX - BANNER_W * 0.5f;
    constexpr float bannerY = 8.0f;

    drawPanel(renderer, SDL_FRect{bannerX, bannerY, BANNER_W, BANNER_H});

    char buf[96];
    std::snprintf(buf, sizeof(buf), "Round %d - %s's Turn (Team %d)",
                  m_round, m_activeUnit->getName().c_str(), m_activeUnit->getTeam());
    drawText(renderer, bannerX + 10.0f, bannerY + (BANNER_H - 8.0f) * 0.5f, TEXT_COLOR, buf);
}