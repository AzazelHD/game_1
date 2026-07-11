#include "data/SkillLoader.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <string>
#include <unordered_set>

using json = nlohmann::json;

// ── Local parser helpers ─────────────────────────────────────────────────────
static Element parseElement(const std::string &s)
{
    if (s == "fire")
        return Element::Fire;
    if (s == "ice")
        return Element::Ice;
    if (s == "lightning")
        return Element::Lightning;
    if (s == "holy")
        return Element::Holy;
    if (s == "dark")
        return Element::Dark;
    return Element::Neutral;
}

static SkillEffectType parseEffectType(const std::string &s)
{
    if (s == "heal")
        return SkillEffectType::Heal;
    if (s == "buff")
        return SkillEffectType::Buff;
    if (s == "debuff")
        return SkillEffectType::Debuff;
    return SkillEffectType::Damage; // default
}

// ── Load a single skill JSON ─────────────────────────────────────────────────
SkillData SkillLoader::load(const std::string &filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        throw std::runtime_error("Could not open skill file: " + filePath);

    const json j = json::parse(file);

    SkillData skill;
    skill.id = j.at("id").get<std::string>();
    skill.name = j.at("name").get<std::string>();
    skill.description = j.value("description", "");
    skill.basePower = j.value("basePower", 0);
    skill.isMagical = j.value("isMagical", false);
    skill.element = parseElement(j.value("element", "neutral"));
    skill.skillAccuracy = j.value("skillAccuracy", 100);
    skill.range = j.value("range", 1);
    skill.area = j.value("area", 0);
    skill.mpCost = j.value("mpCost", 0);
    skill.castOncePerArea = j.value("castOncePerArea", false);

    if (j.contains("effectTypes") && j["effectTypes"].is_array())
    {
        for (const auto &e : j["effectTypes"])
        {
            if (e.is_string())
                skill.effectTypes.push_back(parseEffectType(e.get<std::string>()));
        }
    }
    else
    {
        // default: at least deal damage
        skill.effectTypes.push_back(SkillEffectType::Damage);
    }

    return skill;
}

// ── Load all skills in a directory (with duplicate ID check) ─────────────────
std::vector<SkillData> SkillLoader::loadAll(const std::string &directory)
{
    std::vector<SkillData> skills;
    std::unordered_set<std::string> seenIds;

    for (const auto &entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.path().extension() == ".json")
        {
            SkillData skill = load(entry.path().string());
            if (seenIds.count(skill.id))
            {
                throw std::runtime_error(
                    "Duplicate skill ID '" + skill.id + "' in " + entry.path().string());
            }
            seenIds.insert(skill.id);
            skills.push_back(std::move(skill));
        }
    }
    return skills;
}