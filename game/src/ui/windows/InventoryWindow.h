#pragma once

#include "engine/ui/FocusGroup.h"
#include "engine/ui/IFocusable.h"
#include "inventory/GearCatalog.h"
#include "inventory/Inventory.h"
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
};
