#pragma once
#include <vector>
#include <string>
#include "battle/Unit.h"
#include "engine/math/Vec2.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct GameTile;

class UnitFactory
{
public:
    static Unit *createUnitFromJson(const std::string &jsonPath, Vec2i position, int team);
    static std::vector<Unit *> createUnitsFromSpawns(
        const std::vector<GameTile *> &spawns,
        const std::string &jsonTemplatePath,
        int team);
};
