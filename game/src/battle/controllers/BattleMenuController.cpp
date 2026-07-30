#include "battle/controllers/BattleMenuController.h"

#include "engine/core/App.h"
#include "engine/core/Log.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/renderer/FontManager.h"
#include "config/GameConstants.h"
#include "data/UnitLoader.h"
#include "battle/unit/Unit.h"
#include "battle/unit/UnitProgression.h"
#include "battle/map/MovementRange.h"
#include "scenes/MainMenuState.h"
#include "scenes/BattleState.h"
#include "ui/UIScale.h"
#include "ui/UITheme.h"
#include "ui/UIManager.h"
#include "ui/WindowId.h"
#include "ui/windows/ButtonMenuWindow.h"
#include "ui/windows/ConfirmWindow.h"
#include "battle/ui/UnitInspectWindow.h"

ButtonMenuWindow *BattleMenuController::pushButtonMenu(WindowId id,
                                                       const ButtonMenuConfig &config,
                                                       const std::vector<BattleMenuItem> &items)
{
    auto ctx = m_owner.makeMenuContext();

    ctx.uiManager.popById(id);
    auto *menu = ctx.uiManager.push<ButtonMenuWindow>(id);
    menu->setFont(FontManager::instance().get(FontRole::Body));
    menu->setTextAlign(config.textAlign);

    if (config.anchorBottomRight)
        menu->setAnchorBottomRight(config.bottomRightMargin);
    else
        menu->clearPanelPosition();

    menu->centerHorizontally(config.centerHorizontally);

    if (config.panelPosition)
        menu->setPanelPosition(*config.panelPosition);

    std::vector<ButtonMenuWindow::Item> uiItems;
    uiItems.reserve(items.size());
    for (const auto &item : items)
        uiItems.push_back({.label = item.label, .enabled = item.enabled});
    menu->setItems(std::move(uiItems));

    return menu;
}

void BattleMenuController::showBattleMenu(bool canMove, bool canAttack, bool canWait)
{
    auto ctx = m_owner.makeMenuContext();

    std::vector<BattleMenuItem> items;
    items.push_back(BattleMenuItem{
        .label = "Move",
        .enabled = canMove,
        .onSelect = [this]()
        {
            auto ctx = m_owner.makeMenuContext();
            Unit *active = ctx.session.getCurrentUnit();
            if (active)
            {
                MovementRange::Result range = MovementRange::compute(
                    ctx.grid, ctx.battleMap, active->getPosition(),
                    active->getMoveRangeLeft(), active->getTeam(),
                    ctx.session.getUnitPtrs(), active->getJump());
                ctx.reachableTiles = std::move(range.reachable);
                ctx.reachableCosts = std::move(range.costs);
                ctx.cursor.setPosition(active->getPosition());
            }
            ctx.humanTurnPhase = BattleState::HumanTurnPhase::MoveTarget;
            m_owner.battleMenu().closeAllMenus();
        }});

    items.push_back(BattleMenuItem{
        .label = "Attack",
        .enabled = canAttack,
        .onSelect = [this]()
        {
            auto ctx = m_owner.makeMenuContext();
            Unit *active = ctx.session.getCurrentUnit();
            if (active)
            {
                ctx.currentAttackRange = 1;
                ctx.selectedSkillId.clear();
                m_owner.computeAttackRangeTiles();
                ctx.cursor.setPosition(active->getPosition());
            }
            ctx.humanTurnPhase = BattleState::HumanTurnPhase::AttackTarget;
            m_owner.battleMenu().closeAllMenus();
        }});

    Unit *active = ctx.session.getCurrentUnit();
    if (active && !active->getSkillIds().empty())
    {
        items.push_back(BattleMenuItem{
            .label = "Skills",
            .enabled = canAttack,
            .onSelect = [this]()
            { showSkillMenu(); }});
    }

    items.push_back(BattleMenuItem{
        .label = "Defend",
        .enabled = canAttack,
        .onSelect = [this]()
        {
            auto ctx = m_owner.makeMenuContext();
            Unit *active = ctx.session.getCurrentUnit();
            if (active)
                active->setMajorAction(MajorAction::Defend);
            m_owner.battleMenu().closeAllMenus();
            m_owner.advanceToNextUnit();
            ctx.turnState = BattleState::TurnState::Idle;
        }});

    items.push_back(BattleMenuItem{
        .label = "Wait",
        .enabled = canWait,
        .onSelect = [this]()
        {
            auto ctx = m_owner.makeMenuContext();
            m_owner.battleMenu().closeAllMenus();
            m_owner.advanceToNextUnit();
            ctx.turnState = BattleState::TurnState::Idle;
        }});

    m_actionMenuItems = std::move(items);

    ButtonMenuConfig config;
    config.textAlign = ButtonMenuWindow::TextAlign::Left;
    config.anchorBottomRight = true;
    config.bottomRightMargin = Vec2f{16.0f, 16.0f};
    pushButtonMenu(WindowId::BattleActionMenu, config, m_actionMenuItems);
}

