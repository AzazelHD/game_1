#include "battle/controllers/DeploymentPhaseController.h"

#include "engine/renderer/Camera.h"
#include "scenes/BattleState.h"
#include "battle/map/GameTile.h"
#include "battle/unit/Unit.h"
#include "battle/unit/UnitFactory.h"
#include "battle/unit/UnitProgression.h"
#include "config/BattleCatalog.h"
#include "config/GameConstants.h"
#include "data/UnitLoader.h"
#include "engine/renderer/FontManager.h"
#include "battle/systems/BattleParticipantsBuilder.h"
#include "systems/PartyContext.h"
#include "ui/ActionId.h"
#include "ui/UIEvent.h"
#include "ui/WindowId.h"
#include "ui/windows/ConfirmWindow.h"
#include "ui/windows/UnitDetailWindow.h"

#include <unordered_set>

void DeploymentPhaseController::initializeDeploymentPhase()
{
    BattleState::DeploymentContext ctx = m_owner.makeDeploymentContext();

    PartyContext &partyCtx = PartyContext::instance();
    partyCtx.ensureInitialized();

    std::unordered_set<Vec2i, Vec2iHash> spawnTiles;
    for (GameTile *tile : ctx.battleMap.playerSpawns)
    {
        if (tile)
            spawnTiles.insert(Vec2i{tile->col, tile->row});
    }

    static const std::vector<ForcedUnitRule> kNoForcedUnits;
    m_deployment.initialize(
        ctx.battleDefinition ? ctx.battleDefinition->maxUnits : 1,
        std::move(spawnTiles),
        partyCtx.party().memberIds(),
        partyCtx.roster(),
        ctx.battleDefinition ? ctx.battleDefinition->forcedUnits : kNoForcedUnits);

    if (!ctx.battleMap.playerSpawns.empty() && ctx.battleMap.playerSpawns.front())
    {
        ctx.cursor.setPosition(Vec2i{ctx.battleMap.playerSpawns.front()->col, ctx.battleMap.playerSpawns.front()->row});
    }

    syncDeploymentPreviewUnits();
    refreshDeploymentWindow();
}

void DeploymentPhaseController::syncDeploymentPreviewUnits()
{
    BattleState::DeploymentContext ctx = m_owner.makeDeploymentContext();

    ctx.hoveredUnit = nullptr;
    ctx.inspectTargetUnit = nullptr;
    ctx.uiManager.popById(WindowId::BattleInspectMenu);

    for (Unit *u : m_deploymentPreviewUnits)
        delete u;
    m_deploymentPreviewUnits.clear();

    for (int y = 0; y < ctx.grid.getHeight(); ++y)
    {
        for (int x = 0; x < ctx.grid.getWidth(); ++x)
            ctx.grid.getTile(Vec2i{x, y}).occupied = false;
    }

    for (const DeploymentEntry &placed : m_deployment.deployed())
    {
        Unit *u = UnitFactory::createUnitFromJson(placed.templatePath, placed.position, 0);
        if (!u)
            continue;

        m_deploymentPreviewUnits.push_back(u);
        if (ctx.grid.isValid(u->getPosition()))
            ctx.grid.getTile(u->getPosition()).occupied = true;
    }

    if (!ctx.battleDefinition)
        return;

    std::vector<GameTile *> enemySpawns;
    for (const auto &pair : ctx.battleMap.enemySpawnsByTeam)
    {
        for (GameTile *tile : pair.second)
            enemySpawns.push_back(tile);
    }

    std::size_t spawnIndex = 0;
    for (const EnemyDefinition &enemy : ctx.battleDefinition->enemies)
    {
        if (enemySpawns.empty())
            break;

        GameTile *tile = enemySpawns[spawnIndex % enemySpawns.size()];
        ++spawnIndex;
        if (!tile)
            continue;

        Vec2i pos{tile->col, tile->row};
        if (!ctx.grid.isValid(pos) || ctx.grid.getTile(pos).occupied)
            continue;

        Unit *u = UnitFactory::createUnitFromJson(enemy.templatePath, pos, enemy.team);
        if (!u)
            continue;

        m_deploymentPreviewUnits.push_back(u);
        ctx.grid.getTile(pos).occupied = true;
    }
}

