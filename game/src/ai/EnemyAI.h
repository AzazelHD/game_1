#pragma once

#include <vector>
#include "engine/math/Vec2.h"

class Unit;
class Grid;
class TurnQueue;
class BattleMap;

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
    // the plan never wanted to move). Uses the unit's atkRange with the
    // default RangeRule (same-height, LOS-free).
    static void resolveAttack(Unit &unit, Unit *target, const Grid &grid, const BattleMap &battleMap);

    // Synchronous, instant, unanimated — still used by debug-only paths
    // (immediate AI takeover, autoplay). Internally now just calls
    // planTurn()+resolveAttack() back to back.
    static void takeTurn(Unit &unit, Grid &grid, const BattleMap &battleMap, std::vector<Unit *> &allUnits);

    static int chooseAction(const Unit &unit, const Grid &grid, const BattleMap &battleMap,
                            std::vector<Unit *> &allUnits);
    static Unit *chooseTarget(const Unit &unit, std::vector<Unit *> &allUnits);
};