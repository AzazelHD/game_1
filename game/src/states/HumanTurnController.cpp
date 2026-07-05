#include "engine/core/App.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/MathUtils.h"
#include "states/HumanTurnController.h"
#include "states/BattleState.h"
#include "battle/Unit.h"
#include "battle/CombatSystem.h"
#include "ui/windows/ConfirmWindow.h"

#include <cstdio>

void HumanTurnController::handleActiveTurn(const Input &input)
{
    if (m_state.m_playerControlMode != BattleState::PlayerControlMode::Human)
        return;

    Unit *active = m_state.m_turnQueue.getCurrentUnit();
    if (!active || active->getTeam() != 0)
        return;

    if (m_state.m_hud.isOpen())
        return;

    if (m_state.m_humanTurnPhase == BattleState::HumanTurnPhase::FreeCursor)
    {
        if (input.isKeyPressed(KeyCode::Accept, false))
        {
            Vec2i cursorPos = m_state.m_cursor.getPosition();

            if (cursorPos == active->getPosition())
            {
                m_state.m_humanTurnPhase = BattleState::HumanTurnPhase::ActionMenu;
                m_state.openBattleMenu(m_state.canActiveUnitMove(), !active->hasActed(), true, KeyCode::Accept);
            }
            else
            {
                Unit *hovered = nullptr;
                for (Unit *u : m_state.m_units)
                    if (u && !u->isDead() && u->getPosition() == cursorPos)
                    {
                        hovered = u;
                        break;
                    }

                if (hovered)
                    m_state.showInspectWindow(hovered);
                else
                    m_state.m_cursor.setPosition(active->getPosition());
            }
        }
    }
    else if (m_state.m_humanTurnPhase == BattleState::HumanTurnPhase::MoveTarget)
    {
        if (input.isKeyPressed(KeyCode::Accept, false))
        {
            Vec2i dest = m_state.m_cursor.getPosition();
            if (m_state.m_reachableTiles.find(dest) == m_state.m_reachableTiles.end())
                return;

            m_state.m_moveStartPos = active->getPosition();
            m_state.m_moveStartPointsLeft = active->getMoveRangeLeft();

            const int pathCost = manhattanDistance(m_state.m_moveStartPos, dest);

            Vec2i start = active->getPosition();
            m_state.m_grid.getTile(start).occupied = false;
            active->setPosition(dest);
            m_state.m_grid.getTile(dest).occupied = true;
            active->spendMovePoints(pathCost);
            m_state.m_canUndoLastMove = true;
            m_state.m_eventSystem.emit(BattleTriggerType::OnTileEnter);

            m_state.m_humanTurnPhase = BattleState::HumanTurnPhase::ActionMenu;
            m_state.openBattleMenu(m_state.canActiveUnitMove(), !active->hasActed(), true, KeyCode::Accept);
        }
        else if (input.isKeyPressed(KeyCode::Back, false))
        {
            m_state.m_humanTurnPhase = BattleState::HumanTurnPhase::ActionMenu;
            m_state.openBattleMenu(m_state.canActiveUnitMove(), !active->hasActed(), true, KeyCode::Back);
        }
    }
    else if (m_state.m_humanTurnPhase == BattleState::HumanTurnPhase::AttackTarget)
    {
        Vec2i cursorPos = m_state.m_cursor.getPosition();
        Unit *hoveredEnemy = nullptr;
        for (Unit *u : m_state.m_units)
            if (u && !u->isDead() && u->getTeam() != 0 && u->getPosition() == cursorPos)
            {
                hoveredEnemy = u;
                break;
            }

        if (hoveredEnemy)
        {
            int dist = manhattanDistance(active->getPosition(), hoveredEnemy->getPosition());
            if (dist <= m_state.m_currentAttackRange)
            {
                const SkillData *previewSkill = nullptr;
                if (!m_state.m_selectedSkillId.empty())
                {
                    auto it = m_state.m_skillDB.find(m_state.m_selectedSkillId);
                    if (it != m_state.m_skillDB.end())
                        previewSkill = &it->second;
                }
                m_state.m_damagePreview.show(*active, *hoveredEnemy, previewSkill);

                HitContext ctx = m_state.makeHitContext(active, hoveredEnemy, previewSkill);

                const CombatResult preview = CombatSystem::preview(ctx);
                char textBuf[96];
                std::snprintf(textBuf, sizeof(textBuf), "DMG %d   ACC %d%%", preview.damage, static_cast<int>(preview.hitChance + 0.5f));
                m_state.m_topBattleText = textBuf;
            }
            else
            {
                m_state.m_damagePreview.hide();
                m_state.m_topBattleText.clear();
            }
        }
        else
        {
            m_state.m_damagePreview.hide();
            m_state.m_topBattleText.clear();
        }

        if (input.isKeyPressed(KeyCode::Accept, false))
        {
            Vec2i targetPos = m_state.m_cursor.getPosition();
            Unit *target = nullptr;
            for (Unit *u : m_state.m_units)
                if (u && !u->isDead() && u->getTeam() != 0 && u->getPosition() == targetPos)
                {
                    target = u;
                    break;
                }

            const SkillData *skill = nullptr;
            if (!m_state.m_selectedSkillId.empty())
            {
                auto it = m_state.m_skillDB.find(m_state.m_selectedSkillId);
                if (it != m_state.m_skillDB.end())
                    skill = &it->second;
            }

            bool canAttack = (target != nullptr);

            if (!canAttack && skill && skill->area > 0)
            {
                for (Unit *u : m_state.m_units)
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
            if (dist > m_state.m_currentAttackRange)
                return;

            const SkillData *skillToUse = nullptr;
            if (!m_state.m_selectedSkillId.empty())
            {
                auto it = m_state.m_skillDB.find(m_state.m_selectedSkillId);
                if (it != m_state.m_skillDB.end())
                    skillToUse = &it->second;
            }

            m_state.preparePendingAttack(active, targetPos, target, skillToUse);

            if (m_state.m_pendingAttack.empty())
                return;

            m_state.m_humanTurnPhase = BattleState::HumanTurnPhase::AttackConfirm;
            m_state.m_pendingSkillId = m_state.m_selectedSkillId;
            m_state.m_uiManager.popById("battle.confirm");
            auto *confirm = m_state.m_uiManager.push<ConfirmWindow>("battle.confirm");
            confirm->setFont(App::getDefaultFont());
            confirm->setPrompt("Confirm?");
        }
        else if (input.isKeyPressed(KeyCode::Back, false))
        {
            m_state.m_damagePreview.hide();
            m_state.m_selectedSkillId.clear();
            m_state.m_topBattleText.clear();
            m_state.m_humanTurnPhase = BattleState::HumanTurnPhase::ActionMenu;
            m_state.openBattleMenu(m_state.canActiveUnitMove(), true, true, KeyCode::Back);
        }
    }
}