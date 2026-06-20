#pragma once
#include <string>
#include <vector>

// SkillData: a skill or ability a unit can use.
struct SkillData
{
    std::string name;
    int         power      = 0;    // base damage modifier
    int         range      = 1;    // tile reach
    int         area       = 0;    // splash radius (0 = single target)
    std::string effectType;        // "damage", "heal", "buff", "debuff"
    int         mpCost     = 0;
};

// SkillLoader reads skill definitions from JSON.
//
// Expected JSON schema:
// {
//   "name": "Fireball",
//   "power": 15,
//   "range": 3,
//   "area": 1,
//   "effectType": "damage",
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
    static SkillData              load(const std::string& filePath);
    static std::vector<SkillData> loadAll(const std::string& directory);
};
