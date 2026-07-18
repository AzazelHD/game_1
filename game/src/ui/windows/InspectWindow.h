#pragma once

#include "ui/UIWindow.h"
#include "ui/WindowId.h"

#include <string>
#include <vector>

class Font;
struct UnitData;

class InspectWindow : public UIWindow
{
public:
    explicit InspectWindow(WindowId id);

    void setFont(const Font *font) { m_font = font; }
    void setTitle(std::string title) { m_title = std::move(title); }
    void setLines(std::vector<std::string> lines);

    // Builds the full stat-dump line list from a unit's data.
    static std::vector<std::string> buildLines(const UnitData &data);

    void handleInput(const Input &input) override;
    void update(float dt) override;
    void render(Renderer *renderer) const override;

private:
    const Font *m_font = nullptr;
    std::string m_title = "Inspect";
    std::vector<std::string> m_lines;
    int m_scroll = 0;
};