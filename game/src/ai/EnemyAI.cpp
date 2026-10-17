#include "ai/EnemyAI.h"
#include "battle/map/Grid.h"
#include "battle/unit/Unit.h"
#include "battle/map/BattleMap.h"
#include "battle/map/MovementRange.h"
#include "battle/map/AttackRange.h"
#include "battle/combat/CombatSystem.h"
#include "engine/core/Log.h"
#include "engine/math/Vec2.h"
#include "engine/math/MathUtils.h"

#include <vector>
#include <limits>
#include <unordered_set>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace
{
    // Scores a potential target based on multiple factors:
    // - Distance (closer = better)
    // - HP percentage (low HP = finish them)
    // - Kill potential (can we kill this turn?)
    // - Ally focus (how many allies are targeting this unit)
    float evaluateTarget(const Unit *self, const Unit *target, const std::vector<Unit *> &allUnits)
    {
        float score = 0.0f;

        // Distance: closer is better (inverse, max 100)
        const int dist = manhattanDistance(self->getPosition(), target->getPosition());
        score += 100.0f / (static_cast<float>(dist) + 1.0f);

        // Low HP priority: finish wounded enemies
        const float hpPercent = static_cast<float>(target->getCurrentHp()) / static_cast<float>(target->getMaxHp());
        score += 50.0f * (1.0f - hpPercent);

        // Execute priority: can we kill it this turn?
        if (target->getCurrentHp() <= self->getAttack())
            score += 80.0f;

        // Ally focus: count allies that can attack this target
        int allyFocus = 0;
        for (const Unit *u : allUnits)
        {
            if (!u || u->isDead())
                continue;
            if (u == self)
                continue;
            if (u->getTeam() != self->getTeam())
                continue;

            const int allyDist = manhattanDistance(u->getPosition(), target->getPosition());
            if (allyDist <= u->getAtkRange())
                allyFocus++;
        }
        score += static_cast<float>(allyFocus) * 20.0f;

        return score;
    }

    // Returns the best enemy target for this unit, or nullptr if none.
    // Uses scoring system: closest + low HP + kill potential + ally focus.
    Unit *findBestTarget(const Unit &self, std::vector<Unit *> &allUnits)
    {
        Unit *bestUnit = nullptr;
        float bestScore = -1.0f;

        for (Unit *u : allUnits)
        {
            if (!u || u->isDead())
                continue;
            if (u->getTeam() == self.getTeam())
                continue; // Don't target allies

            const float score = evaluateTarget(&self, u, allUnits);
            if (score > bestScore)
            {
                bestScore = score;
                bestUnit = u;
            }
        }

        return bestUnit;
    }

    // Among the reachable tiles, find the one that is closest (Manhattan) to
    // the target position.  Prefers the unit's current position as a fallback
    // so the unit always ends up somewhere valid even when the set is empty.
    Vec2i bestTileToward(const Unit &unit,
                         const std::unordered_set<Vec2i, Vec2iHash> &reachable,
                         Vec2i targetPos,
                         const Grid &grid,
                         const std::vector<Unit *> &allUnits)
    {
        // Build a set of positions occupied by OTHER living units so we do not
        // move into them.
        std::unordered_set<Vec2i, Vec2iHash> blocked;
        for (const Unit *u : allUnits)
        {
            if (!u || u->isDead())
                continue;
            if (u == &unit)
                continue;
            blocked.insert(u->getPosition());
        }

        Vec2i best = unit.getPosition(); // safe default: stay put
        int bestDist = manhattanDistance(unit.getPosition(), targetPos);

        for (const Vec2i &tile : reachable)
        {
            // Don't step into a tile another unit already occupies.
            if (blocked.count(tile))
                continue;

            const int d = manhattanDistance(tile, targetPos);
            if (d < bestDist)
            {
                bestDist = d;
                best = tile;
            }
        }
        return best;
    }
} // namespace

EnemyAI::EnemyTurnPlan EnemyAI::planTurn(Unit &unit, Grid &grid, const BattleMap &battleMap, std::vector<Unit *> &allUnits)
{
    EnemyTurnPlan plan;

    if (unit.hasMoved() && unit.hasActed())
        return plan;

    plan.target = findBestTarget(unit, allUnits);
    if (!plan.target)
    {
        LOG_INFO("EnemyAI", "%s has no target – waiting", unit.getName().c_str());
        return plan;
    }

    if (!unit.hasMoved())
    {
        const Vec2i startPos = unit.getPosition();
        const int moveRange = unit.getMoveRangeLeft();

        auto reachable = MovementRange::compute(grid, battleMap, startPos, moveRange,
                                                unit.getTeam(), allUnits, unit.getJump(),
                                                MovementRangeOptions{.ignoreUnitCollisionAlongPath = unit.hasGearSpecialEffect(GearSpecialEffect::TeleportMovement)});

        const Vec2i destPos = bestTileToward(unit, reachable.reachable, plan.target->getPosition(),
                                             grid, allUnits);

        bool destinationBlocked = false;
        for (const Unit *u : allUnits)
        {
            if (u && !u->isDead() && u != &unit && u->getPosition() == destPos)
            {
                destinationBlocked = true;
                break;
            }
        }

        if (destPos != startPos && !destinationBlocked)
        {
            plan.wantsToMove = true;
            plan.destination = destPos;
        }
    }

    return plan;
}

