#pragma once

#include "engine/ui/Button.h"
#include "engine/ui/FocusGroup.h"
#include "engine/ui/IFocusable.h"
#include "inventory/Gear.h"
#include "inventory/Inventory.h"
#include "ui/UIKeyRepeat.h"
#include "ui/UIWindow.h"
#include "ui/WindowId.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class Font;
class Renderer;
class GearCatalog;

// ShopWindow implements a single town-shop list: either the Buy list (what
// the shop stocks, priced at goldValue * markup) or the Sell list (inventory
// items the shop will buy, worth goldValue each). The player picks which mode
// to enter from the shop hub scene, so a window never toggles between them.
//
// The owning scene performs the actual gold/inventory transactions on
// ActionSelected (BuyItem/SellItem + row index) and calls refresh() when they
// settle.
class ShopWindow final : public UIWindow
{
public:
    enum class Mode
    {
        Buy,
        Sell,
    };

    // Buy list row — an item the shop stocks.
    struct BuyRow
    {
        const Gear *gear = nullptr;
        int price = 0;
    };

    // Sell list row — an inventory item.
    struct SellRow
    {
        ItemId itemId = 0;
        std::string name;
        std::string description;
        std::uint32_t quantity = 0;
        std::uint32_t equippedCount = 0; // copies currently worn by the party
        int price = 0;
    };

    ShopWindow(WindowId id, Mode mode, const GearCatalog &catalog, const Inventory &inventory,
               std::function<int()> goldProvider, const std::vector<const Gear *> &stock,
               std::function<std::uint32_t(ItemId)> equippedProvider);

    void setFont(const Font *font) { m_font = font; }
    Mode mode() const { return m_mode; }

    // Rebuilds the displayed list after a transaction changed the inventory
    // or the purse. Keeps the selection as best it can.
    void refresh();

    void handleInput(const Input &input) override;
    void update(float dt) override;
    void render(Renderer *renderer) const override;

    const BuyRow &buyRow(std::size_t index) const { return m_buyRows.at(index); }
    const SellRow &sellRow(std::size_t index) const { return m_sellRows.at(index); }
    std::size_t buyCount() const { return m_buyRows.size(); }
    std::size_t sellCount() const { return m_sellRows.size(); }

private:
    void rebuildRows();

    const GearCatalog *m_catalog = nullptr;
    const Inventory *m_inventory = nullptr;
    std::function<int()> m_goldProvider;
    std::function<std::uint32_t(ItemId)> m_equippedProvider;
    std::vector<const Gear *> m_stock;

    Mode m_mode = Mode::Buy;
    const Font *m_font = nullptr;

    std::vector<BuyRow> m_buyRows;
    std::vector<SellRow> m_sellRows;
    std::vector<std::unique_ptr<Button>> m_focusRows;
    FocusGroup m_focus;
    int m_scroll = 0;

    float m_lastDt = 0.f;
    UIKeyRepeat m_upRepeat;
    UIKeyRepeat m_downRepeat;
};