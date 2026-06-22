#pragma once
#include <vector>
#include <string>
#include <functional>

class Renderer;
class Font;

// ActionMenu shows the actions available for the active unit: Move, Attack, Wait.
// Navigation is keyboard-only.
//
// [x]: Implemented with:
//   - show(std::vector<std::string> options): populate m_options, set m_selectedIndex = 0, m_visible = true.
//   - hide(): m_visible = false.
//   - update(dt): if Up/Down pressed, change m_selectedIndex (wrap around).
//                 If Confirm pressed, call m_onConfirm(m_selectedIndex).
//                 If Cancel pressed, call m_onCancel().
//   - render(Renderer*, const Font*): draw a box with each option listed. Highlight the selected entry.
//   - setOnConfirm(std::function<void(int)>), setOnCancel(std::function<void()>)
//
// Using std::function for callbacks keeps the menu decoupled from BattleState.
// BattleState sets the callbacks when it shows the menu.
class ActionMenu
{
public:
    void show(std::vector<std::string> options, std::vector<bool> enabled = {});
    void hide();
    bool isVisible() const { return m_visible; }

    void update(float dt);
    void render(Renderer *renderer, const Font *font) const;

    void setOnConfirm(std::function<void(int)> callback) { m_onConfirm = callback; }
    void setOnCancel(std::function<void()> callback) { m_onCancel = callback; }

    void setSelectedIndex(int index)
    {
        if (index >= 0 && index < static_cast<int>(m_options.size()))
            m_selectedIndex = index;
    }
    void simulateAccept()
    {
        if (m_onConfirm)
            m_onConfirm(m_selectedIndex);
        hide();
    }

private:
    std::vector<std::string> m_options;
    std::vector<bool> m_enabled;
    int m_selectedIndex = 0;
    bool m_visible = false;
    bool m_justShown = false;

    std::function<void(int)> m_onConfirm;
    std::function<void()> m_onCancel;
};