#pragma once

class Unit;
struct SDL_Renderer;

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

    // Pass activeUnit = nullptr to hide the banner.
    void setTurnInfo(int round, const Unit *activeUnit);

    void render(SDL_Renderer *renderer) const;

private:
    void renderStatsPanel(SDL_Renderer *renderer) const;
    void renderTurnBanner(SDL_Renderer *renderer) const;

    const Unit *m_unit = nullptr;
    bool m_visible = false;

    int m_round = 1;
    const Unit *m_activeUnit = nullptr;
};