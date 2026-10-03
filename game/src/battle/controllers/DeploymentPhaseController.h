#pragma once

#include "battle/systems/DeploymentSystem.h"
#include "battle/ui/DeploymentWindow.h"
#include "engine/math/Vec2.h"

#include <vector>

class BattleState;
class Unit;
struct UIEvent;

// Owns everything specific to the Deployment phase: the DeploymentSystem
// itself (roster/grab/place/swap rules), the preview Unit objects used to
// render placed/enemy units before combat starts, and the DeploymentWindow
// pointer. BattleState still owns the state this needs to read/write
// (cursor, grid, UIManager, etc.) — reached through DeploymentContext, the
// same pattern AttackResolutionController already uses via
// AttackResolutionContext.
//
// BattleState keeps only read-only accessors (deployment(), previewUnits())
// for its own render() needs; every mutation happens in here.
class DeploymentPhaseController
{
public:
    explicit DeploymentPhaseController(BattleState &owner) : m_owner(owner) {}

    // Called once from BattleState::onEnter(), right after DeploymentWindow
    // is pushed onto the UI stack.
    void setDeploymentWindow(DeploymentWindow *window) { m_deploymentWindow = window; }

    // Builds roster/spawn-tile state for a fresh battle, positions the
    // cursor on the first player spawn.
    void initializeDeploymentPhase();

    // Tutorial only: skips the manual deploy step by dropping the party's
    // units straight onto their spawn tiles (facing the nearest enemy) and
    // starting combat immediately, so the opening duel never shows the facing
    // picker before facing has been explained. Call right after
    // initializeDeploymentPhase().
    void autoDeployForTutorial();

    // Per-frame update while BattleFlowPhase::Deployment is active.
    void update(float dt);

    // Routes a UIManager event addressed to WindowId::BattleDeployment or
    // WindowId::BattleDeploymentConfirm. Returns true if the event belonged
    // to (and was fully handled by) the deployment phase.
    bool handleUIEvent(const UIEvent &event);

    // Tears down deployment-only state — called from BattleState::onExit().
    void resetOnExit();

    [[nodiscard]] bool hasGrabbedUnit() const { return m_deployment.hasGrabbedUnit(); }
    void releaseGrabbedUnit();

    // True while the player is picking the facing of a freshly placed unit.
    // The unit stays on the field (not grabbed); Back reverts placement and
    // puts the unit back on the cursor.
    [[nodiscard]] bool isChoosingFacing() const { return m_isChoosingFacing; }
    [[nodiscard]] const Unit *facingChoiceUnit() const;
    [[nodiscard]] IsoDirection facingChoiceSelection() const;
    // ESC/Back during facing choice: undo the placement and grab the unit
    // back onto the cursor. Shared by the DeploymentWindow Back action and
    // BattleState's own Esc fallback (deployment has no blocking window).
    void undoFacingChoice();

    // Read-only access for BattleState::render().
    [[nodiscard]] const DeploymentSystem &deployment() const { return m_deployment; }
    [[nodiscard]] const std::vector<Unit *> &previewUnits() const { return m_deploymentPreviewUnits; }
    [[nodiscard]] Unit *previewUnitAt(Vec2i pos) const;

private:
    void syncDeploymentPreviewUnits();
    void refreshDeploymentWindow();
    void syncCursorToSelection();
    void setUnitPanelPreviewFromEntry(const DeploymentEntry *entry);
    void startCombatPhase();
    // Points a freshly placed unit's default facing at the nearest non-player
    // (enemy/neutral) preview unit, so the Accept→Accept→Accept placement flow
    // faces toward the action without extra arrow presses. No-op if nothing
    // hostile is on field (or that unit isn't placed yet).
    void presetFacingTowardNearestEnemy(int instanceId, Vec2i pos);

    BattleState &m_owner;

    DeploymentSystem m_deployment;
    std::vector<Unit *> m_deploymentPreviewUnits;   // owned — deleted in syncDeploymentPreviewUnits()/resetOnExit()
    DeploymentWindow *m_deploymentWindow = nullptr; // non-owning, lives on BattleState's UIManager stack
    bool m_isChoosingFacing = false;
    int m_facingChoiceInstanceId = -1;
};