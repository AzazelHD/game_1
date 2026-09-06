#include "battle/systems/MovementCommitSystem.h"

#include "battle/BattleSession.h"
#include "battle/map/BattleMap.h"
#include "battle/map/Grid.h"
#include "battle/map/MovementRange.h"
#include "battle/map/Pathfinder.h"
#include "battle/unit/Unit.h"
#include "inventory/Gear.h"

#include <cmath>
#include <unordered_map>

MovementCommitResult MovementCommitSystem::commitMovement(BattleSession &session,
                                                          const Grid &grid,
                                                          const BattleMap &map,
                                                          Unit *unit,
                                                          Vec2i destination)
{
    MovementCommitResult result;

    if (!unit)
        return result;

    const Vec2i start = unit->getPosition();
    const bool teleporting = unit->hasGearSpecialEffect(GearSpecialEffect::TeleportMovement);

    if (teleporting)
    {
        result.valid = true;
        result.teleport = true;
        return result;
    }

    if (!grid.isValid(destination))
        return result;

    std::unordered_map<Vec2i, int, Vec2iHash> unitTeamAt;
    for (Unit *u : session.getUnitPtrs())
        if (u && !u->isDead())
            unitTeamAt[u->getPosition()] = u->getTeam();

    const int team = unit->getTeam();
    const int jump = unit->getJump();
    const Grid *gridPtr = &grid;
    const BattleMap *battleMapPtr = &map;

    PathRules rules;
    rules.getNeighbors = [gridPtr, battleMapPtr, jump, team, unitTeamAt](Vec2i from) -> std::vector<Vec2i>
    {
        static const Vec2i dirs[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        std::vector<Vec2i> out;
        for (const Vec2i &d : dirs)
        {
            Vec2i to{from.x + d.x, from.y + d.y};
            if (!gridPtr->isValid(to))
                continue;
            if (std::abs(battleMapPtr->at(to.x, to.y).height - battleMapPtr->at(from.x, from.y).height) > jump)
                continue;
            auto it = unitTeamAt.find(to);
            if (it != unitTeamAt.end() && it->second != team)
                continue; // enemy fully blocks
            out.push_back(to);
        }
        return out;
    };
    rules.moveCost = [gridPtr](Vec2i from, Vec2i to) -> int
    {
        return gridPtr->getMoveCost(from, to);
    };

    result.path = Pathfinder::findPath(grid, PathRequest{start, destination}, rules);
    result.valid = !result.path.empty();
    return result;
}