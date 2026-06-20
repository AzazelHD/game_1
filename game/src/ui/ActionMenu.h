#pragma once
#include <vector>
#include <string>

// ActionMenu shows the actions available for the active unit: Move, Attack, Wait.
// Navigation is keyboard-only.
//
// TODO: Implement the class with:
//   - show(std::vector<std::string> options): populate m_options, set m_selectedIndex = 0, m_visible = true.
//   - hide(): m_visible = false.
//   - update(dt): if Up/Down pressed, change m_selectedIndex (wrap around).
//                 If Confirm pressed, call m_onConfirm(m_selectedIndex).
//                 If Cancel pressed, call m_onCancel().
//   - render(): draw a box with each option listed. Highlight the selected entry.
//   - setOnConfirm(std::function<void(int)>), setOnCancel(std::function<void()>)
//
// Using std::function for callbacks keeps the menu decoupled from BattleState.
// BattleState sets the callbacks when it shows the menu.
class ActionMenu
{
public:
    // TODO: implement

private:
    std::vector<std::string> m_options;
    int m_selectedIndex = 0;
    bool m_visible = false;
};