void DeploymentPhaseController::refreshDeploymentWindow()
{
    if (!m_deploymentWindow)
        return;

    const DeploymentEntry *selected = m_deployment.selectedEntry();
    const DeploymentEntry *grabbed = m_deployment.grabbedEntry();
    m_deploymentWindow->setSelectedUnitLabel(selected ? loadUnitDisplayName(selected->templatePath) : std::string());
    m_deploymentWindow->setGrabbedState(grabbed != nullptr, grabbed ? loadUnitDisplayName(grabbed->templatePath) : std::string());
    m_deploymentWindow->setDeploymentStatus(m_deployment.placedCount(), m_deployment.maxUnits(), m_deployment.canStartBattle());
}

void DeploymentPhaseController::syncCursorToSelection()
{
    BattleState::DeploymentContext ctx = m_owner.makeDeploymentContext();
    if (const DeploymentEntry *entry = m_deployment.selectedEntry())
    {
        if (const DeploymentEntry *placed = m_deployment.deployedEntryFor(entry->instanceId))
            ctx.cursor.setPosition(placed->position);
    }
}

void DeploymentPhaseController::setUnitPanelPreviewFromEntry(const DeploymentEntry *entry)
{
    BattleState::DeploymentContext ctx = m_owner.makeDeploymentContext();
    if (!ctx.unitPanelWindow)
        return;
    if (!entry)
    {
        ctx.unitPanelWindow->clearPreview();
        return;
    }

    const bool isPlaced = m_deployment.isUnitPlaced(entry->instanceId);

    try
    {
        PartyContext &partyContext = PartyContext::instance();
        partyContext.ensureInitialized();

        const UnitData templateData = UnitLoader::load(entry->templatePath);
        const RaceData &raceData = getRaceData(templateData.race);
        const GenderData &genderData = getGenderData(templateData.gender);
        Unit previewUnit(templateData, raceData, genderData, Vec2i{0, 0});
        if (const RosterUnit *rosterUnit = partyContext.roster().findById(entry->instanceId))
        {
            previewUnit.setResolvedEquipmentLoadout(
                partyContext.roster().resolveLoadout(*rosterUnit, partyContext.gearCatalog()));
        }
        const UnitData &data = previewUnit.getData();
        ctx.unitPanelWindow->setPreview(data.name, previewUnit.getLevel(),
                                        previewUnit.getMaxHp(), previewUnit.getMaxMp(), false, isPlaced);
    }
    catch (...)
    {
        ctx.unitPanelWindow->setPreview("Unit " + std::to_string(entry->instanceId), 1, 1, 0, 0, isPlaced);
    }
}

void DeploymentPhaseController::startCombatPhase()
{
    BattleState::DeploymentContext ctx = m_owner.makeDeploymentContext();

    if (ctx.flowPhase != BattleState::BattleFlowPhase::Deployment || !ctx.battleDefinition)
        return;

    ctx.hoveredUnit = nullptr;
    ctx.inspectTargetUnit = nullptr;
    ctx.uiManager.popById(WindowId::BattleInspectMenu);

    for (int y = 0; y < ctx.grid.getHeight(); ++y)
    {
        for (int x = 0; x < ctx.grid.getWidth(); ++x)
            ctx.grid.getTile(Vec2i{x, y}).occupied = false;
    }

    std::vector<UnitSpawn> spawns = BattleParticipantsBuilder::build(*ctx.battleDefinition, ctx.battleMap, m_deployment);
    if (spawns.empty())
        return;

    ctx.session.init(spawns, ctx.battleDefinition->victoryRule, ctx.battleDefinition->defeatRule);

    for (Unit *u : ctx.session.getUnitPtrs())
    {
        if (u && ctx.grid.isValid(u->getPosition()))
            ctx.grid.getTile(u->getPosition()).occupied = true;
    }

    if (Unit *first = ctx.session.getCurrentUnit(); first)
        ctx.cursor.setPosition(first->getPosition());

    ctx.uiManager.popById(WindowId::BattleDeployment);
    ctx.uiManager.popById(WindowId::BattleDeploymentConfirm);
    m_deploymentWindow = nullptr;
    if (ctx.unitPanelWindow)
        ctx.unitPanelWindow->clearPreview();

    ctx.flowPhase = BattleState::BattleFlowPhase::Combat;
    ctx.pendingResolution = BattleState::PendingResolution::None;
    ctx.pendingActor = nullptr;
    ctx.pendingTarget = nullptr;
    ctx.pendingActionLabel.clear();
    ctx.topBattleText.clear();
    ctx.turnState = BattleState::TurnState::ProcessingTurn;
    m_owner.processCurrentTurn();

    ctx.eventSystem.emit(BattleTriggerType::OnBattleStart);
}

