#include "systems/BattleParticipantsBuilder.h"

#include "battle/BattleMap.h"
#include "battle/UnitFactory.h"
#include "config/BattleCatalog.h"

BattleParticipants BattleParticipantsBuilder::build(const BattleDefinition &definition,
                                                    const BattleMap &battleMap,
                                                    const DeploymentSystem &deployment)
{
    BattleParticipants result;

    for (const DeploymentEntry &placed : deployment.deployed())
    {
        Unit *u = UnitFactory::createUnitFromJson(placed.templatePath, placed.position, 0);
        if (u)
            result.playerUnits.push_back(u);
    }

    std::vector<GameTile *> enemySpawns;
    for (const auto &pair : battleMap.enemySpawnsByTeam)
    {
        for (GameTile *tile : pair.second)
            enemySpawns.push_back(tile);
    }

    std::size_t spawnIndex = 0;
    for (const EnemyDefinition &enemy : definition.enemies)
    {
        Vec2i pos{0, 0};
        if (!enemySpawns.empty())
        {
            GameTile *tile = enemySpawns[spawnIndex % enemySpawns.size()];
            pos = Vec2i{tile->col, tile->row};
            ++spawnIndex;
        }

        Unit *u = UnitFactory::createUnitFromJson(enemy.templatePath, pos, enemy.team);
        if (u)
            result.enemyUnits.push_back(u);
    }

    return result;
}
