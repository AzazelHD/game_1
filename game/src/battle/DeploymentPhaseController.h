#pragma once

#include "systems/DeploymentSystem.h"
#include "ui/windows/DeploymentWindow.h"
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
// for its own render()/unitAt() needs; every mutation happens in here.
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

    // Read-only access for BattleState::render() and unitAt().
    [[nodiscard]] const DeploymentSystem &deployment() const { return m_deployment; }
    [[nodiscard]] const std::vector<Unit *> &previewUnits() const { return m_deploymentPreviewUnits; }
    [[nodiscard]] Unit *previewUnitAt(Vec2i pos) const;

private:
    void syncDeploymentPreviewUnits();
    void refreshDeploymentWindow();
    void syncCursorToSelection();
    void setUnitPanelPreviewFromEntry(const DeploymentEntry *entry);
    void startCombatPhase();

    BattleState &m_owner;

    DeploymentSystem m_deployment;
    std::vector<Unit *> m_deploymentPreviewUnits;   // owned — deleted in syncDeploymentPreviewUnits()/resetOnExit()
    DeploymentWindow *m_deploymentWindow = nullptr; // non-owning, lives on BattleState's UIManager stack
};