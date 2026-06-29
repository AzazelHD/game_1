#pragma once

#include "battle/UnitData.h"

#include <string>
#include <vector>

enum class SkillEffectType
{
    Damage,
    Heal,
    Buff,
    Debuff
};

// SkillData: a skill or ability a unit can use.
struct SkillData
{
    std::string id;          // unique key (e.g. "slash", "fireball")
    std::string name;        // display name
    std::string description; // flavour text (optional)

    int basePower = 0;                        // flat damage before stats
    bool isMagical = false;                   // true = uses magic stat
    Element element = Element::Neutral;       // "neutral", "fire", "ice", etc.
    int skillAccuracy = 100;                  // base hit% (0–100)
    int range = 1;                            // tile reach
    int area = 0;                             // AoE radius (0 = single target)
    int mpCost = 0;                           // MP required to use
    std::vector<SkillEffectType> effectTypes; // "damage", "heal", "buff", "debuff"
};

// SkillLoader reads skill definitions from JSON.
//
// Expected JSON schema:
// {
//   "name": "Fireball",
//   "power": 15,
//   "range": 3,
//   "area": 1,
//   "effectTypes": ["damage"],
//   "mpCost": 5
// }
//
// TODO: Implement:
//   static SkillData load(const std::string& filePath)
//   static std::vector<SkillData> loadAll(const std::string& directory)
//
// After implementing, update CombatSystem::resolve() to accept a const SkillData* (nullable —
// nullptr means basic attack, non-null means skill use).
class SkillLoader
{
public:
    static SkillData load(const std::string &filePath);
    static std::vector<SkillData> loadAll(const std::string &directory);
};
