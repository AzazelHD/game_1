#pragma once

#include "ui/windowId.h"
#include "ui/UIWindow.h"

#include <string>
#include <vector>

class Font;
class Renderer;

class PartyWindow : public UIWindow
{
public:
    struct Entry
    {
        std::string name;
        bool hasSprite = false;
    };

    explicit PartyWindow(WindowId id);

    void setFont(const Font *font) { m_font = font; }
    void setEntries(std::vector<Entry> entries);

    void handleInput(const Input &input) override;
    void update(float dt) override;
    void render(Renderer *renderer) const override;

private:
    const Font *m_font = nullptr;
    std::vector<Entry> m_entries;
    int m_selected = 0;
    int m_scroll = 0;
};
