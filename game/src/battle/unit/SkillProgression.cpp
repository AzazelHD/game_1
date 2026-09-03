#include "battle/unit/SkillProgression.h"

#include <algorithm>
#include <array>

namespace
{
    const std::vector<ProgressionSkillDefinition> kSkills = []
    {
        std::vector<ProgressionSkillDefinition> skills = {
            ProgressionSkillDefinition{
                .id = "placeholder_soldier_mastery",
                .name = "Placeholder Soldier Mastery",
                .category = ProgressionSkillCategory::Active,
                .maxLevel = 5,
                .pointCosts = {1, 1, 2, 2, 3},
                .eligibleBaseClasses = {BaseClass::Soldier},
                .placeholder = true,
            },
            ProgressionSkillDefinition{
                .id = "placeholder_soldier_vitality",
                .name = "Placeholder Soldier Vitality",
                .category = ProgressionSkillCategory::Passive,
                .maxLevel = 3,
                .pointCosts = {1, 1, 2},
                .eligibleBaseClasses = {BaseClass::Soldier},
                .placeholder = true,
            },
            ProgressionSkillDefinition{
                .id = "placeholder_soldier_counter",
                .name = "Placeholder Soldier Counter",
                .category = ProgressionSkillCategory::Reaction,
                .maxLevel = 1,
                .pointCosts = {1},
                .eligibleBaseClasses = {BaseClass::Soldier},
                .placeholder = true,
            },
            ProgressionSkillDefinition{
                .id = "placeholder_templar_aegis",
                .name = "Placeholder Aegis",
                .category = ProgressionSkillCategory::Passive,
                .maxLevel = 1,
                .pointCosts = {1},
                .eligibleBaseClasses = {BaseClass::Soldier},
                .requiredPromotion = PromotionClass::Templar,
                .placeholder = true,
                .autoGranted = true,
            },
            ProgressionSkillDefinition{
                .id = "placeholder_templar_smite",
                .name = "Placeholder Smite",
                .category = ProgressionSkillCategory::Active,
                .maxLevel = 1,
                .pointCosts = {1},
                .eligibleBaseClasses = {BaseClass::Soldier},
                .requiredPromotion = PromotionClass::Templar,
                .placeholder = true,
                .autoGranted = true,
            },
            ProgressionSkillDefinition{
                .id = "placeholder_knight_guard",
                .name = "Placeholder Guard",
                .category = ProgressionSkillCategory::Passive,
                .maxLevel = 1,
                .pointCosts = {1},
                .eligibleBaseClasses = {BaseClass::Soldier},
                .requiredPromotion = PromotionClass::Knight,
                .placeholder = true,
                .autoGranted = true,
            },
            ProgressionSkillDefinition{
                .id = "placeholder_knight_strike",
                .name = "Placeholder Strike",
                .category = ProgressionSkillCategory::Active,
                .maxLevel = 1,
                .pointCosts = {1},
                .eligibleBaseClasses = {BaseClass::Soldier},
                .requiredPromotion = PromotionClass::Knight,
                .placeholder = true,
                .autoGranted = true,
            },
            ProgressionSkillDefinition{
                .id = "placeholder_paladin_grace",
                .name = "Placeholder Grace",
                .category = ProgressionSkillCategory::Passive,
                .maxLevel = 1,
                .pointCosts = {1},
                .eligibleBaseClasses = {BaseClass::Soldier},
                .requiredPromotion = PromotionClass::Paladin,
                .placeholder = true,
                .autoGranted = true,
            },
            ProgressionSkillDefinition{
                .id = "placeholder_paladin_rebuke",
                .name = "Placeholder Rebuke",
                .category = ProgressionSkillCategory::Reaction,
                .maxLevel = 1,
                .pointCosts = {1},
                .eligibleBaseClasses = {BaseClass::Soldier},
                .requiredPromotion = PromotionClass::Paladin,
                .placeholder = true,
                .autoGranted = true,
            },
        };

        // PLACEHOLDER CONTENT: promotion skills are auto-granted and have no
        // point cost/levels. Elf and Elin intentionally share Archer/Mage
        // promotion names until Elin-specific flavor is designed.
        const auto addPromotionPair = [&skills](BaseClass baseClass,
                                                PromotionClass promotion,
                                                const char *key,
                                                const char *name)
        {
            skills.push_back(ProgressionSkillDefinition{
                .id = std::string("placeholder_") + key + "_technique",
                .name = std::string("Placeholder ") + name + " Technique",
                .category = ProgressionSkillCategory::Active,
                .maxLevel = 1,
                .pointCosts = {1},
                .eligibleBaseClasses = {baseClass},
                .requiredPromotion = promotion,
                .placeholder = true,
                .autoGranted = true,
            });
            skills.push_back(ProgressionSkillDefinition{
                .id = std::string("placeholder_") + key + "_instinct",
                .name = std::string("Placeholder ") + name + " Instinct",
                .category = ProgressionSkillCategory::Passive,
                .maxLevel = 1,
                .pointCosts = {1},
                .eligibleBaseClasses = {baseClass},
                .requiredPromotion = promotion,
                .placeholder = true,
                .autoGranted = true,
            });
        };

        addPromotionPair(BaseClass::Archer, PromotionClass::Sharpshooter, "sharpshooter", "Sharpshooter");
        addPromotionPair(BaseClass::Archer, PromotionClass::Ranger, "ranger", "Ranger");
        addPromotionPair(BaseClass::Archer, PromotionClass::Windrunner, "windrunner", "Windrunner");
        addPromotionPair(BaseClass::Mage, PromotionClass::Elementalist, "elementalist", "Elementalist");
        addPromotionPair(BaseClass::Mage, PromotionClass::Enchanter, "enchanter", "Enchanter");
        addPromotionPair(BaseClass::Mage, PromotionClass::Warden, "warden", "Warden");
        addPromotionPair(BaseClass::Scout, PromotionClass::Infiltrator, "infiltrator", "Infiltrator");
        addPromotionPair(BaseClass::Scout, PromotionClass::Trapper, "trapper", "Trapper");
        addPromotionPair(BaseClass::Scout, PromotionClass::Pathfinder, "pathfinder", "Pathfinder");
        addPromotionPair(BaseClass::Duelist, PromotionClass::BladeDancer, "blade_dancer", "Blade Dancer");
        addPromotionPair(BaseClass::Duelist, PromotionClass::Assassin, "assassin", "Assassin");
        addPromotionPair(BaseClass::Duelist, PromotionClass::Fencer, "fencer", "Fencer");
        return skills;
    }();

