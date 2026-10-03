#include "ui/windows/ShopWindow.h"

#include "config/GameConstants.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Aligment.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/FontManager.h"
#include "engine/renderer/Renderer.h"
#include "engine/ui/Insets.h"
#include "engine/ui/TextBlock.h"
#include "inventory/GearCatalog.h"
#include "ui/ActionId.h"
#include "ui/UITheme.h"
#include "ui/UIUtils.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kPanelW = 600.0f;
    constexpr float kPanelH = 440.0f;
    constexpr float kHeaderH = 36.0f;
    constexpr float kFooterH = 28.0f;
    constexpr float kRowH = 28.0f;
    constexpr int kVisibleRows = 6;

    // The shop sells stock items for a markup over their goldValue; it buys
    // the party's items at goldValue exactly.
    constexpr int kBuyMarkup = 2;

    const char *kPlaceholderLorem =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
        "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
        "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris.";
}

ShopWindow::ShopWindow(WindowId id, Mode mode, const GearCatalog &catalog, const Inventory &inventory,
                       std::function<int()> goldProvider, const std::vector<const Gear *> &stock,
                       std::function<std::uint32_t(ItemId)> equippedProvider)
    : UIWindow(id, true, false),
      m_catalog(&catalog),
      m_inventory(&inventory),
      m_goldProvider(std::move(goldProvider)),
      m_equippedProvider(std::move(equippedProvider)),
      m_stock(stock),
      m_mode(mode)
{
    refresh();
}

void ShopWindow::refresh()
{
    m_buyRows.clear();
    m_sellRows.clear();

    if (m_catalog)
    {
        for (const Gear *gear : m_stock)
        {
            if (!gear || gear->goldValue <= 0)
                continue;
            m_buyRows.push_back(BuyRow{.gear = gear, .price = gear->goldValue * kBuyMarkup});
        }
    }

    if (m_inventory)
    {
        for (const ItemStack &stack : m_inventory->stacks())
        {
            std::string name = "Item #" + std::to_string(stack.itemId);
            std::string desc = kPlaceholderLorem;
            int price = 0;

            if (m_catalog)
            {
                if (const Gear *gear = m_catalog->find(stack.itemId))
                {
                    if (gear->key.rfind("debug_", 0) == 0)
                        continue; // debug/test gear is not merchandisable

                    name = gear->displayName;
                    price = gear->goldValue;
                    if (!gear->description.empty())
                    {
                        desc = gear->description;
                        if (gear->description.size() < 140)
                        {
                            desc += " ";
                            desc += kPlaceholderLorem;
                        }
                    }
                }
            }

            if (price <= 0)
                continue; // shop will not buy items it can't value

            const std::uint32_t equippedCount = m_equippedProvider ? m_equippedProvider(stack.itemId) : 0;
            if (equippedCount > 0 && desc.size() < 140)
                desc += " (Equipped — cannot be sold while in use)";

            m_sellRows.push_back(SellRow{
                .itemId = stack.itemId,
                .name = std::move(name),
                .description = std::move(desc),
                .quantity = stack.quantity,
                .equippedCount = equippedCount,
                .price = price,
            });
        }
    }

    rebuildRows();
}

void ShopWindow::rebuildRows()
{
    m_focusRows.clear();
    const int count = (m_mode == Mode::Buy) ? static_cast<int>(m_buyRows.size())
                                            : static_cast<int>(m_sellRows.size());
    m_focusRows.reserve(static_cast<std::size_t>(count));

    std::vector<IFocusable *> focusableRows;
    focusableRows.reserve(static_cast<std::size_t>(count));

    for (int i = 0; i < count; ++i)
    {
        auto row = std::make_unique<Button>(Rectf{0.0f, 0.0f, 0.0f, 0.0f}, "");
        row->setOnClick([this]()
                        {
                        });
        focusableRows.push_back(row.get());
        m_focusRows.push_back(std::move(row));
    }

    m_focus.resetFromPointers(std::move(focusableRows));
    m_scroll = 0;
}

void ShopWindow::handleInput(const Input &input)
{
    const int count = (m_mode == Mode::Buy) ? static_cast<int>(m_buyRows.size())
                                            : static_cast<int>(m_sellRows.size());

    const bool upHit = m_upRepeat.tick(m_lastDt, input.isKeyDown(KeyCode::Up) || input.isKeyDown(KeyCode::W));
    const bool downHit = m_downRepeat.tick(m_lastDt, input.isKeyDown(KeyCode::Down) || input.isKeyDown(KeyCode::S));

    if (count > 0)
    {
        const int selected = m_focus.getSelectedIndex();

        if (upHit)
            m_focus.focusPrevious(); // FocusGroup wraps
        else if (downHit)
            m_focus.focusNext(); // FocusGroup wraps

        const int current = m_focus.getSelectedIndex();
        if (current < m_scroll)
            m_scroll = current;
        if (current >= m_scroll + kVisibleRows)
            m_scroll = current - kVisibleRows + 1;
    }

    if (input.isKeyPressed(KeyCode::Accept, false))
    {
        const int selected = m_focus.getSelectedIndex();
        if (selected >= 0 && selected < count)
        {
            // Equipped gear is display-only in the Sell list: the player may
            // not sell a copy that is currently worn, so Enter is a no-op.
            if (m_mode == Mode::Sell)
            {
                const SellRow &row = m_sellRows[static_cast<std::size_t>(selected)];
                if (row.equippedCount > 0)
                    return;
            }

            emit(UIEvent{
                .type = UIEventType::ActionSelected,
                .windowId = id(),
                .actionId = (m_mode == Mode::Buy) ? ActionId::BuyItem : ActionId::SellItem,
                .index = selected,
            });
        }
        return;
    }

    if (input.isKeyPressed(KeyCode::Back, false))
    {
        emit(UIEvent{.type = UIEventType::ActionCanceled, .windowId = id(), .actionId = ActionId::Close});
        return;
    }
}

