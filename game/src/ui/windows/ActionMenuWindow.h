#pragma once

#include "ui/UIWindow.h"
#include "engine/math/Vec2.h"

#include <string>
#include <vector>

class Font;

class ActionMenuWindow : public UIWindow
{
public:
    struct Item
    {
        std::string id;
        std::string label;
        bool enabled = true;
    };

    explicit ActionMenuWindow(std::string id);

    void setFont(const Font *font) { m_font = font; }
    void setItems(std::vector<Item> items);
    void setPanelPosition(Vec2f topLeft)
    {
        m_panelPos = topLeft;
        m_useCustomPanelPos = true;
    }
    void clearPanelPosition() { m_useCustomPanelPos = false; }
    // Keep the panel horizontally centered on screen while using m_panelPos.y
    // for its vertical placement. Lets the box grow width-wise (with the font)
    // and stay centered regardless of the resulting width.
    void centerHorizontally(bool on = true) { m_centerX = on; }

    void handleInput(const Input &input) override;
    void update(float dt) override;
    void render(Renderer *renderer) const override;

private:
    void moveSelection(int delta);

    std::vector<Item> m_items;
    int m_selected = 0;
    int m_scroll = 0;
    Vec2f m_panelPos{0.0f, 0.0f};
    bool m_useCustomPanelPos = false;
    bool m_centerX = false;
    const Font *m_font = nullptr;
};
