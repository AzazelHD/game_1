#pragma once

#include <vector>
#include "engine/math/Vec2.h"
#include "battle/combat/CombatSystem.h"
#include "data/SkillLoader.h"

class Unit;
class Grid;
class TurnQueue;
class BattleMap;

// Outcome of EnemyAI::resolveAttack(). The attack math is done here, but the
// damage is NOT applied — the caller owns applying it (through BattleSession,
// so victory/defeat and VFX can be driven from one place).
struct EnemyAttackResult
{
    bool attacked = false; // true when a legal in-range attack was resolved (hit OR miss)
    CombatResult result;
    Unit *target = nullptr;
};

// EnemyAI decides and executes one enemy unit's turn.
class EnemyAI
{
public:
    // Pure decision, no mutation — lets the caller (BattleState) drive the
    // actual move via its own animated-movement path, then resolve the
    // attack once arrival completes.
    struct EnemyTurnPlan
    {
        Unit *target = nullptr;
        bool wantsToMove = false;
        Vec2i destination{};
    };

    static EnemyTurnPlan planTurn(Unit &unit, Grid &grid, const BattleMap &battleMap, std::vector<Unit *> &allUnits);

    // Resolves the attack-if-in-range step only (no movement) — called once
    // the unit has arrived at its planned destination (or immediately, if
    // the plan never wanted to move). The AI attacks with the same shared
    // basic-attack skill the player's "Attack" button selects, so both camps
    // resolve through one code path. Returns the roll result but does NOT
    // apply damage; see EnemyAttackResult.
    static EnemyAttackResult resolveAttack(Unit &unit, Unit *target, const Grid &grid, const BattleMap &battleMap,
                                          const SkillData &basicAttack);

    // Synchronous, instant, unanimated — still used by debug-only paths
    // (immediate AI takeover, autoplay). Internally now just calls
    // planTurn()+resolveAttack() back to back.
    static void takeTurn(Unit &unit, Grid &grid, const BattleMap &battleMap, std::vector<Unit *> &allUnits,
                         const SkillData &basicAttack);

    static int chooseAction(const Unit &unit, const Grid &grid, const BattleMap &battleMap,
                            std::vector<Unit *> &allUnits, const SkillData &basicAttack);
    static Unit *chooseTarget(const Unit &unit, std::vector<Unit *> &allUnits);
};