void DeploymentPhaseController::update(float dt)
{
    BattleState::DeploymentContext ctx = m_owner.makeDeploymentContext();

    if (!ctx.uiManager.hasBlockingWindow() && !m_deployment.isSelectionLocked())
    {
        ctx.cursor.update(ctx.battleMap.cols(), ctx.battleMap.rows(), dt);
        const Vec2i pos = ctx.cursor.getPosition();
        const Vec2f isoPos = tileToIso(pos, ctx.mapData.tileWidth, ctx.mapData.tileHeight);
        ctx.camera.trackTarget(isoPos, Vec2f{GameConstants::VIEW_W, GameConstants::VIEW_H}, dt);
        ctx.camera.clampToBounds();
    }

    Vec2i cursorPos = ctx.cursor.getPosition();
    ctx.hoveredUnit = previewUnitAt(cursorPos);

    refreshDeploymentWindow();

    if (ctx.unitPanelWindow)
    {
        if (m_deployment.hasGrabbedUnit())
        {
            setUnitPanelPreviewFromEntry(m_deployment.grabbedEntry());
        }
        else if (ctx.hoveredUnit)
        {
            if (const DeploymentEntry *placed = m_deployment.deployedEntryAt(cursorPos))
                setUnitPanelPreviewFromEntry(placed);
            else
                ctx.unitPanelWindow->setSingle(ctx.hoveredUnit, ctx.hoveredUnit->getTeam());
        }
        else
        {
            setUnitPanelPreviewFromEntry(m_deployment.selectedEntry());
        }
    }
}

Unit *DeploymentPhaseController::previewUnitAt(Vec2i pos) const
{
    return unitAt(m_deploymentPreviewUnits, pos);
}

void DeploymentPhaseController::releaseGrabbedUnit()
{
    m_deployment.releaseGrabbed();
    refreshDeploymentWindow();
}

void DeploymentPhaseController::resetOnExit()
{
    for (Unit *u : m_deploymentPreviewUnits)
        delete u;
    m_deploymentPreviewUnits.clear();
    m_deploymentWindow = nullptr;
}