void ShopWindow::update(float dt)
{
    m_lastDt = dt;
    m_upRepeat.start(0.35f, 0.09f);
    m_downRepeat.start(0.35f, 0.09f);
}

void ShopWindow::render(Renderer *renderer) const
{
    if (!renderer || !m_font)
        return;

    const float panelX = (GameConstants::VIEW_W - kPanelW) * 0.5f;
    const float panelY = (GameConstants::VIEW_H - kPanelH) * 0.5f;

    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(UITheme::PopupBG);
    renderer->fillRect(Rectf{panelX, panelY, kPanelW, kPanelH});
    renderer->setDrawColor(UITheme::PopupBorder);
    renderer->drawRect(Rectf{panelX, panelY, kPanelW, kPanelH});

    const Insets outer{16.0f, 20.0f, 14.0f, 20.0f};
    const float contentX = panelX + outer.left;
    const float contentY = panelY + outer.top;
    const float contentW = kPanelW - outer.left - outer.right;
    const float contentH = kPanelH - outer.top - outer.bottom;

    const Font *titleFont = FontManager::instance().get(FontRole::Heading);
    if (!titleFont)
        titleFont = m_font;

    // Header title + mode note + gold purse
    const char *titleText = (m_mode == Mode::Buy) ? "Shop — Buy" : "Shop — Sell";
    renderer->renderTextInRect(titleFont, titleText,
                               Rectf{contentX, contentY, contentW, kHeaderH},
                               UITheme::SelectedText,
                               HorizontalAlign::Center, VerticalAlign::Middle,
                               false, false, false);

    const int gold = m_goldProvider ? m_goldProvider() : 0;
    renderer->renderTextInRect(m_font, "Gold: " + std::to_string(gold),
                               Rectf{contentX, contentY + 2.0f, contentW - 16.0f, 20.0f},
                               UITheme::Gold,
                               HorizontalAlign::Right, VerticalAlign::Middle,
                               false, false, false);

    const float bodyY = contentY + kHeaderH + 10.0f;
    const float bodyH = contentH - kHeaderH - kFooterH - 12.0f;

    const float topSectionH = std::floor(bodyH * (2.0f / 3.0f));
    const float bottomSectionH = bodyH - topSectionH;

    // Top Section (2/3): scrollable list
    const Rectf listRect{contentX, bodyY, contentW, topSectionH};
    renderer->setDrawColor(Color{30, 36, 48, 180});
    renderer->fillRect(listRect);
    renderer->setDrawColor(Color{60, 75, 95, 255});
    renderer->drawRect(listRect);

    const int count = (m_mode == Mode::Buy) ? static_cast<int>(m_buyRows.size())
                                            : static_cast<int>(m_sellRows.size());

    if (count == 0)
    {
        const std::string emptyText = (m_mode == Mode::Buy) ? "(Nothing for sale)" : "(Nothing to sell)";
        renderer->renderTextInRect(m_font, emptyText,
                                   listRect,
                                   UITheme::Info,
                                   HorizontalAlign::Center, VerticalAlign::Middle,
                                   false, false, false);
    }
    else
    {
        const int start = std::clamp(m_scroll, 0, std::max(0, count - 1));
        const int end = std::min(count, start + kVisibleRows);

        float rowY = bodyY + 6.0f;
        for (int i = start; i < end; ++i)
        {
            const bool isSelected = (i == m_focus.getSelectedIndex());
            const Rectf itemRect{contentX + 6.0f, rowY, contentW - 12.0f, kRowH};

            if (isSelected)
            {
                renderer->setDrawColor(Color{80, 96, 122, 160});
                renderer->fillRect(itemRect);
            }

            std::string name;
            std::string priceText;
            bool showQty = false;
            std::uint32_t quantity = 0;

            if (m_mode == Mode::Buy)
            {
                const BuyRow &row = m_buyRows[static_cast<std::size_t>(i)];
                name = row.gear ? row.gear->displayName : "?";
                priceText = std::to_string(row.price) + "g";
            }
            else
            {
                const SellRow &row = m_sellRows[static_cast<std::size_t>(i)];
                name = row.name;
                priceText = std::to_string(row.price) + "g";
                showQty = true;
                quantity = row.quantity;
            }

            renderer->renderTextInRect(m_font, name,
                                       Rectf{itemRect.x + 10.0f, itemRect.y, itemRect.w * 0.7f, itemRect.h},
                                       isSelected ? UITheme::SelectedText : UITheme::Text,
                                       HorizontalAlign::Left, VerticalAlign::Middle,
                                       false, false, false);

            if (showQty)
            {
                if (m_mode == Mode::Sell)
                {
                    const SellRow &row = m_sellRows[static_cast<std::size_t>(i)];
                    if (row.equippedCount > 0)
                    {
                        renderer->renderTextInRect(m_font, "E",
                                                   Rectf{itemRect.x + itemRect.w * 0.6f - 20.0f, itemRect.y, 18.0f, itemRect.h},
                                                   Color{255, 210, 60, 255},
                                                   HorizontalAlign::Right, VerticalAlign::Middle,
                                                   false, false, false);
                    }
                }
                renderer->renderTextInRect(m_font, "x" + std::to_string(quantity),
                                           Rectf{itemRect.x + itemRect.w * 0.6f, itemRect.y, itemRect.w * 0.15f - 8.0f, itemRect.h},
                                           isSelected ? UITheme::SelectedText : UITheme::Info,
                                           HorizontalAlign::Right, VerticalAlign::Middle,
                                           false, false, false);
            }

            renderer->renderTextInRect(m_font, priceText,
                                       Rectf{itemRect.x + itemRect.w * 0.75f, itemRect.y, itemRect.w * 0.25f - 10.0f, itemRect.h},
                                       isSelected ? UITheme::SelectedText : UITheme::Gold,
                                       HorizontalAlign::Right, VerticalAlign::Middle,
                                       false, false, false);

            rowY += kRowH + 2.0f;
        }

        if (start > 0)
        {
            renderer->renderTextInRect(m_font, "^",
                                       Rectf{listRect.x + listRect.w - 24.0f, listRect.y + 4.0f, 16.0f, 16.0f},
                                       UITheme::SelectedText,
                                       HorizontalAlign::Center, VerticalAlign::Middle,
                                       false, false, false);
        }
        if (end < count)
        {
            renderer->renderTextInRect(m_font, "v",
                                       Rectf{listRect.x + listRect.w - 24.0f, listRect.y + listRect.h - 20.0f, 16.0f, 16.0f},
                                       UITheme::SelectedText,
                                       HorizontalAlign::Center, VerticalAlign::Middle,
                                       false, false, false);
        }
    }

    // Bottom Section (1/3): description panel
    const float descY = bodyY + topSectionH + 8.0f;
    const Rectf descRect{contentX, descY, contentW, bottomSectionH - 8.0f};
    renderer->setDrawColor(Color{25, 30, 42, 200});
    renderer->fillRect(descRect);
    renderer->setDrawColor(Color{60, 75, 95, 255});
    renderer->drawRect(descRect);

    const int selected = m_focus.getSelectedIndex();
    std::string selectedDescription;

    if (m_mode == Mode::Buy)
    {
        if (selected >= 0 && selected < static_cast<int>(m_buyRows.size()))
        {
            const BuyRow &row = m_buyRows[static_cast<std::size_t>(selected)];
            if (row.gear)
            {
                selectedDescription = row.gear->description.empty()
                                          ? kPlaceholderLorem
                                          : row.gear->description;
                selectedDescription += "\nPrice: " + std::to_string(row.price) + " gold";
            }
        }
    }
    else
    {
        if (selected >= 0 && selected < static_cast<int>(m_sellRows.size()))
        {
            const SellRow &row = m_sellRows[static_cast<std::size_t>(selected)];
            selectedDescription = row.description + "\n\nSell each: " + std::to_string(row.price) + " gold";
        }
    }

    if (!selectedDescription.empty())
    {
        const float descW = descRect.w - 24.0f;
        constexpr float kDescLineSpacing = 3.0f;
        TextBlock::render(*renderer,
                          {TextBlock::Line{m_font, selectedDescription, UITheme::Text}},
                          Rectf{descRect.x + 12.0f, descRect.y + 8.0f, descW, descRect.h - 16.0f},
                          TextBlock::Anchor::TopLeft,
                          descW,
                          kDescLineSpacing,
                          HorizontalAlign::Left,
                          true);
    }

    // Footer Hint
    const char *confirmHint = (m_mode == Mode::Buy) ? "Enter: Buy" : "Enter: Sell";
    renderer->renderTextInRect(m_font, std::string("W/S: Navigate | ") + confirmHint + " | Esc: Back",
                               Rectf{contentX, panelY + kPanelH - outer.bottom - kFooterH, contentW, kFooterH},
                               UITheme::Info,
                               HorizontalAlign::Center, VerticalAlign::Middle,
                               false, false, false);
}