EnemyAttackResult EnemyAI::resolveAttack(Unit &unit, Unit *target, const Grid &grid, const BattleMap &battleMap,
                                        const SkillData &basicAttack)
{
    EnemyAttackResult out;

    if (unit.hasActed())
        return out;

    if (!target || target->isDead())
    {
        // No strike rolled — leave hasActed() clear so the turn keeps its
        // move-only cost instead of paying for an attack that never happened.
        return out;
    }

    const RangeRule rule = basicAttack.toRangeRuleFor(unit.getAtkRange(), unit.getJump());
    if (AttackRange::canTarget(grid, battleMap, unit.getPosition(), target->getPosition(), rule))
    {
        const Vec2i delta = target->getPosition() - unit.getPosition();
        if (delta.x != 0 || delta.y != 0)
            unit.setFacing(isoDirectionForTileDelta(delta));

        HitContext ctx = CombatSystem::hitContextFor(basicAttack, unit, *target);
        CombatSystem::applyTileBonuses(ctx, battleMap);
        out.result = CombatSystem::resolve(ctx);
        out.target = target;
        out.attacked = true;
        // Only a rolled strike counts as the turn's action. Marking it here —
        // and not on the no-target/out-of-range paths above — keeps a move-only
        // enemy turn at move cost instead of move+action cost.
        unit.setMajorAction(MajorAction::Attack);

        if (out.result.hit)
        {
            LOG_INFO("EnemyAI", "%s attacks %s for %d damage! (HP: %d/%d)",
                     unit.getName().c_str(), target->getName().c_str(),
                     out.result.damage, target->getCurrentHp(), target->getMaxHp());
        }
        else
        {
            LOG_INFO("EnemyAI", "%s misses %s", unit.getName().c_str(), target->getName().c_str());
        }
    }
    else
    {
        LOG_INFO("EnemyAI", "%s out of range – cannot attack %s (range %d)",
                 unit.getName().c_str(), target->getName().c_str(), unit.getAtkRange());
    }
    return out;
}

void EnemyAI::takeTurn(Unit &unit, Grid &grid, const BattleMap &battleMap, std::vector<Unit *> &allUnits,
                       const SkillData &basicAttack)
{
    if (unit.hasMoved() && unit.hasActed())
        return;

    EnemyTurnPlan plan = planTurn(unit, grid, battleMap, allUnits);
    if (!plan.target)
        return;

    if (!unit.hasMoved())
    {
        if (plan.wantsToMove)
        {
            const Vec2i from = unit.getPosition();
            const Vec2i delta = plan.destination - from;
            grid.getTile(from).occupied = false;
            unit.setPosition(plan.destination);
            grid.getTile(plan.destination).occupied = true;
            if (delta.x != 0 || delta.y != 0)
                unit.setFacing(isoDirectionForTileDelta(delta));
            LOG_INFO("EnemyAI", "%s moves to (%d,%d)", unit.getName().c_str(), plan.destination.x, plan.destination.y);
        }
        unit.exhaustMovement();
    }

    // Sync/debug path: apply the roll immediately (the live battle path goes
    // through BattleState so it can pace + enqueue VFX).
    const EnemyAttackResult atk = resolveAttack(unit, plan.target, grid, battleMap, basicAttack);
    if (atk.attacked && atk.result.hit && atk.target)
        atk.target->takeDamage(atk.result.damage);
}

int EnemyAI::chooseAction(const Unit &unit, const Grid &grid, const BattleMap &battleMap,
                          std::vector<Unit *> &allUnits, const SkillData &basicAttack)
{
    Unit *target = findBestTarget(unit, allUnits);
    if (!target)
        return 2;

    const RangeRule rule = basicAttack.toRangeRuleFor(unit.getAtkRange(), unit.getJump());
    if (AttackRange::canTarget(grid, battleMap, unit.getPosition(), target->getPosition(), rule))
        return 1;
    if (!unit.hasMoved())
        return 0;
    return 2;
}

Unit *EnemyAI::chooseTarget(const Unit &unit, std::vector<Unit *> &allUnits)
{
    return findBestTarget(unit, allUnits);
}