    bool containsBaseClass(const ProgressionSkillDefinition &skill, BaseClass baseClass)
    {
        return std::find(skill.eligibleBaseClasses.begin(),
                         skill.eligibleBaseClasses.end(),
                         baseClass) != skill.eligibleBaseClasses.end();
    }
}

const ProgressionSkillDefinition *findProgressionSkill(const std::string &skillId)
{
    for (const auto &skill : kSkills)
        if (skill.id == skillId)
            return &skill;
    return nullptr;
}

std::vector<const ProgressionSkillDefinition *> availableProgressionSkills(
    BaseClass baseClass, PromotionClass promotion)
{
    std::vector<const ProgressionSkillDefinition *> result;
    for (const auto &skill : kSkills)
    {
        if (containsBaseClass(skill, baseClass) &&
            (skill.requiredPromotion == PromotionClass::None ||
             skill.requiredPromotion == promotion))
            result.push_back(&skill);
    }
    return result;
}

SkillProgression::SkillProgression(BaseClass baseClass, PromotionClass promotion)
    : m_baseClass(baseClass), m_promotion(promotion)
{
    grantPromotionSkills();
}

void SkillProgression::setClass(BaseClass baseClass, PromotionClass promotion)
{
    m_baseClass = baseClass;
    m_promotion = promotion;
    grantPromotionSkills();
}

