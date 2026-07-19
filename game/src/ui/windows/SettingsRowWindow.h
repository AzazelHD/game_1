#pragma once

#include "ui/UIWindow.h"
#include "ui/ActionId.h"
#include "engine/ui/Slider.h" // for Slider*
#include <functional>
#include <string>
#include <vector>

class Font;
class Input;
class Renderer;

class SettingsRowWindow : public UIWindow
{
public:
    enum class RowType
    {
        Button,
        Setting,
        Slider
    };

    struct RowItem
    {
        std::string label;
        RowType type;

        // ── Button row ────────────────────────────────────────────────
        ActionId actionId = ActionId::None; // emitted on Accept

        // ── Setting row (label + left/right-adjustable value) ─────────
        std::function<std::string()> getDisplayValue; // called each frame
        std::function<void(bool right)> onAdjust;     // left/right handler

        // ── Slider row ────────────────────────────────────────────────
        Slider *slider = nullptr; // owned by state
    };

    explicit SettingsRowWindow(WindowId id);

    void setFont(const Font *font) { m_font = font; }
    void setRows(std::vector<RowItem> rows);

    // UIWindow overrides
    void handleInput(const Input &input) override;
    void update(float dt) override;
    void render(Renderer *renderer) const override;

private:
    void moveFocus(int delta);
    void adjustFocusedSetting(bool right);
    void adjustFocusedSlider(bool right);
    void activateFocusedButton();

    std::vector<RowItem> m_rows;
    int m_focusedIndex = 0;
    const Font *m_font = nullptr;
};