void BattleMenuController::showSkillMenu()
{
    auto ctx = m_owner.makeMenuContext();
    Unit *active = ctx.session.getCurrentUnit();
    if (!active)
        return;

    m_skillMenuItems.clear();

    for (const std::string &skillId : active->getSkillIds())
    {
        auto it = ctx.skillDB.find(skillId);
        if (it == ctx.skillDB.end())
            continue;

        const SkillData &skill = it->second;
        std::string label = skill.name + " (MP:" + std::to_string(skill.mpCost) + ")";
        bool canUse = (active->getCurrentMp() >= skill.mpCost);

        m_skillMenuItems.push_back(BattleMenuItem{
            .label = label,
            .enabled = canUse,
            .onSelect = [this, skillId]()
            {
                auto ctx = m_owner.makeMenuContext();
                auto it = ctx.skillDB.find(skillId);
                if (it != ctx.skillDB.end())
                {
                    ctx.currentAttackRange = it->second.range;
                    ctx.selectedSkillId = skillId;
                    m_owner.computeAttackRangeTiles();
                }
                if (Unit *active = ctx.session.getCurrentUnit())
                    ctx.cursor.setPosition(active->getPosition());
                ctx.humanTurnPhase = BattleState::HumanTurnPhase::AttackTarget;
                ctx.uiManager.popById(WindowId::BattleSkillMenu);
                m_owner.battleMenu().closeAllMenus();
                ;
            }});
    }

    ButtonMenuConfig config;
    config.textAlign = ButtonMenuWindow::TextAlign::Left;
    config.anchorBottomRight = true;
    pushButtonMenu(WindowId::BattleSkillMenu, config, m_skillMenuItems);
}

void BattleMenuController::showSystemMenu()
{
    auto ctx = m_owner.makeMenuContext();
    const bool deploying = (ctx.flowPhase == BattleState::BattleFlowPhase::Deployment);

    BattleMenuItem firstItem;
    if (deploying)
    {
        firstItem = BattleMenuItem{
            .label = "Start Battle",
            .enabled = ctx.deploymentPhase.deployment().canStartBattle(),
            .onSelect = [this]()
            {
                auto ctx = m_owner.makeMenuContext();
                ctx.uiManager.popById(WindowId::BattleDeploymentConfirm);
                auto *confirm = ctx.uiManager.push<ConfirmWindow>(WindowId::BattleDeploymentConfirm);
                confirm->setFont(FontManager::instance().get(FontRole::Body));
                confirm->setPrompt("Start Battle?");
            }};
    }
    else
    {
        firstItem = BattleMenuItem{
            .label = "Resume",
            .enabled = true,
            .onSelect = [this]()
            { m_owner.battleMenu().closeAllMenus(); }};
    }

    BattleMenuItem quitItem{
        .label = "Quit",
        .enabled = true,
        .onSelect = [this]()
        {
            auto ctx = m_owner.makeMenuContext();
            ctx.sm.replace(std::make_unique<MainMenuState>(ctx.sm, ctx.renderer));
        }};

    m_systemMenuItems = {std::move(firstItem), std::move(quitItem)};

    ButtonMenuConfig config; // all defaults → dead center, both axes, matches actual computed size
    pushButtonMenu(WindowId::BattleSystemMenu, config, m_systemMenuItems);
}

