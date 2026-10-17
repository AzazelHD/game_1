#pragma once

#include "ui/BattleMenuItem.h"
#include "ui/WindowId.h"
#include "ui/windows/ButtonMenuWindow.h"
#include "engine/math/MathUtils.h"
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

    // Wait flow. selectWait() enters the facing picker — used by the Wait menu
    // item and to auto-fire Wait when it is the only option. cancelWaitToMenu()
    // backs out of the picker (Esc) when the unit still has something to do,
    // restoring the entry facing; it returns false when the turn is exhausted
    // and Esc must confirm instead.
    void selectWait();
    bool cancelWaitToMenu();

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
    class UIWindow *m_inspectWindow = nullptr;

    // Whether a skill description is currently shown in the top battle text.
    // A row move then re-targets that text instead of leaving a stale
    // description up. Reset whenever the skill list is (re)opened.
    bool m_skillDetailVisible = false;

    std::vector<BattleMenuItem> m_skillMenuItems;
    std::vector<BattleMenuItem> m_actionMenuItems;
    std::vector<BattleMenuItem> m_systemMenuItems;

    // Last skill the active unit aimed, per unit — so Esc'ing out of
    // target-selection reopens the skill list with the same row hovered
    // instead of resetting to the first skill. The wait entry below is the
    // same idea for the facing picker.
    const Unit *m_lastSkillUnit = nullptr;
    int m_lastSkillIndex = -1;

    // Unit + facing captured when Wait is picked, so Esc-cancel restores the
    // preview-only facing changes made in the picker.
    Unit *m_waitEntryUnit = nullptr;
    IsoDirection m_waitEntryFacing = IsoDirection::SouthWest;
};