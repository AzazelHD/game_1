#include "engine/core/App.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "battle/controllers/HumanTurnController.h"
#include "battle/unit/Unit.h"
#include "battle/map/AttackRange.h"
#include "battle/combat/CombatSystem.h"
#include "battle/combat/SkillEffects.h"
#include "scenes/BattleState.h"

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
            {
                // Unreachable destination: refuse visibly instead of eating the
                // press. Consume so the held Accept cannot re-fire next frame.
                m_state.flashInvalidTarget();
                Input::instance().consumeKey(KeyCode::Accept);
                return;
            }

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
        const SkillData *curSkill = nullptr;
        if (!ctx.selectedSkillId.empty())
        {
            auto it = ctx.skillDB.find(ctx.selectedSkillId);
            if (it != ctx.skillDB.end())
                curSkill = &it->second;
        }
        const bool supportive = curSkill && skill_effects::isSupportive(*curSkill);

        // Deliberately no preview while the tile is still being chosen: the
        // numbers are recomputed every frame for a target that may not be the
        // one cast. The preview appears once a tile is locked in, and from
        // then on it is the confirmation — Accept casts.
        ctx.damagePreview.hide();

        // Any press that cannot produce a valid cast gets the same feedback:
        // the refused-cast vignette plus a swallowed Accept so a held key does
        // not immediately retry. The player stays in AttackTarget.
        const auto refuse = [this]()
        {
            m_state.flashInvalidTarget();
            Input::instance().consumeKey(KeyCode::Accept);
        };

        if (input.isKeyPressed(KeyCode::Accept, false))
        {
            Vec2i targetPos = ctx.cursor.getPosition();
            Unit *target = unitAt(ctx.session.getUnitPtrs(), targetPos, /*enemiesOnly=*/!supportive);

            // Supportive cast on a hostile tile / hostile cast on a friendly
            // tile is not allowed. For single-target (area 0) the tile must
            // even hold a unit at all.
            if (target && supportive && target->getTeam() != active->getTeam())
            {
                refuse();
                return;
            }
            if (target && !supportive && target->getTeam() == active->getTeam())
            {
                refuse();
                return;
            }

            bool canAttack = (target != nullptr);

            if (!canAttack && curSkill && curSkill->area > 0 &&
                AttackRange::canTarget(ctx.grid, ctx.battleMap, active->getPosition(),
                                       targetPos, ctx.currentRangeRule))
            {
                const std::unordered_set<Vec2i, Vec2iHash> splash =
                    AttackRange::computeSplashTiles(ctx.grid, ctx.battleMap, targetPos,
                                                    ctx.currentRangeRule);
                for (Unit *u : ctx.session.getUnitPtrs())
                {
                    if (!u || u->isDead())
                        continue;
                    const bool teamMatch = supportive ? u->getTeam() == active->getTeam()
                                                       : u->getTeam() != active->getTeam();
                    if (teamMatch && splash.count(u->getPosition()))
                    {
                        canAttack = true;
                        break;
                    }
                }
            }

            if (!canAttack)
            {
                refuse();
                return;
            }

            if (!AttackRange::canTarget(ctx.grid, ctx.battleMap, active->getPosition(),
                                        targetPos, ctx.currentRangeRule))
            {
                refuse();
                return;
            }

            m_state.preparePendingAttack(active, targetPos, target, curSkill);

            // Single-target skills that missed, or an AoE splash that catches
            // nobody, are not a legal cast. Stay in AttackTarget so the player
            // can pick a different tile, and flash as feedback.
            if (ctx.pendingAttack.empty())
            {
                refuse();
                return;
            }

            // From here the cursor snaps onto whichever unit is focused in the
            // splash — the blast centre can be empty or adjacent to the real
            // target. Q/E walk that focus through the units caught in it, so
            // the preview can be read for each one in turn.
            ctx.phase = BattleState::HumanTurnPhase::AttackConfirm;
            ctx.pendingSkillId = ctx.selectedSkillId;
            Input::instance().consumeKey(KeyCode::Accept);
        }
        else if (input.isKeyPressed(KeyCode::Back, false))
        {
            ctx.damagePreview.hide();
            // Only a learned skill has a submenu to go back to; the basic
            // attack shares the action menu's own "Attack" button.
            const bool hadSkill = m_state.isLearnedSkillSelected();
            ctx.selectedSkillId.clear();
            ctx.topBattleText.clear();
            ctx.phase = BattleState::HumanTurnPhase::ActionMenu;
            if (hadSkill)
            {
                m_state.battleMenu().showSkillMenu();
                Input::instance().consumeKey(KeyCode::Back);
            }
            else
            {
                m_state.openBattleMenu(m_state.canActiveUnitMove(), true, true, KeyCode::Back);
            }
        }
    }
    else if (ctx.phase == BattleState::HumanTurnPhase::ChooseFacing)
    {
        // Wait was already picked — the turn is committed. Pick the direction
        // the unit faces (previewed live on its feet arrow), then Accept ends
        // the turn. There is deliberately no Back here: an undo is impossible
        // once Wait has been chosen.
        // Facing must equal the direction the cursor moves with the same key
        // (see Cursor::readMovementInput: Up/W = col-1 = NW, Right/D = row-1 =
        // NE, Down/S = col+1 = SE, Left/A = row+1 = SW).
        if (input.isKeyPressed(KeyCode::Up, false) || input.isKeyPressed(KeyCode::W, false))
            active->setFacing(IsoDirection::NorthWest);
        else if (input.isKeyPressed(KeyCode::Right, false) || input.isKeyPressed(KeyCode::D, false))
            active->setFacing(IsoDirection::NorthEast);
        else if (input.isKeyPressed(KeyCode::Down, false) || input.isKeyPressed(KeyCode::S, false))
            active->setFacing(IsoDirection::SouthEast);
        else if (input.isKeyPressed(KeyCode::Left, false) || input.isKeyPressed(KeyCode::A, false))
            active->setFacing(IsoDirection::SouthWest);

        if (input.isKeyPressed(KeyCode::Accept, false))
        {
            ctx.damagePreview.hide();
            ctx.topBattleText.clear();
            ctx.phase = BattleState::HumanTurnPhase::ActionMenu;
            Input::instance().consumeKey(KeyCode::Accept);
            m_state.advanceToNextUnit();
            ctx.turnState = BattleState::TurnState::Idle;
        }
    }
}
