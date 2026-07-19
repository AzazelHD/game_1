#pragma once

#include "ui/BattleMenuItem.h"

#include <vector>

class UIManager;
class ButtonMenuWindow;

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
    // Created once on first setItems(), then only shown/hidden. Never
    // popped, so this pointer stays valid for BattleHud's lifetime.
    ButtonMenuWindow *m_menu = nullptr;
};