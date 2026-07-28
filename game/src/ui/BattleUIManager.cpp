#include "ui/BattleUIManager.h"

#include "battle/Unit.h"
#include "config/GameConstants.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/FontManager.h"
#include "ui/UIManager.h"
#include "ui/UIScale.h"
#include "ui/windows/ButtonMenuWindow.h"
#include "ui/windows/ConfirmWindow.h"
#include "ui/windows/UnitInspectWindow.h"

namespace
{
    std::string trimLabel(std::string text)
    {
        const auto first = text.find_first_not_of(" \t\n\r");
        if (first == std::string::npos)
            return {};

        const auto last = text.find_last_not_of(" \t\n\r");
        return text.substr(first, last - first + 1);
    }
}

BattleUIManager::BattleUIManager(UIManager &ui)
    : m_ui(ui)
{
}

ButtonMenuWindow *BattleUIManager::ensureActionMenu()
{
    if (!m_actionMenu)
    {
        m_actionMenu = m_ui.push<ButtonMenuWindow>(WindowId::BattleActionMenu);
        m_actionMenu->setFont(FontManager::instance().get(FontRole::Body));
    }

    return m_actionMenu;
}

ButtonMenuWindow *BattleUIManager::ensureSystemMenu()
{
    if (!m_systemMenu)
    {
        m_systemMenu = m_ui.push<ButtonMenuWindow>(WindowId::BattleSystemMenu);
        m_systemMenu->setFont(FontManager::instance().get(FontRole::Body));
    }

    return m_systemMenu;
}

UnitInspectWindow *BattleUIManager::ensureInspectWindow()
{
    if (!m_inspect)
    {
        m_inspect = m_ui.push<UnitInspectWindow>(WindowId::BattleInspect);
        m_inspect->setFont(FontManager::instance().get(FontRole::Body));
    }

    return m_inspect;
}

ConfirmWindow *BattleUIManager::ensureConfirmWindow()
{
    if (!m_confirm)
    {
        m_confirm = m_ui.push<ConfirmWindow>(WindowId::BattleActionConfirm);
        m_confirm->setFont(FontManager::instance().get(FontRole::Body));
    }

    return m_confirm;
}

void BattleUIManager::showActionMenu(std::vector<BattleMenuItem> items)
{
    m_actionMenuItems = std::move(items);

    auto *menu = ensureActionMenu();

    menu->setAnchorBottomRight({16.0f, 16.0f});
    menu->setTextAlign(ButtonMenuWindow::TextAlign::Left);

    menu->setVisible(true);

    std::vector<ButtonMenuWindow::Item> uiItems;
    uiItems.reserve(m_actionMenuItems.size());

    for (const auto &item : m_actionMenuItems)
    {
        uiItems.push_back({
            .label = trimLabel(item.label),
            .enabled = item.enabled,
        });
    }

    menu->setItems(std::move(uiItems));
}

void BattleUIManager::hideActionMenu()
{
    if (m_actionMenu)
        m_ui.hideById(WindowId::BattleActionMenu);
}

void BattleUIManager::showSystemMenu(std::vector<BattleMenuItem> items)
{
    auto *menu = ensureSystemMenu();

    menu->clearPanelPosition();
    menu->centerHorizontally(true);
    menu->setTextAlign(ButtonMenuWindow::TextAlign::Center);
    menu->setPanelPosition({0.0f, (GameConstants::VIEW_H - 224.0f) * 0.5f});

    menu->setVisible(true);

    std::vector<ButtonMenuWindow::Item> uiItems;
    uiItems.reserve(items.size());

    for (const auto &item : items)
    {
        uiItems.push_back({
            .label = trimLabel(item.label),
            .enabled = item.enabled,
        });
    }

    menu->setItems(std::move(uiItems));
}

void BattleUIManager::hideSystemMenu()
{
    if (m_systemMenu)
        m_ui.hideById(WindowId::BattleSystemMenu);
}

bool BattleUIManager::hasBlockingWindow() const
{
    return (m_actionMenu && m_actionMenu->isVisible()) ||
           (m_systemMenu && m_systemMenu->isVisible()) ||
           (m_confirm && m_confirm->isVisible()) ||
           (m_inspect && m_inspect->isVisible());
}

const std::vector<BattleMenuItem> &BattleUIManager::actionMenuItems() const
{
    return m_actionMenuItems;
}

void BattleUIManager::showConfirm(const std::string &text)
{
    auto *confirm = ensureConfirmWindow();
    confirm->setVisible(true);
    confirm->setPrompt(text);
}

void BattleUIManager::hideConfirm()
{
    if (m_confirm)
        m_ui.hideById(WindowId::BattleActionConfirm);
}

void BattleUIManager::showInspect(Unit *unit)
{
    if (!unit)
        return;

    auto *inspect = ensureInspectWindow();
    inspect->setVisible(true);

    const int team = unit->getTeam();
    if (team == 0)
        inspect->setRelation(UnitInspectWindow::Relation::Player);
    else if (team == 1)
        inspect->setRelation(UnitInspectWindow::Relation::Ally);
    else
        inspect->setRelation(UnitInspectWindow::Relation::Enemy);

    inspect->setHeader(unit->getData().name, unit->getData().className);
    inspect->setSections({UnitInspectWindow::buildStatsSection(unit->getData())});
}

void BattleUIManager::hideInspect()
{
    if (m_inspect)
        m_ui.hideById(WindowId::BattleInspect);
}

void BattleUIManager::clear()
{
    hideActionMenu();
    hideSystemMenu();
    hideConfirm();
    hideInspect();

    m_actionMenuItems.clear();
}