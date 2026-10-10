#pragma once

#include "engine/ui/ButtonControl.h"
#include "engine/ui/FocusGroup.h"
#include "ui/WindowId.h"
#include "ui/UIWindow.h"

#include <memory>
#include <string>

class Font;

// ConfirmWindow — a modal question with exactly two answers: Cancel and
// Confirm.
//
// There is ONE appearance by design. Every confirm in the game handles input,
// emits ConfirmResult and renders identically, and differs only in content
// (prompt text, button labels), so those differences are passed in as data
// rather than expressed as subclasses. A subclass per appearance would override
// nothing while every call site had to know which class to pick.
//
// Promote one to a subclass only when a variant needs genuinely different
// BEHAVIOUR — extra buttons, a text field, an auto-confirm countdown. Until
// then, prefer a factory (see ui/ConfirmDialogs.h) so a given question is
// worded in one place instead of copy-pasted at each site that asks it.
class ConfirmWindow : public UIWindow
{
public:
    explicit ConfirmWindow(WindowId id);

    void setFont(const Font *font) { m_font = font; }
    // Optional: buttons/prompts can differ in size (e.g. a heading-sized
    // prompt with body-sized buttons). Falls back to setFont() when unset.
    void setButtonFont(const Font *font) { m_buttonFont = font; }
    void setPrompt(std::string prompt) { m_prompt = std::move(prompt); }

    void handleInput(const Input &input) override;
    void update(float dt) override;
    void render(Renderer *renderer) const override;

    void setButtonLabels(std::string cancelLabel, std::string confirmLabel)
    {
        m_cancelLabelText = std::move(cancelLabel);
        m_confirmLabelText = std::move(confirmLabel);
    }

private:
    const Font *m_font = nullptr;
    const Font *m_buttonFont = nullptr;
    std::string m_prompt = "???";

    std::unique_ptr<ButtonControl> m_cancelButton;
    std::unique_ptr<ButtonControl> m_confirmButton;
    FocusGroup m_focus;

    std::string m_cancelLabelText = "Cancel";
    std::string m_confirmLabelText = "Confirm";
};