void SkillProgression::grantPromotionSkills()
{
    for (const auto &skill : kSkills)
    {
        if (skill.autoGranted && isEligible(skill))
            m_levels[skill.id] = skill.maxLevel;
    }
}

bool SkillProgression::isEligible(const ProgressionSkillDefinition &skill) const
{
    return containsBaseClass(skill, m_baseClass) &&
           (skill.requiredPromotion == PromotionClass::None ||
            skill.requiredPromotion == m_promotion);
}

int SkillProgression::levelOf(const std::string &skillId) const
{
    const auto it = m_levels.find(skillId);
    return it == m_levels.end() ? 0 : it->second;
}

bool SkillProgression::isLearned(const std::string &skillId) const
{
    return levelOf(skillId) > 0;
}

bool SkillProgression::canLearnOrUpgrade(const std::string &skillId, int availablePoints) const
{
    const ProgressionSkillDefinition *skill = findProgressionSkill(skillId);
    if (!skill || !isEligible(*skill))
        return false;

    if (skill->autoGranted)
        return false;

    const int currentLevel = levelOf(skillId);
    if (currentLevel >= skill->maxLevel)
        return false;

    const std::size_t costIndex = static_cast<std::size_t>(currentLevel);
    if (costIndex >= skill->pointCosts.size())
        return false;

    return availablePoints >= skill->pointCosts[costIndex];
}

bool SkillProgression::learnOrUpgrade(const std::string &skillId, int &availablePoints)
{
    if (!canLearnOrUpgrade(skillId, availablePoints))
        return false;

    const ProgressionSkillDefinition *skill = findProgressionSkill(skillId);
    const int currentLevel = levelOf(skillId);
    availablePoints -= skill->pointCosts[static_cast<std::size_t>(currentLevel)];
    m_levels[skillId] = currentLevel + 1;
    return true;
}

bool SkillProgression::isEquipped(const std::string &skillId) const
{
    const ProgressionSkillDefinition *skill = findProgressionSkill(skillId);
    return skill && equippedFor(skill->category).count(skillId) != 0;
}

int SkillProgression::equipLimitFor(ProgressionSkillCategory category) const
{
    switch (category)
    {
    case ProgressionSkillCategory::Active:
        return activeEquipLimit();
    case ProgressionSkillCategory::Passive:
        return passiveEquipLimit();
    case ProgressionSkillCategory::Reaction:
        return reactionEquipLimit();
    }
    return 0;
}

std::unordered_set<std::string> &SkillProgression::equippedFor(ProgressionSkillCategory category)
{
    switch (category)
    {
    case ProgressionSkillCategory::Active:
        return m_equippedActive;
    case ProgressionSkillCategory::Passive:
        return m_equippedPassive;
    case ProgressionSkillCategory::Reaction:
        return m_equippedReaction;
    }
    return m_equippedActive;
}

const std::unordered_set<std::string> &SkillProgression::equippedFor(ProgressionSkillCategory category) const
{
    switch (category)
    {
    case ProgressionSkillCategory::Active:
        return m_equippedActive;
    case ProgressionSkillCategory::Passive:
        return m_equippedPassive;
    case ProgressionSkillCategory::Reaction:
        return m_equippedReaction;
    }
    return m_equippedActive;
}

bool SkillProgression::equip(const std::string &skillId)
{
    const ProgressionSkillDefinition *skill = findProgressionSkill(skillId);
    if (!skill || !isEligible(*skill) || !isLearned(skillId))
        return false;

    auto &equipped = equippedFor(skill->category);
    if (equipped.count(skillId) != 0)
        return true;
    if (static_cast<int>(equipped.size()) >= equipLimitFor(skill->category))
        return false;

    equipped.insert(skillId);
    return true;
}

bool SkillProgression::unequip(const std::string &skillId)
{
    const ProgressionSkillDefinition *skill = findProgressionSkill(skillId);
    if (!skill)
        return false;
    return equippedFor(skill->category).erase(skillId) != 0;
}