#pragma once

#include "engine/ui/FocusGroup.h"
#include "engine/ui/Insets.h"
#include "ui/UIWindow.h"

#include <memory>
#include <string>
#include <vector>
#include <optional>

class Font;
class Input;
class Renderer;
class IRowControl;

class SettingsRowWindow : public UIWindow
{
public:
    struct RowItem
    {
        std::string label; // empty for full-width controls (e.g. buttons)
        std::unique_ptr<IRowControl> control;
        std::optional<Insets> padding;
    };

    explicit SettingsRowWindow(WindowId id);

    void setFont(const Font *font) { m_font = font; }
    void setRows(std::vector<RowItem> rows);

    void handleInput(const Input &input) override;
    void update(float dt) override;
    void render(Renderer *renderer) const override;

private:
    std::vector<RowItem> m_rows;
    FocusGroup m_focus;
    const Font *m_font = nullptr;
};
