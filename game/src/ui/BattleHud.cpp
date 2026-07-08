#include "ui/BattleHud.h"
#include "ui/UIManager.h"
#include "ui/UIScale.h"
#include "ui/windows/ActionMenuWindow.h"
#include "config/GameConstants.h"
#include "engine/core/App.h"
#include "engine/math/Vec2.h"

#include <string>

namespace
{
    std::string trimLabel(std::string text)
    {
        const std::size_t first = text.find_first_not_of(" \t\n\r");
        if (first == std::string::npos)
            return std::string();
        const std::size_t last = text.find_last_not_of(" \t\n\r");
        return text.substr(first, last - first + 1);
    }
}

void BattleHud::setItems(std::vector<BattleMenuItem> items, bool combatPhase)
{
    m_items = std::move(items);

    // Created once; every later call just re-shows and re-populates the
    // same instance instead of destroying/reconstructing it.
    if (!m_menu)
    {
        m_menu = m_uiManager.push<ActionMenuWindow>("battle.actionmenu");
        m_menu->setFont(App::getDefaultFont());
    }
    else
    {
        m_menu->setVisible(true);
    }

    if (combatPhase)
    {
        UIScale::refresh();
        const float ui = UIScale::factor();
        m_menu->setPanelPosition(Vec2f{GameConstants::VIEW_W - 280.0f * ui, GameConstants::VIEW_H - 240.0f * ui});
    }

    std::vector<ActionMenuWindow::Item> uiItems;
    uiItems.reserve(m_items.size());
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
    {
        const BattleMenuItem &item = m_items[i];
        uiItems.push_back(ActionMenuWindow::Item{
            .id = std::to_string(i),
            .label = trimLabel(item.label),
            .enabled = item.enabled,
        });
    }
    // NOTE: setItems() still resets m_selected/m_scroll on every call. Kept
    // as-is on purpose: the item set (and which ones are enabled) genuinely
    // changes each time this reopens for a new unit/context, so re-landing
    // on the first enabled item is the correct behavior, not stale reset
    // logic left over from the old push/pop lifecycle.
    m_menu->setItems(std::move(uiItems));
}

void BattleHud::clear()
{
    m_items.clear();
    // hideById, not popById: popById would destroy m_menu and leave our
    // cached pointer dangling.
    if (m_menu)
        m_uiManager.hideById("battle.actionmenu");
    m_uiManager.popById("battle.confirm");
}

bool BattleHud::isOpen() const
{
    return (m_menu && m_menu->isVisible()) || m_uiManager.hasWindow("battle.confirm");
}