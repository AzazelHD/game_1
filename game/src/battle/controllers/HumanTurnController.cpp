#include "engine/core/App.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/renderer/FontManager.h"
#include "battle/controllers/HumanTurnController.h"
#include "battle/unit/Unit.h"
#include "battle/map/AttackRange.h"
#include "battle/combat/CombatSystem.h"
#include "scenes/BattleState.h"
#include "ui/windows/ConfirmWindow.h"

#include <cstdio>
#include <unordered_set>

void HumanTurnController::handleActiveTurn(const Input &input)
{
    BattleState::HumanTurnContext ctx = m_state.makeHumanTurnContext();

    if (ctx.controlMode != BattleState::PlayerControlMode::Human)
        return;

    Unit *active = ctx.session.getCurrentUnit();
    if (!active || active->getTeam() != 0)
        return;

    if (ctx.uiManager.hasBlockingWindow())
        return;

    if (m_state.movementAnimation().isAnimating())
        return;

    if (ctx.phase == BattleState::HumanTurnPhase::FreeCursor)
    {
        if (input.isKeyPressed(KeyCode::Details, false))
        {
            Vec2i cursorPos = ctx.cursor.getPosition();
            Unit *hovered = unitAt(ctx.session.getUnitPtrs(), cursorPos);

            if (hovered)
                m_state.battleMenu().showInspectWindow(hovered);
            return;
        }

        if (input.isKeyPressed(KeyCode::Accept, false))
        {
            Vec2i cursorPos = ctx.cursor.getPosition();

            if (cursorPos == active->getPosition())
            {
                ctx.phase = BattleState::HumanTurnPhase::ActionMenu;
                m_state.openBattleMenu(m_state.canActiveUnitMove(), !active->hasActed(), true, KeyCode::Accept);
            }
            else
            {
                Unit *hovered = unitAt(ctx.session.getUnitPtrs(), cursorPos);

                if (hovered)
                {
                    if (hovered == active)
                        m_state.battleMenu().showInspectWindow(hovered); // yourself: straight to stats
                    else
                        m_state.battleMenu().openUnitInspectMenu(hovered); // anyone else — ally, neutral, or enemy
                }
                else
                    ctx.cursor.setPosition(active->getPosition());
            }
        }
    }
    else if (ctx.phase == BattleState::HumanTurnPhase::MoveTarget)
    {
        if (input.isKeyPressed(KeyCode::Accept, false))
        {
            Vec2i dest = ctx.cursor.getPosition();
            if (ctx.reachableTiles.find(dest) == ctx.reachableTiles.end())
                return;

            ctx.moveStartPos = active->getPosition();
            ctx.moveStartPointsLeft = active->getMoveRangeLeft();

            // Real BFS-accumulated cost from MovementRange — never re-derive
            // via manhattanDistance.
            const int pathCost = ctx.reachableCosts.at(dest);

            ctx.phase = BattleState::HumanTurnPhase::ActionMenu;
            m_state.beginUnitWalk(active, dest, pathCost,
                                  [this, active]()
                                  { m_state.openBattleMenu(m_state.canActiveUnitMove(), !active->hasActed(), true, KeyCode::Accept); });
        }
        else if (input.isKeyPressed(KeyCode::Back, false))
        {
            ctx.phase = BattleState::HumanTurnPhase::ActionMenu;
            m_state.openBattleMenu(m_state.canActiveUnitMove(), !active->hasActed(), true, KeyCode::Back);
        }
    }
    else if (ctx.phase == BattleState::HumanTurnPhase::AttackTarget)
    {
        Vec2i cursorPos = ctx.cursor.getPosition();
        Unit *hoveredEnemy = unitAt(ctx.session.getUnitPtrs(), cursorPos, true);

        if (hoveredEnemy)
        {
            if (AttackRange::canTarget(ctx.grid, ctx.battleMap, active->getPosition(),
                                       hoveredEnemy->getPosition(), ctx.currentRangeRule))
            {
                const SkillData *previewSkill = nullptr;
                if (!ctx.selectedSkillId.empty())
                {
                    auto it = ctx.skillDB.find(ctx.selectedSkillId);
                    if (it != ctx.skillDB.end())
                        previewSkill = &it->second;
                }
                ctx.damagePreview.show(*active, *hoveredEnemy, previewSkill);

                HitContext hitCtx = m_state.makeHitContext(active, hoveredEnemy, previewSkill);

                const CombatResult preview = CombatSystem::preview(hitCtx);
                char textBuf[96];
                std::snprintf(textBuf, sizeof(textBuf), "DMG %d   ACC %d%%", preview.damage, static_cast<int>(preview.hitChance + 0.5f));
                ctx.topBattleText = textBuf;
            }
            else
            {
                ctx.damagePreview.hide();
                ctx.topBattleText.clear();
            }
        }
        else
        {
            ctx.damagePreview.hide();
            ctx.topBattleText.clear();
        }

        if (input.isKeyPressed(KeyCode::Accept, false))
        {
            Vec2i targetPos = ctx.cursor.getPosition();
            Unit *target = unitAt(ctx.session.getUnitPtrs(), targetPos, true);

            const SkillData *skill = nullptr;
            if (!ctx.selectedSkillId.empty())
            {
                auto it = ctx.skillDB.find(ctx.selectedSkillId);
                if (it != ctx.skillDB.end())
                    skill = &it->second;
            }

            bool canAttack = (target != nullptr);

            if (!canAttack && skill && skill->area > 0 &&
                AttackRange::canTarget(ctx.grid, ctx.battleMap, active->getPosition(),
                                       targetPos, ctx.currentRangeRule))
            {
                const std::unordered_set<Vec2i, Vec2iHash> splash =
                    AttackRange::computeSplashTiles(ctx.grid, ctx.battleMap, targetPos,
                                                    ctx.currentRangeRule);
                for (Unit *u : ctx.session.getUnitPtrs())
                {
                    if (u && !u->isDead() && u->getTeam() != 0 &&
                        splash.count(u->getPosition()))
                    {
                        canAttack = true;
                        break;
                    }
                }
            }

            if (!canAttack)
                return;

            if (!AttackRange::canTarget(ctx.grid, ctx.battleMap, active->getPosition(),
                                        targetPos, ctx.currentRangeRule))
                return;

            const SkillData *skillToUse = nullptr;
            if (!ctx.selectedSkillId.empty())
            {
                auto it = ctx.skillDB.find(ctx.selectedSkillId);
                if (it != ctx.skillDB.end())
                    skillToUse = &it->second;
            }

            m_state.preparePendingAttack(active, targetPos, target, skillToUse);

            if (ctx.pendingAttack.empty())
                return;

            // Snap the cursor to whichever target ended up focused first —
            // regardless of who's first in the vector — instead of leaving
            // it on the casting tile.
            if (Unit *focus = ctx.pendingAttack.focusedTarget())
                ctx.cursor.setPosition(focus->getPosition());

            ctx.phase = BattleState::HumanTurnPhase::AttackConfirm;
            ctx.pendingSkillId = ctx.selectedSkillId;
            ctx.uiManager.popById(WindowId::BattleActionConfirm);
            auto *confirm = ctx.uiManager.push<ConfirmWindow>(WindowId::BattleActionConfirm);
            confirm->setFont(FontManager::instance().get(FontRole::Body));
            confirm->setPrompt("Confirm?");
            confirm->setConfirmSelectedByDefault();
        }
        else if (input.isKeyPressed(KeyCode::Back, false))
        {
            ctx.damagePreview.hide();
            ctx.selectedSkillId.clear();
            ctx.topBattleText.clear();
            ctx.phase = BattleState::HumanTurnPhase::ActionMenu;
            m_state.openBattleMenu(m_state.canActiveUnitMove(), true, true, KeyCode::Back);
        }
    }
}
