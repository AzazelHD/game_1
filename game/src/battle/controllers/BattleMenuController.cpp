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
#include "battle/map/AttackRange.h"
#include "battle/map/MovementRange.h"
#include "battle/combat/SkillEffects.h"
#include "scenes/BattleState.h"
#include "ui/ConfirmDialogs.h"
#include "ui/UIScale.h"
#include "ui/UITheme.h"
#include "ui/UIManager.h"
#include "ui/WindowId.h"
#include "ui/windows/ButtonMenuWindow.h"
#include "ui/windows/UnitDetailWindow.h"
#include "systems/PartyContext.h"
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
        uiItems.push_back({.label = item.label, .enabled = item.enabled, .detail = item.detail});
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
                    ctx.session.getUnitPtrs(), active->getJump(),
                    MovementRangeOptions{.ignoreUnitCollisionAlongPath = active->hasGearSpecialEffect(GearSpecialEffect::TeleportMovement)});
                ctx.reachableTiles = std::move(range.reachable);
                ctx.reachableCosts = std::move(range.costs);
                ctx.cursor.setPosition(active->getPosition());
            }
            ctx.humanTurnPhase = BattleState::HumanTurnPhase::MoveTarget;
            m_owner.battleMenu().closeAllMenus();
            m_owner.maybeShowTutorialTip(BattleState::TutorialTip::Move);
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
                // The Attack button is just a shortcut that selects the shared
                // basic-attack skill — the same targeting path every other skill
                // takes. Its reach comes from the unit's atkRange and its height
                // tolerance from its jump (see SkillData::toRangeRuleFor).
                auto it = ctx.skillDB.find(kBasicAttackSkillId);
                if (it != ctx.skillDB.end())
                {
                    ctx.currentRangeRule =
                        it->second.toRangeRuleFor(active->getAtkRange(), active->getJump());
                    ctx.selectedSkillId = it->first;
                    m_owner.computeAttackRangeTiles();
                }
                ctx.cursor.setPosition(active->getPosition());
            }
            ctx.humanTurnPhase = BattleState::HumanTurnPhase::AttackTarget;
            m_owner.battleMenu().closeAllMenus();
            m_owner.maybeShowTutorialTip(BattleState::TutorialTip::Attack);
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

    // Future design: add a dedicated Equip action here for broken/stolen gear
    // during active combat, after a separate turn-economy decision (free action
    // vs full action, AP cost, etc.) is defined.
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
            m_owner.maybeShowTutorialTip(BattleState::TutorialTip::Defend);
        }});

    items.push_back(BattleMenuItem{
        .label = "Wait",
        .enabled = canWait,
        .onSelect = [this]()
        {
            auto ctx = m_owner.makeMenuContext();
            ctx.topBattleText = "Choose facing: Arrows + Accept";
            ctx.humanTurnPhase = BattleState::HumanTurnPhase::ChooseFacing;
            m_owner.battleMenu().closeAllMenus();
            m_owner.maybeShowTutorialTip(BattleState::TutorialTip::Wait);
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

    // Remember selection only for the same unit (a different caster's list
    // should start fresh on its first skill).
    if (active != m_lastSkillUnit)
        m_lastSkillIndex = -1;

    m_skillMenuItems.clear();

    std::size_t menuIndex = 0;
    for (const std::string &skillId : active->getSkillIds())
    {
        auto it = ctx.skillDB.find(skillId);
        if (it == ctx.skillDB.end())
            continue;

        const SkillData &skill = it->second;
        // The basic attack has its own action-menu button and belongs to every
        // unit, so it never appears in a unit's skill list.
        if (skill.isBasicAttack)
            continue;

        std::string label = skill.name + " (MP:" + std::to_string(skill.mpCost) + ")";
        bool canUse = active->getCurrentMp() >= skill.mpCost;

        m_skillMenuItems.push_back(BattleMenuItem{
            .label = label,
            .enabled = canUse,
            .onSelect = [this, skillId, menuIndex, active]()
            {
                m_lastSkillUnit = active;
                m_lastSkillIndex = static_cast<int>(menuIndex);
                auto ctx = m_owner.makeMenuContext();
                auto it = ctx.skillDB.find(skillId);
                if (it != ctx.skillDB.end())
                {
                    if (Unit *active = ctx.session.getCurrentUnit())
                        ctx.currentRangeRule =
                            it->second.toRangeRuleFor(active->getAtkRange(), active->getJump());
                    ctx.selectedSkillId = skillId;
                    m_owner.computeAttackRangeTiles();
                }
                if (Unit *active = ctx.session.getCurrentUnit())
                    ctx.cursor.setPosition(active->getPosition());
                ctx.humanTurnPhase = BattleState::HumanTurnPhase::AttackTarget;
                ctx.topBattleText.clear();
                m_skillDetailVisible = false;
                ctx.uiManager.popById(WindowId::BattleSkillMenu);
                m_owner.battleMenu().closeAllMenus();
            },
            // Revealed with Details (Tab) on the highlighted row. The riders
            // matter before you commit, and there is nowhere else for them:
            // the damage preview only shows damage and accuracy.
            .detail = skill_effects::describeSkillDetail(skill)});
        ++menuIndex;
    }

    // Any previously shown description belonged to the old list, and the menu
    // is about to reopen on a different selection.
    m_skillDetailVisible = false;

    ButtonMenuConfig config;
    config.textAlign = ButtonMenuWindow::TextAlign::Left;
    config.anchorBottomRight = true;
    ButtonMenuWindow *menu = pushButtonMenu(WindowId::BattleSkillMenu, config, m_skillMenuItems);
    menu->setInitialSelectedIndex(m_lastSkillIndex);
    if (menu->hasDetails())
        menu->setHintLine("Tab: Details");
    m_owner.maybeShowTutorialTip(BattleState::TutorialTip::Skill);
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
                pushStartBattleConfirm(ctx.uiManager);
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
            // Abandoning the battle counts as a defeat: show the defeat
            // banner, then the normal end-of-battle flow returns the player
            // to the world map at the node they entered from (never the
            // title screen). In a tutorial this also escapes the retry-on-
            // defeat loop (see BattleState::quitFromSystemMenu).
            m_owner.quitFromSystemMenu();
        }};

    m_systemMenuItems = {std::move(firstItem)};

    // The tutorial duel offers no way out. Abandoning it counts as a defeat and
    // drops the player back on the world map with the tutorial half-finished,
    // which is strictly worse than just playing the one-round fight. Testers
    // have tools\clear_saves.bat to reset the campaign instead.
    if (!ctx.tutorial)
        m_systemMenuItems.push_back(std::move(quitItem));

    ButtonMenuConfig config; // all defaults → dead center, both axes, matches actual computed size
    pushButtonMenu(WindowId::BattleSystemMenu, config, m_systemMenuItems);
}

