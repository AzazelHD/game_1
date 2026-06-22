#pragma once
#include <vector>

class Unit;
class Grid;
class TurnQueue;

// EnemyAI decides and executes one enemy unit's turn.
// It drives the same systems as player input — no special back-door access.
//
// [x]: Implement:
//     void takeTurn(Unit& unit, Grid& grid, std::vector<Unit*>& allUnits)
//     Heuristic decision tree:
//       1. Score all living enemy units (evaluateTarget): distance, HP%,
//          kill potential, and ally focus. Pick the highest-scoring target.
//       2. If not yet moved this turn: compute movement range
//          (MovementRange::compute), pick the reachable, unblocked tile
//          closest to the target, and move there (or stay put if no tile
//          improves position).
//       3. If not yet acted and the target is within attack range,
//          resolve combat (CombatSystem::resolve).
//       4. Mark unit.hasMoved = true, unit.hasActed = true as each
//          action completes.
//
// Design: EnemyAI is stateless — all data comes in via parameters. No member variables.
// This makes it easy to test and to swap in a smarter AI later without changing the interface.
class EnemyAI
{
public:
    static void takeTurn(Unit &unit, Grid &grid, std::vector<Unit *> &allUnits);
    static int chooseAction(const Unit &unit, const Grid &grid,
                            std::vector<Unit *> &allUnits);
};