void BattleMenuController::showInspectWindow(Unit *unit)
{
    if (!unit)
        return;

    auto ctx = m_owner.makeMenuContext();
    ctx.uiManager.popById(WindowId::BattleInspect);
    auto *inspect = ctx.uiManager.push<UnitInspectWindow>(WindowId::BattleInspect);
    inspect->setFont(FontManager::instance().get(FontRole::Body));
    const int team = unit->getTeam();
    if (team == 0)
        inspect->setRelation(UnitInspectWindow::Relation::Player);
    else if (team == 1)
        inspect->setRelation(UnitInspectWindow::Relation::Ally);
    else
        inspect->setRelation(UnitInspectWindow::Relation::Enemy);

    inspect->setHeader(unit->getData().name, unit->getData().className);
    inspect->setSections({UnitInspectWindow::buildStatsSection(unit->getData())});
    m_inspectWindow = inspect;
}

void BattleMenuController::showStatusMenu(Unit *unit)
{
    if (!unit)
        return;
    showInspectWindow(unit);
}

void BattleMenuController::openStatusMenu(Unit *unit)
{
    showStatusMenu(unit);
    Input::instance().consumeKey(KeyCode::Accept);
}

void BattleMenuController::showInspectWindowFromTemplate(const std::string &templatePath)
{
    // For a roster unit that hasn't been placed yet there's no live Unit
    // object to inspect — build the same effective stats a live Unit would
    // have (race/gender bonuses applied) from its template instead.
    try
    {
        const UnitData templateData = UnitLoader::load(templatePath);
        const RaceData &raceData = getRaceData(templateData.race);
        const GenderData &genderData = getGenderData(templateData.gender);
        const Unit previewUnit(templateData, raceData, genderData, Vec2i{0, 0});

        auto ctx = m_owner.makeMenuContext();
        ctx.uiManager.popById(WindowId::BattleInspect);
        auto *inspect = ctx.uiManager.push<UnitInspectWindow>(WindowId::BattleInspect);
        inspect->setFont(FontManager::instance().get(FontRole::Body));
        // Deployment roster preview units are always the player's own —
        // same reasoning as PartyWindow's Accept flow.
        inspect->setRelation(UnitInspectWindow::Relation::Player);
        inspect->setHeader(previewUnit.getData().name, previewUnit.getData().className);
        inspect->setSections({UnitInspectWindow::buildStatsSection(previewUnit.getData())});
        m_inspectWindow = inspect;
    }
    catch (...)
    {
        // Template failed to load — nothing sensible to show.
    }
}

void BattleMenuController::openUnitInspectMenu(Unit *unit)
{
    if (!unit)
        return;

    auto ctx = m_owner.makeMenuContext();
    ctx.inspectTargetUnit = unit;
    ctx.uiManager.popById(WindowId::BattleInspectMenu);
    auto *menu = ctx.uiManager.push<ButtonMenuWindow>(WindowId::BattleInspectMenu);
    menu->setFont(FontManager::instance().get(FontRole::Body));
    menu->setTextAlign(ButtonMenuWindow::TextAlign::Left);
    menu->setAnchorBottomRight();
    menu->setItems({
        ButtonMenuWindow::Item{.id = ActionId::Inspect, .label = "Inspect", .enabled = true},
    });
}

