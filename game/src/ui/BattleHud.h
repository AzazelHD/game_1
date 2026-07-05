#pragma once

#include "ui/BattleMenuItem.h"

#include <vector>

class UIManager;

class BattleHud
{
public:
    explicit BattleHud(UIManager &uiManager) : m_uiManager(uiManager) {}

    void setItems(std::vector<BattleMenuItem> items, bool combatPhase);
    void clear();
    bool isOpen() const;

    const std::vector<BattleMenuItem> &items() const { return m_items; }

private:
    UIManager &m_uiManager;
    std::vector<BattleMenuItem> m_items;
};