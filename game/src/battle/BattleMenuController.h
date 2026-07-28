#pragma once

#include "ui/BattleMenuItem.h"
#include "ui/WindowId.h"
#include "ui/windows/ButtonMenuWindow.h"
#include "engine/math/Vec2.h"

#include <optional>
#include <string>
#include <vector>

class BattleState;
class Unit;
struct UIEvent;

// Placement/alignment for a pushed ButtonMenuWindow. Each call site still
// decides where its own menu goes (a per-use decision, not something to
// generalize away) — this struct just replaces repeated setter calls with
// one value passed to pushButtonMenu().
struct ButtonMenuConfig
{
    ButtonMenuWindow::TextAlign textAlign = ButtonMenuWindow::TextAlign::Center;
    bool anchorBottomRight = false;
    bool centerHorizontally = false;
    Vec2f bottomRightMargin{16.0f, 16.0f};
    std::optional<Vec2f> panelPosition;
};

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

    void closeAllMenus();

private:
    ButtonMenuWindow *pushButtonMenu(WindowId id,
                                     const ButtonMenuConfig &config,
                                     const std::vector<BattleMenuItem> &items);

    BattleState &m_owner;
    class UnitInspectWindow *m_inspectWindow = nullptr;

    std::vector<BattleMenuItem> m_skillMenuItems;
    std::vector<BattleMenuItem> m_actionMenuItems;
    std::vector<BattleMenuItem> m_systemMenuItems;
};