#pragma once

#include "engine/math/Vec2.h"
#include "battle/map/RangeRule.h"
#include "battle/map/MovementRange.h"

#include <unordered_set>
#include <vector>

class Grid;
class BattleMap;

// AttackRange computes which tiles are valid targets for an attack or
// AoE center, using a per-skill RangeRule rather than raw Manhattan
// distance. It mirrors MovementRange::compute as a static, pure-function
// module — no member state.
class AttackRange
{
public:
    struct Result
    {
        std::unordered_set<Vec2i, Vec2iHash> tiles;
    };

    // Computes the set of valid tiles for a direct attack (or AoE center
    // placement) from `origin` using `rule`. Returns the set of tiles
    // that pass the distance, height, and optional LOS checks.
    static Result compute(const Grid &grid,
                          const BattleMap &battleMap,
                          Vec2i origin,
                          const RangeRule &rule);

    // True if `target` is a legal direct-attack target for `attackerPos`
    // under `rule`. Used by both the human gate and the enemy AI.
    static bool canTarget(const Grid &grid,
                          const BattleMap &battleMap,
                          Vec2i attackerPos,
                          Vec2i target,
                          const RangeRule &rule);

    // Returns the set of tiles within the AoE splash of `center` under
    // `rule`. Each tile is individually LOS-checked from `center` when
    // `rule.splashRespectsWalls` is true. Includes `center` itself.
    static std::unordered_set<Vec2i, Vec2iHash>
    computeSplashTiles(const Grid &grid,
                       const BattleMap &battleMap,
                       Vec2i center,
                       const RangeRule &rule);

    // True if an unobstructed line of sight exists from `from` to `to`
    // given the height tolerance in `rule`. Exposed for unit tests.
    static bool hasLineOfSight(const Grid &grid,
                               const BattleMap &battleMap,
                               Vec2i from,
                               Vec2i to,
                               const RangeRule &rule);

private:
    static bool distanceCheck(Vec2i origin, Vec2i target, const RangeRule &rule);
    static bool heightCheck(Vec2i origin, Vec2i target, const BattleMap &battleMap,
                            const RangeRule &rule);
};
