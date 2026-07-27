#include "battle/BattleMenuController.h"

#include "engine/core/App.h"
#include "engine/core/Log.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/renderer/FontManager.h"
#include "config/GameConstants.h"
#include "data/UnitLoader.h"
#include "battle/Unit.h"
#include "battle/UnitProgression.h"
#include "battle/MovementRange.h"
#include "scenes/MainMenuState.h"
#include "scenes/BattleState.h"
#include "ui/UIScale.h"
#include "ui/UITheme.h"
#include "ui/windows/ButtonMenuWindow.h"
#include "ui/windows/ConfirmWindow.h"
#include "ui/windows/UnitInspectWindow.h"

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
                ctx.reachableTiles = MovementRange::compute(ctx.grid, ctx.battleMap,
                                                            active->getPosition(),
                                                            active->getMoveRangeLeft(),
                                                            active->getTeam(),
                                                            ctx.session.getUnitPtrs(),
                                                            active->getJump());
                ctx.cursor.setPosition(active->getPosition());
            }
            ctx.humanTurnPhase = BattleState::HumanTurnPhase::MoveTarget;
            ctx.hud.clear();
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
            ctx.hud.clear();
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
            ctx.hud.clear();
            m_owner.advanceToNextUnit();
            ctx.turnState = BattleState::TurnState::Idle;
        }});

    items.push_back(BattleMenuItem{
        .label = "Wait",
        .enabled = canWait,
        .onSelect = [this]()
        {
            auto ctx = m_owner.makeMenuContext();
            ctx.hud.clear();
            m_owner.advanceToNextUnit();
            ctx.turnState = BattleState::TurnState::Idle;
        }});

    ctx.hud.setItems(std::move(items), ctx.flowPhase == BattleState::BattleFlowPhase::Combat);
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
                ctx.hud.clear();
            }});
    }

    // A real push onto the UI stack — a second, distinct level above
    // battle.actionmenu, which stays untouched underneath. Back pops just
    // this window and battle.actionmenu is revealed exactly as it was, the
    // same way any other stacked window already behaves.
    ctx.uiManager.popById(WindowId::BattleSkillMenu);
    auto *menu = ctx.uiManager.push<ButtonMenuWindow>(WindowId::BattleSkillMenu);
    menu->setFont(FontManager::instance().get(FontRole::Body));
    if (ctx.flowPhase == BattleState::BattleFlowPhase::Combat)
    {
        UIScale::refresh();
        const float ui = UIScale::factor();
        menu->setPanelPosition(Vec2f{GameConstants::VIEW_W - 280.0f * ui, GameConstants::VIEW_H - 240.0f * ui});
    }

    std::vector<ButtonMenuWindow::Item> uiItems;
    uiItems.reserve(m_skillMenuItems.size());
    for (int i = 0; i < static_cast<int>(m_skillMenuItems.size()); ++i)
    {
        uiItems.push_back(ButtonMenuWindow::Item{
            .label = m_skillMenuItems[i].label,
            .enabled = m_skillMenuItems[i].enabled,
        });
    }
    menu->setItems(std::move(uiItems));
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
            { m_owner.makeMenuContext().hud.clear(); }};
    }

    BattleMenuItem quitItem{
        .label = "Quit",
        .enabled = true,
        .onSelect = [this]()
        {
            auto ctx = m_owner.makeMenuContext();
            ctx.sm.replace(std::make_unique<MainMenuState>(ctx.sm, ctx.renderer));
        }};

    ctx.hud.setItems({std::move(firstItem), std::move(quitItem)}, false);
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

    if (ctx.flowPhase == BattleState::BattleFlowPhase::Deployment && event.windowId == WindowId::BattleActionMenu)
    {
        if (event.type == UIEventType::ActionSelected)
        {
            const int index = event.index;
            if (index < 0 || index >= static_cast<int>(ctx.hud.items().size()))
                return true;

            BattleMenuItem item = ctx.hud.items()[index];
            // hideById, not popById: the action menu is a persistent
            // window owned by BattleUIManager (see BattleUIManager::m_menu) — popById
            // would destroy it out from under BattleUIManager's cached pointer.
            ctx.uiManager.hideById(WindowId::BattleActionMenu);
            if (item.enabled && item.onSelect)
                item.onSelect();
            return true;
        }
        if (event.type == UIEventType::ActionCanceled)
        {
            ctx.hud.clear();
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
        if (index < 0 || index >= static_cast<int>(ctx.hud.items().size()))
            return true;

        BattleMenuItem item = ctx.hud.items()[index];
        ctx.uiManager.hideById(WindowId::BattleActionMenu);
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
            ctx.hud.clear();
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
            ctx.hud.clear();
            ctx.humanTurnPhase = BattleState::HumanTurnPhase::FreeCursor;
            ctx.cursor.setPosition(active->getPosition());
        }
        return true;
    }

    return false;
}