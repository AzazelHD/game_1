#pragma once

#include "engine/math/Vec2.h"
#include "engine/math/Rect.h"
#include "engine/renderer/Color.h"
#include "config/GameConstants.h"

class Unit;
class Font;
class Renderer;

// UnitPanel displays a selected unit's stats in a corner of the screen.
// Appears when the cursor hovers over a unit; disappears when no unit is selected.
//
// [x]: Implement the class with:
//   - show(const Unit* unit): store the unit pointer, set m_visible = true.
//   - hide(): m_visible = false.
//   - render(): draw a panel box in the bottom-left corner.
//               Display: name, HP bar (current/max), team indicator.
//               For now draw colored rects for bars — text rendering comes later.
//
// Note: UnitPanel holds a non-owning const pointer. It must never delete the unit.
class UnitPanel
{
public:
    void show(const Unit *unit);
    void hide();
    bool isVisible() const { return m_visible; }

    void setPosition(Vec2f pos) { m_panelPos = pos; }
    void setTeamColor(Color color) { m_teamColor = color; }

    void setFont(const Font *font) { m_font = font; }
    void setTurnInfo(int round, const Unit *activeUnit);

    void render(Renderer *renderer) const;

private:
    void renderStatsPanel(Renderer *renderer) const;
    void renderTurnBanner(Renderer *renderer) const;

    const Unit *m_unit = nullptr;
    bool m_visible = false;

    int m_round = 1;
    const Font *m_font = nullptr;
    const Unit *m_activeUnit = nullptr;

    Vec2f m_panelPos{16.0f, GameConstants::VIEW_H - 116.0f - 16.0f};
    Color m_teamColor = {180, 180, 180, 255};

    static constexpr float PORTRAIT_W = 20.0f;
    static constexpr float PORTRAIT_H = 30.0f;
};