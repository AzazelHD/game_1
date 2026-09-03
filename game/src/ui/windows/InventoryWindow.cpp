#include "ui/windows/InventoryWindow.h"

#include "config/GameConstants.h"
#include "engine/core/Log.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Aligment.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/FontManager.h"
#include "engine/renderer/Renderer.h"
#include "engine/ui/Insets.h"
#include "engine/ui/TextWrap.h"
#include "engine/ui/VerticalLayout.h"
#include "inventory/Gear.h"
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

    const char *kPlaceholderLorem =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
        "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
        "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris.";
}

InventoryWindow::InventoryWindow(WindowId id, const Inventory &inventory, const GearCatalog &catalog)
    : UIWindow(id, true, false),
      m_inventory(&inventory),
      m_catalog(&catalog)
{
    rebuildRows();
}

void InventoryWindow::rebuildRows()
{
    m_rows.clear();
    if (!m_inventory)
        return;

    for (const ItemStack &stack : m_inventory->stacks())
    {
        std::string name = "Item #" + std::to_string(stack.itemId);
        std::string desc = kPlaceholderLorem;

        if (m_catalog)
        {
            if (const Gear *gear = m_catalog->find(stack.itemId))
            {
                name = gear->displayName;
                if (!gear->description.empty())
                    desc = gear->description + "\n" + kPlaceholderLorem;
            }
        }

        m_rows.push_back(ItemRow{
            .itemId = stack.itemId,
            .name = std::move(name),
            .description = std::move(desc),
            .quantity = stack.quantity,
        });
    }

    m_selectedIndex = 0;
    m_scroll = 0;
}

void InventoryWindow::handleInput(const Input &input)
{
    const int count = static_cast<int>(m_rows.size());

    // The four key-repeat trackers drive continuous Up/Down navigation
    // and continuous A/D page-jumps (Part K). All four use the same
    // initial-delay / interval tuning so the feel is consistent.
    const bool upHit    = m_upRepeat.tick(m_lastDt,    input.isKeyDown(KeyCode::Up)    || input.isKeyDown(KeyCode::W));
    const bool downHit  = m_downRepeat.tick(m_lastDt,  input.isKeyDown(KeyCode::Down)  || input.isKeyDown(KeyCode::S));
    const bool leftHit  = m_leftRepeat.tick(m_lastDt,  input.isKeyDown(KeyCode::Left)  || input.isKeyDown(KeyCode::A));
    const bool rightHit = m_rightRepeat.tick(m_lastDt, input.isKeyDown(KeyCode::Right) || input.isKeyDown(KeyCode::D));

    if (count > 0)
    {
        if (upHit)
        {
            if (m_selectedIndex > 0)
                --m_selectedIndex;
        }
        else if (downHit)
        {
            if (m_selectedIndex < count - 1)
                ++m_selectedIndex;
        }
        else if (leftHit)
        {
            m_selectedIndex = std::max(0, m_selectedIndex - 5);
        }
        else if (rightHit)
        {
            m_selectedIndex = std::min(count - 1, m_selectedIndex + 5);
        }

        if (m_selectedIndex < m_scroll)
            m_scroll = m_selectedIndex;
        if (m_selectedIndex >= m_scroll + kVisibleRows)
            m_scroll = m_selectedIndex - kVisibleRows + 1;
    }

    if (input.isKeyPressed(KeyCode::Back, false) || input.isKeyPressed(KeyCode::Accept, false))
    {
        LOG_INFO("InventoryWindow", "Back/Accept pressed -> Closing inventory window");
        emit(UIEvent{.type = UIEventType::ActionCanceled, .windowId = id(), .actionId = ActionId::Close});
    }
}

void InventoryWindow::update(float dt)
{
    // Cache dt so handleInput (which the engine dispatches *before* update
    // on the same frame) can drive the hold-to-repeat trackers.
    m_lastDt = dt;
    m_upRepeat.start(0.35f, 0.09f);
    m_downRepeat.start(0.35f, 0.09f);
    m_leftRepeat.start(0.35f, 0.09f);
    m_rightRepeat.start(0.35f, 0.09f);
}

