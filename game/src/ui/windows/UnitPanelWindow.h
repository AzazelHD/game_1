#pragma once

#include "ui/UIWindow.h"

#include <utility>

class Unit;
class Font;

class UnitPanelWindow : public UIWindow
{
public:
    explicit UnitPanelWindow(std::string id);

    void setFont(const Font *font) { m_font = font; }
    void setTurnInfo(const Unit *activeUnit, int round);
    void setSingle(const Unit *unit, int team);
    void setDuel(const Unit *left, const Unit *right, bool rightIsEnemy);
    void setPreview(std::string name, int level, int hp, int mp, int team, bool isPlaced = false);
    void clearPreview();
    void clearPanels();

    void handleInput(const Input &input) override;
    void update(float dt) override;
    void render(Renderer *renderer) const override;

private:
    void renderTurnBanner(Renderer *renderer) const;

    const Font *m_font = nullptr;

    const Unit *m_turnUnit = nullptr;
    int m_round = 0;

    const Unit *m_leftUnit = nullptr;
    const Unit *m_rightUnit = nullptr;
    int m_singleTeam = 2;
    bool m_rightIsEnemy = true;

    bool m_hasPreview = false;
    std::string m_previewName;
    int m_previewLevel = 1;
    int m_previewHp = 1;
    int m_previewMp = 0;
    int m_previewTeam = 2;
    bool m_previewPlaced = false;
};