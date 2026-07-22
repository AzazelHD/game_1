#include "engine/core/App.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/MathUtils.h"
#include "engine/renderer/FontManager.h"
#include "battle/HumanTurnController.h"
#include "scenes/BattleState.h"
#include "battle/Unit.h"
#include "battle/CombatSystem.h"
#include "ui/windows/ConfirmWindow.h"

#include <cstdio>

void HumanTurnController::handleActiveTurn(const Input &input)
{
    BattleState::HumanTurnContext ctx = m_state.makeHumanTurnContext();

    if (ctx.controlMode != BattleState::PlayerControlMode::Human)
        return;

    Unit *active = ctx.session.getCurrentUnit();
    if (!active || active->getTeam() != 0)
        return;

    if (ctx.hudOpen)
        return;

    if (ctx.phase == BattleState::HumanTurnPhase::FreeCursor)
    {
        if (input.isKeyPressed(KeyCode::Details, false))
        {
            Vec2i cursorPos = ctx.cursor.getPosition();
            Unit *hovered = nullptr;
            for (Unit *u : ctx.session.getUnitPtrs())
                if (u && !u->isDead() && u->getPosition() == cursorPos)
                {
                    hovered = u;
                    break;
                }

            if (hovered)
                m_state.showInspectWindow(hovered);
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
                Unit *hovered = nullptr;
                for (Unit *u : ctx.session.getUnitPtrs())
                    if (u && !u->isDead() && u->getPosition() == cursorPos)
                    {
                        hovered = u;
                        break;
                    }

                if (hovered)
                {
                    if (hovered == active)
                        m_state.showInspectWindow(hovered); // yourself: straight to stats
                    else
                        m_state.openUnitInspectMenu(hovered); // anyone else — ally, neutral, or enemy
                }
                else
                    ctx.cursor.setPosition(active->getPosition());
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

                const int pathCost = manhattanDistance(ctx.moveStartPos, dest);

                Vec2i start = active->getPosition();
                ctx.grid.getTile(start).occupied = false;
                active->setPosition(dest);
                ctx.grid.getTile(dest).occupied = true;
                active->spendMovePoints(pathCost);
                ctx.canUndoLastMove = true;
                ctx.eventSystem.emit(BattleTriggerType::OnTileEnter);

                ctx.phase = BattleState::HumanTurnPhase::ActionMenu;
                m_state.openBattleMenu(m_state.canActiveUnitMove(), !active->hasActed(), true, KeyCode::Accept);
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
            Unit *hoveredEnemy = nullptr;
            for (Unit *u : ctx.session.getUnitPtrs())
                if (u && !u->isDead() && u->getTeam() != 0 && u->getPosition() == cursorPos)
                {
                    hoveredEnemy = u;
                    break;
                }

            if (hoveredEnemy)
            {
                int dist = manhattanDistance(active->getPosition(), hoveredEnemy->getPosition());
                if (dist <= ctx.currentAttackRange)
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
                Unit *target = nullptr;
                for (Unit *u : ctx.session.getUnitPtrs())
                    if (u && !u->isDead() && u->getTeam() != 0 && u->getPosition() == targetPos)
                    {
                        target = u;
                        break;
                    }

                const SkillData *skill = nullptr;
                if (!ctx.selectedSkillId.empty())
                {
                    auto it = ctx.skillDB.find(ctx.selectedSkillId);
                    if (it != ctx.skillDB.end())
                        skill = &it->second;
                }

                bool canAttack = (target != nullptr);

                if (!canAttack && skill && skill->area > 0)
                {
                    for (Unit *u : ctx.session.getUnitPtrs())
                    {
                        if (u && !u->isDead() && u->getTeam() != 0 &&
                            manhattanDistance(u->getPosition(), targetPos) <= skill->area)
                        {
                            canAttack = true;
                            break;
                        }
                    }
                }

                if (!canAttack)
                    return;

                int dist = manhattanDistance(active->getPosition(), targetPos);
                if (dist > ctx.currentAttackRange)
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
}
