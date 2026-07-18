#pragma once

#include "ui/WindowId.h"
#include "ui/UIWindow.h"

#include <string>

class Font;

class ConfirmWindow : public UIWindow
{
public:
    explicit ConfirmWindow(WindowId id);

    void setFont(const Font *font) { m_font = font; }
    void setPrompt(std::string prompt) { m_prompt = std::move(prompt); }

    void handleInput(const Input &input) override;
    void update(float dt) override;
    void render(Renderer *renderer) const override;

private:
    const Font *m_font = nullptr;
    std::string m_prompt = "Confirm?";
    bool m_confirmSelected = true;
};
