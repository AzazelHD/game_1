#include "config/BattleCatalog.h"
#include "battle/BattleMap.h"
#include "systems/BattleParticipantsBuilder.h"

std::vector<UnitSpawn> BattleParticipantsBuilder::build(const BattleDefinition &definition,
                                                        const BattleMap &battleMap,
                                                        const DeploymentSystem &deployment)
{
    std::vector<UnitSpawn> spawns;

    for (const DeploymentEntry &placed : deployment.deployed())
    {
        spawns.push_back(UnitSpawn{
            .unitFilePath = placed.templatePath,
            .startPos = placed.position,
            .team = 0,
        });
    }

    std::vector<GameTile *> enemySpawns;
    for (const auto &pair : battleMap.enemySpawnsByTeam)
        for (GameTile *tile : pair.second)
            enemySpawns.push_back(tile);

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
        spawns.push_back(UnitSpawn{
            .unitFilePath = enemy.templatePath,
            .startPos = pos,
            .team = enemy.team,
        });
    }

    return spawns;
}