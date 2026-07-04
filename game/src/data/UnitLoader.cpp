#include "data/UnitLoader.h"
#include "battle/UnitData.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <filesystem>

using json = nlohmann::json;

// ── Helpers ───────────────────────────────────────────────────────────────────
static Race parseRace(const std::string &str)
{
    if (str == "Human")
        return Race::Human;
    if (str == "Elf")
        return Race::Elf;
    if (str == "Undead")
        return Race::Undead;
    throw std::runtime_error("Unknown race: " + str);
}

static Gender parseGender(const std::string &str)
{
    if (str == "Male")
        return Gender::Male;
    if (str == "Female")
        return Gender::Female;
    if (str == "None")
        return Gender::None;
    throw std::runtime_error("Unknown gender: " + str);
}

// ── Load single file ──────────────────────────────────────────────────────────
UnitData UnitLoader::load(const std::string &filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        throw std::runtime_error("Could not open unit file: " + filePath);

    const json j = json::parse(file);

    UnitData data;
    data.name = j.at("name").get<std::string>();
    data.className = j.value("className", std::string("Unknown"));
    data.spriteSetId = j.value("spriteSetId", std::string());
    data.acquisitionIndex = j.value("acquisitionIndex", 0);
    data.race = parseRace(j.at("race").get<std::string>());
    data.gender = parseGender(j.at("gender").get<std::string>());
    data.level = j.value("level", 1);
    data.maxHp = j.value("maxHp", 30);
    data.maxMp = j.value("maxMp", 8);
    data.attack = j.value("attack", 10);
    data.defense = j.value("defense", 5);
    data.magic = j.value("magic", 5);
    data.magicDefense = j.value("magicDefense", 5);
    data.moveRange = j.value("moveRange", 4);
    data.atkRange = j.value("atkRange", 1);
    data.evasion = j.value("evasion", 10);
    data.jump = j.value("jump", 1);
    data.speed = j.value("speed", 25);
    data.team = j.value("team", 0);
    if (j.contains("skills") && j["skills"].is_array())
    {
        for (const auto &s : j["skills"])
            data.skillIds.push_back(s.get<std::string>());
    }
    return data;
}

// ── Load all files in directory ───────────────────────────────────────────────
std::vector<UnitData> UnitLoader::loadAll(const std::string &directory)
{
    std::vector<UnitData> units;
    for (const auto &entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.path().extension() == ".json")
            units.push_back(load(entry.path().string()));
    }
    return units;
}