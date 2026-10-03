#include "data/SkillLoader.h"
#include <nlohmann/json.hpp>
#include <algorithm>
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
    if (s == "ko")
        return SkillEffectType::Ko;
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
    skill.isBasicAttack = j.value("isBasicAttack", false);
    skill.basePower = j.value("basePower", 0);
    skill.powerPerLevel = j.value("powerPerLevel", 0);
    skill.isMagical = j.value("isMagical", false);
    skill.element = parseElement(j.value("element", "neutral"));
    skill.skillAccuracy = j.value("skillAccuracy", 100);
    skill.range = j.value("range", 1);
    skill.area = j.value("area", 0);
    skill.mpCost = j.value("mpCost", 0);
    skill.castOncePerArea = j.value("castOncePerArea", false);

    skill.neverMiss = j.value("neverMiss", false);

    // visualOnHit gates the consequence behind an animation. Accepted either as
    // a flat string (duration falls back to kDefaultVisualDuration) or as an
    // object { "id": "skull", "duration": 1.1 } when the effect needs a
    // bespoke timing.
    if (j.contains("visualOnHit"))
    {
        if (j["visualOnHit"].is_string())
            skill.visualOnHit.id = j["visualOnHit"].get<std::string>();
        else if (j["visualOnHit"].is_object())
        {
            skill.visualOnHit.id = j["visualOnHit"].value("id", "");
            skill.visualOnHit.duration = j["visualOnHit"].value("duration", 0.0f);
        }
        if (!skill.visualOnHit.empty() && skill.visualOnHit.duration <= 0.0f)
            skill.visualOnHit.duration = kDefaultSkillVisualDuration;
    }

    skill.heightTolerance = j.value("heightTolerance", 0);
    skill.splashHeightTolerance = j.value("splashHeightTolerance", -1);
    skill.requiresLineOfSight = j.value("requiresLineOfSight", false);
    skill.splashRespectsWalls = j.value("splashRespectsWalls", true);

    skill.statKey = j.value("statKey", "");
    skill.statAmount = j.value("statAmount", 0);
    skill.buffTurns = j.value("buffTurns", 0);

    if (j.contains("statuses") && j["statuses"].is_array())
    {
        for (const auto &e : j["statuses"])
        {
            StatusEffectApply apply;
            apply.type = statusTypeFromString(e.value("type", "poison"));
            apply.turns = e.value("turns", 0);
            apply.damagePerTick = e.value("damagePerTick", 0);
            apply.chance = e.value("chance", 100);
            if (apply.turns > 0)
                skill.statuses.push_back(apply);
        }
    }

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

RangeRule SkillData::toRangeRule() const
{
    return RangeRule{
        .range = range,
        .area = area,
        .heightTolerance = heightTolerance,
        .splashHeightTolerance = splashHeightTolerance,
        .requiresLineOfSight = requiresLineOfSight,
        .splashRespectsWalls = splashRespectsWalls,
    };
}

RangeRule SkillData::toRangeRuleFor(int unitAtkRange, int unitJump) const
{
    RangeRule rule = toRangeRule();
    if (isBasicAttack)
    {
        rule.range = unitAtkRange;
        // Clamp at 0 so a jump of 0 stays same-height-only rather than turning
        // into the -1 "ignore height entirely" wildcard.
        rule.heightTolerance = std::max(0, unitJump - 1);
    }
    return rule;
}

SkillData SkillData::makeBasicAttack()
{
    SkillData skill;
    skill.id = kBasicAttackSkillId;
    skill.name = "Attack";
    skill.description = "A basic attack using the unit's own attack stat.";
    skill.isBasicAttack = true;
    skill.basePower = 0; // the attacker's own stat is the whole base
    skill.powerPerLevel = 0;
    skill.isMagical = false;
    skill.element = Element::Neutral;
    skill.skillAccuracy = 95;
    skill.range = 1;
    skill.area = 0;
    skill.mpCost = 0;
    skill.effectTypes.push_back(SkillEffectType::Damage);
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
