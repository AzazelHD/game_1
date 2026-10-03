#pragma once

#include "ui/UIKeyRepeat.h"
#include "ui/UIWindow.h"
#include "ui/ActionId.h"
#include "ui/WindowId.h"
#include "engine/math/Vec2.h"
#include "engine/ui/Button.h"
#include "engine/ui/FocusGroup.h"

#include <memory>
#include <string>
#include <vector>

class Font;

class ButtonMenuWindow : public UIWindow
{
public:
    struct Item
    {
        ActionId id = ActionId::None;
        std::string label;
        bool enabled = true;
        // Longer text revealed with Details (Tab) while this row is
        // highlighted. Empty = no detail available.
        std::string detail;
    };

    enum class TextAlign
    {
        Left,
        Center,
        Right,
    };

    explicit ButtonMenuWindow(WindowId id);

    void setFont(const Font *font) { m_font = font; }
    void setTextAlign(TextAlign align) { m_textAlign = align; }

    // Scrollable (default): the panel is capped at a bounded height and the
    // list scrolls to reveal items past the fold. When disabled, the panel
    // grows just tall enough to show every row — nothing scrolls.
    void setScrollable(bool on) { m_scrollable = on; }
    bool scrollable() const { return m_scrollable; }

    // Wrappable (default): navigating past the last row wraps around to the
    // first (and vice-versa). When disabled, focus stops at the top/bottom
    // instead of cycling.
    void setWrappable(bool on)
    {
        m_wrappable = on;
        m_focus.setWrap(on);
    }
    bool wrappable() const { return m_wrappable; }
    void setItems(std::vector<Item> items);

    // Restores keyboard focus to `index` right after setItems() (a rebuilt
    // list starts on its first enabled row). Disabled rows are skipped, so a
    // now-unusable row (e.g. skills the caster no longer has MP for) gracefully
    // falls back to the nearest enabled one.
    void setInitialSelectedIndex(int index);
    void setPanelPosition(Vec2f topLeft)
    {
        m_panelPos = topLeft;
        m_useCustomPanelPos = true;
        m_anchorBottomRight = false;
    }
    void setAnchorBottomRight(Vec2f margin = Vec2f{16.0f, 16.0f})
    {
        m_anchorBottomRight = true;
        m_useCustomPanelPos = false;
        m_bottomRightMargin = margin;
    }
    void clearPanelPosition()
    {
        m_useCustomPanelPos = false;
        m_anchorBottomRight = false;
    }

    // Keep the panel horizontally centered on screen while using m_panelPos.y
    // for its vertical placement. Lets the box grow width-wise (with the font)
    // and stay centered regardless of the resulting width.
    void centerHorizontally(bool on = true) { m_centerX = on; }

    // Enables delete-style lists: each row renders a red "X" at the right edge
    // and pressing X emits ActionSelected(deleteAction, selected_index). The
    // hint is rendered as a footer under the panel. Passing ActionId::None
    // disables the mode.
    void setDeleteMode(ActionId deleteAction, std::string hintLabel)
    {
        m_deleteAction = deleteAction;
        m_deleteHint = std::move(hintLabel);
    }

    bool hasDeleteMode() const { return m_deleteAction != ActionId::None; }

    // Short footer hint rendered under the panel, e.g. "Tab: Details".
    void setHintLine(std::string hint) { m_hintLine = std::move(hint); }

    // Details (Tab): rows whose `detail` is non-empty report themselves via an
    // ActionSelected event carrying ActionId::Details and the row index, so the
    // owner decides where the text is shown (the battle scene puts it in its
    // top battle text). hasDetails() tells the owner whether the key is worth
    // advertising at all.
    bool hasDetails() const;
    [[nodiscard]] int selectedIndex() const { return m_focus.getSelectedIndex(); }
    [[nodiscard]] std::string detailAt(int index) const;

    // Emits an ActionChanged/ActionId::Details event whenever the highlighted
    // row moves. Used to keep a visible description following the cursor
    // without re-reading the whole menu each frame.
    void notifyRowChanged(int previousIndex);

    void handleInput(const Input &input) override;
    void update(float dt) override;
    void render(Renderer *renderer) const override;

private:
    struct MenuGeometry
    {
        float menuH = 0.0f;
        int visibleCount = 0;
        bool isScrollMenu = true;
    };

    MenuGeometry computeGeometry(float ui, int count, float itemH) const;
    void keepSelectionVisible();
    bool canClampStep(int selected, int direction) const;

    std::vector<Item> m_items;
    std::vector<std::unique_ptr<Button>> m_focusRows;
    FocusGroup m_focus;
    int m_scroll = 0;
    Vec2f m_panelPos{0.0f, 0.0f};
    bool m_useCustomPanelPos = false;
    bool m_anchorBottomRight = false;
    Vec2f m_bottomRightMargin{16.0f, 16.0f};
    bool m_centerX = false;
    const Font *m_font = nullptr;
    TextAlign m_textAlign = TextAlign::Center;
    ActionId m_deleteAction = ActionId::None;
    std::string m_deleteHint;
    std::string m_hintLine;
    float m_lastDt = 0.0f;
    mutable float m_lastItemH = 0.0f;
    bool m_scrollable = true;
    bool m_wrappable = true;
    UIKeyRepeat m_upRepeat;
    UIKeyRepeat m_downRepeat;
};
