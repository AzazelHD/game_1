#include "battle/map/AttackRange.h"
#include "battle/map/Grid.h"
#include "battle/map/BattleMap.h"
#include "engine/math/MathUtils.h"

#include <cstdlib> // for std::abs

// ── Distance check (Manhattan) ────────────────────────────────────────────────

bool AttackRange::distanceCheck(Vec2i origin, Vec2i target, const RangeRule &rule)
{
    return manhattanDistance(origin, target) <= rule.range;
}

// ── Height check ─────────────────────────────────────────────────────────────

bool AttackRange::heightCheck(Vec2i origin, Vec2i target, const BattleMap &battleMap,
                              const RangeRule &rule)
{
    if (rule.heightTolerance < 0)
        return true;

    const int hOrigin = battleMap.at(origin.x, origin.y).height;
    const int hTarget = battleMap.at(target.x, target.y).height;
    return std::abs(hTarget - hOrigin) <= rule.heightTolerance;
}

// ── Bresenham line (intermediate cells only) ─────────────────────────────────

static std::vector<Vec2i> bresenhamIntermediate(Vec2i from, Vec2i to)
{
    std::vector<Vec2i> cells;
    int x0 = from.x, y0 = from.y;
    const int x1 = to.x, y1 = to.y;
    const int sx = (x0 < x1) ? 1 : -1;
    const int sy = (y0 < y1) ? 1 : -1;
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int err = dx + dy;

    for (;;)
    {
        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }

        cells.push_back({x0, y0});
    }
    return cells;
}

// ── Line of sight ────────────────────────────────────────────────────────────

bool AttackRange::hasLineOfSight(const Grid & /*grid*/,
                                 const BattleMap &battleMap,
                                 Vec2i from,
                                 Vec2i to,
                                 const RangeRule &rule)
{
    if (rule.heightTolerance < 0)
        return true;

    const int hFrom = battleMap.at(from.x, from.y).height;
    const auto cells = bresenhamIntermediate(from, to);

    for (const Vec2i &cell : cells)
    {
        if (!battleMap.isValid(cell.x, cell.y))
            return false;

        const int hCell = battleMap.at(cell.x, cell.y).height;
        if (std::abs(hCell - hFrom) > rule.heightTolerance)
            return false;
    }
    return true;
}

// ── compute (valid attack tiles) ─────────────────────────────────────────────

AttackRange::Result AttackRange::compute(const Grid &grid,
                                         const BattleMap &battleMap,
                                         Vec2i origin,
                                         const RangeRule &rule)
{
    Result result;
    const int mapW = battleMap.cols();
    const int mapH = battleMap.rows();

    for (int r = 0; r < mapH; ++r)
    {
        for (int c = 0; c < mapW; ++c)
        {
            const Vec2i target{c, r};
            if (target == origin)
                continue;

            if (!battleMap.isValid(c, r))
                continue;

            if (!distanceCheck(origin, target, rule))
                continue;

            if (!heightCheck(origin, target, battleMap, rule))
                continue;

            if (rule.requiresLineOfSight &&
                !hasLineOfSight(grid, battleMap, origin, target, rule))
                continue;

            result.tiles.insert(target);
        }
    }
    return result;
}

// ── canTarget (single tile check) ────────────────────────────────────────────

bool AttackRange::canTarget(const Grid &grid,
                            const BattleMap &battleMap,
                            Vec2i attackerPos,
                            Vec2i target,
                            const RangeRule &rule)
{
    if (!battleMap.isValid(target.x, target.y))
        return false;

    if (attackerPos == target)
        return false;

    if (!distanceCheck(attackerPos, target, rule))
        return false;

    if (!heightCheck(attackerPos, target, battleMap, rule))
        return false;

    if (rule.requiresLineOfSight &&
        !hasLineOfSight(grid, battleMap, attackerPos, target, rule))
        return false;

    return true;
}

// ── computeSplashTiles ───────────────────────────────────────────────────────

std::unordered_set<Vec2i, Vec2iHash>
AttackRange::computeSplashTiles(const Grid &grid,
                                const BattleMap &battleMap,
                                Vec2i center,
                                const RangeRule &rule)
{
    std::unordered_set<Vec2i, Vec2iHash> tiles;
    const int mapW = battleMap.cols();
    const int mapH = battleMap.rows();

    if (!battleMap.isValid(center.x, center.y))
        return tiles;

    for (int r = 0; r < mapH; ++r)
    {
        for (int c = 0; c < mapW; ++c)
        {
            const Vec2i tile{c, r};

            if (!battleMap.isValid(c, r))
                continue;

            if (tile == center)
            {
                tiles.insert(tile);
                continue;
            }

            const bool inRange = manhattanDistance(center, tile) <= rule.area;

            if (!inRange)
                continue;

            if (rule.splashRespectsWalls &&
                !hasLineOfSight(grid, battleMap, center, tile, rule))
                continue;

            tiles.insert(tile);
        }
    }
    return tiles;
}
