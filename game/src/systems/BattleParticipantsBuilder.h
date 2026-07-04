#pragma once

#include "systems/DeploymentSystem.h"

#include <vector>

class Unit;
class BattleMap;
struct BattleDefinition;

struct BattleParticipants
{
    std::vector<Unit *> playerUnits;
    std::vector<Unit *> enemyUnits;
};

class BattleParticipantsBuilder
{
public:
    static BattleParticipants build(const BattleDefinition &definition,
                                    const BattleMap &battleMap,
                                    const DeploymentSystem &deployment);
};
