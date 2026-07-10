#pragma once

#include "systems/DeploymentSystem.h"
#include "battle/BattleSession.h"

#include <vector>

class BattleMap;
struct BattleDefinition;

class BattleParticipantsBuilder
{
public:
    // Builds the spawn spec list (player + enemies) for a battle.
    // BattleSession::init() is what actually constructs the Unit objects —
    // this class no longer owns or allocates any Unit itself.
    static std::vector<UnitSpawn> build(const BattleDefinition &definition,
                                        const BattleMap &battleMap,
                                        const DeploymentSystem &deployment);
};