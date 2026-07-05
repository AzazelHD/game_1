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
    m_uiManager.popById("battle.actionmenu");

    auto *menu = m_uiManager.push<ActionMenuWindow>("battle.actionmenu");
    menu->setFont(App::getDefaultFont());
    if (combatPhase)
    {
        UIScale::refresh();
        const float ui = UIScale::factor();
        menu->setPanelPosition(Vec2f{GameConstants::VIEW_W - 280.0f * ui, GameConstants::VIEW_H - 240.0f * ui});
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
    menu->setItems(std::move(uiItems));
}

void BattleHud::clear()
{
    m_items.clear();
    m_uiManager.popById("battle.actionmenu");
    m_uiManager.popById("battle.confirm");
}

bool BattleHud::isOpen() const
{
    return m_uiManager.hasWindow("battle.actionmenu") || m_uiManager.hasWindow("battle.confirm");
}