void InventoryWindow::render(Renderer *renderer) const
{
    if (!renderer || !m_font)
        return;

    const float panelX = (GameConstants::VIEW_W - kPanelW) * 0.5f;
    const float panelY = (GameConstants::VIEW_H - kPanelH) * 0.5f;

    // Background panel
    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(UITheme::PopupBG);
    renderer->fillRect(Rectf{panelX, panelY, kPanelW, kPanelH});
    renderer->setDrawColor(UITheme::PopupBorder);
    renderer->drawRect(Rectf{panelX, panelY, kPanelW, kPanelH});

    // Content area inside insets
    const Insets outer{16.0f, 20.0f, 14.0f, 20.0f};
    const float contentX = panelX + outer.left;
    const float contentY = panelY + outer.top;
    const float contentW = kPanelW - outer.left - outer.right;
    const float contentH = kPanelH - outer.top - outer.bottom;

    // Header Title
    const Font *titleFont = FontManager::instance().get(FontRole::Heading);
    if (!titleFont)
        titleFont = m_font;

    renderer->renderTextInRect(titleFont, "Inventory",
                               Rectf{contentX, contentY, contentW, kHeaderH},
                               UITheme::SelectedText,
                               HorizontalAlign::Center, VerticalAlign::Middle,
                               false, false, false);

    // Remaining height for 2/3 list split and 1/3 description split
    const float bodyY = contentY + kHeaderH + 6.0f;
    const float bodyH = contentH - kHeaderH - kFooterH - 12.0f;

    const float topSectionH = std::floor(bodyH * (2.0f / 3.0f));
    const float bottomSectionH = bodyH - topSectionH;

    // Top Section (2/3): Scrollable Item List
    const Rectf listRect{contentX, bodyY, contentW, topSectionH};
    renderer->setDrawColor(Color{30, 36, 48, 180});
    renderer->fillRect(listRect);
    renderer->setDrawColor(Color{60, 75, 95, 255});
    renderer->drawRect(listRect);

    if (m_rows.empty())
    {
        renderer->renderTextInRect(m_font, "(Inventory is empty)",
                                   listRect,
                                   UITheme::Info,
                                   HorizontalAlign::Center, VerticalAlign::Middle,
                                   false, false, false);
    }
    else
    {
        const int count = static_cast<int>(m_rows.size());
        const int start = std::clamp(m_scroll, 0, std::max(0, count - 1));
        const int end = std::min(count, start + kVisibleRows);

        float rowY = bodyY + 6.0f;
        for (int i = start; i < end; ++i)
        {
            const ItemRow &row = m_rows[static_cast<std::size_t>(i)];
            const bool isSelected = (i == m_selectedIndex);
            const Rectf itemRect{contentX + 6.0f, rowY, contentW - 12.0f, kRowH};

            if (isSelected)
            {
                renderer->setDrawColor(Color{80, 96, 122, 160});
                renderer->fillRect(itemRect);
            }

            renderer->renderTextInRect(m_font, row.name,
                                       Rectf{itemRect.x + 10.0f, itemRect.y, itemRect.w * 0.7f, itemRect.h},
                                       isSelected ? UITheme::SelectedText : UITheme::Text,
                                       HorizontalAlign::Left, VerticalAlign::Middle,
                                       false, false, false);

            renderer->renderTextInRect(m_font, "x" + std::to_string(row.quantity),
                                       Rectf{itemRect.x + itemRect.w * 0.7f, itemRect.y, itemRect.w * 0.3f - 10.0f, itemRect.h},
                                       isSelected ? UITheme::SelectedText : UITheme::Info,
                                       HorizontalAlign::Right, VerticalAlign::Middle,
                                       false, false, false);

            rowY += kRowH + 2.0f;
        }

        // Scroll indicators
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

    // Bottom Section (1/3): Description panel
    const float descY = bodyY + topSectionH + 8.0f;
    const Rectf descRect{contentX, descY, contentW, bottomSectionH - 8.0f};
    renderer->setDrawColor(Color{25, 30, 42, 200});
    renderer->fillRect(descRect);
    renderer->setDrawColor(Color{60, 75, 95, 255});
    renderer->drawRect(descRect);

    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_rows.size()))
    {
        const ItemRow &sel = m_rows[static_cast<std::size_t>(m_selectedIndex)];
        const float maxTextW = descRect.w - 24.0f;
        const std::vector<std::string> lines = TextWrap::wrap(renderer, m_font, sel.description, maxTextW);
        const float lineH = renderer->measureText(m_font, "Ag").y;
        constexpr float kDescLineSpacing = 3.0f;

        float lineY = descRect.y + 8.0f;
        const float maxDescY = descRect.y + descRect.h - 8.0f - lineH;

        for (const std::string &line : lines)
        {
            if (lineY > maxDescY)
                break;

            if (!line.empty())
            {
                renderer->renderTextInRect(m_font, line,
                                           Rectf{descRect.x + 12.0f, lineY, maxTextW, lineH},
                                           UITheme::Text,
                                           HorizontalAlign::Left, VerticalAlign::Middle,
                                           false, false, false);
            }
            lineY += lineH + kDescLineSpacing;
        }
    }

    // Footer Hint
    renderer->renderTextInRect(m_font, "W/S: Navigate | A/D: Page | Esc/Enter: Back",
                               Rectf{contentX, panelY + kPanelH - outer.bottom - kFooterH, contentW, kFooterH},
                               UITheme::Info,
                               HorizontalAlign::Center, VerticalAlign::Middle,
                               false, false, false);
}
