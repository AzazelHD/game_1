#pragma once

#include "engine/ui/FocusGroup.h"
#include "engine/ui/IFocusable.h"
#include "inventory/GearCatalog.h"
#include "inventory/Inventory.h"
#include "ui/UIKeyRepeat.h"
#include "ui/UIWindow.h"
#include "ui/WindowId.h"

#include <memory>
#include <string>
#include <vector>

class Font;
class Renderer;

class InventoryWindow final : public UIWindow
{
public:
    struct ItemRow
    {
        ItemId itemId = 0;
        std::string name;
        std::string description;
        uint32_t quantity = 0;
    };

    InventoryWindow(WindowId id, const Inventory &inventory, const GearCatalog &catalog);

    void setFont(const Font *font) { m_font = font; }

    void handleInput(const Input &input) override;
    void update(float dt) override;
    void render(Renderer *renderer) const override;

private:
    void rebuildRows();

    const Inventory *m_inventory = nullptr;
    const GearCatalog *m_catalog = nullptr;
    const Font *m_font = nullptr;

    std::vector<ItemRow> m_rows;
    int m_selectedIndex = 0;
    int m_scroll = 0;

    // Most recent update() dt, cached so handleInput() (which the engine
    // dispatches before update() on the same frame) can drive the
    // hold-to-repeat trackers.
    float m_lastDt = 0.f;

    // Hold-to-repeat trackers for list navigation (Part K). The window
    // also accepts page jumps via A/D; those remain single-step because
    // they're intentional big moves, not continuous browsing.
    UIKeyRepeat m_upRepeat;
    UIKeyRepeat m_downRepeat;
    UIKeyRepeat m_leftRepeat;
    UIKeyRepeat m_rightRepeat;
};