void BattleMenuController::showInspectWindow(Unit *unit)
{
    if (!unit)
        return;

    auto ctx = m_owner.makeMenuContext();
    ctx.uiManager.popById(WindowId::BattleInspect);
    auto *inspect = ctx.uiManager.push<UnitDetailWindow>(
        WindowId::BattleInspect, *unit, PartyContext::instance().gearCatalog());
    inspect->setFont(FontManager::instance().get(FontRole::Body));
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
        auto *inspect = ctx.uiManager.push<UnitDetailWindow>(
            WindowId::BattleInspect, previewUnit, PartyContext::instance().gearCatalog());
        inspect->setFont(FontManager::instance().get(FontRole::Body));
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

    // The battle-inspect detail window. Closing it must reset m_inspectWindow
    // so the render dim (keyed on hasInspectWindowOpen()) is cleared — the
    // same reset closeAllMenus() performs for the system-menu close path.
    if (event.windowId == WindowId::BattleInspect && event.type == UIEventType::ActionCanceled)
    {
        ctx.uiManager.popById(WindowId::BattleInspect);
        m_inspectWindow = nullptr;
        return true;
    }

    // System menu (Resume / Quit, or Start Battle / Quit during deployment)
    // is shared across flow phases. The handler must run before the
    // Combat-only early-return below, otherwise Resume/Quit pressed in
    // combat are dropped (Part L).
    if (event.windowId == WindowId::BattleSystemMenu)
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

    // Tab (Details) toggles the highlighted skill's description in the top
    // battle text. Both the key press and a row move arrive as Details events:
    // the press toggles, a row change re-targets an already-visible
    // description. Without this the top text keeps showing whatever skill was
    // highlighted when Tab was first pressed.
    if (event.windowId == WindowId::BattleSkillMenu && event.actionId == ActionId::Details &&
        (event.type == UIEventType::ActionSelected || event.type == UIEventType::ActionChanged))
    {
        const int index = event.index;
        if (index < 0 || index >= static_cast<int>(m_skillMenuItems.size()))
            return true;

        const std::string &detail = m_skillMenuItems[index].detail;
        if (detail.empty())
            return true;

        if (event.type == UIEventType::ActionSelected)
        {
            // Tab again on the same skill dismisses it.
            if (ctx.topBattleText == detail)
            {
                ctx.topBattleText.clear();
                m_skillDetailVisible = false;
            }
            else
            {
                ctx.topBattleText = detail;
                m_skillDetailVisible = true;
            }
            Input::instance().consumeKey(KeyCode::Details);
        }
        else if (m_skillDetailVisible)
        {
            ctx.topBattleText = detail;
        }
        return true;
    }

    if (event.windowId == WindowId::BattleSkillMenu && event.type == UIEventType::ActionCanceled)
    {
        // The skill submenu was opened on top of a popped action menu (the
        // Skills item's handler pops BattleActionMenu, then shows this), so
        // popping just this level would strand phase=ActionMenu with nothing
        // open — the "ESC during skill selection" dead state. Re-open the
        // battle action menu underneath instead.
        ctx.uiManager.popById(WindowId::BattleSkillMenu);
        ctx.selectedSkillId.clear();
        m_owner.openBattleMenu(m_owner.canActiveUnitMove(), active && !active->hasActed(), true, KeyCode::Back);
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
    ctx.uiManager.popById(WindowId::BattleInspect);
    m_inspectWindow = nullptr;
}