bool DeploymentPhaseController::handleUIEvent(const UIEvent &event)
{
    BattleState::DeploymentContext ctx = m_owner.makeDeploymentContext();

    if (event.windowId == WindowId::Equipment && event.type == UIEventType::ActionCanceled)
    {
        ctx.uiManager.popById(WindowId::Equipment);
        return true;
    }

    if (event.windowId == WindowId::BattleDeploymentConfirm && event.type == UIEventType::ConfirmResult)
    {
        ctx.uiManager.popById(WindowId::BattleDeploymentConfirm);
        if (event.confirmed)
            startCombatPhase();
        return true;
    }

    if (event.windowId != WindowId::BattleDeployment || event.type != UIEventType::ActionSelected)
        return false;

    const bool wasGrabbed = m_deployment.hasGrabbedUnit();

    if (event.actionId == ActionId::CyclePrev)
    {
        m_deployment.cycleSelection(-1);
        if (!wasGrabbed)
            syncCursorToSelection();
        else
            syncDeploymentPreviewUnits();
        refreshDeploymentWindow();
        return true;
    }
    if (event.actionId == ActionId::CycleNext)
    {
        m_deployment.cycleSelection(1);
        if (!wasGrabbed)
            syncCursorToSelection();
        else
            syncDeploymentPreviewUnits();
        refreshDeploymentWindow();
        return true;
    }
    if (event.actionId == ActionId::Accept)
    {
        const Vec2i cursorPos = ctx.cursor.getPosition();

        if (wasGrabbed)
        {
            if (!m_deployment.isSpawnTile(cursorPos))
                return true; // TODO: play "impossible action" sound

            if (m_deployment.isOccupied(cursorPos))
            {
                if (m_deployment.swapGrabbedWithPlacedAt(cursorPos))
                {
                    syncDeploymentPreviewUnits();
                    refreshDeploymentWindow();
                }
                return true;
            }

            if (m_deployment.placeGrabbed(cursorPos))
            {
                syncDeploymentPreviewUnits();
                syncCursorToSelection();
                refreshDeploymentWindow();
            }
            return true;
        }

        if (ctx.hoveredUnit && ctx.hoveredUnit->getTeam() != 0)
        {
            m_owner.battleMenu().showInspectWindow(ctx.hoveredUnit);
            return true;
        }

        if (const DeploymentEntry *placedHere = m_deployment.deployedEntryAt(cursorPos))
        {
            const int instanceId = placedHere->instanceId;
            if (m_deployment.unplaceUnit(instanceId) && m_deployment.grabUnit(instanceId))
            {
                syncDeploymentPreviewUnits();
                refreshDeploymentWindow();
            }
            return true;
        }

        const DeploymentEntry *selected = m_deployment.selectedEntry();
        if (!selected)
            return true;

        if (m_deployment.isUnitPlaced(selected->instanceId))
        {
            if (m_deployment.unplaceUnit(selected->instanceId) && m_deployment.grabUnit(selected->instanceId))
            {
                syncDeploymentPreviewUnits();
                refreshDeploymentWindow();
            }
            return true;
        }

        if (!m_deployment.isOccupied(cursorPos))
        {
            if (m_deployment.placedCount() >= m_deployment.maxUnits())
                return true; // TODO: play "impossible action" sound

            if (m_deployment.grabSelected())
                refreshDeploymentWindow();
        }
        else
        {
            refreshDeploymentWindow();
        }
        return true;
    }
    if (event.actionId == ActionId::Details)
    {
        PartyContext &partyContext = PartyContext::instance();
        partyContext.ensureInitialized();

        const DeploymentEntry *target = m_deployment.grabbedEntry();
        if (!target)
        {
            if (ctx.hoveredUnit && !ctx.hoveredUnit->isDead())
            {
                if (ctx.hoveredUnit->getTeam() != 0)
                {
                    // Enemy/neutral preview unit — no roster entry to equip;
                    // show its live stats instead of the selected player unit.
                    m_owner.battleMenu().showInspectWindow(ctx.hoveredUnit);
                    return true;
                }
                const DeploymentEntry *hoverEntry = m_deployment.deployedEntryAt(ctx.hoveredUnit->getPosition());
                if (hoverEntry)
                    target = hoverEntry;
            }
        }
        if (!target)
            target = m_deployment.selectedEntry();
        if (!target)
            return true;

        const RosterUnit *rosterUnit = partyContext.roster().findById(target->instanceId);
        if (!rosterUnit)
            return true;

        try
        {
            auto *details = ctx.uiManager.push<UnitDetailWindow>(
                WindowId::Equipment, *rosterUnit, partyContext.roster(),
                partyContext.inventory(), partyContext.gearCatalog());
            details->setFont(FontManager::instance().get(FontRole::Body));
        }
        catch (...)
        {
        }
        return true;
    }
    if (event.actionId == ActionId::Back)
    {
        if (wasGrabbed)
        {
            m_deployment.releaseGrabbed();
            refreshDeploymentWindow();
            return true;
        }
        m_owner.battleMenu().showSystemMenu();
        return true;
    }
    if (event.actionId == ActionId::StartCombat)
    {
        if (!m_deployment.canStartBattle())
            return true;

        ctx.uiManager.popById(WindowId::BattleDeploymentConfirm);
        auto *confirm = ctx.uiManager.push<ConfirmWindow>(WindowId::BattleDeploymentConfirm);
        confirm->setFont(FontManager::instance().get(FontRole::Body));
        confirm->setPrompt("Start Battle?");
        return true;
    }

    return true; // matched BattleDeployment/ActionSelected but no known actionId — still "ours"
}