#pragma once

#include "engine/math/Vec2.h"

#include <vector>

class BattleSession;
class Grid;
class BattleMap;
class Unit;

// Outcome of MovementCommitSystem::commitMovement(). `valid` is false when the
// move cannot happen (no unit, out of grid, destination unreachable). When
// valid && teleport, the move is confirmed but there is no walking path
// (TeleportMovement gear effect — the caller skips the walk animation).
struct MovementCommitResult
{
    bool valid = false;
    bool teleport = false;
    std::vector<Vec2i> path;
};

// MovementCommitSystem validates a unit's move to a destination and computes
// the path to walk.
//
// It uses the same walkability rules MovementRange used (height/jump
// restriction, enemy-blocks/ally-passes-through occupancy), so Pathfinder
// reconstructs the SAME path MovementRange already found reachable.
// Teleporting units skip path reconstruction — the move commits instantly.
class MovementCommitSystem
{
public:
    static MovementCommitResult commitMovement(BattleSession &session,
                                               const Grid &grid,
                                               const BattleMap &map,
                                               Unit *unit,
                                               Vec2i destination);
};