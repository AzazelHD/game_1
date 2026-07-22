#include "data/UnitLoader.h"
#include "battle/UnitData.h"
#include "battle/UnitFactory.h"
#include "battle/BattleMap.h"
#include "engine/core/Log.h"

Unit *UnitFactory::createUnitFromJson(const std::string &jsonPath, Vec2i position, int team)
{
    UnitData unitData = UnitLoader::load(jsonPath);
    unitData.team = team;

    RaceData raceData = getRaceData(unitData.race);
    GenderData genderData = getGenderData(unitData.gender);

    return new Unit(unitData, raceData, genderData, position);
}

std::vector<Unit *> UnitFactory::createUnitsFromSpawns(
    const std::vector<GameTile *> &spawns,
    const std::string &jsonTemplatePath,
    int team)
{
    std::vector<Unit *> units;
    for (GameTile *tile : spawns)
    {
        Vec2i pos(tile->col, tile->row);
        Unit *unit = createUnitFromJson(jsonTemplatePath, pos, team);
        if (unit)
            units.push_back(unit);
    }
    return units;
}