bool BattleMenuController::handleUIEvent(const UIEvent &event, Unit *active)
{
    auto ctx = m_owner.makeMenuContext();

    if (event.windowId == WindowId::BattleInspectMenu)
    {
        if (event.type == UIEventType::ActionSelected && event.actionId == ActionId::Inspect)
        {
            ctx.uiManager.popById(WindowId::BattleInspectMenu);
            if (ctx.inspectTargetUnit && !ctx.inspectTargetUnit->isDead())
                showInspectWindow(ctx.inspectTargetUnit);
            return true;
        }
        if (event.type == UIEventType::ActionCanceled)
        {
            ctx.uiManager.popById(WindowId::BattleInspectMenu);
            ctx.inspectTargetUnit = nullptr;
            return true;
        }
    }

    if (ctx.flowPhase == BattleState::BattleFlowPhase::Deployment &&
        event.windowId == WindowId::BattleSystemMenu)
    {
        if (event.type == UIEventType::ActionSelected)
        {
            const int index = event.index;
            if (index < 0 || index >= static_cast<int>(m_systemMenuItems.size()))
                return true;

            BattleMenuItem item = m_systemMenuItems[index];
            ctx.uiManager.popById(WindowId::BattleSystemMenu);
            if (item.enabled && item.onSelect)
                item.onSelect();
            return true;
        }
        if (event.type == UIEventType::ActionCanceled)
        {
            closeAllMenus();
            return true;
        }
    }

    if (event.windowId == WindowId::BattleInspect && event.type == UIEventType::ActionCanceled)
    {
        ctx.uiManager.popById(WindowId::BattleInspect);
        m_inspectWindow = nullptr;
        return true;
    }

    if (ctx.flowPhase != BattleState::BattleFlowPhase::Combat)
        return false;

    if (event.windowId == WindowId::BattleActionMenu && event.type == UIEventType::ActionSelected)
    {
        const int index = event.index;
        if (index < 0 || index >= static_cast<int>(m_actionMenuItems.size()))
            return true;
        BattleMenuItem item = m_actionMenuItems[index];
        ctx.uiManager.popById(WindowId::BattleActionMenu);
        if (item.enabled && item.onSelect)
            item.onSelect();
        // The Accept press that selected this menu item is still "live" for
        // the rest of this frame — without consuming it here, the very next
        // input pass sees the same edge and can immediately fire whatever
        // new phase we just entered.
        Input::instance().consumeKey(KeyCode::Accept);
        return true;
    }

    if (event.windowId == WindowId::BattleSkillMenu && event.type == UIEventType::ActionSelected)
    {
        const int index = event.index;
        if (index < 0 || index >= static_cast<int>(m_skillMenuItems.size()))
            return true;

        BattleMenuItem item = m_skillMenuItems[index];
        if (item.enabled && item.onSelect)
            item.onSelect();
        Input::instance().consumeKey(KeyCode::Accept);
        return true;
    }

    if (event.windowId == WindowId::BattleSkillMenu && event.type == UIEventType::ActionCanceled)
    {
        // Pop just this level — battle.actionmenu underneath was never
        // touched, so it's revealed exactly as it was.
        ctx.uiManager.popById(WindowId::BattleSkillMenu);
        return true;
    }

    if (event.windowId == WindowId::BattleActionMenu && event.type == UIEventType::ActionCanceled)
    {
        if (ctx.humanTurnPhase == BattleState::HumanTurnPhase::AttackConfirm)
        {
            ctx.attackResolution.cancel();
            return true;
        }

        if (!active)
        {
            closeAllMenus();
            return true;
        }

        if (ctx.canUndoLastMove)
        {
            Vec2i currentPos = active->getPosition();
            ctx.grid.getTile(currentPos).occupied = false;
            active->setPosition(ctx.moveStartPos);
            ctx.grid.getTile(ctx.moveStartPos).occupied = true;
            active->refundMovePoints(ctx.moveStartPointsLeft - active->getMoveRangeLeft());
            ctx.canUndoLastMove = false;
            ctx.cursor.setPosition(ctx.moveStartPos);

            ctx.humanTurnPhase = BattleState::HumanTurnPhase::ActionMenu;
            m_owner.openBattleMenu(m_owner.canActiveUnitMove(), !active->hasActed(), true, KeyCode::Back);
            LOG_INFO("Battle", "Move undone for %s", active->getName().c_str());
        }
        else if (!active->hasMoved() && !active->hasActed())
        {
            m_owner.battleMenu().closeAllMenus();
            ctx.humanTurnPhase = BattleState::HumanTurnPhase::FreeCursor;
            ctx.cursor.setPosition(active->getPosition());
        }
        return true;
    }

    return false;
}

void BattleMenuController::closeAllMenus()
{
    auto ctx = m_owner.makeMenuContext();
    ctx.uiManager.popById(WindowId::BattleActionMenu);
    ctx.uiManager.popById(WindowId::BattleSystemMenu);
    ctx.uiManager.popById(WindowId::BattleActionConfirm);
    ctx.uiManager.popById(WindowId::BattleInspect);
    m_inspectWindow = nullptr;
}