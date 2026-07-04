#pragma once

#include "ui/UIWindow.h"

#include <string>
#include <vector>

class Font;

class DialogWindow : public UIWindow
{
public:
    struct Line
    {
        std::string speaker;
        std::string text;
    };

    explicit DialogWindow(std::string id);

    void setFont(const Font *font) { m_font = font; }
    void start(std::vector<Line> lines);

    void handleInput(const Input &input) override;
    void update(float dt) override;
    void render(Renderer *renderer) const override;

private:
    bool isCurrentLineFinished() const;

    const Font *m_font = nullptr;
    std::vector<Line> m_lines;
    int m_lineIndex = 0;
    int m_visibleChars = 0;
    float m_timer = 0.0f;
};
