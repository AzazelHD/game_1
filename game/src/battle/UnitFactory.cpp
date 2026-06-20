#include "battle/UnitFactory.h"
#include "battle/BattleMap.h"
#include "engine/core/Log.h"
#include <fstream>

Unit *UnitFactory::createUnitFromJson(const std::string &jsonPath, Vec2i position, int team)
{
    std::ifstream file(jsonPath);
    if (!file.is_open())
    {
        LOG_ERROR("UnitFactory", "Failed to open unit JSON: %s", jsonPath.c_str());
        return nullptr;
    }

    json data;
    file >> data;

    UnitData unitData;
    unitData.name = data.value("name", "Unknown");
    unitData.team = team;
    unitData.level = data.value("level", 1);
    unitData.maxHp = data.value("maxHp", 30);
    unitData.maxMp = data.value("maxMp", 8);
    unitData.attack = data.value("attack", 10);
    unitData.defense = data.value("defense", 5);
    unitData.magic = data.value("magic", 5);
    unitData.magicDefense = data.value("magicDefense", 5);
    unitData.moveRange = data.value("moveRange", 4);
    unitData.atkRange = data.value("atkRange", 1);
    unitData.evasion = data.value("evasion", 10);
    unitData.speed = data.value("speed", 25);

    // Parse race
    std::string raceStr = data.value("race", "Human");
    if (raceStr == "Human")
        unitData.race = Race::Human;
    else if (raceStr == "Elf")
        unitData.race = Race::Elf;
    else if (raceStr == "Undead")
        unitData.race = Race::Undead;

    // Parse gender
    std::string genderStr = data.value("gender", "None");
    if (genderStr == "Male")
        unitData.gender = Gender::Male;
    else if (genderStr == "Female")
        unitData.gender = Gender::Female;
    else
        unitData.gender = Gender::None;

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