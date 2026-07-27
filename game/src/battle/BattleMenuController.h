#pragma once

#include "ui/BattleMenuItem.h"
#include "engine/math/Vec2.h"

#include <string>
#include <vector>

class BattleState;
class Unit;
struct UIEvent;

// Owns everything specific to Battle's menu layer: the main action menu
// (Move/Attack/Skills/Defend/Wait), the skill submenu, the system menu
// (Start Battle/Resume + Quit), and the unit-inspect flow (inspect menu +
// the InspectWindow itself). Shared turn-flow state (HumanTurnPhase,
// TurnState, move-undo bookkeeping, etc.) stays owned by BattleState and is
// reached through MenuContext — the same pattern
// AttackResolutionController/DeploymentPhaseController already use.
class BattleMenuController
{
public:
    explicit BattleMenuController(BattleState &owner) : m_owner(owner) {}

    void showBattleMenu(bool canMove, bool canAttack, bool canWait);
    void showSkillMenu();
    void showSystemMenu();

    void showInspectWindow(Unit *unit);
    [[nodiscard]] bool hasInspectWindowOpen() const { return m_inspectWindow != nullptr; }
    void showStatusMenu(Unit *unit);
    void openStatusMenu(Unit *unit);
    void openUnitInspectMenu(Unit *unit);
    void showInspectWindowFromTemplate(const std::string &templatePath);

    // Routes a UIManager event addressed to any window this controller
    // owns (BattleActionMenu, BattleSkillMenu, BattleInspectMenu,
    // BattleInspect). Returns true if the event was handled.
    bool handleUIEvent(const UIEvent &event, Unit *active);

private:
    BattleState &m_owner;

    std::vector<BattleMenuItem> m_skillMenuItems;
    class UnitInspectWindow *m_inspectWindow = nullptr;
};