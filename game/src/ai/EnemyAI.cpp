#include "ai/EnemyAI.h"
#include "battle/Grid.h"
#include "battle/Unit.h"
#include "battle/BattleMap.h"
#include "battle/MovementRange.h"
#include "battle/CombatSystem.h"
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

    // Build a minimal HitContext for a basic physical attack.
    HitContext makeAttackContext(const Unit &attacker, const Unit &target)
    {
        HitContext ctx;
        ctx.attacker = &attacker;
        ctx.target = &target;
        ctx.basePower = attacker.getAttack();
        ctx.isMagical = false;
        ctx.element = Element::Neutral;
        ctx.skillAccuracy = 100; // base accuracy for a normal attack
        ctx.side = AttackSide::Front;
        ctx.tileEvasionBonus = 0;
        return ctx;
    }
} // namespace

// ---------------------------------------------------------------------------
// EnemyAI::takeTurn
// ---------------------------------------------------------------------------
//
// NOTE: The AI deliberately uses the simple "move once, then act once" turn
// shape rather than the full player turn-grammar (multi-segment movement,
// undo, etc). It still goes through the same Unit API (spendMovePoints /
// exhaustMovement / setMajorAction) so Unit's turn-state stays consistent
// regardless of which controller (human or AI) drove the turn.

void EnemyAI::takeTurn(Unit &unit, Grid &grid, const BattleMap &battleMap, std::vector<Unit *> &allUnits)
{
    // Already spent both budgets somehow — nothing to do.
    if (unit.hasMoved() && unit.hasActed())
        return;

    // ── 1. Find the best enemy target using scoring system ──────────────────
    Unit *target = findBestTarget(unit, allUnits);

    if (!target)
    {
        // No enemies alive — this is a genuine Wait, not a major action.
        // Waiting is cheaper (BASE_WAIT_COST) than acting, and the AI
        // shouldn't be charged a major-action's CT cost for doing nothing.
        // BattleState is the one that calls advanceToNextUnit() and derives
        // CT cost from hasMoved()/hasActed(), so leave both false here.
        LOG_INFO("EnemyAI", "%s has no target – waiting", unit.getName().c_str());
        return;
    }

    // ── 2. Compute movement range (only if this unit hasn't moved yet) ──────
    if (!unit.hasMoved())
    {
        const Vec2i startPos = unit.getPosition();
        const int moveRange = unit.getMoveRangeLeft();

        auto reachable = MovementRange::compute(grid, battleMap, startPos, moveRange,
                                                unit.getTeam(), allUnits, unit.getJump());

        // ── 3. Pick the reachable tile closest to the target ─────────────────
        const Vec2i destPos = bestTileToward(unit, reachable, target->getPosition(),
                                             grid, allUnits);

        // ── 4. Move there (if destination is valid and not blocked) ──────────
        if (destPos != startPos)
        {
            // Check if destination is occupied by another unit
            bool destinationBlocked = false;
            for (const Unit *u : allUnits)
            {
                if (u && !u->isDead() && u != &unit && u->getPosition() == destPos)
                {
                    destinationBlocked = true;
                    break;
                }
            }

            if (!destinationBlocked)
            {
                grid.getTile(startPos).occupied = false;
                unit.setPosition(destPos);
                grid.getTile(destPos).occupied = true;
                LOG_INFO("EnemyAI", "%s moves from (%d,%d) to (%d,%d)",
                         unit.getName().c_str(), startPos.x, startPos.y,
                         destPos.x, destPos.y);
            }
            else
            {
                LOG_INFO("EnemyAI", "%s wants to move to (%d,%d) but it's blocked – staying",
                         unit.getName().c_str(), destPos.x, destPos.y);
            }
        }
        // AI doesn't do multi-segment movement, so it simply spends its
        // whole pool in one shot regardless of the actual path cost.
        unit.exhaustMovement();
    }

    // ── 5. Attack if the target is now within attack range ──────────────────
    if (!unit.hasActed())
    {
        const int distToTarget = manhattanDistance(unit.getPosition(), target->getPosition());
        if (distToTarget <= unit.getAtkRange())
        {
            HitContext ctx = makeAttackContext(unit, *target);
            CombatResult result = CombatSystem::resolve(ctx);

            // TODO: trigger attack animation (unit -> target) here once the
            // engine has an animation system; apply damage/log on completion
            // instead of immediately, to let the visual play out.
            if (result.hit)
            {
                target->takeDamage(result.damage);
                LOG_INFO("EnemyAI", "%s attacks %s for %d damage! (HP: %d/%d)",
                         unit.getName().c_str(), target->getName().c_str(),
                         result.damage, target->getCurrentHp(), target->getMaxHp());
            }
            else
            {
                LOG_INFO("EnemyAI", "%s misses %s", unit.getName().c_str(), target->getName().c_str());
            }
        }
        else
        {
            LOG_INFO("EnemyAI", "%s out of range – cannot attack %s (dist %d > range %d)",
                     unit.getName().c_str(), target->getName().c_str(), distToTarget, unit.getAtkRange());
        }
        unit.setMajorAction(MajorAction::Attack);
    }
}

int EnemyAI::chooseAction(const Unit &unit, const Grid &grid,
                          std::vector<Unit *> &allUnits)
{
    Unit *target = findBestTarget(unit, allUnits);
    if (!target)
        return 2; // Wait

    if (manhattanDistance(unit.getPosition(), target->getPosition()) <= unit.getAtkRange())
        return 1; // Attack

    if (!unit.hasMoved())
        return 0; // Move

    return 2; // Wait
}

Unit *EnemyAI::chooseTarget(const Unit &unit, std::vector<Unit *> &allUnits)
{
    return findBestTarget(unit, allUnits);
}
