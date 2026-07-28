#pragma once

#include "ui/BattleMenuItem.h"

#include <string>
#include <vector>

class UIManager;
class ButtonMenuWindow;
class ConfirmWindow;
class UnitInspectWindow;
class Unit;

class BattleUIManager
{
public:
    explicit BattleUIManager(UIManager &ui);

    // Action menu
    void showActionMenu(std::vector<BattleMenuItem> items);
    void hideActionMenu();
    const std::vector<BattleMenuItem> &actionMenuItems() const;
    void showSystemMenu(std::vector<BattleMenuItem> items);
    void hideSystemMenu();

    // Inspect
    void showInspect(Unit *unit);
    void hideInspect();

    // Confirm
    void showConfirm(const std::string &text);
    void hideConfirm();

    // Cleanup
    bool hasBlockingWindow() const;
    void clear();

private:
    ButtonMenuWindow *ensureActionMenu();
    ButtonMenuWindow *ensureSystemMenu();
    UnitInspectWindow *ensureInspectWindow();
    ConfirmWindow *ensureConfirmWindow();

    UIManager &m_ui;

    std::vector<BattleMenuItem> m_actionMenuItems;

    ButtonMenuWindow *m_actionMenu = nullptr;
    ButtonMenuWindow *m_systemMenu = nullptr;
    UnitInspectWindow *m_inspect = nullptr;
    ConfirmWindow *m_confirm = nullptr;
};
